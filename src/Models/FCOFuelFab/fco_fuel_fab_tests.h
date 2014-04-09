#ifndef CYCAMORE_MODELS_FCOFUELFAB_FCO_FUEL_FAB_TESTS_
#define CYCAMORE_MODELS_FCOFUELFAB_FCO_FUEL_FAB_TESTS_

#include <gtest/gtest.h>

#include <map>
#include <string>

#include "test_context.h"

#include "fco_fuel_fab.h"

namespace cycamore {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class FCOFuelFabTest : public ::testing::Test {
 protected:
  cyclus::TestContext tc_;
  FCOFuelFab* src_facility;

  // init params
  std::string in_c1, in_c2, out_c1, out_c2;
  std::string in_r1, in_r2, out_r1, out_r2;
  cyclus::CommodityRecipeContext crctx;

  int process_time;

  std::string commodity;
  double capacity, cost;

  // init conds
  std::string rsrv_c, rsrv_r, core_c, core_r, stor_c, stor_r;
  int rsrv_n, core_n, stor_n;
  
  virtual void SetUp();
  virtual void TearDown();
  void InitParameters();
  void SetUpSourceFacility();

  /// @brief tests the initial state of a facility
  void TestInitState(FCOFuelFab* fac);

  /// @brief tests the number of batches in each buffer
  void TestBuffs(int nreserves, int ncore, int nstorage);
  
  /// @brief tests the FCOFuelFab's reserves_, by calling AddBatches_(mat),
  /// and confirming that there are n items and the last item has quantity qty
  void TestReserveBatches(cyclus::Material::Ptr mat, std::string commod,
                          int n, double qty);

  /// @brief calls MoveBatchIn_ and tests that the number of objects in core_ is
  /// n_core and the number of objects in reserves_ is n_reserves
  void TestBatchIn(int n_core, int n_reserves);
      
  /// @brief calls MoveBatchOut_ and tests that the number of objects in core_ is
  /// n_core and the number of objects in storage_ is n_storage
  void TestBatchOut(int n_core, int n_storage);
};

} // namespace cycamore

#endif // CYCAMORE_MODELS_FCOFUELFAB_FCO_FUEL_FAB_TESTS_
