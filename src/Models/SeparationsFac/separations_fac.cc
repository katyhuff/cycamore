// separations_fac.cc
// Implements the SeparationsFac class
#include <sstream>
#include <cmath>

#include <boost/lexical_cast.hpp>

#include "comp_math.h"
#include "context.h"
#include "cyc_limits.h"
#include "error.h"
#include "logger.h"

#include "separations_fac.h"

namespace cycamore {

// static members
std::map<SeparationsFac::Phase, std::string> SeparationsFac::phase_names_ =
    std::map<Phase, std::string>();

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SeparationsFac::SeparationsFac(cyclus::Context* ctx)
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
SeparationsFac::~SeparationsFac() {}
  
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string SeparationsFac::schema() {
  return
      "  <!-- cyclus::Material In/Out  -->           \n"
      "  <element name=\"inpair\">                   \n"
      "   <ref name=\"incommodity\"/>                \n"
      "   <ref name=\"inrecipe\"/>                   \n"
      "  </element>                                  \n"
      "  <interleave>                                \n"
      "  <oneOrMore>                                 \n"
      "  <element name=\"outpair\">                  \n"
      "   <ref name=\"outcommodity\"/>               \n"
      "   <element name=\"z\">                       \n"
      "     <data type=\"integer\"/>                 \n"
      "   </element>                                 \n"
      "  </element>                                  \n"
      "  </oneOrMore>                                \n"
      "                                              \n"
      "  <!-- Facility Parameters -->                \n"
      "  <element name=\"processtime\">              \n"
      "    <data type=\"nonNegativeInteger\"/>       \n"
      "  </element>                                  \n"
      "  <optional>                                  \n"
      "  <element name =\"capacity\">                \n"
      "    <data type=\"double\"/>                   \n"
      "  </element>                                  \n"
      "  </optional>                                 \n"
      "                                              \n"
      "  <!-- Separations Production  -->            \n"
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
      "  </interleave>                               \n";
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFac::InitFrom(cyclus::QueryEngine* qe) {  
  cyclus::FacilityModel::InitFrom(qe);
  qe = qe->QueryElement("model/" + ModelImpl());
  
  using boost::lexical_cast;
  using cyclus::Commodity;
  using cyclus::CommodityProducer;
  using cyclus::GetOptionalQuery;
  using cyclus::QueryEngine;
  using std::string;

  // out goal recipe
  QueryEngine* inpair = qe->QueryElement("inpair", 0);
  std::string in_c = inpair->GetElementContent("incommodity");
  std::string in_r = inpair->GetElementContent("inrecipe");
  in_recipe_ = in_r;
  in_commod(in_c);
  
  // in/out pair
  int npairs = qe->NElementsMatchingQuery("outpair");
  for (int i = 0; i < npairs; ++i) {
    QueryEngine* outpair = qe->QueryElement("outpair", i);
    std::string out_c = outpair->GetElementContent("outcommodity");
    int out_z = lexical_cast<int>(outpair->GetElementContent("z"));
    // add to map
    out_commod_elem_map_.insert(make_pair(out_c, out_z));
    // also add to sets
    out_commods_.insert(out_c);
    out_elems_.insert(out_z);
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
cyclus::Model* SeparationsFac::Clone() {
  SeparationsFac* m = new SeparationsFac(context());
  m->InitFrom(this);
  return m;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFac::InitFrom(SeparationsFac* m) {
  FacilityModel::InitFrom(m);
  
  // in/out commodity & resource context
  crctx_ = m->crctx_;
  out_elems(m->out_elems());
  out_commods(m->out_commods());
  in_recipe(m->in_recipe());
  in_commod(m->in_commod());
  out_commod_elem_map(m->out_commod_elem_map());

  // facility params
  process_time(m->process_time());
  capacity(m->capacity());

  // commodity production
  CopyProducedCommoditiesFrom(m);

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string SeparationsFac::str() {
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
void SeparationsFac::Deploy(cyclus::Model* parent) {
  using cyclus::Material;

  FacilityModel::Deploy(parent);
  phase(INITIAL);

  LOG(cyclus::LEV_DEBUG2, "SEPSF") << "Simple Separations entering the simuluation";
  LOG(cyclus::LEV_DEBUG2, "SEPSF") << str();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFac::Tick(int time) {
  LOG(cyclus::LEV_INFO3, "SEPSF") << name() << " is ticking at time "
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
  LOG(cyclus::LEV_INFO3, "SEPSF") << "}";
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFac::EndLife_(){
    int nprocessing = ProcessingCount_();
    LOG(cyclus::LEV_DEBUG1, "SEPSF") << "lifetime reached, dumping:"
                                      << nprocessing << " commods.";
  std::set<std::string>::const_iterator it;
  for (it = out_commods().begin(); it != out_commods().end(); it++){
      Separate_(*it); // unload
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFac::Tock(int time) {
  LOG(cyclus::LEV_INFO3, "SEPSF") << name() << " is tocking {";
  PrintStatus("at the beginning of the tock ");
  
  BeginProcessing_(); // place reserves into processing

  std::set<std::string>::const_iterator it;
  for (it = out_commods().begin(); it != out_commods().end(); it++){
      Separate_(*it); 
  }

  PrintStatus("at the end of the tock ");
  LOG(cyclus::LEV_INFO3, "SEPSF") << "}";
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr>
SeparationsFac::GetMatlRequests() {
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
int SeparationsFac::ReservesCount_(std::string commod){
  int count = reserves_[commod].count();
  return count;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SeparationsFac::ReservesCount_(){
  std::map< std::string, cyclus::ResourceBuff >::const_iterator it;
  int count = 0;
  for (it = reserves_.begin(); it != reserves_.end(); ++it){
    count += ReservesCount_((*it).first);
  }
  return count;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SeparationsFac::ReservesQty_(){
  std::map< std::string, cyclus::ResourceBuff >::const_iterator it;
  double amt = 0;
  for (it = reserves_.begin(); it != reserves_.end(); ++it){
    amt += it->second.quantity();
  }
  return amt;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFac::AcceptMatlTrades(
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
SeparationsFac::GetMatlBids(const cyclus::CommodMap<cyclus::Material>::type&
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
void SeparationsFac::PrintStatus(std::string when) { 
  LOG(cyclus::LEV_DEBUG4, "SEPSF") << "Current facility parameters for "
                                    << name()
                                    << " at " << when << " are:";
  LOG(cyclus::LEV_DEBUG4, "SEPSF") << "    Phase: " << phase_names_[phase_]; 
  LOG(cyclus::LEV_DEBUG4, "SEPSF") << "    NReserves: " << ReservesQty_();
  LOG(cyclus::LEV_DEBUG4, "SEPSF") << "    NProcessing: " << ProcessingCount_();
  LOG(cyclus::LEV_DEBUG4, "SEPSF") << "    NStocks: " << StocksCount();  

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFac::GetMatlTrades(
    const std::vector< cyclus::Trade<cyclus::Material> >& trades,
    std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                          cyclus::Material::Ptr> >& responses) {
  using cyclus::Material;
  using cyclus::Trade;

  std::vector< cyclus::Trade<cyclus::Material> >::const_iterator it;
  for (it = trades.begin(); it != trades.end(); ++it) {
    LOG(cyclus::LEV_INFO5, "SEPSF") << name() << " just received an order.";

    std::string commodity = it->request->commodity();
    double qty = it->amt;
    Material::Ptr response = TradeResponse_(qty, &stocks_[commodity]);

    responses.push_back(std::make_pair(*it, response));
    LOG(cyclus::LEV_INFO5, "SeparationsFac") << name()
                                           << " just received an order"
                                           << " for " << qty
                                           << " of " << commodity;
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SeparationsFac::ProcessingCount_() {
  int count = 0;
  std::map<int, cyclus::ResourceBuff>::const_iterator found;
  found = processing_.find(Ready_());
  if( found != processing_.end()) {
    count = processing_[Ready_()].count();
  }
  return count;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SeparationsFac::StocksCount() {
  int count = 0;
  std::map<std::string, cyclus::ResourceBuff>::const_iterator it;
  for (it = stocks_.begin(); it != stocks_.end(); ++it) {
    count += it->second.count();
  }
  return count;
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SeparationsFac::StocksCount(std::string commod) {
  int count = 0;
  std::map<std::string, cyclus::ResourceBuff>::iterator it = stocks_.find(commod);
  if ( it!=stocks_.end() ) {
    count = it->second.count();
  }
  return count;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SeparationsFac::out_elem(std::string commod) const {
  // get the commodity index
  std::map<std::string, int>::const_iterator found;
  found = out_commod_elem_map_.find(commod);
  if( found!=out_commod_elem_map_.end()){
    return found->second;
  } else {
    std::string e = "SepFac: Invalid commodity. There is no element associated with : ";
    e+=commod;
    throw cyclus::KeyError(e);
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFac::phase(SeparationsFac::Phase p) {
  LOG(cyclus::LEV_DEBUG2, "SEPSF") << "SeparationsFac " << name()
                                    << " is changing phases -";
  LOG(cyclus::LEV_DEBUG2, "SEPSF") << "  * from phase: " << phase_names_[phase_];
  LOG(cyclus::LEV_DEBUG2, "SEPSF") << "  * to phase: " << phase_names_[p];
  
  phase_ = p;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFac::EmptyReserves_() {
  /// @TODO could add process capacity constraint here
  while(ReservesQty_() > 0) {
    BeginProcessing_();
    phase(PROCESS);
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFac::BeginProcessing_() {
  LOG(cyclus::LEV_DEBUG2, "SEPSF") << "SeparationsFac " << name() << " added"
                                    <<  " a resource to processing.";
  std::map<std::string, cyclus::ResourceBuff>::iterator it;
  for (it = reserves_.begin(); it != reserves_.end(); ++it){
    while (!(*it).second.empty()){
      try {
        processing_[context()->time()].Push((*it).second.Pop());
      } catch(cyclus::Error& e) {
        e.msg(Model::InformErrorMsg(e.msg()));
        throw e;
      }
    }
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::pair<double, cyclus::Composition::Ptr> SeparationsFac::CompPossible_(int z, cyclus::CompMap comp){
  std::map<int, double>::const_iterator entry;
  int iso;
  double amt = 0;
  cyclus::CompMap to_ret;
  for(entry = comp.begin(); entry != comp.end(); ++entry){
    iso = entry->first;
    if (int(iso/1000.0) == z){
      to_ret.insert(std::make_pair(iso,entry->second));
      amt += entry->second;
    }
  }
  return std::make_pair(amt, cyclus::Composition::CreateFromMass(to_ret));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFac::Separate_(std::string out_commod){
  using cyclus::Material;
  using cyclus::ResourceBuff;
  using cyclus::Composition;

  // get z
  int z = out_elem(out_commod);

  // separate
  Material::Ptr mat = cyclus::ResCast<Material>(processing_[Ready_()].Pop());
  std::pair<double, Composition::Ptr> poss = CompPossible_(z, mat->comp()->mass()); 
  double poss_qty = poss.first;
  Composition::Ptr poss_comp = poss.second;

  // move to stocks
  std::map< std::string, cyclus::ResourceBuff >::const_iterator found;
  found = stocks_.find(out_commod);
  if( found == stocks_.end() ) {
    stocks_[out_commod] = cyclus::ResourceBuff();
  } 
  stocks_[out_commod].Push(mat->ExtractComp(poss_qty, poss_comp));

  processing_[Ready_()].Push(mat);

  LOG(cyclus::LEV_DEBUG2, "SEPSF") << "SeparationsFac " << name() << " is separating material.";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SeparationsFac::Ready_(){
  int ready = context()->time()-process_time();  
  return ready;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::RequestPortfolio<cyclus::Material>::Ptr
SeparationsFac::GetOrder_(double size) {
  using cyclus::CapacityConstraint;
  using cyclus::Material;
  using cyclus::RequestPortfolio;
  using cyclus::Request;
  
  RequestPortfolio<Material>::Ptr port(new RequestPortfolio<Material>());
  
  std::string recipe;
  Material::Ptr mat;
  recipe = crctx_.in_recipe(in_commod());
  assert(recipe != "");
  mat = Material::CreateUntracked(size, context()->GetRecipe(recipe));
  port->AddRequest(mat, this, in_commod());
  
  LOG(cyclus::LEV_DEBUG3, "SEPSF") << "SeparationsFac " << name()
                                    << " is making an order:";
  LOG(cyclus::LEV_DEBUG3, "SEPSF") << "          size: " << size;
  LOG(cyclus::LEV_DEBUG3, "SEPSF") << "     commodity: " << in_commod();

  CapacityConstraint<Material> cc(size);
  port->AddConstraint(cc);

  return port;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFac::AddCommods_(std::string commod, cyclus::Material::Ptr mat) {
  using cyclus::Material;

  LOG(cyclus::LEV_DEBUG3, "SEPSF") << "SeparationsFac " << name()
                                    << " is adding " << mat->quantity()
                                    << " of material to its reserves.";

  assert(commod != "");
  crctx_.AddRsrc(commod, mat);
  reserves_[commod].Push(mat); 
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::BidPortfolio<cyclus::Material>::Ptr SeparationsFac::GetBids_(
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
cyclus::Material::Ptr SeparationsFac::TradeResponse_(
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
void SeparationsFac::SetUpPhaseNames_() {
  phase_names_.insert(std::make_pair(INITIAL, "initialization"));
  phase_names_.insert(std::make_pair(PROCESS, "processing commodities"));
  phase_names_.insert(std::make_pair(WAITING, "waiting for stocks"));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Model* ConstructSeparationsFac(cyclus::Context* ctx) {
  return new SeparationsFac(ctx);
}

} // namespace cycamore
