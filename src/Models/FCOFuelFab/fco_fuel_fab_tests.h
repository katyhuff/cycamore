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
  std::string in_c1, in_c2, in_c3, in_c4, out_c1;
  std::string in_r1, in_r2, in_r3, in_r4, out_r1;
  int iso_1, iso_2;
  std::set<std::string> pref_1, pref_2;
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

  /// @brief tests the initial state of a facility
  /// mat is the material to add to the reserves.
  /// mat is of commodity commod
  /// n is the expected number of mats of that commod currently in reserves
  void TestAddCommods(cyclus::Material::Ptr mat, std::string commod, int n);

  /// @brief calls BeginProcessing and tests that 
  /// the number of objects of commod type commod 
  /// in reserves is n_reserves
  /// in processing is n_processing
  /// and the number of objects in stocks_ is n_stocks
  void TestBeginProcessing(int n_reserves, int n_processing, int n_stocks, std::string commod);
      
  /// @brief calls _ and tests that the number of objects in processing_ is
  /// n_processing and the number of objects in stocks_ is n_stocks
  void TestFinishProcessing(int n_processing, int n_stocks);
};

} // namespace cycamore

#endif // CYCAMORE_MODELS_FCOFUELFAB_FCO_FUEL_FAB_TESTS_
