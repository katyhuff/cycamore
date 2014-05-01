// commod_converter_tests.cc

#include <sstream>

#include "commodity.h"
#include "composition.h"
#include "error.h"
#include "facility_model_tests.h"
#include "model_tests.h"
#include "model.h"
#include "xml_query_engine.h"

#include "commod_converter_tests.h"

namespace cycamore {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverterTest::SetUp() {
  src_facility = new CommodConverter(tc_.get());
  InitParameters();
  SetUpSourceFacility();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverterTest::TearDown() {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverterTest::InitParameters() {
  // init params
  in_c1 = "in_c1";
  in_c2 = "in_c2";
  out_c1 = "out_c1";
  out_c2 = "out_c2";
  in_r1 = "in_r1";
  in_r2 = "in_r2";
  out_r1 = "out_r1";
  out_r2 = "out_r2";
  crctx.AddInCommod(in_c1, in_r1, out_c1, out_r1);
  crctx.AddInCommod(in_c2, in_r2, out_c2, out_r2);
  
  process_time = 10;
  
  commodity = out_c1;
  capacity = 200;
  cost = capacity;
  
  // recipes
  cyclus::CompMap v;
  v[92235] = 1;
  v[92238] = 2;
  cyclus::Composition::Ptr recipe = cyclus::Composition::CreateFromAtom(v);
  tc_.get()->AddRecipe(in_r1, recipe);
  tc_.get()->AddRecipe(in_r2, recipe);

  v[94239] = 0.25;
  recipe = cyclus::Composition::CreateFromAtom(v);
  tc_.get()->AddRecipe(out_r1, recipe);
  tc_.get()->AddRecipe(out_r2, recipe);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverterTest::SetUpSourceFacility() {
  src_facility->crctx(crctx);
  src_facility->process_time(process_time);
  
  src_facility->AddCommodity(commodity);
  src_facility->cyclus::CommodityProducer::SetCapacity(commodity, capacity);
  src_facility->cyclus::CommodityProducer::SetCost(commodity, capacity);

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverterTest::TestBuffs(int nreserves, int nprocessing, int nstocks) {
  EXPECT_EQ(nreserves, src_facility->reserves_.count());
  EXPECT_EQ(nprocessing, src_facility->ProcessingCount());
  EXPECT_EQ(nstocks, src_facility->stocks_[out_c1].count());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverterTest::TestAddCommod(cyclus::Material::Ptr mat,
                                          std::string commod,
                                          int n,
                                          double qty) {
  src_facility->AddCommods_(commod, mat);
  EXPECT_EQ(n, src_facility->reserves_.count());
  
  cyclus::Material::Ptr back = cyclus::ResCast<cyclus::Material>(
      src_facility->reserves_.Pop(cyclus::ResourceBuff::BACK));
  EXPECT_EQ(commod, src_facility->crctx_.commod(back));
  src_facility->reserves_.Push(back);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverterTest::TestBeginProcessing(int n_processing, int n_reserves) {
  src_facility->BeginProcessing_();
  EXPECT_EQ(n_processing, src_facility->ProcessingCount());
  EXPECT_EQ(n_reserves, src_facility->reserves_.count());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverterTest::TestFinishProcessing(int n_processing, int n_stocks) {
  src_facility->Convert_();
  EXPECT_EQ(n_processing, src_facility->ProcessingCount());
  EXPECT_EQ(n_stocks, src_facility->stocks_[out_c1].count());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverterTest::TestInitState(CommodConverter* fac) {
  EXPECT_EQ(crctx, fac->crctx());
  EXPECT_EQ(process_time, fac->process_time());
  EXPECT_EQ(0, fac->ProcessingCount());
  EXPECT_EQ(CommodConverter::INITIAL, fac->phase());

  cyclus::Commodity commod(commodity);
  EXPECT_TRUE(fac->ProducesCommodity(commod));
  EXPECT_EQ(capacity, fac->ProductionCapacity(commod));
  EXPECT_EQ(cost, fac->ProductionCost(commod));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CommodConverterTest, InitialState) {
  TestInitState(src_facility);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CommodConverterTest, XMLInit) {
  std::stringstream ss;
  ss << "<start>"
     << "<name>fooname</name>"
     << "<model>"
     << "<UNSPECIFIED>"
     << "  <commodpair>"
     << "    <incommodity>" << in_c1 << "</incommodity>"
     << "    <inrecipe>" << in_r1 << "</inrecipe>"
     << "    <outcommodity>" << out_c1 << "</outcommodity>"
     << "    <outrecipe>" << out_r1 << "</outrecipe>"
     << "  </commodpair>"
     << "  <commodpair>"
     << "    <incommodity>" << in_c2 << "</incommodity>"
     << "    <inrecipe>" << in_r2 << "</inrecipe>"
     << "    <outcommodity>" << out_c2 << "</outcommodity>"
     << "    <outrecipe>" << out_r2 << "</outrecipe>"
     << "  </commodpair>"
     << "  <processtime>" << process_time << "</processtime>"
     << "  <capacity>" << capacity << "</capacity>"
     << "  <commodity_production>"
     << "    <commodity>" << out_c1 << "</commodity>"
     << "    <capacity>" << capacity << "</capacity>"
     << "    <cost>" << cost << "</cost>"
     << "  </commodity_production>"
     << "  <commodity_production>"
     << "    <commodity>" << out_c2 << "</commodity>"
     << "    <capacity>" << capacity << "</capacity>"
     << "    <cost>" << cost << "</cost>"
     << "  </commodity_production>"
     << "</UNSPECIFIED>"
     << "</model>"
     << "</start>";

  cyclus::XMLParser p;
  p.Init(ss);
  cyclus::XMLQueryEngine engine(p);
  cycamore::CommodConverter* fac = new cycamore::CommodConverter(tc_.get());
  fac->InitFrom(&engine);

  TestInitState(fac);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CommodConverterTest, Clone) {
  cycamore::CommodConverter* cloned_fac =
    dynamic_cast<cycamore::CommodConverter*>(src_facility->Clone());
  TestInitState(cloned_fac);  
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CommodConverterTest, Print) {
  EXPECT_NO_THROW(std::string s = src_facility->str());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CommodConverterTest, Tick) {
  EXPECT_EQ(src_facility->crctx().in_recipe(in_c1), in_r1);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CommodConverterTest, Tock) {
  int time = 1;
  src_facility->Tock(time);
  //EXPECT_NO_THROW(src_facility->Tock(time));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CommodConverterTest, StartProcess) {
  int t = tc_.get()->time();
  src_facility->phase(CommodConverter::PROCESS);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CommodConverterTest, AddCommods) {
  using cyclus::Material;
  double mat_size = 100; 
  Material::Ptr mat = Material::CreateBlank(mat_size);
  // mat to add, commodity, nreserves, qty of spillover
  TestAddCommod(mat, in_c1, 1, 0);
  
  mat = Material::CreateBlank(mat_size - (1 + cyclus::eps()));
  TestAddCommod(mat, in_c1, 2, mat_size- (1 + cyclus::eps()));
  
  mat = Material::CreateBlank((1 + cyclus::eps()));
  TestAddCommod(mat, in_c1, 3, 0);

  mat = Material::CreateBlank(mat_size + (1 + cyclus::eps()));
  TestAddCommod(mat, in_c1, 4, 1 + cyclus::eps());
  
  mat = Material::CreateBlank(mat_size - (1 + cyclus::eps()));
  TestAddCommod(mat, in_c1, 5, 0);
  
  mat = Material::CreateBlank(1 + cyclus::eps());
  TestAddCommod(mat, in_c1, 6, 1 + cyclus::eps());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CommodConverterTest, CommodsInOut) {
  using cyclus::Material;
  double mat_size = 100; 

  EXPECT_THROW(TestBeginProcessing(1, 0), cyclus::Error);
  
  Material::Ptr mat = Material::CreateBlank(mat_size);
  TestAddCommod(mat, in_c1, 1, 0);
  TestBeginProcessing(1, 0);

  mat = Material::CreateBlank(mat_size * 2);
  TestAddCommod(mat, in_c1, 2, 0);
  TestBeginProcessing(2, 1);
  
  TestFinishProcessing(1, 1);
  TestFinishProcessing(0, 2);

  EXPECT_THROW(TestFinishProcessing(1, 0), cyclus::Error);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Model* CommodConverterModelConstructor(cyclus::Context* ctx) {
  using cycamore::CommodConverter;
  return dynamic_cast<cyclus::Model*>(new CommodConverter(ctx));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::FacilityModel* CommodConverterConstructor(cyclus::Context* ctx) {
  using cycamore::CommodConverter;
  return dynamic_cast<cyclus::FacilityModel*>(new CommodConverter(ctx));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
INSTANTIATE_TEST_CASE_P(CommodConverter, FacilityModelTests,
                        Values(&CommodConverterConstructor));
INSTANTIATE_TEST_CASE_P(CommodConverter, ModelTests,
                        Values(&CommodConverterModelConstructor));

} // namespace cycamore
