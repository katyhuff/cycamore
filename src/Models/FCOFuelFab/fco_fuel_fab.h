// fco_fuel_fab.h
#ifndef CYCAMORE_MODELS_FCOFUELFAB_FCO_FUEL_FAB_H_
#define CYCAMORE_MODELS_FCOFUELFAB_FCO_FUEL_FAB_H_

#include <map>
#include <queue>
#include <string>

#include "bid_portfolio.h"
#include "capacity_constraint.h"
#include "commodity_producer.h"
#include "commodity_recipe_context.h"
#include "exchange_context.h"
#include "facility_model.h"
#include "material.h"
#include "request_portfolio.h"
#include "resource_buff.h"

// forward declarations
namespace cycamore {
class FCOFuelFab;
} // namespace cycamore
namespace cyclus {  
class Context;
} // namespace cyclus

namespace cycamore {

/// @class FCOFuelFab
///
/// @section introduction Introduction
/// The FCOFuelFab is a facility that receives commodities, holds onto them  
/// for some number of months, offers them to the market of the new commodity. It 
/// has three  stocks areas which hold commods of commodities: reserves, 
/// processing, and  stocks. Incoming commodity orders are placed into reserves, 
/// from which the  processing area is populated. When a process (some number of 
/// months spent waiting)  has been completed, the commodity is converted and 
/// moved into stocks. Requests for  commodities are bid upon based on the state 
/// of the commodities in the stocks.  
///
/// The FCOFuelFab can manage multiple input-output commodity pairs, and keeps
/// track of the pair that each resource belongs to. Resources move through the
/// system independently of their input/output commodity types, but when they
/// reach the stocks area, they are offered as bids depedent on their output
/// commodity type.
///
/// @section params Parameters
/// A FCOFuelFab has the following tuneable parameters:
///   #. n_commods : the number of commods that constitute a full processing <nix>
///   #. process_time : the number of timesteps a conversion process takes
///   #. refuel_time : the number of timesteps required to reload the processing after
///   a process has finished <0>
/// 
/// The FCOFuelFab also maintains a cyclus::CommodityRecipeContext, which
/// allows it to track incommodity-inrecipe/outcommodity-outrecipe groupings.
/// <keep?>
/// 
/// @section operation Operation  
/// After a FCOFuelFab enters the simulation, it will begin requesting all 
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
/// A FCOFuelFab will make as many requests as it has possible input
/// commodities. It provides a constraint based on a total request amount
/// determined by its processing capacity.
///
/// @section bids Bids
/// A FCOFuelFab will bid on any request for any of its out_commodities, as
/// long as there is a positive quantity of material in its stocks area
/// associated with that output commodity.
///
/// @section ics Initial Conditions
/// A FCOFuelFab can be deployed with any number of commods in its reserve,
/// processing, and stocks buffers. Recipes and commodities for each of these
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
/// reserves. See the AddCommods_ member function.
class FCOFuelFab : public cyclus::FacilityModel,
      public cyclus::CommodityProducer {
 public:
  /// @brief defines all possible phases this facility can be in
  enum Phase {
    INITIAL, ///< The initial phase, after the facility is built but before it is
             /// filled
    PROCESS, ///< The processing phase, which this facility should be in all the time.
    WAITING, ///< The waiting phase, while the factility has nothing in its reserves
  };

  /* --- Module Members --- */
  /// @param ctx the cyclus context for access to simulation-wide parameters
  FCOFuelFab(cyclus::Context* ctx);
  
  virtual ~FCOFuelFab();
  
  virtual cyclus::Model* Clone();
  
  virtual std::string schema();

  /// Initialize members related to derived module class
  /// @param qe a pointer to a cyclus::QueryEngine object containing
  /// initialization data
  virtual void InitFrom(cyclus::QueryEngine* qe);

  /// initialize members from a different model
  void InitFrom(FCOFuelFab* m);
  
  /// Print information about this model
  virtual std::string str();
  /* --- */

  /* --- Facility Members --- */
  /// perform module-specific tasks when entering the simulation 
  virtual void Deploy(cyclus::Model* parent = NULL);
  /* --- */

  /* --- Agent Members --- */  
  /// The Tick function specific to the FCOFuelFab.
  /// @param time the time of the tick
  virtual void Tick(int time);
  
  /// The Tick function specific to the FCOFuelFab.
  /// @param time the time of the tock
  virtual void Tock(int time);
  
  /// @brief The FCOFuelFab requests Materials of its given
  /// commodity. 
  virtual std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr>
      GetMatlRequests();

  /// @brief The FCOFuelFab place accepted trade Materials in their
  /// Inventory
  virtual void AcceptMatlTrades(
      const std::vector< std::pair<cyclus::Trade<cyclus::Material>,
      cyclus::Material::Ptr> >& responses);
  
  /// @brief Responds to each request for this facility's commodity.  If a given
  /// request is more than this facility's inventory orcapacity, it will
  /// offer its minimum of its capacities.
  virtual std::set<cyclus::BidPortfolio<cyclus::Material>::Ptr>
      GetMatlBids(const cyclus::CommodMap<cyclus::Material>::type&
                  commod_requests);
  
  /// @brief respond to each trade with a material based on the recipe
  ///
  /// @param trades all trades in which this trader is the supplier
  /// @param responses a container to populate with responses to each trade
  virtual void GetMatlTrades(
    const std::vector< cyclus::Trade<cyclus::Material> >& trades,
    std::vector<std::pair<cyclus::Trade<cyclus::Material>,
    cyclus::Material::Ptr> >& responses);
  /* --- */

  /* --- FCOFuelFab Members --- */
  /// @brief prints the status of the facility
  /// @param when gives text to insert (i.e. "at the beginning of the tock")
  void PrintStatus(std::string when);

  /// @return the total number of commods in processing
  int ProcessingCount();

  /// @return the total quantity of commods in reserves
  double ReservesQty_();

  /// @return the total number of commods in stocks
  int StocksCount();

  /// @brief the processing time required for a full process
  inline void process_time(int t) { process_time_ = t; }
  inline int process_time() const { return process_time_; }
  
  /// @brief the name of the goal out recipe
  inline void out_recipe(std::string s) { out_recipe_ = s; }
  inline std::string out_recipe() const { return out_recipe_; }
  
  /// @brief the maximum amount in processing at a single time
  inline void capacity(double c) { capacity_ = c; }
  inline int capacity() const { return capacity_; }
  
  /// @brief this facility's commodity-recipe context
  inline void crctx(const cyclus::CommodityRecipeContext& crctx) {
    crctx_ = crctx;
  }
  inline cyclus::CommodityRecipeContext crctx() const { return crctx_; }

  /// @brief the current phase
  void phase(Phase p);
  inline Phase phase() const { return phase_; }

 protected:
  /// @brief moves commodities from processing_ to stocks_
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
  
  /// @brief a cyclus::ResourceBuff for material while they are processing
  /// there is one processing buffer for each processing start time and incommod
  std::map<int, std::map< std::string, cyclus::ResourceBuff > > processing_;

  /// @brief a cyclus::ResourceBuff for material once they are done processing.
  /// there is one stocks for each outcommodity
  /// @warning no guarantee can be made to the size of each item in stocks_
  std::map<std::string, cyclus::ResourceBuff> stocks_;

 private:
  /// @brief Processes until reserves_ is out of commods.
  /// The Phase is set to PROCESS.
  void EmptyReserves_();

  /// @brief moves everything from reserves_ to processing_
  /// and, for each object, adds a start_time to the list of start times
  void BeginProcessing_();

  /// @brief takes action appropriate for the tick on the last timestep
  /// @TODO check that this does the right stuff.
  void EndLife_();
  
  /// @brief calculates goal material composition
  cyclus::Composition::Ptr GoalComp_();
  
  /// @brief sorts through the processing buffer to meet the need 
  void MeetNeed_();
  
  /// @brief construct a request portfolio for an order of a given size
  cyclus::RequestPortfolio<cyclus::Material>::Ptr GetOrder_(double size);

  /// @brief Add a blob of incoming material to reserves_
  void AddCommods_(std::string commod, cyclus::Material::Ptr mat);
  
  /// @brief adds phase names to phase_names_ map
  void SetUpPhaseNames_();

  /// @brief for materials that are now ready, determines the time received 
  int Ready_();
  
  static std::map<Phase, std::string> phase_names_;
  int process_time_;
  double capacity_;
  Phase phase_;

  /// @brief the name of the goal recipe
  std::string out_recipe_;
  cyclus::CommodityRecipeContext crctx_;
  
  /// @brief a map from commods to cyclus::ResourceBuff for resources before 
  /// they enter processing
  std::map<std::string, cyclus::ResourceBuff> reserves_;

  friend class FCOFuelFabTest;
  /* --- */
};

} // namespace cycamore

#endif // CYCAMORE_MODELS_FCOFUELFAB_FCO_FUEL_FAB_H_
