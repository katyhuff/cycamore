// supply_deploy_inst.h
#ifndef _SUPPLYDEPLOYINST_H
#define _SUPPLYDEPLOYINST_H

#include "inst_model.h"
#include "builder.h"
#include "commodity_producer_manager.h"
#include "commodity_producer.h"
#include "supply_demand_manager.h"

#include <utility>
#include <set>
#include <map>

namespace cycamore {

/**
   @class SupplyDeployInst
   The SupplyDeployInst class inherits from the InstModel
   class and is dynamically loaded by the Model class when requested.

   This model implements a simple institution model that decomissions
   facilities according to a commodity availability rule specified in the input 
   file. Then, it optionally replaces those facilities with some number of 
   another prototype.
 */
class SupplyDeployInst : public cyclus::InstModel {
  /* --------------------
   * all MODEL classes have these members
   * --------------------
   */
 public:
  /**
     Default constructor
   */
  SupplyDeployInst(cyclus::Context* ctx);

  /**
     Destructor
   */
  virtual ~SupplyDeployInst();

  virtual std::string schema();

  virtual cyclus::Model* Clone() {
    SupplyDeployInst* m = new SupplyDeployInst(context());
    m->InitFrom(this);
    return m;
  }

  /**
     initialize members from a different model
  */
  void InitFrom(SupplyDeployInst* m);

  /**
     Initialize members related to derived module class
     @param qe a pointer to a cyclus::QueryEngine object containing initialization data
   */
  virtual void InitFrom(cyclus::QueryEngine* qe);

  /* ------------------- */

  /* --------------------
   * all INSTMODEL classes have these members
   * --------------------
   */
 public:
  /**
     tick handling function for this inst
   */
  virtual void Tick(int time);

  /* ------------------- */

  /* --------------------
   * This INSTMODEL class has these members
   * --------------------
   */
  /**
     returns the number of facilities to decommission
     based on the rule specified in the input
     @param time the time at whichthe decommissioning is happening
   */
  int NumToDecommission(int time);

  /**
     Returns the quantity of a commod that was offered in the last timestep.  
     Note that this should be in the Cyclus toolkit rather than bogarted by 
     this class. 
     @param commod a string, the commodity of interest
     @return a double, the available quantity.
    */
  double QuantityAvailable(std::string commod);

  /// @brief the name of the prototype that this inst decommissions
  inline void to_decomm(std::string to_decomm) { to_decomm_ = to_decomm; } 
  inline std::string to_decomm() const { return to_decomm_; } 

  /// @brief the name of the prototype that this inst builds as a replacement
  inline void replacement(std::string replacement) { replacement_ = replacement; } 
  inline std::string replacement() const { return replacement_; } 

  /// @brief the name of the prototype that this inst decommissions
  inline void repl_rate(int repl_rate) { repl_rate_ = repl_rate; } 
  inline int repl_rate() const { return repl_rate_; } 

  /// @brief the quantity of the commod necessary to trigger a decommisioning
  inline void rule_quantity(double rule_quantity) { rule_quantity_ = rule_quantity; } 
  inline double rule_quantity() const { return rule_quantity_; } 

  /// @brief the quantity of the commod necessary to trigger a decommisioning
  inline void rule_commod(std::string rule_commod) { rule_commod_ = rule_commod; } 
  inline std::string rule_commod() const { return rule_commod_; } 

 protected:
  /// manager for supply and demand
  cyclus::SupplyDemandManager sdmanager_;

  /// the list of facility prototypes to decommission
  std::string to_decomm_;

  /// the name of the prototype to replace decommissioned facilities
  std::string replacement_;

  /// the name of the commod of interest in this decommissioning rule
  std::string rule_commod_;

  /// the key quantity demanded by the amount
  double rule_quantity_;

  /// The number of replacement facilities that should be built when a facility 
  /// is decommissioned 
  int repl_rate_;

  /* ------------------- */

};
} // namespace cycamore
#endif
