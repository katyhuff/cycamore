// separations_fac_tests.cc

#include <sstream>

#include "commodity.h"
#include "composition.h"
#include "error.h"
#include "facility_model_tests.h"
#include "model_tests.h"
#include "model.h"
#include "xml_query_engine.h"

#include "separations_fac_tests.h"

namespace cycamore {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFacTest::SetUp() {
  src_facility = new SeparationsFac(tc_.get());
  InitParameters();
  SetUpSourceFacility();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFacTest::TearDown() {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFacTest::InitParameters() {
  // init params
  in_c1 = "in_c1";
  out_c1 = "sep_u";
  out_c2 = "sep_pu";
  out_c3 = "sep_am";
  in_r1 = "in_r1";
  out_z1 = 92;
  out_z2 = 94;
  out_z3 = 95;

  out_commods.insert(out_c1);
  out_commods.insert(out_c2);
  out_commods.insert(out_c3);
  out_elems.insert(out_z1);
  out_elems.insert(out_z2);
  out_elems.insert(out_z3);
  out_commod_elem_map.insert(std::make_pair(out_c1, out_z1));
  out_commod_elem_map.insert(std::make_pair(out_c2, out_z2));
  out_commod_elem_map.insert(std::make_pair(out_c3, out_z3));

  process_time = 0;
  
  commodity = "swu";
  capacity = 200;
  cost = capacity;
  
  // recipes
  // sources for 92235
  cyclus::CompMap v;
  v[92235] = 10;
  v[94240] = 10;
  v[95241] = 10;
  cyclus::Composition::Ptr recipe = cyclus::Composition::CreateFromMass(v);
  tc_.get()->AddRecipe(in_r1, recipe);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFacTest::SetUpSourceFacility() {
  src_facility->crctx(crctx);
  src_facility->process_time(process_time);
  src_facility->out_elems(out_elems);
  src_facility->out_commods(out_commods);
  src_facility->out_commod_elem_map(out_commod_elem_map);

  src_facility->AddCommodity(commodity);
  src_facility->cyclus::CommodityProducer::SetCapacity(commodity, capacity);
  src_facility->cyclus::CommodityProducer::SetCost(commodity, capacity);

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFacTest::TestAddCommods(cyclus::Material::Ptr mat, std::string 
    commod, int n) {
  src_facility->AddCommods_(commod, mat);
  EXPECT_EQ(n, src_facility->reserves_[commod].count());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFacTest::TestBeginProcessing(int n_reserves, int n_processing, int n_stocks, std::string commod) {
  src_facility->BeginProcessing_();
  EXPECT_EQ(n_reserves, src_facility->ReservesCount_(commod));
  EXPECT_EQ(n_processing, src_facility->ProcessingCount_());
  EXPECT_EQ(n_stocks, src_facility->StocksCount());
  EXPECT_EQ(n_stocks, src_facility->StocksCount(out_c1));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFacTest::TestFinishProcessing(int n_processing, int n_stocks, std::string commod) {
  int initial_stocks = src_facility->StocksCount();
  src_facility->Separate_(commod);
  EXPECT_TRUE(src_facility->StocksCount() >= initial_stocks);
  EXPECT_EQ(n_processing, src_facility->ProcessingCount_());
  EXPECT_EQ(n_stocks, src_facility->StocksCount(commod));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFacTest::TestInitState(SeparationsFac* fac) {
  EXPECT_EQ(crctx, fac->crctx());
  EXPECT_EQ(process_time, fac->process_time());
  EXPECT_EQ(0, fac->ProcessingCount_());
  EXPECT_EQ(SeparationsFac::INITIAL, fac->phase());

  cyclus::Commodity commod = commodity;
  EXPECT_TRUE(fac->ProducesCommodity(commod));
  EXPECT_EQ(capacity, fac->ProductionCapacity(commod));
  EXPECT_EQ(cost, fac->ProductionCost(commod));
  EXPECT_EQ(out_z1, fac->out_elem(out_c1));
  EXPECT_EQ(out_z2, fac->out_elem(out_c2));
  EXPECT_EQ(out_z3, fac->out_elem(out_c3));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SeparationsFacTest::TestCompPossible(int z, cyclus::CompMap comp_in, cyclus::CompMap comp_out, double amt){
  std::pair<double, cyclus::Composition::Ptr> result = src_facility->CompPossible_(z, comp_in);
  EXPECT_EQ(comp_out, result.second->mass());
  EXPECT_EQ(amt, result.first);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(SeparationsFacTest, InitialState) {
  TestInitState(src_facility);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(SeparationsFacTest, XMLInit) {
  std::stringstream ss;
  ss << "<start>"
     << "<name>fooname</name>"
     << "<model>"
     << "<UNSPECIFIED>"
     << "  <inpair>"
     << "    <incommodity>" << in_c1 << "</incommodity>"
     << "    <inrecipe>" << in_r1 << "</inrecipe>"
     << "  </inpair>"
     << "  <outpair>"
     << "    <outcommodity>" << out_c1 << "</outcommodity>"
     << "    <z>" << out_z1 << "</z>"
     << "  </outpair>"
     << "  <outpair>"
     << "    <outcommodity>" << out_c2 << "</outcommodity>"
     << "    <z>" << out_z2 << "</z>"
     << "  </outpair>"
     << "  <outpair>"
     << "    <outcommodity>" << out_c3 << "</outcommodity>"
     << "    <z>" << out_z3 << "</z>"
     << "  </outpair>"
     << "  <processtime>" << process_time << "</processtime>"
     << "  <capacity>" << capacity << "</capacity>"
     << "  <commodity_production>"
     << "    <commodity>" << "swu" << "</commodity>"
     << "    <capacity>" << capacity << "</capacity>"
     << "    <cost>" << cost << "</cost>"
     << "  </commodity_production>"
     << "</UNSPECIFIED>"
     << "</model>"
     << "</start>";

  cyclus::XMLParser p;
  p.Init(ss);
  cyclus::XMLQueryEngine engine(p);
  cycamore::SeparationsFac* fac = new cycamore::SeparationsFac(tc_.get());
  fac->InitFrom(&engine);

  TestInitState(fac);

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(SeparationsFacTest, Clone) {
  cycamore::SeparationsFac* cloned_fac =
    dynamic_cast<cycamore::SeparationsFac*>(src_facility->Clone());
  TestInitState(cloned_fac);  
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(SeparationsFacTest, Print) {
  EXPECT_NO_THROW(std::string s = src_facility->str());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(SeparationsFacTest, OutElems) {
  EXPECT_EQ(out_z1, src_facility->out_elem(out_c1));
  EXPECT_EQ(out_z2, src_facility->out_elem(out_c2));
  EXPECT_EQ(out_z3, src_facility->out_elem(out_c3));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(SeparationsFacTest, Tick) {
  int time = 1;
  EXPECT_NO_THROW(src_facility->Tick(time));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(SeparationsFacTest, Tock) {
  int time = 1;
  EXPECT_NO_THROW(src_facility->Tock(time));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(SeparationsFacTest, StartProcess) {
  int t = tc_.get()->time();
  src_facility->phase(SeparationsFac::PROCESS);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(SeparationsFacTest, ExtractZero) {
  using cyclus::Material;
  using cyclus::Composition;

  double mat_size = 10.0; 
  Material::Ptr mat = Material::Create(src_facility, mat_size, tc_.get()->GetRecipe(in_r1));
  EXPECT_NO_THROW(mat->ExtractComp(0.0, mat->comp()));
  cyclus::CompMap emptycomp;
  EXPECT_NO_THROW(mat->ExtractComp(3.333, Composition::CreateFromMass(emptycomp)));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(SeparationsFacTest, AddCommods) {
  using cyclus::Material;
  double mat_size = 10.0; 
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
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(SeparationsFacTest, CommodsInOut) {
  using cyclus::Material;
  double mat_size = 10.0; 

  TestBeginProcessing(0, 0, 0, in_c1);

  Material::Ptr mat = Material::Create(src_facility, mat_size, tc_.get()->GetRecipe(in_r1));
  TestAddCommods(mat, in_c1, 1);
  TestBeginProcessing(0, 1, 0,  in_c1);
  mat = Material::Create(src_facility, 2*mat_size, tc_.get()->GetRecipe(in_r1));
  TestAddCommods(mat, in_c1, 1);
  TestBeginProcessing(0, 2, 0,  in_c1);
  TestFinishProcessing(2, 2, out_c1);
  TestFinishProcessing(2, 2, out_c2);
  TestFinishProcessing(0, 2, out_c3);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(SeparationsFacTest, CompPossible){
  cyclus::CompMap u;
  cyclus::CompMap u_pu;
  cyclus::CompMap pu;
  cyclus::CompMap empty;

  double ten = 10.0;
  u.insert(std::make_pair(92235, ten));

  pu.insert(std::make_pair(94240, ten));

  u_pu.insert(std::make_pair(92235, ten));
  u_pu.insert(std::make_pair(94240, ten));

  TestCompPossible(92, pu, empty, 0.0);
  TestCompPossible(92, u, u, ten);
  TestCompPossible(92, u_pu, u, ten);

  TestCompPossible(94, u, empty, 0.0);
  TestCompPossible(94, pu, pu, ten);
  TestCompPossible(94, u_pu, pu, ten);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Model* SeparationsFacModelConstructor(cyclus::Context* ctx) {
  using cycamore::SeparationsFac;
  return dynamic_cast<cyclus::Model*>(new SeparationsFac(ctx));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::FacilityModel* SeparationsFacConstructor(cyclus::Context* ctx) {
  using cycamore::SeparationsFac;
  return dynamic_cast<cyclus::FacilityModel*>(new SeparationsFac(ctx));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
INSTANTIATE_TEST_CASE_P(SeparationsFac, FacilityModelTests,
                        Values(&SeparationsFacConstructor));
INSTANTIATE_TEST_CASE_P(SeparationsFac, ModelTests,
                        Values(&SeparationsFacModelConstructor));

} // namespace cycamore
