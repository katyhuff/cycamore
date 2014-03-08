// commod_converter.h
#ifndef CYCAMORE_MODELS_COMMODCONVERTER _COMMOD_CONVERTER _H_
#define CYCAMORE_MODELS_COMMODCONVERTER _COMMOD_CONVERTER _H_

#include <map>
#include <queue>
#include <string>

#include "bid_portfolio.h"
#include "capacity_constraint.h"
#include "commodity_producer.h"
#include "commodity_recipe_context.h"
#include "enrichment.h"
#include "exchange_context.h"
#include "facility_model.h"
#include "material.h"
#include "request_portfolio.h"
#include "resource_buff.h"

// forward declarations
namespace cycamore {
class CommodConverter;
} // namespace cycamore
namespace cyclus {  
class Context;
} // namespace cyclus

namespace cycamore {

/// @class CommodConverter
///
/// @section introduction Introduction
/// The CommodConverter is a facility that receives commodities, holds onto them  
/// for some number of months, offers them to the market of the new commodity. It 
/// has three  stocks areas which hold commods of commodities: reserves, 
/// processing, and  stocks. Incoming commodity orders are placed into reserves, 
/// from which the  processing area is populated. When a process (some number of 
/// months spent waiting)  has been completed, the commodity is converted and 
/// moved into stocks. Requests for  commodities are bid upon based on the state 
/// of the commodities in the stocks.  
///
/// The CommodConverter can manage multiple input-output commodity pairs, and keeps
/// track of the pair that each resource belongs to. Resources move through the
/// system independently of their input/output commodity types, but when they
/// reach the stocks area, they are offered as bids depedent on their output
/// commodity type.
///
/// @section params Parameters
/// A CommodConverter has the following tuneable parameters:
///   #. batch_size : the size of commods <nix>
///   #. n_commods : the number of commods that constitute a full processing <nix>
///   #. process_time : the number of timesteps a batch process takes
///   #. n_load : the number of commods processed at any given time (i.e.,
///   n_load is unloaded and reloaded after a process is finished <nix?>
///   #. n_reserves : the preferred number of commods in reserve <nix>
///   #. refuel_time : the number of timesteps required to reload the processing after
///   a process has finished <0>
/// 
/// The CommodConverter also maintains a cyclus::CommodityRecipeContext, which
/// allows it to track incommodity-inrecipe/outcommodity-outrecipe groupings.
/// <keep?>
/// 
/// @section operation Operation  
/// After a CommodConverter enters the simulation, it will begin requesting all 
/// incommodities.
///
/// As soon as it receives a commodity, that commodity is placed in the 
/// processing storage area. 
/// 
/// On the tick of the timestep in which that incommodity's time is up, it is 
/// converted to the outcommodity type, by simply changing the commodity name. 
/// Then, it is offered to the  outcommodity market.
/// 
/// This happens continuously, in each timestep. That is, this facility is 
/// greedy. It seeks to collect as much of the incommodity as possible and to 
/// eject as much of the outcommodity as possible. 
/// 
/// @section end End of Life
/// If the current time step is equivalent to the facility's lifetime, the
/// reactor will move all material in its processing to its stocks containers, 
/// converted or not.
/// 
/// @section requests Requests
/// A CommodConverter will make as many requests as it has possible input
/// commodities. It provides a constraint based on a total request amount
/// determined by its processing capacity and n_reserves parameters. The
/// n_reserves parameter allows modelers to order fuel in advance of when it is
/// needed. The order size is capacity +  n_reserves/process_time?
///
/// @section bids Bids
/// A CommodConverter will bid on any request for any of its out_commodities, as
/// long as there is a positive quantity of material in its stocks area
/// associated with that output commodity.
///
/// @section ics Initial Conditions
/// A CommodConverter can be deployed with any number of commods in its reserve,
/// processing, and stocks buffers. Recipes and commodities for each of these batch
/// groupings must be specified.
///
/// @todo add decommissioning behavior if material is still in stocks
///
/// @warning preference time changing is based on *full simulation time*, not
/// relative time
/// @warning the reactor's commodity context *can not* current remove resources
/// reliably because of the implementation of ResourceBuff::PopQty()'s
/// implementation. Resource removal from the context requires pointer equality
/// in order to remove material, and PopQty will split resources, making new
/// pointers.
/// @warning the reactor uses a hackish way to input materials into its
/// reserves. See the AddBatches_ member function.
class CommodConverter : public cyclus::FacilityModel,
      public cyclus::CommodityProducer {
 public:
  /// @brief defines all possible phases this facility can be in
  enum Phase {
    INITIAL, ///< The initial phase, after the facility is built but before it is
             /// filled
    PROCESS, ///< The processing phase, which this facility should be in all the time.
    WAITING, ///< The waiting phase, while the factility has nothing in its reserves
  };

  /// @brief a struct for initial conditions
  struct InitCond {
   InitCond() : reserves(false), processing(false), stocks(false) {};

    void AddReserves(int n, std::string rec, std::string commod) {
      reserves = true;
      n_reserves = n;
      reserves_rec = rec;
      reserves_commod = commod;
    }

    void AddCore(int n, std::string rec, std::string commod) {
      processing = true;
      n_processing = n;
      processing_rec = rec;
      processing_commod = commod;
    }

    void AddStocks(int n, std::string rec, std::string commod) {
      stocks = true;
      n_stocks = n;
      stocks_rec = rec;
      stocks_commod = commod;
    }

    bool reserves;
    int n_reserves;
    std::string reserves_rec;
    std::string reserves_commod;

    bool processing;
    int n_processing;
    std::string processing_rec;
    std::string processing_commod;

    bool stocks;
    int n_stocks;
    std::string stocks_rec;
    std::string stocks_commod;
  };
  
  /* --- Module Members --- */
  /// @param ctx the cyclus context for access to simulation-wide parameters
  CommodConverter(cyclus::Context* ctx);
  
  virtual ~CommodConverter();
  
  virtual cyclus::Model* Clone();
  
  virtual std::string schema();

  /// Initialize members related to derived module class
  /// @param qe a pointer to a cyclus::QueryEngine object containing
  /// initialization data
  virtual void InitFrom(cyclus::QueryEngine* qe);

  /// initialize members from a different model
  void InitFrom(CommodConverter* m);
  
  /// Print information about this model
  virtual std::string str();
  /* --- */

  /* --- Facility Members --- */
  /// perform module-specific tasks when entering the simulation 
  virtual void Deploy(cyclus::Model* parent = NULL);
  /* --- */

  /* --- Agent Members --- */  
  /// The Tick function specific to the CommodConverter.
  /// @param time the time of the tick
  virtual void Tick(int time);
  
  /// The Tick function specific to the CommodConverter.
  /// @param time the time of the tock
  virtual void Tock(int time);
  
  /// @brief The CommodConverter requests Materials of its given
  /// commodity. 
  virtual std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr>
      GetMatlRequests();

  /// @brief The CommodConverter requests GenericResources of its given 
  /// commodity. 
  virtual std::set<cyclus::RequestPortfolio<cyclus::GenericResource>::Ptr>
      GetGenRsrcRequests();

  /// @brief The CommodConverter place accepted trade Materials in their
  /// Inventory
  virtual void AcceptMatlTrades(
      const std::vector< std::pair<cyclus::Trade<cyclus::Material>,
      cyclus::Material::Ptr> >& responses);
  
  /// @brief The CommodConverter place accepted trade GenericResources in 
  //their Inventory 
  virtual void AcceptGenRsrcTrades(
      const std::vector< std::pair<cyclus::Trade<cyclus::GenericResource>,
      cyclus::GenericResource::Ptr> >& responses);
  
  /// @brief Responds to each request for this facility's commodity.  If a given
  /// request is more than this facility's inventory or SWU capacity, it will
  /// offer its minimum of its capacities.
  virtual std::set<cyclus::BidPortfolio<cyclus::Material>::Ptr>
      GetMatlBids(const cyclus::CommodMap<cyclus::Material>::type&
                  commod_requests);
  
  /// @brief Responds to each request for this facility's commodity.  
  virtual std::set<cyclus::BidPortfolio<cyclus::GenericResource>::Ptr>
      GetGenRsrcBids(const cyclus::CommodMap<cyclus::GenericResource>::type&
                  commod_requests);
  
  /// @brief respond to each trade with a material based on the recipe
  ///
  /// @param trades all trades in which this trader is the supplier
  /// @param responses a container to populate with responses to each trade
  virtual void GetMatlTrades(
    const std::vector< cyclus::Trade<cyclus::Material> >& trades,
    std::vector<std::pair<cyclus::Trade<cyclus::Material>,
    cyclus::Material::Ptr> >& responses);

  /// @brief respond to each trade with a generic resource based
  /// level given this facility's inventory
  ///
  /// @param trades all trades in which this trader is the supplier
  /// @param responses a container to populate with responses to each trade
  virtual void GetGenRsrcTrades(
    const std::vector< cyclus::Trade<cyclus::GenericResource> >& trades,
    std::vector<std::pair<cyclus::Trade<cyclus::GenericResource>,
    cyclus::GenericResource::Ptr> >& responses);
  /* --- */

  /* --- CommodConverter Members --- */
  /// @return the total number of commods in stocks
  int StocksCount();
  
  /// @brief the processing time required for a full batch process before
  /// refueling
  inline void process_time(int t) { process_time_ = t; }
  inline int process_time() const { return process_time_; }
  
  /// @brief the time it takes to refuel
  inline void refuel_time(int t) { refuel_time_ = t; }
  inline int refuel_time() const { return refuel_time_; }
  
  /// @brief the starting time of the last (current) process
  inline void start_time(int t) { start_time_ = t; }
  inline int start_time() const { return start_time_; }

  /// @brief the ending time of the last (current) process
  /// @warning the - 1 is to ensure that a 1 period process time that begins on
  /// the tick ends on the tock
  inline int end_time() const { return start_time() + process_time() - 1; }

  /// @brief the beginning time for the next phase, set internally
  inline int to_begin_time() const { return to_begin_time_; }

  /// @brief the number of commods in a full reactor
  inline void n_commods(int n) { n_commods_ = n; }
  inline int n_commods() const { return n_commods_; }

  /// @brief the number of commods in reactor refuel loading/unloading
  inline void n_load(int n) { n_load_ = n; }
  inline int n_load() const { return n_load_; }

  /// @brief the preferred number of fresh fuel commods to keep in reserve
  inline void n_reserves(int n) { n_reserves_ = n; }
  inline int n_reserves() const { return n_reserves_; }

  /// @brief the number of commods currently in the reactor
  inline int n_processing() const { return processing_.count(); }

  /// @brief this facility's commodity-recipe context
  inline void crctx(const cyclus::CommodityRecipeContext& crctx) {
    crctx_ = crctx;
  }
  inline cyclus::CommodityRecipeContext crctx() const { return crctx_; }

  /// @brief the current phase
  void phase(Phase p);
  inline Phase phase() const { return phase_; }

  /// @brief this facility's preference for input commodities
  inline void commod_prefs(const std::map<std::string, double>& prefs) {
    commod_prefs_ = prefs;
  }
  inline const std::map<std::string, double>& commod_prefs() const {
    return commod_prefs_;
  }

 protected:
  /// @brief moves a batch from processing_ to stocks_
  virtual void Convert_();

  /// @brief gets bids for a commodity from a buffer
  cyclus::BidPortfolio<cyclus::Material>::Ptr GetBids_(
      const cyclus::CommodMap<cyclus::Material>::type& commod_requests,
      std::string commod,
      cyclus::ResourceBuff* buffer);
  
  /// @brief returns a qty of material from the a buffer
  cyclus::Material::Ptr TradeResponse_(
      double qty,
      cyclus::ResourceBuff* buffer);
  
  /// @brief a cyclus::ResourceBuff for material while they are inside the processing,
  /// with all materials guaranteed to be of batch_size_
  cyclus::ResourceBuff processing_;

  /// @brief a cyclus::ResourceBuff for material once they leave the processing.
  /// there is one stocks for each outcommodity
  /// @warning no guarantee can be made to the size of each item in stocks_, as
  /// requests can be met that are larger or smaller than batch_size_
  std::map<std::string, cyclus::ResourceBuff> stocks_;

 private:
  /// @brief Processes until reserves_ is out of commods.
  /// The Phase is set to PROCESS.
  void EmptyReserves_();

  /// @brief moves a batch from reserves_ to processing_
  void BeginProcessing_();
  
  /// @brief construct a request portfolio for an order of a given size
  cyclus::RequestPortfolio<cyclus::Material>::Ptr GetOrder_(double size);

  /// @brief Add a blob of incoming material to reserves_
  ///
  /// The last material to join reserves_ is first investigated to see if it is
  /// of batch_size_. If not, material from mat is added to it and it is
  /// returned to reserves_. If more material remains, chunks of batch_size_ are
  /// removed and added to reserves_. The final chunk may be <= batch_size_.
  void AddBatches_(std::string commod, cyclus::Material::Ptr mat);
  
  /// @brief adds phase names to phase_names_ map
  void SetUpPhaseNames_();
  
  static std::map<Phase, std::string> phase_names_;
  int process_time_;
  int start_time_;
  int to_begin_time_;
  int n_reserves_;
  Phase phase_;
  
  InitCond ics_;

  cyclus::CommodityRecipeContext crctx_;
  
  /// @warning as is, the int key is **simulation time**, i.e., context()->time
  /// @brief allows only commods to enter reserves_
  cyclus::Material::Ptr spillover_;
  
  /// @brief a cyclus::ResourceBuff for resources before they enter the processing,
  /// with all materials guaranteed to be of batch_size_
  cyclus::ResourceBuff reserves_;

  friend class CommodConverterTest;
  /* --- */
};

} // namespace cycamore

#endif // CYCAMORE_MODELS_COMMODCONVERTER _COMMOD_CONVERTER _H_
