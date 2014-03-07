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
bool operator==(const CommodConverter::InitCond& l,
                const CommodConverter::InitCond& r) {
  bool reserves = (l.reserves == r.reserves &&
                   l.n_reserves == r.n_reserves &&
                   l.reserves_rec == r.reserves_rec &&
                   l.reserves_commod == r.reserves_commod);
  bool processing = (l.processing == r.processing &&
                   l.n_processing == r.n_processing &&
                   l.processing_rec == r.processing_rec &&
                   l.processing_commod == r.processing_commod);
  bool processing = (l.processing == r.processing &&
                   l.n_processing == r.n_processing &&
                   l.processing_rec == r.processing_rec &&
                   l.processing_commod == r.processing_commod);
  return (reserves && processing && processing);
}

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
  
  n_batches = 5;
  n_load = 2;
  n_reserves = 3;
  process_time = 10;
  refuel_time = 2;
  preorder_time = 1;
  batch_size = 4.5;
  
  commodity = "power";
  capacity = 200;
  cost = capacity;

  // init conds
  rsrv_c = in_c1;
  rsrv_r = in_r1;
  processing_c = in_c2;
  processing_r = in_r2;
  stor_c = out_c1;
  stor_r = out_r1;
  rsrv_n = 2;
  processing_n = 3;
  stor_n = 1;
  ics.AddReserves(rsrv_n, rsrv_r, rsrv_c);
  ics.AddProcessing(processing_n, processing_r, processing_c);
  ics.AddStocks(stor_n, stor_r, stor_c);
  
  // commod prefs
  frompref1 = 7.5;
  topref1 = frompref1 - 1;
  frompref2 = 5.5;
  topref2 = frompref2 - 2;
  commod_prefs[in_c1] = frompref1;
  commod_prefs[in_c2] = frompref2;

  // changes
  change_time = 5;
  pref_changes[change_time].push_back(std::make_pair(in_c1, topref1));
  pref_changes[change_time].push_back(std::make_pair(in_c2, topref2));
  recipe_changes[change_time].push_back(std::make_pair(in_c1, in_r2));
  
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
  src_facility->n_batches(n_batches);
  src_facility->n_load(n_load);
  src_facility->n_reserves(n_reserves);
  src_facility->process_time(process_time);
  src_facility->refuel_time(refuel_time);
  src_facility->preorder_time(preorder_time);
  src_facility->batch_size(batch_size);
  src_facility->ics(ics);
  
  src_facility->AddCommodity(commodity);
  src_facility->cyclus::CommodityProducer::SetCapacity(commodity, capacity);
  src_facility->cyclus::CommodityProducer::SetCost(commodity, capacity);

  src_facility->commod_prefs(commod_prefs);

  src_facility->pref_changes_[change_time].push_back(
      std::make_pair(in_c1, topref1));
  src_facility->pref_changes_[change_time].push_back(
      std::make_pair(in_c2, topref2));
  src_facility->recipe_changes_[change_time].push_back(
      std::make_pair(in_c1, in_r2));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverterTest::TestBuffs(int nreserves, int nprocessing, int nprocessing) {
  EXPECT_EQ(nreserves, src_facility->reserves_.count());
  EXPECT_EQ(nprocessing, src_facility->processing_.count());
  EXPECT_EQ(nprocessing, src_facility->processing_[out_c1].count());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverterTest::TestReserveBatches(cyclus::Material::Ptr mat,
                                          std::string commod,
                                          int n,
                                          double qty) {
  src_facility->AddBatches_(commod, mat);
  EXPECT_EQ(n, src_facility->reserves_.count());
  EXPECT_DOUBLE_EQ(qty, src_facility->spillover_->quantity());
  
  cyclus::Material::Ptr back = cyclus::ResCast<cyclus::Material>(
      src_facility->reserves_.Pop(cyclus::ResourceBuff::BACK));
  EXPECT_EQ(commod, src_facility->crctx_.commod(back));
  src_facility->reserves_.Push(back);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverterTest::TestBatchIn(int n_processing, int n_reserves) {
  src_facility->MoveBatchIn_();
  EXPECT_EQ(n_processing, src_facility->n_processing());
  EXPECT_EQ(n_reserves, src_facility->reserves_.count());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverterTest::TestBatchOut(int n_processing, int n_processing) {
  src_facility->MoveBatchOut_();
  EXPECT_EQ(n_processing, src_facility->n_processing());
  EXPECT_EQ(n_processing, src_facility->processing_[out_c1].count());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommodConverterTest::TestInitState(CommodConverter* fac) {
  EXPECT_EQ(crctx, fac->crctx());
  EXPECT_EQ(n_batches, fac->n_batches());
  EXPECT_EQ(n_load, fac->n_load());
  EXPECT_EQ(n_reserves, fac->n_reserves());
  EXPECT_EQ(process_time, fac->process_time());
  EXPECT_EQ(refuel_time, fac->refuel_time());
  EXPECT_EQ(preorder_time, fac->preorder_time());
  EXPECT_EQ(batch_size, fac->batch_size());
  EXPECT_EQ(0, fac->n_processing());
  EXPECT_EQ(CommodConverter::INITIAL, fac->phase());
  EXPECT_EQ(ics, fac->ics());

  cyclus::Commodity commod(commodity);
  EXPECT_TRUE(fac->ProducesCommodity(commod));
  EXPECT_EQ(capacity, fac->ProductionCapacity(commod));
  EXPECT_EQ(cost, fac->ProductionCost(commod));

  EXPECT_EQ(commod_prefs, fac->commod_prefs());

  EXPECT_EQ(pref_changes, fac->pref_changes_);
  EXPECT_EQ(recipe_changes, fac->recipe_changes_);
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
     << "  <fuel>"
     << "    <incommodity>" << in_c1 << "</incommodity>"
     << "    <inrecipe>" << in_r1 << "</inrecipe>"
     << "    <outcommodity>" << out_c1 << "</outcommodity>"
     << "    <outrecipe>" << out_r1 << "</outrecipe>"
     << "  </fuel>"
     << "  <fuel>"
     << "    <incommodity>" << in_c2 << "</incommodity>"
     << "    <inrecipe>" << in_r2 << "</inrecipe>"
     << "    <outcommodity>" << out_c2 << "</outcommodity>"
     << "    <outrecipe>" << out_r2 << "</outrecipe>"
     << "  </fuel>"
     << "  <processtime>" << process_time << "</processtime>"
     << "  <nbatches>" << n_batches << "</nbatches>"
     << "  <batchsize>" << batch_size << "</batchsize>"
     << "  <refueltime>" << refuel_time << "</refueltime>"
     << "  <orderlookahead>" << preorder_time << "</orderlookahead>"
     << "  <norder>" << n_reserves << "</norder>"
     << "  <nreload>" << n_load << "</nreload>"
     << "  <initial_condition>"
     << "    <reserves>"
     << "      <nbatches>" << rsrv_n << "</nbatches>"
     << "      <commodity>" << rsrv_c << "</commodity>"
     << "      <recipe>" << rsrv_r << "</recipe>"
     << "    </reserves>"
     << "    <processing>"
     << "      <nbatches>" << processing_n << "</nbatches>"
     << "      <commodity>" << processing_c << "</commodity>"
     << "      <recipe>" << processing_r << "</recipe>"
     << "    </processing>"
     << "    <processing>"
     << "      <nbatches>" << stor_n << "</nbatches>"
     << "      <commodity>" << stor_c << "</commodity>"
     << "      <recipe>" << stor_r << "</recipe>"
     << "    </processing>"
     << "  </initial_condition>"
     << "  <recipe_change>"
     << "    <incommodity>" << in_c1 << "</incommodity>"
     << "    <new_recipe>" << in_r2 << "</new_recipe>"
     << "    <time>" << change_time << "</time>"
     << "  </recipe_change>"
     << "  <commodity_production>"
     << "    <commodity>" << commodity << "</commodity>"
     << "    <capacity>" << capacity << "</capacity>"
     << "    <cost>" << cost << "</cost>"
     << "  </commodity_production>"
     << "  <commod_pref>"
     << "    <incommodity>" << in_c1 << "</incommodity>"
     << "    <preference>" << frompref1 << "</preference>"
     << "  </commod_pref>"
     << "  <commod_pref>"
     << "    <incommodity>" << in_c2 << "</incommodity>"
     << "    <preference>" << frompref2 << "</preference>"
     << "  </commod_pref>"
     << "  <pref_change>"
     << "    <incommodity>" << in_c1 << "</incommodity>"
     << "    <new_pref>" << topref1 << "</new_pref>"
     << "    <time>" << change_time << "</time>"
     << "  </pref_change>"
     << "  <pref_change>"
     << "    <incommodity>" << in_c2 << "</incommodity>"
     << "    <new_pref>" << topref2 << "</new_pref>"
     << "    <time>" << change_time << "</time>"
     << "  </pref_change>"
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
  EXPECT_EQ(src_facility->commod_prefs().at(in_c1), frompref1);
  EXPECT_EQ(src_facility->commod_prefs().at(in_c2), frompref2);
  EXPECT_EQ(src_facility->crctx().in_recipe(in_c1), in_r1);
  EXPECT_NO_THROW(src_facility->Tick(change_time););
  EXPECT_EQ(src_facility->commod_prefs().at(in_c1), topref1);
  EXPECT_EQ(src_facility->commod_prefs().at(in_c2), topref2);
  EXPECT_EQ(src_facility->crctx().in_recipe(in_c1), in_r2);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CommodConverterTest, Tock) {
  int time = 1;
  EXPECT_NO_THROW(src_facility->Tock(time));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CommodConverterTest, StartProcess) {
  int t = tc_.get()->time();
  src_facility->phase(CommodConverter::PROCESS);
  EXPECT_EQ(t, src_facility->start_time());
  EXPECT_EQ(t + process_time - 1, src_facility->end_time());
  EXPECT_EQ(t + process_time - preorder_time -1, src_facility->order_time());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CommodConverterTest, InitCond) {
  src_facility->Deploy();
  TestBuffs(rsrv_n, processing_n, stor_n);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CommodConverterTest, AddBatches) {
  using cyclus::Material;
  
  Material::Ptr mat = Material::CreateBlank(batch_size);
  // mat to add, commodity, nreserves, qty of spillover
  TestReserveBatches(mat, in_c1, 1, 0);
  
  mat = Material::CreateBlank(batch_size - (1 + cyclus::eps()));
  TestReserveBatches(mat, in_c1, 1, batch_size - (1 + cyclus::eps()));
  
  mat = Material::CreateBlank((1 + cyclus::eps()));
  TestReserveBatches(mat, in_c1, 2, 0);

  mat = Material::CreateBlank(batch_size + (1 + cyclus::eps()));
  TestReserveBatches(mat, in_c1, 3, 1 + cyclus::eps());
  
  mat = Material::CreateBlank(batch_size - (1 + cyclus::eps()));
  TestReserveBatches(mat, in_c1, 4, 0);
  
  mat = Material::CreateBlank(1 + cyclus::eps());
  TestReserveBatches(mat, in_c1, 4, 1 + cyclus::eps());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(CommodConverterTest, BatchInOut) {
  using cyclus::Material;

  EXPECT_THROW(TestBatchIn(1, 0), cyclus::Error);
  
  Material::Ptr mat = Material::CreateBlank(batch_size);
  TestReserveBatches(mat, in_c1, 1, 0);
  TestBatchIn(1, 0);

  mat = Material::CreateBlank(batch_size * 2);
  TestReserveBatches(mat, in_c1, 2, 0);
  TestBatchIn(2, 1);
  
  TestBatchOut(1, 1);
  TestBatchOut(0, 2);

  EXPECT_THROW(TestBatchOut(1, 0), cyclus::Error);
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