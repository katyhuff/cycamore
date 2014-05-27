#ifndef CYCAMORE_MODELS_SEPARATIONSFAC_SEPARATIONS_FAC_TESTS_
#define CYCAMORE_MODELS_SEPARATIONSFAC_SEPARATIONS_FAC_TESTS_

#include <gtest/gtest.h>

#include <map>
#include <string>

#include "test_context.h"

#include "separations_fac.h"

namespace cycamore {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class SeparationsFacTest : public ::testing::Test {
 protected:
  cyclus::TestContext tc_;
  SeparationsFac* src_facility;

  // init params
  std::string in_c1, out_c1, out_c2, out_c3;
  std::string in_r1;
  int out_z1, out_z2, out_z3;
  cyclus::CommodityRecipeContext crctx;
  std::set<std::string> out_commods;
  std::set<int> out_elems;
  std::map<std::string, int> out_commod_elem_map;

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
  void TestInitState(SeparationsFac* fac);

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
      
  /// @brief calls FabFuel_ and tests that the number of objects in processing_ is
  /// n_processing and the number of objects in stocks_ is n_stocks
  /// out commod is the commodity to separate
  void TestFinishProcessing(int n_processing, int n_stocks, std::string out_commod);

  /// @brief called CompPossible and checks that the comp_out is the result 
  void TestCompPossible(int z, cyclus::CompMap comp_in, cyclus::CompMap comp_out, double amt);
};

} // namespace cycamore

#endif // CYCAMORE_MODELS_SEPARATIONSFAC_SEPARATIONS_FAC_TESTS_
