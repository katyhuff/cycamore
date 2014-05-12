#ifndef CYCAMORE_MODELS_COMMODCONVERTER_COMMOD_CONVERTER_TESTS_
#define CYCAMORE_MODELS_COMMODCONVERTER_COMMOD_CONVERTER_TESTS_

#include <gtest/gtest.h>

#include <map>
#include <string>

#include "test_context.h"

#include "commod_converter.h"

namespace cycamore {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class CommodConverterTest : public ::testing::Test {
 protected:
  cyclus::TestContext tc_;
  CommodConverter* src_facility;

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
  void TestInitState(CommodConverter* fac);

  /// @brief tests the number of batches in each buffer
  void TestBuffs(int nreserves, int ncore, int nstorage);
  
  /// @brief tests the CommodConverter's reserves_, by calling AddCommod_,
  /// and confirming that there are n items and the last item has quantity qty
  void TestAddCommod(cyclus::Material::Ptr mat, std::string commod,
                          int n, double qty);

  /// @brief calls Convert_ and tests that
  /// the number of objects in reserves_ is n_reserves
  /// the number of objects in processing_ is n_processing
  /// the number of objects in stocks_ is n_stocks
  void TestBeginProcessing(int n_reserves, int n_processing, int n_stocks);
      
  /// @brief calls Convert?_ and tests that the number of objects in core_ is
  /// n_processing and the number of objects in stocks_ is n_stocks
  void TestFinishProcessing(int n_processing, int n_stocks);
};

} // namespace cycamore

#endif // CYCAMORE_MODELS_COMMODCONVERTER_COMMOD_CONVERTER_TESTS_
