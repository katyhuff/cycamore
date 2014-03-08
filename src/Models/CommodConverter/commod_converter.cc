// commod_converter.cc
// Implements the CommodConverter class
#include <sstream>
#include <cmath>

#include <boost/lexical_cast.hpp>

#include "cyc_limits.h"
#include "context.h"
#include "error.h"
#include "logger.h"

#include "commod_converter.h"

namespace cycamore {

// static members
std::map<CommodConverter::Phase, std::string> CommodConverter::phase_names_ =
    std::map<Phase, std::string>();

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CommodConverter::CommodConverter(cyclus::Context* ctx)
    : cyclus::FacilityModel(ctx),
      cyclus::Model(ctx),
      process_time_(1),
      preorder_time_(0),
      start_time_(-1),
      to_begin_time_(std::numeric_limits<int>::max()),
      n_reserves_(0),
      phase_(INITIAL) {
  if (phase_names_.empty()) {
    SetUpPhaseNames_();
  }
  spillover_ = cyclus::Material::CreateBlank(0);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CommodConverter::~CommodConverter() {}
  
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string CommodConverter::schema() {
  return
      "  <!-- cyclus::Material In/Out  -->           \n"
      "  <oneOrMore>                                 \n"
      "  <element name=\"fuel\">                     \n"
      "   <ref name=\"incommodity\"/>                \n"
      "   <ref name=\"inrecipe\"/>                   \n"
      "   <ref name=\"outcommodity\"/>               \n"
      "   <ref name=\"outrecipe\"/>                  \n"
      "  </element>                                  \n"
      "  </oneOrMore>                                \n"
      "                                              \n"
      "  <!-- Facility Parameters -->                \n"
      "  <interleave>                                \n"
      "  <element name=\"processtime\">              \n"
      "    <data type=\"nonNegativeInteger\"/>       \n"
      "  </element>                                  \n"
      "  <optional>                                  \n"
      "    <element name =\"orderlookahead\">        \n"
      "      <data type=\"nonNegativeInteger\"/>     \n"
      "    </element>                                \n"
      "  </optional>                                 \n"
      "                                              \n"
      "  <!-- Recipe Changes  -->                    \n"
      "  <optional>                                  \n"
      "  <oneOrMore>                                 \n"
      "  <element name=\"recipe_change\">            \n"
      "   <element name=\"incommodity\">             \n"
      "     <data type=\"string\"/>                  \n"
      "   </element>                                 \n"
      "   <element name=\"new_recipe\">              \n"
      "     <data type=\"string\"/>                  \n"
      "   </element>                                 \n"
      "   <element name=\"time\">                    \n"
      "     <data type=\"nonNegativeInteger\"/>      \n"
      "   </element>                                 \n"
      "  </element>                                  \n"
      "  </oneOrMore>                                \n"
      "  </optional>                                 \n"
      "  </interleave>                               \n"
      "                                              \n"
      "  <!-- Power Production  -->                  \n"
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
      "  </element>                                  \n";
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverter::InitFrom(cyclus::QueryEngine* qe) {  
  cyclus::FacilityModel::InitFrom(qe);
  qe = qe->QueryElement("model/" + ModelImpl());
  
  using boost::lexical_cast;
  using cyclus::Commodity;
  using cyclus::CommodityProducer;
  using cyclus::GetOptionalQuery;
  using cyclus::QueryEngine;
  using std::string;

  // in/out fuel
  int nfuel = qe->NElementsMatchingQuery("commodpair");
  for (int i = 0; i < nfuel; i++) {
    QueryEngine* fuel = qe->QueryElement("commodpair", i);
    std::string in_c = fuel->GetElementContent("incommodity");
    std::string in_r = fuel->GetElementContent("inrecipe");
    std::string out_c = fuel->GetElementContent("outcommodity");
    std::string out_r = fuel->GetElementContent("outrecipe");
    crctx_.AddInCommod(in_c, in_r, out_c, out_r);
  }

  // facility data required
  string data;
  data = qe->GetElementContent("processtime");
  process_time(lexical_cast<int>(data));

  // facility data optional
  int time;
  time = GetOptionalQuery<int>(qe, "orderlookahead", preorder_time());
  preorder_time(time);

  int n;
  n = GetOptionalQuery<int>(qe, "norder", n_reserves());
  n_reserves(n);

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
cyclus::Model* CommodConverter::Clone() {
  CommodConverter* m = new CommodConverter(context());
  m->InitFrom(this);
  return m;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverter::InitFrom(CommodConverter* m) {
  FacilityModel::InitFrom(m);
  
  // in/out commodity & resource context
  crctx_ = m->crctx_;
  
  // facility params
  process_time(m->process_time());
  preorder_time(m->preorder_time());
  n_load(m->n_load());
  n_reserves(m->n_reserves());

  // commodity production
  CopyProducedCommoditiesFrom(m);

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string CommodConverter::str() {
  std::stringstream ss;
  ss << cyclus::FacilityModel::str();
  ss << " has facility parameters {" << "\n"
     << "     Process Time = " << process_time() << ",\n"
     << "     Preorder Time = " << preorder_time() << ",\n"
     << "     Commods To Reserve = " << n_reserves() << ",\n"
     << "'}";
  return ss.str();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverter::Deploy(cyclus::Model* parent) {
  using cyclus::Material;

  FacilityModel::Deploy(parent);
  phase(INITIAL);
  std::string rec = crctx_.in_recipe(*crctx_.in_commods().begin());
  //<nix?> This seems to create a material... of 0 mass?... just placeholder?
  spillover_ = Material::Create(this, 0.0, context()->GetRecipe(rec));

  LOG(cyclus::LEV_DEBUG2, "ComCnv") << "Commod Converter entering the simuluation";
  LOG(cyclus::LEV_DEBUG2, "ComCnv") << str();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverter::Tick(int time) {
  LOG(cyclus::LEV_INFO3, "ComCnv") << name() << " is ticking at time "
                                   << time << " {";
                                    
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "Current facility parameters for "
                                    << name()
                                    << " at the beginning of the tick are:";
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    Phase: " << phase_names_[phase_]; 
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    Start time: " << start_time_;
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    End time: " << end_time();  
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    Order time: " << order_time();  
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    NReserves: " << reserves_.count();
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    NProcessing: " << processing_.count();  
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    NStocks: " << StocksCount();  
  //<nix?> I think spillover is unnecessary
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    Spillover Qty: " << spillover_->quantity();  

  if (context()->time() == FacLifetime()) {
    int nprocessing = processing_.count();
    LOG(cyclus::LEV_DEBUG1, "ComCnv") << "lifetime reached, dumping:"
                                      << nprocessing << " commods.";
    for (int i = 0; i < nprocessing; i++) {
      Convert_(); // unload
    }
  } else {
    switch (phase()) {
      case WAITING:
        if (n_processing() > 0 &&
            to_begin_time() <= context()->time()) {
          phase(PROCESS);
        } 
        break;
        
      case INITIAL:
        // special case for a processing primed to go
        if (n_processing() == n_batches()) phase(PROCESS);
        break;
    }
  }

  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "Current facility parameters for "
                                    << name()
                                    << " at the end of the tick are:";
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    Phase: " << phase_names_[phase_]; 
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    Start time: " << start_time_;
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    End time: " << end_time();  
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    Order time: " << order_time();  
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    NReserves: " << reserves_.count();
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    NProcessing: " << processing_.count();  
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    NStocks: " << StocksCount();  
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    Spillover Qty: " << spillover_->quantity();  
  LOG(cyclus::LEV_INFO3, "ComCnv") << "}";
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverter::Tock(int time) {
  LOG(cyclus::LEV_INFO3, "ComCnv") << name() << " is tocking {";
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "Current facility parameters for "
                                    << name()
                                    << " at the beginning of the tock are:";
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    Phase: " << phase_names_[phase_]; 
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    Start time: " << start_time_;
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    End time: " << end_time();  
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    Order time: " << order_time();  
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    NReserves: " << reserves_.count();
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    NProcessing: " << processing_.count();  
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    NStocks: " << StocksCount();  
  LOG(cyclus::LEV_DEBUG4, "ComCnv") << "    Spillover Qty: " << spillover_->quantity();  
  
  switch (phase()) {
    case PROCESS:
      if (time == end_time()) {
        for (int i = 0; i < n_load(); i++) {
          Convert_(); // place processing into stocks
        }
        BeginProcessing_(); // place reserves into processing
        phase(WAITING);
      }
      break;
    default:
      BeginProcessing_(); // always try to place reserves into processing 
      break;
  }

  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "Current facility parameters for "
                                    << name()
                                    << " at the end of the tock are:";
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    Phase: " << phase_names_[phase_]; 
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    Start time: " << start_time_;
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    End time: " << end_time();  
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    Order time: " << order_time();  
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    NReserves: " << reserves_.count();
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    NProcessing: " << processing_.count();  
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    NStocks: " << StocksCount();  
  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    Spillover Qty: " << spillover_->quantity();  
  LOG(cyclus::LEV_INFO3, "ComCnv") << "}";
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr>
CommodConverter::GetMatlRequests() {
  using cyclus::RequestPortfolio;
  using cyclus::Material;
  
  std::set<RequestPortfolio<Material>::Ptr> set;
  double order_size;

  switch (phase()) {
    // by default, this facility requests as much incommodity as there is capacity for.
    // maybe the only exception should be when we're decommissioning...
    case DECOMM:
      break;
    default:
      order_size = n_batches() * batch_size()
                   - processing_.quantity() - reserves_.quantity()
                   - spillover_->quantity();
      if (preorder_time() == 0) order_size += batch_size() * n_reserves();
      if (order_size > 0) {
        RequestPortfolio<Material>::Ptr p = GetOrder_(order_size);
        set.insert(p);
      }
      break;
  }

  return set;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverter::AcceptMatlTrades(
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
CommodConverter::GetMatlBids(const cyclus::CommodMap<cyclus::Material>::type&
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
void CommodConverter::GetMatlTrades(
    const std::vector< cyclus::Trade<cyclus::Material> >& trades,
    std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                          cyclus::Material::Ptr> >& responses) {
  using cyclus::Material;
  using cyclus::Trade;

  std::vector< cyclus::Trade<cyclus::Material> >::const_iterator it;
  for (it = trades.begin(); it != trades.end(); ++it) {
    LOG(cyclus::LEV_INFO5, "ComCnv") << name() << " just received an order.";

    std::string commodity = it->request->commodity();
    double qty = it->amt;
    Material::Ptr response = TradeResponse_(qty, &stocks_[commodity]);

    responses.push_back(std::make_pair(*it, response));
    LOG(cyclus::LEV_INFO5, "CommodConverter") << name()
                                           << " just received an order"
                                           << " for " << qty
                                           << " of " << commodity;
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CommodConverter::StocksCount() {
  int count = 0;
  std::map<std::string, cyclus::ResourceBuff>::const_iterator it;
  for (it = stocks_.begin(); it != stocks_.end(); ++it) {
    count += it->second.count();
  }
  return count;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverter::phase(CommodConverter::Phase p) {
  LOG(cyclus::LEV_DEBUG2, "ComCnv") << "CommodConverter " << name()
                                    << " is changing phases -";
  LOG(cyclus::LEV_DEBUG2, "ComCnv") << "  * from phase: " << phase_names_[phase_];
  LOG(cyclus::LEV_DEBUG2, "ComCnv") << "  * to phase: " << phase_names_[p];
  
  switch (p) {
    case PROCESS:
      start_time(context()->time());
  }
  phase_ = p;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverter::Refuel_() {
  while(n_processing() < n_batches() && reserves_.count() > 0) {
    BeginProcessing_();
    if(n_processing() == n_batches()) {
      to_begin_time_ = start_time_ + process_time_ + refuel_time_;
    }
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverter::BeginProcessing_() {
  LOG(cyclus::LEV_DEBUG2, "ComCnv") << "CommodConverter " << name() << " added"
                                    <<  " a batch from its processing.";
  try {
    processing_.Push(reserves_.Pop());
  } catch(cyclus::Error& e) {
      e.msg(Model::InformErrorMsg(e.msg()));
      throw e;
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverter::Convert_() {
  using cyclus::Material;
  using cyclus::ResCast;
  
  LOG(cyclus::LEV_DEBUG2, "ComCnv") << "CommodConverter " << name() << " removed"
                                    <<  " a batch from its processing.";
  try {
    Material::Ptr mat = ResCast<Material>(processing_.Pop());
    std::string incommod = crctx_.commod(mat);
    assert(incommod != "");
    std::string outcommod = crctx_.out_commod(incommod);
    assert(outcommod != "");
    std::string outrecipe = crctx_.out_recipe(crctx_.in_recipe(incommod));
    assert(outrecipe != "");
    mat->Transmute(context()->GetRecipe(outrecipe));
    crctx_.UpdateRsrc(outcommod, mat);
    stocks_[outcommod].Push(mat);
  } catch(cyclus::Error& e) {
      e.msg(Model::InformErrorMsg(e.msg()));
      throw e;
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::RequestPortfolio<cyclus::Material>::Ptr
CommodConverter::GetOrder_(double size) {
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
    port->AddRequest(mat, this, *it, commod_prefs_[*it]);
    
    LOG(cyclus::LEV_DEBUG3, "ComCnv") << "CommodConverter " << name()
                                      << " is making an order:";
    LOG(cyclus::LEV_DEBUG3, "ComCnv") << "          size: " << size;
    LOG(cyclus::LEV_DEBUG3, "ComCnv") << "     commodity: " << *it;
    LOG(cyclus::LEV_DEBUG3, "ComCnv") << "    preference: "
                                      << commod_prefs_[*it];
  }

  CapacityConstraint<Material> cc(size);
  port->AddConstraint(cc);

  return port;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverter::AddCommods_(std::string commod, cyclus::Material::Ptr mat) {
  using cyclus::Material;
  using cyclus::ResCast;

  LOG(cyclus::LEV_DEBUG3, "ComCnv") << "CommodConverter " << name()
                                    << " is adding " << mat->quantity()
                                    << " of material to its reserves.";

  // this is a hack! whatever *was* left in spillover now magically becomes this
  // new commodity. we need to do something different (maybe) for recycle.
  spillover_->Absorb(mat);
  
  while (!cyclus::IsNegative(spillover_->quantity() - batch_size())) {
    Material::Ptr batch;
    // this is a hack to deal with close-to-equal issues between batch size and
    // the amount of fuel in spillover
    if (spillover_->quantity() >= batch_size()) {
      batch = spillover_->ExtractQty(batch_size());
    } else {
      batch = spillover_->ExtractQty(spillover_->quantity());
    }
    assert(commod != "");
    crctx_.AddRsrc(commod, batch);
    reserves_.Push(batch);    
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::BidPortfolio<cyclus::Material>::Ptr CommodConverter::GetBids_(
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
cyclus::Material::Ptr CommodConverter::TradeResponse_(
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
void CommodConverter::SetUpPhaseNames_() {
  phase_names_.insert(std::make_pair(INITIAL, "initialization"));
  phase_names_.insert(std::make_pair(PROCESS, "processing batch(es)"));
  phase_names_.insert(std::make_pair(WAITING, "waiting for fuel"));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Model* ConstructCommodConverter(cyclus::Context* ctx) {
  return new CommodConverter(ctx);
}

} // namespace cycamore
