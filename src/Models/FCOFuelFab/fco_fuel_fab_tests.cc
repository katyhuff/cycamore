// fco_fuel_fab_tests.cc

#include <sstream>

#include "commodity.h"
#include "composition.h"
#include "error.h"
#include "facility_model_tests.h"
#include "model_tests.h"
#include "model.h"
#include "xml_query_engine.h"

#include "fco_fuel_fab_tests.h"

namespace cycamore {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFabTest::SetUp() {
  src_facility = new FCOFuelFab(tc_.get());
  InitParameters();
  SetUpSourceFacility();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFabTest::TearDown() {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFabTest::InitParameters() {
  // init params
  in_c1 = "in_c1";
  in_c2 = "in_c2";
  in_c3 = "in_c3";
  in_c4 = "in_c4";
  out_c1 = "out_c1";
  in_r1 = "in_r1";
  in_r2 = "in_r2";
  in_r3 = "in_r3";
  in_r4 = "in_r4";
  out_r1 = "out_r1";
  crctx.AddInCommod(in_c1, in_r1, out_c1, out_r1);
  crctx.AddInCommod(in_c2, in_r2, out_c1, out_r1);
  crctx.AddInCommod(in_c3, in_r3, out_c1, out_r1);
  crctx.AddInCommod(in_c4, in_r4, out_c1, out_r1);

  iso_1 = 92235;
  pref_1.insert(in_c1);
  pref_1.insert(in_c2);
  iso_2 = 94240;
  pref_2.insert(in_c3);
  pref_2.insert(in_c4);
  
  process_time = 0;
  
  commodity = out_c1;
  capacity = 200;
  cost = capacity;
  
  // recipes
  // sources for 92235
  cyclus::CompMap v;
  v[92235] = 1;
  cyclus::Composition::Ptr recipe = cyclus::Composition::CreateFromAtom(v);
  tc_.get()->AddRecipe(in_r1, recipe);
  tc_.get()->AddRecipe(in_r2, recipe);
  // sources for 94240
  cyclus::CompMap w;
  w[94240] = 2;
  recipe = cyclus::Composition::CreateFromAtom(w);
  tc_.get()->AddRecipe(in_r3, recipe);
  tc_.get()->AddRecipe(in_r4, recipe);
  // goal recipe includes all the things.
  cyclus::CompMap y;
  y[92235] = 1;
  y[94240] = 2;
  recipe = cyclus::Composition::CreateFromAtom(y);
  tc_.get()->AddRecipe(out_r1, recipe);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFabTest::SetUpSourceFacility() {
  src_facility->crctx(crctx);
  src_facility->process_time(process_time);
  src_facility->prefs(iso_1, pref_1);
  src_facility->prefs(iso_2, pref_2);
  src_facility->out_recipe(out_r1);

  src_facility->AddCommodity(commodity);
  src_facility->cyclus::CommodityProducer::SetCapacity(commodity, capacity);
  src_facility->cyclus::CommodityProducer::SetCost(commodity, capacity);

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFabTest::TestAddCommods(cyclus::Material::Ptr mat, std::string 
    commod, int n) {
  src_facility->AddCommods_(commod, mat);
  EXPECT_EQ(n, src_facility->reserves_[commod].count());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFabTest::TestBeginProcessing(int n_reserves, int n_processing, int n_stocks, std::string commod) {
  src_facility->BeginProcessing_();
  EXPECT_EQ(n_reserves, src_facility->ReservesCount_(commod));
  EXPECT_EQ(n_processing, src_facility->ProcessingCount_());
  EXPECT_EQ(n_stocks, src_facility->StocksCount(commod));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFabTest::TestFinishProcessing(int n_processing, int n_stocks) {
  src_facility->FabFuel_();
  EXPECT_EQ(n_processing, src_facility->ProcessingCount_());
  EXPECT_EQ(n_stocks, src_facility->StocksCount(in_c1));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FCOFuelFabTest::TestInitState(FCOFuelFab* fac) {
  EXPECT_EQ(crctx, fac->crctx());
  EXPECT_EQ(process_time, fac->process_time());
  EXPECT_EQ(0, fac->ProcessingCount_());
  EXPECT_EQ(FCOFuelFab::INITIAL, fac->phase());

  cyclus::Commodity commod(commodity);
  EXPECT_TRUE(fac->ProducesCommodity(commod));
  EXPECT_EQ(capacity, fac->ProductionCapacity(commod));
  EXPECT_EQ(cost, fac->ProductionCost(commod));
  EXPECT_EQ(pref_1, fac->prefs(iso_1));
  EXPECT_EQ(pref_2, fac->prefs(iso_2));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(FCOFuelFabTest, InitialState) {
  TestInitState(src_facility);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(FCOFuelFabTest, XMLInit) {
  std::stringstream ss;
  ss << "<start>"
     << "<name>fooname</name>"
     << "<model>"
     << "<UNSPECIFIED>"
     << "  <inpair>"
     << "    <incommodity>" << in_c1 << "</incommodity>"
     << "    <inrecipe>" << in_r1 << "</inrecipe>"
     << "  </inpair>"
     << "  <inpair>"
     << "    <incommodity>" << in_c2 << "</incommodity>"
     << "    <inrecipe>" << in_r2 << "</inrecipe>"
     << "  </inpair>"
     << "  <inpair>"
     << "    <incommodity>" << in_c3 << "</incommodity>"
     << "    <inrecipe>" << in_r3 << "</inrecipe>"
     << "  </inpair>"
     << "  <inpair>"
     << "    <incommodity>" << in_c4 << "</incommodity>"
     << "    <inrecipe>" << in_r4 << "</inrecipe>"
     << "  </inpair>"
     << "  <outpair>"
     << "    <outcommodity>" << out_c1 << "</outcommodity>"
     << "    <outrecipe>" << out_r1 << "</outrecipe>"
     << "  </outpair>"
     << "  <preflist>"
     << "    <prefiso>" << iso_1 << "</prefiso>"
     << "    <sourcecommod>" << in_c1 << "</sourcecommod>"
     << "    <sourcecommod>" << in_c2 << "</sourcecommod>"
     << "  </preflist>"
     << "  <preflist>"
     << "    <prefiso>" << iso_2 << "</prefiso>"
     << "    <sourcecommod>" << in_c3 << "</sourcecommod>"
     << "    <sourcecommod>" << in_c4 << "</sourcecommod>"
     << "  </preflist>"
     << "  <processtime>" << process_time << "</processtime>"
     << "  <capacity>" << capacity << "</capacity>"
     << "  <commodity_production>"
     << "    <commodity>" << out_c1 << "</commodity>"
     << "    <capacity>" << capacity << "</capacity>"
     << "    <cost>" << cost << "</cost>"
     << "  </commodity_production>"
     << "</UNSPECIFIED>"
     << "</model>"
     << "</start>";

  cyclus::XMLParser p;
  p.Init(ss);
  cyclus::XMLQueryEngine engine(p);
  cycamore::FCOFuelFab* fac = new cycamore::FCOFuelFab(tc_.get());
  fac->InitFrom(&engine);

  TestInitState(fac);

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(FCOFuelFabTest, Clone) {
  cycamore::FCOFuelFab* cloned_fac =
    dynamic_cast<cycamore::FCOFuelFab*>(src_facility->Clone());
  TestInitState(cloned_fac);  
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(FCOFuelFabTest, Print) {
  EXPECT_NO_THROW(std::string s = src_facility->str());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(FCOFuelFabTest, OutRecipe) {
  EXPECT_EQ("out_r1", src_facility->out_recipe());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(FCOFuelFabTest, Tick) {
  EXPECT_EQ(src_facility->crctx().in_recipe(in_c1), in_r1);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(FCOFuelFabTest, Tock) {
  int time = 1;
  src_facility->Tock(time);
  //EXPECT_NO_THROW(src_facility->Tock(time));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(FCOFuelFabTest, StartProcess) {
  int t = tc_.get()->time();
  src_facility->phase(FCOFuelFab::PROCESS);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(FCOFuelFabTest, AddCommods) {
  using cyclus::Material;
  double mat_size = 100; 
  Material::Ptr mat = Material::CreateBlank(mat_size);
  // mat to add, commodity
  TestAddCommods(mat, in_c1, 1);
  
  mat = Material::CreateBlank(mat_size - (1 + cyclus::eps()));
  TestAddCommods(mat, in_c1, 2);
  
  mat = Material::CreateBlank((1 + cyclus::eps()));
  TestAddCommods(mat, in_c1, 3);
  
  mat = Material::CreateBlank(mat_size + (1 + cyclus::eps()));
  TestAddCommods(mat, in_c1, 4);
  
  mat = Material::CreateBlank(mat_size - (1 + cyclus::eps()));
  TestAddCommods(mat, in_c1, 5);

  TestAddCommods(mat, in_c2, 1);
  
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(FCOFuelFabTest, CommodsInOut) {
  using cyclus::Material;
  double mat_size = 100; 

  TestBeginProcessing(0, 0, 0, in_c1);
  
  Material::Ptr mat = Material::CreateBlank(mat_size);
  TestAddCommods(mat, in_c1, 1);
  TestBeginProcessing(0, 1, 0,  in_c1);

  mat = Material::CreateBlank(mat_size * 2);
  TestAddCommods(mat, in_c1, 1);
  TestBeginProcessing(0, 2, 0, in_c1);
  
  TestFinishProcessing(1, 1);
  //TestFinishProcessing(0, 2);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Model* FCOFuelFabModelConstructor(cyclus::Context* ctx) {
  using cycamore::FCOFuelFab;
  return dynamic_cast<cyclus::Model*>(new FCOFuelFab(ctx));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::FacilityModel* FCOFuelFabConstructor(cyclus::Context* ctx) {
  using cycamore::FCOFuelFab;
  return dynamic_cast<cyclus::FacilityModel*>(new FCOFuelFab(ctx));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
INSTANTIATE_TEST_CASE_P(FCOFuelFab, FacilityModelTests,
                        Values(&FCOFuelFabConstructor));
INSTANTIATE_TEST_CASE_P(FCOFuelFab, ModelTests,
                        Values(&FCOFuelFabModelConstructor));

} // namespace cycamore
