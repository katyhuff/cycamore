// fco_fuel_fab.cc
// Implements the FCOFuelFab class
#include <sstream>
#include <cmath>

#include <boost/lexical_cast.hpp>

#include "comp_math.h"
#include "context.h"
#include "cyc_limits.h"
#include "error.h"
#include "logger.h"

#include "fco_fuel_fab.h"

namespace cycamore {

// static members
std::map<FCOFuelFab::Phase, std::string> FCOFuelFab::phase_names_ =
    std::map<Phase, std::string>();

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FCOFuelFab::FCOFuelFab(cyclus::Context* ctx)
    : cyclus::FacilityModel(ctx),
      cyclus::Model(ctx),
      process_time_(0),
      capacity_(std::numeric_limits<double>::max()),
      phase_(INITIAL) {
  if (phase_names_.empty()) {
    SetUpPhaseNames_();
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FCOFuelFab::~FCOFuelFab() {}
  
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string FCOFuelFab::schema() {
  return
      "  <!-- cyclus::Material In/Out  -->           \n"
      "  <oneOrMore>                                 \n"
      "  <element name=\"inpair\">                   \n"
      "   <ref name=\"incommodity\"/>                \n"
      "   <ref name=\"inrecipe\"/>                   \n"
      "  </element>                                  \n"
      "  </oneOrMore>                                \n"
      "  <element name=\"outpair\">                  \n"
      "   <ref name=\"outcommodity\"/>               \n"
      "   <ref name=\"outrecipe\"/>                  \n"
      "  </element>                                  \n"
      "  <oneOrMore>                                 \n"
      "  <element name=\"preflist\">                 \n"
      "   <element name=\"prefiso\">                 \n"
      "     <data type=\"integer\"/>                 \n"
      "   </element>                                 \n"
      "   <oneOrMore>                                \n"
      "   <element name=\"sourcecommod\">            \n"
      "     <data type=\"string\"/>                  \n"
      "   </element>                                 \n"
      "   </oneOrMore>                               \n"
      "  </element>                                  \n"
      "  </oneOrMore>                                \n"
      "                                              \n"
      "  <!-- Facility Parameters -->                \n"
      "  <interleave>                                \n"
      "  <element name=\"processtime\">              \n"
      "    <data type=\"nonNegativeInteger\"/>       \n"
      "  </element>                                  \n"
      "  <optional>                                  \n"
      "  <element name =\"capacity\">                \n"
      "    <data type=\"double\"/>                   \n"
      "  </element>                                  \n"
      "  </optional>                                 \n"
      "                                              \n"
      "  <!-- Fuel Fab Production  -->               \n"
      "  <element name=\"commodity_production\">     \n"
      "   <element name=\"commodity\">               \n"
      "     <data type=\"string\"/>                  \n"
      "   </element>                                 \n"
      "   <element name=\"capacity\">                \n"
      "     <data type=\"double\"/>                  \n"
      "   </element>                                 \n"
      "   <element name=\"cost\">                    \n"
      "     <data type=\"double\"/>                  \n"
      "   </element>                                 \n"
      "  </element>                                  \n"
      "  </interleave>                                \n";
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::InitFrom(cyclus::QueryEngine* qe) {  
  cyclus::FacilityModel::InitFrom(qe);
  qe = qe->QueryElement("model/" + ModelImpl());
  
  using boost::lexical_cast;
  using cyclus::Commodity;
  using cyclus::CommodityProducer;
  using cyclus::GetOptionalQuery;
  using cyclus::QueryEngine;
  using std::string;

  // out goal recipe
  QueryEngine* outpair = qe->QueryElement("outpair", 0);
  std::string out_c = outpair->GetElementContent("outcommodity");
  std::string out_r = outpair->GetElementContent("outrecipe");
  out_recipe_ = out_r;
  out_commod_ = out_c;
  
  // in/out pair
  int npairs = qe->NElementsMatchingQuery("inpair");
  for (int i = 0; i < npairs; ++i) {
    QueryEngine* inpair = qe->QueryElement("inpair", i);
    std::string in_c = inpair->GetElementContent("incommodity");
    std::string in_r = inpair->GetElementContent("inrecipe");
    crctx_.AddInCommod(in_c, in_r, out_c, out_r);
  }

  // isotopic source preferences
  int nlists = qe->NElementsMatchingQuery("preflist");
  for (int i = 0; i < nlists; i++) {
    QueryEngine* preflist = qe->QueryElement("preflist", i);
    int prefiso = lexical_cast<int>(preflist->GetElementContent("prefiso"));
    int ncommods = preflist->NElementsMatchingQuery("sourcecommod");
    std::vector<std::string> commods;
    for (int j = 0; j < ncommods; j++){
      std::string commod = preflist->GetElementContent("sourcecommod",j);
      commods.push_back(commod); 
    }
    prefs(prefiso, commods);
  }

  // facility data required
  string data;
  data = qe->GetElementContent("processtime");
  process_time(lexical_cast<int>(data));

  // facility data optional
  double cap;
  cap = GetOptionalQuery<double>(qe, "capacity", capacity());
  capacity(cap);

  // commodity production
  QueryEngine* commodity = qe->QueryElement("commodity_production");
  Commodity commod(commodity->GetElementContent("commodity"));
  AddCommodity(commod);
  data = commodity->GetElementContent("capacity");
  CommodityProducer::SetCapacity(commod, lexical_cast<double>(data));
  data = commodity->GetElementContent("cost");
  CommodityProducer::SetCost(commod, lexical_cast<double>(data));

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Model* FCOFuelFab::Clone() {
  FCOFuelFab* m = new FCOFuelFab(context());
  m->InitFrom(this);
  return m;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::InitFrom(FCOFuelFab* m) {
  FacilityModel::InitFrom(m);
  
  // in/out commodity & resource context
  crctx_ = m->crctx_;
  out_recipe(m->out_recipe());
  out_commod(m->out_commod());

  // facility params
  process_time(m->process_time());
  capacity(m->capacity());
  prefs(m->prefs());

  // commodity production
  CopyProducedCommoditiesFrom(m);

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string FCOFuelFab::str() {
  std::stringstream ss;
  ss << cyclus::FacilityModel::str();
  ss << " has facility parameters {" << "\n"
     << "     Process Time = " << process_time() << ",\n"
     << "     Capacity = " << capacity() << ",\n"
     // list commodities?
     << "'}";
  return ss.str();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::Deploy(cyclus::Model* parent) {
  using cyclus::Material;

  FacilityModel::Deploy(parent);
  phase(INITIAL);

  LOG(cyclus::LEV_DEBUG2, "FCOFF") << "FCO Fuel Fab entering the simuluation";
  LOG(cyclus::LEV_DEBUG2, "FCOFF") << str();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::Tick(int time) {
  LOG(cyclus::LEV_INFO3, "FCOFF") << name() << " is ticking at time "
                                   << time << " {";
  PrintStatus("at the beginning of the tick ");
                                    
  if (context()->time() == FacLifetime()) {
    EndLife_();
  } else {
    switch (phase()) {
      case INITIAL:
        if (ProcessingCount_() > 0) {
          phase(PROCESS);
        } else { 
          phase(WAITING);
        }
        break;
      case PROCESS:
        break; // process on the tock.
      case WAITING:
        if (ProcessingCount_() > 0) {
          phase(PROCESS);
        } 
        break;
    }
  }

  PrintStatus("at the end of the tick ");
  LOG(cyclus::LEV_INFO3, "FCOFF") << "}";
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::EndLife_(){
    int nprocessing = ProcessingCount_();
    LOG(cyclus::LEV_DEBUG1, "FCOFF") << "lifetime reached, dumping:"
                                      << nprocessing << " commods.";
    for (int i = 0; i < nprocessing; i++) {
      FabFuel_(); // unload
    }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::Tock(int time) {
  LOG(cyclus::LEV_INFO3, "FCOFF") << name() << " is tocking {";
  PrintStatus("at the beginning of the tock ");
  
  BeginProcessing_(); // place reserves into processing

  std::vector<std::string>::const_iterator it;
  for (it = crctx_.in_commods().begin(); it != crctx_.in_commods().end(); it++){
    while (processing_[Ready_()].count((*it)) > 0) {
      FabFuel_(); 
    }
  }

  PrintStatus("at the end of the tock ");
  LOG(cyclus::LEV_INFO3, "FCOFF") << "}";
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr>
FCOFuelFab::GetMatlRequests() {
  using cyclus::RequestPortfolio;
  using cyclus::Material;
  
  std::set<RequestPortfolio<Material>::Ptr> set;
  double order_size;

  // by default, this facility requests as much incommodity as there is capacity for.
  // maybe the only exception should be when we're decommissioning...
  order_size = capacity() - ReservesQty_();
  if (order_size > 0) {
    RequestPortfolio<Material>::Ptr p = GetOrder_(order_size);
    set.insert(p);
  }

  return set;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FCOFuelFab::ReservesCount_(std::string commod){
  int count = reserves_[commod].count();
  return count;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FCOFuelFab::ReservesCount_(){
  std::map< std::string, cyclus::ResourceBuff >::const_iterator it;
  int count = 0;
  for (it = reserves_.begin(); it != reserves_.end(); ++it){
    count += ReservesCount_((*it).first);
  }
  return count;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double FCOFuelFab::ReservesQty_(){
  std::map< std::string, cyclus::ResourceBuff >::const_iterator it;
  double amt = 0;
  for (it = reserves_.begin(); it != reserves_.end(); ++it){
    amt += it->second.quantity();
  }
  return amt;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::AcceptMatlTrades(
    const std::vector< std::pair<cyclus::Trade<cyclus::Material>,
                                 cyclus::Material::Ptr> >& responses) {
  using cyclus::Material;
  
  std::map<std::string, Material::Ptr> mat_commods;
   
  std::vector< std::pair<cyclus::Trade<cyclus::Material>,
                         cyclus::Material::Ptr> >::const_iterator trade;

  // blob each material by commodity
  std::string commod;
  Material::Ptr mat;
  for (trade = responses.begin(); trade != responses.end(); ++trade) {
    commod = trade->first.request->commodity();
    mat = trade->second;
    if (mat_commods.count(commod) == 0) {
      mat_commods[commod] = mat;
    } else {
      mat_commods[commod]->Absorb(mat);
    }
  }

  // add each blob to reserves
  std::map<std::string, Material::Ptr>::iterator it;
  for (it = mat_commods.begin(); it != mat_commods.end(); ++it) {
    AddCommods_(it->first, it->second);
  }
}
  
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set<cyclus::BidPortfolio<cyclus::Material>::Ptr>
FCOFuelFab::GetMatlBids(const cyclus::CommodMap<cyclus::Material>::type&
                          commod_requests) {
  using cyclus::BidPortfolio;
  using cyclus::Material;

  std::set<BidPortfolio<Material>::Ptr> ports;

  const std::vector<std::string>& commods = crctx_.out_commods();
  std::vector<std::string>::const_iterator it;
  for (it = commods.begin(); it != commods.end(); ++it) {
    BidPortfolio<Material>::Ptr port = GetBids_(commod_requests,
                                                *it,
                                                &stocks_[*it]);
    if (!port->bids().empty()) ports.insert(port);
  }
  
  return ports;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::PrintStatus(std::string when) { 
  LOG(cyclus::LEV_DEBUG4, "FCOFF") << "Current facility parameters for "
                                    << name()
                                    << " at " << when << " are:";
  LOG(cyclus::LEV_DEBUG4, "FCOFF") << "    Phase: " << phase_names_[phase_]; 
  LOG(cyclus::LEV_DEBUG4, "FCOFF") << "    NReserves: " << ReservesQty_();
  LOG(cyclus::LEV_DEBUG4, "FCOFF") << "    NProcessing: " << ProcessingCount_();
  LOG(cyclus::LEV_DEBUG4, "FCOFF") << "    NStocks: " << StocksCount();  

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::GetMatlTrades(
    const std::vector< cyclus::Trade<cyclus::Material> >& trades,
    std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                          cyclus::Material::Ptr> >& responses) {
  using cyclus::Material;
  using cyclus::Trade;

  std::vector< cyclus::Trade<cyclus::Material> >::const_iterator it;
  for (it = trades.begin(); it != trades.end(); ++it) {
    LOG(cyclus::LEV_INFO5, "FCOFF") << name() << " just received an order.";

    std::string commodity = it->request->commodity();
    double qty = it->amt;
    Material::Ptr response = TradeResponse_(qty, &stocks_[commodity]);

    responses.push_back(std::make_pair(*it, response));
    LOG(cyclus::LEV_INFO5, "FCOFuelFab") << name()
                                           << " just received an order"
                                           << " for " << qty
                                           << " of " << commodity;
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FCOFuelFab::ProcessingQty_() {
  double amt = 0;
  std::map< std::string, cyclus::ResourceBuff >::const_iterator it;
  for(it = processing_[Ready_()].begin(); it != processing_[Ready_()].end(); ++it) {
    amt += it->second.quantity();
  }
  return amt;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FCOFuelFab::ProcessingCount_() {
  int count = 0;
  std::map< std::string, cyclus::ResourceBuff >::const_iterator it;
  for(it = processing_[Ready_()].begin(); it != processing_[Ready_()].end(); ++it) {
    count += it->second.count();
  }
  return count;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FCOFuelFab::ProcessingCount_(std::string commod) {
  int count = 0;
  std::map<std::string, cyclus::ResourceBuff>::iterator it;
  it = processing_[Ready_()].find(commod);
  if ( it!=processing_[Ready_()].end() ) {
    count = it->second.count();
  }
  return count;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FCOFuelFab::StocksCount() {
  int count = 0;
  std::map<std::string, cyclus::ResourceBuff>::const_iterator it;
  for (it = stocks_.begin(); it != stocks_.end(); ++it) {
    count += it->second.count();
  }
  return count;
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FCOFuelFab::StocksCount(std::string commod) {
  int count = 0;
  std::map<std::string, cyclus::ResourceBuff>::iterator it = stocks_.find(commod);
  if ( it!=stocks_.end() ) {
    count = it->second.count();
  }
  return count;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::phase(FCOFuelFab::Phase p) {
  LOG(cyclus::LEV_DEBUG2, "FCOFF") << "FCOFuelFab " << name()
                                    << " is changing phases -";
  LOG(cyclus::LEV_DEBUG2, "FCOFF") << "  * from phase: " << phase_names_[phase_];
  LOG(cyclus::LEV_DEBUG2, "FCOFF") << "  * to phase: " << phase_names_[p];
  
  phase_ = p;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::EmptyReserves_() {
  /// @TODO could add process capacity constraint here
  while(ReservesQty_() > 0) {
    BeginProcessing_();
    phase(PROCESS);
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::BeginProcessing_() {
  LOG(cyclus::LEV_DEBUG2, "FCOFF") << "FCOFuelFab " << name() << " added"
                                    <<  " a resource to processing.";
  std::map<std::string, cyclus::ResourceBuff>::iterator it;
  for (it = reserves_.begin(); it != reserves_.end(); ++it){
    while (!(*it).second.empty()){
      try {
        processing_[context()->time()][(*it).first].Push((*it).second.Pop());
      } catch(cyclus::Error& e) {
        e.msg(Model::InformErrorMsg(e.msg()));
        throw e;
      }
    }
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Composition::Ptr FCOFuelFab::GoalComp_(){
  std::string out = out_recipe();
  cyclus::Composition::Ptr recipe = context()->GetRecipe(out_recipe());
  return recipe;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::CompMap FCOFuelFab::GoalCompMap_(){
  return GoalComp_()->mass();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double FCOFuelFab::GoalCompMass_(){
  double amt = 0;
  std::map<int, double>::const_iterator it;
  for(it=GoalCompMap_().begin(); it!=GoalCompMap_().end(); ++it){
    amt += it->second;
  }
  return amt;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::ResourceBuff FCOFuelFab::MeetNeed_(int iso, int n){
  double need = n*GoalCompMap_()[iso];
  cyclus::ResourceBuff fabbed_fuel_buff =  cyclus::ResourceBuff();
  std::vector<std::string>::const_iterator pref;
  std::vector<std::string> preflist = prefs(iso);
  for(pref = preflist.begin(); pref != preflist.end(); ++pref){
      double avail = processing_[Ready_()][*pref].quantity();
      double diff = need - avail;
      if( need > avail ){
        fabbed_fuel_buff.PushAll(processing_[Ready_()][*pref].PopQty(avail));
        need = diff;
      } else if ( need < avail ){
        fabbed_fuel_buff.PushAll(processing_[Ready_()][*pref].PopQty(need));
      }
  }
  return fabbed_fuel_buff;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FCOFuelFab::NPossible_(){
  int n_poss = 0;
  std::map<int, double>::const_iterator it;
  cyclus::CompMap goal = GoalCompMap_();
  for(it = goal.begin(); it != goal.end(); ++it){
    int iso = it->first;
    double amt = it->second;
    double avail = 0;
    std::vector<std::string>::const_iterator pref;
    for(pref = prefs(iso).begin(); pref != prefs(iso).end(); ++pref){
      std::map< std::string, cyclus::ResourceBuff >::iterator found;
      found = processing_[Ready_()].find(*pref);
      bool isfound = (found!=processing_[Ready_()].end());
      if(isfound){
        avail += processing_[Ready_()][*pref].quantity();
      }
    }
    int prev = n_poss;
    int curr = int(std::floor(avail/amt));
    n_poss = (prev > curr)? prev : curr;
  }
  return n_poss;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::MoveToStocks_(cyclus::ResourceBuff fabbed_fuel_buff){
  using cyclus::Manifest;
  using cyclus::Material;
  using cyclus::ResCast;

  while(!fabbed_fuel_buff.empty()){
    Manifest::const_iterator it;
    Manifest manifest = fabbed_fuel_buff.PopQty(GoalCompMass_());
    Material::Ptr back = ResCast<Material>(manifest.back());
    manifest.pop_back();
    while ( !manifest.empty() ){
      back->Absorb(ResCast<Material>(manifest.back()));
      manifest.pop_back();
    }
    stocks_[out_commod()].Push(back);
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::FabFuel_(){
  using cyclus::Material;
  using cyclus::ResourceBuff;

  int n = NPossible_();

  std::map< int, std::vector<std::string> >::const_iterator pref;
  for(pref = prefs_.begin(); pref != prefs_.end(); ++pref){
    int iso = pref->first;
    ResourceBuff fabbed_fuel_buff = MeetNeed_(iso, n);
    std::cout << "met need " << std::endl;
    MoveToStocks_(fabbed_fuel_buff);
    std::cout << "moved to stocks" << std::endl;
  }
  LOG(cyclus::LEV_DEBUG2, "FCOFF") << "FCOFuelFab " << name() << " is fabricating fuel.";

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FCOFuelFab::Ready_(){
  int ready = context()->time()-process_time();  
  return ready;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<std::string> FCOFuelFab::prefs(int iso){
  std::vector<std::string> preflist;
  std::map<int, std::vector<std::string > >::const_iterator it;
  it = prefs_.find(iso);
  if(it != prefs_.end()){
    preflist = it->second;
  } else { 
    throw cyclus::ValueError("Invalid pref iso. There is no source named for this iso.");
  }
  return preflist;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::RequestPortfolio<cyclus::Material>::Ptr
FCOFuelFab::GetOrder_(double size) {
  using cyclus::CapacityConstraint;
  using cyclus::Material;
  using cyclus::RequestPortfolio;
  using cyclus::Request;
  
  RequestPortfolio<Material>::Ptr port(new RequestPortfolio<Material>());
  
  const std::vector<std::string>& commods = crctx_.in_commods();
  std::vector<std::string>::const_iterator it;
  std::string recipe;
  Material::Ptr mat;
  for (it = commods.begin(); it != commods.end(); ++it) {
    recipe = crctx_.in_recipe(*it);
    assert(recipe != "");
    mat =
        Material::CreateUntracked(size, context()->GetRecipe(recipe));
    port->AddRequest(mat, this, *it);
    
    LOG(cyclus::LEV_DEBUG3, "FCOFF") << "FCOFuelFab " << name()
                                      << " is making an order:";
    LOG(cyclus::LEV_DEBUG3, "FCOFF") << "          size: " << size;
    LOG(cyclus::LEV_DEBUG3, "FCOFF") << "     commodity: " << *it;
  }

  CapacityConstraint<Material> cc(size);
  port->AddConstraint(cc);

  return port;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::AddCommods_(std::string commod, cyclus::Material::Ptr mat) {
  using cyclus::Material;

  LOG(cyclus::LEV_DEBUG3, "FCOFF") << "FCOFuelFab " << name()
                                    << " is adding " << mat->quantity()
                                    << " of material to its reserves.";

  assert(commod != "");
  crctx_.AddRsrc(commod, mat);
  reserves_[commod].Push(mat); 
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::BidPortfolio<cyclus::Material>::Ptr FCOFuelFab::GetBids_(
    const cyclus::CommodMap<cyclus::Material>::type& commod_requests,
    std::string commod,
    cyclus::ResourceBuff* buffer) {
  using cyclus::Bid;
  using cyclus::BidPortfolio;
  using cyclus::CapacityConstraint;
  using cyclus::Composition;
  using cyclus::Converter;
  using cyclus::Material;
  using cyclus::Request;
  using cyclus::ResCast;
  using cyclus::ResourceBuff;
    
  BidPortfolio<Material>::Ptr port(new BidPortfolio<Material>());
  
  if (commod_requests.count(commod) > 0 && buffer->quantity() > 0) {
    const std::vector<Request<Material>::Ptr>& requests =
        commod_requests.at(commod);

    // get offer composition
    Material::Ptr back = ResCast<Material>(buffer->Pop(ResourceBuff::BACK));
    Composition::Ptr comp = back->comp();
    buffer->Push(back);
    
    std::vector<Request<Material>::Ptr>::const_iterator it;
    for (it = requests.begin(); it != requests.end(); ++it) {
      const Request<Material>::Ptr req = *it;
      double qty = std::min(req->target()->quantity(), buffer->quantity());
      Material::Ptr offer =
          Material::CreateUntracked(qty, comp);
      port->AddBid(req, offer, this);
    }
    
    CapacityConstraint<Material> cc(buffer->quantity());
    port->AddConstraint(cc);
  }

  return port;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Material::Ptr FCOFuelFab::TradeResponse_(
    double qty,
    cyclus::ResourceBuff* buffer) {
  using cyclus::Material;
  using cyclus::ResCast;

  std::vector<Material::Ptr> manifest;
  try {
    // pop amount from inventory and blob it into one material
    manifest = ResCast<Material>(buffer->PopQty(qty));  
  } catch(cyclus::Error& e) {
    e.msg(Model::InformErrorMsg(e.msg()));
    throw e;
  }
  
  Material::Ptr response = manifest[0];
  crctx_.RemoveRsrc(response);
  for (int i = 1; i < manifest.size(); i++) {
    crctx_.RemoveRsrc(manifest[i]);
    response->Absorb(manifest[i]);
  }
  return response;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFab::SetUpPhaseNames_() {
  phase_names_.insert(std::make_pair(INITIAL, "initialization"));
  phase_names_.insert(std::make_pair(PROCESS, "processing commodities"));
  phase_names_.insert(std::make_pair(WAITING, "waiting for stocks"));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Model* ConstructFCOFuelFab(cyclus::Context* ctx) {
  return new FCOFuelFab(ctx);
}

} // namespace cycamore
