// Deployinst.h
#ifndef _SUPPLYDEPLOYINST_H
#define _SUPPLYDEPLOYINST_H

#include "inst_model.h"

#include <utility>
#include <set>
#include <map>

namespace cycamore {

typedef std::pair<std::string, int> BuildOrder;

/**
   a helper class for storing and extracting build orders
 */
class SupplyBuildOrderList {
 public:
  /// add a build order
  void AddBuildOrder(std::string prototype, int number, int time);

  /// extract a set of build orders
  std::set<BuildOrder> ExtractOrders(int time);

 private:
  std::map<int, std::set<BuildOrder> > all_orders_;
};

/**
   @class SupplyDeployInst
   The SupplyDeployInst class inherits from the InstModel
   class and is dynamically loaded by the Model class when requested.

   This model implements a simple institution model that deploys
   specific facilities as defined explicitly in the input file.
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
  void InitFrom(SupplyDeployInst* m) {
    cyclus::InstModel::InitFrom(m);
    build_orders_ = m->build_orders_;
  };

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
   * This INSTMODEL classes have these members
   * --------------------
   */
 protected:
  /**
     a collection of orders to build
   */
  SupplyBuildOrderList build_orders_;

  /* ------------------- */

};
} // namespace cycamore
#endif
