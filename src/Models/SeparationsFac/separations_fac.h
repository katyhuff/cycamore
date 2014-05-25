// separations_fac.h
#ifndef CYCAMORE_MODELS_SEPARATIONSFAC_SEPARATIONS_FAC_H_
#define CYCAMORE_MODELS_SEPARATIONSFAC_SEPARATIONS_FAC_H_

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
class SeparationsFac;
} // namespace cycamore
namespace cyclus {  
class Context;
} // namespace cyclus

namespace cycamore {

/// @class SeparationsFac
///
/// @section introduction Introduction
/// The SeparationsFac is a facility that receives commodities, holds onto them  
/// for some number of months, offers them to the market of the new commodity. It 
/// has three  stocks areas which hold commods of commodities: reserves, 
/// processing, and  stocks. Incoming commodity orders are placed into reserves, 
/// from which the  processing area is populated. When a process (some number of 
/// months spent waiting)  has been completed, the commodity is converted and 
/// moved into stocks. Requests for  commodities are bid upon based on the state 
/// of the commodities in the stocks.  
///
/// The SeparationsFac can manage multiple input-output commodity pairs, and keeps
/// track of the pair that each resource belongs to. Resources move through the
/// system independently of their input/output commodity types, but when they
/// reach the stocks area, they are offered as bids depedent on their output
/// commodity type.
///
/// @section params Parameters
/// A SeparationsFac has the following tuneable parameters:
///   #. n_commods : the number of commods that constitute a full processing <nix>
///   #. process_time : the number of timesteps a conversion process takes
/// 
/// The SeparationsFac also maintains a cyclus::CommodityRecipeContext, which
/// allows it to track incommodity-inrecipe/outcommodity-outrecipe groupings.
/// <keep?>
/// 
/// @section operation Operation  
/// After a SeparationsFac enters the simulation, it will begin requesting all 
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
/// A SeparationsFac will make as many requests as it has possible input
/// commodities. It provides a constraint based on a total request amount
/// determined by its processing capacity.
///
/// @section bids Bids
/// A SeparationsFac will bid on any request for any of its out_commodities, as
/// long as there is a positive quantity of material in its stocks area
/// associated with that output commodity.
///
/// @section ics Initial Conditions
/// A SeparationsFac can be deployed with any number of commods in its reserve,
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
class SeparationsFac : public cyclus::FacilityModel,
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
  SeparationsFac(cyclus::Context* ctx);
  
  virtual ~SeparationsFac();
  
  virtual cyclus::Model* Clone();
  
  virtual std::string schema();

  /// Initialize members related to derived module class
  /// @param qe a pointer to a cyclus::QueryEngine object containing
  /// initialization data
  virtual void InitFrom(cyclus::QueryEngine* qe);

  /// initialize members from a different model
  void InitFrom(SeparationsFac* m);
  
  /// Print information about this model
  virtual std::string str();
  /* --- */

  /* --- Facility Members --- */
  /// perform module-specific tasks when entering the simulation 
  virtual void Deploy(cyclus::Model* parent = NULL);
  /* --- */

  /* --- Agent Members --- */  
  /// The Tick function specific to the SeparationsFac.
  /// @param time the time of the tick
  virtual void Tick(int time);
  
  /// The Tick function specific to the SeparationsFac.
  /// @param time the time of the tock
  virtual void Tock(int time);
  
  /// @brief The SeparationsFac requests Materials of its given
  /// commodity. 
  virtual std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr>
      GetMatlRequests();

  /// @brief The SeparationsFac place accepted trade Materials in their
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

  /* --- SeparationsFac Members --- */
  /// @brief prints the status of the facility
  /// @param when gives text to insert (i.e. "at the beginning of the tock")
  void PrintStatus(std::string when);

  /// @return the total number of commods in processing
  int ProcessingCount_();

  /// @return the number of a specific commod in processing
  int ProcessingCount_(std::string commod);

  /// @return the total amt of commods in processing
  int ProcessingQty_();

  /// @return the total quantity of all commods in reserves
  int ReservesCount_();

  /// @return the total quantity of a specific commod in reserves
  int ReservesCount_(std::string commod);

  /// @return the total quantity of commods in reserves
  double ReservesQty_();

  /// @return the total number of commods in stocks
  int StocksCount();

  /// @return the total number of commods of commodtype in stocks
  int StocksCount(std::string commodtype);

  /// @brief the processing time required for a full process
  inline void process_time(int t) { process_time_ = t; }
  inline int process_time() const { return process_time_; }

  /// @brief out recipes
  inline void out_recipes(std::set<std::string> s){ out_recipes_ = s; }
  inline std::set<std::string> out_recipes() const { return out_recipes_; }
  
  /// @brief out commods
  inline void out_commods(std::set<std::string> s) { out_commods_ = s; }
  inline std::set<std::string> out_commods() const { return out_commods_; }
  
  /// @brief the name of the in recipe
  inline void in_recipe(std::string s) { in_recipe_ = s; }
  inline std::string in_recipe() const { return in_recipe_; }
  
  /// @brief the name of the in commod
  inline void in_commod(std::string s) { in_commod_ = s; }
  inline std::string in_commod() const { return in_commod_; }
  
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
  
  /// @brief calculates the total mass of the goal material composition [kg]
  /// @param commod specifies the associated commod type
  double GoalCompMass_(std::string commod);

  /// @brief calculates goal material composition
  /// @param commod specifies the associated commod type
  cyclus::CompMap GoalCompMap_(std::string commod);

  /// @brief calculates goal material composition
  /// @param commod specifies the associated commod type
  cyclus::Composition::Ptr GoalComp_(std::string commod);

  /// @brief sorts through the processing buffer to meet the need 
  /// @param iso the isotope which is needed
  /// @return the number of possible separated mats
  cyclus::ResourceBuff MeetNeed_(int iso, int n);

  /// used by Separate_, this function moves ready mats into the stocks in 
  /// chunks the size of the goal composition 
  /// @param sep_mat_buff is the ResourceBuff holding the materials
  /// @param n_poss is the number of possible goal units in that material
  void MoveToStocks_(cyclus::ResourceBuff sep_mat_buff, int n_poss);

  /// @brief conducts the separation step, separating as much material as 
  /// possible.
  /// @param the out_commod to separate out of the in_commod
  void Separate_(std::string out_commod);

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
  std::map< int, std::set< std::string > > prefs_;

  /// @brief the names of the incoming recipe
  std::string in_recipe_;
  /// @brief the names of the incoming commod
  std::string in_commod_;

  /// @brief the names of the goal recipes
  std::set<std::string> out_recipes_;
  /// @brief the names of the goal commods
  std::set<std::string> out_commods_;

  /// @brief the commodity recipe context keeping track of the commods/recipes
  cyclus::CommodityRecipeContext crctx_;
  
  /// @brief a map from commods to cyclus::ResourceBuff for resources before 
  /// they enter processing
  std::map<std::string, cyclus::ResourceBuff> reserves_;

  friend class SeparationsFacTest;
  /* --- */
};

} // namespace cycamore

#endif // CYCAMORE_MODELS_SEPARATIONSFAC_SEPARATIONS_FAC_H_
