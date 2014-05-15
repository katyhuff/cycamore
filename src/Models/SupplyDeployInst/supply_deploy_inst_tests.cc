// supply_deploy_inst_tests.cc
#include <gtest/gtest.h>

#include "context.h"
#include "supply_deploy_inst.h"
#include "inst_model_tests.h"
#include "model_tests.h"

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::Model* SupplyDeployInstModelConstructor(cyclus::Context* ctx) {
  return dynamic_cast<cyclus::Model*>(new cycamore::SupplyDeployInst(ctx));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cyclus::InstModel* SupplyDeployInstConstructor(cyclus::Context* ctx) {
  return dynamic_cast<cyclus::InstModel*>(new cycamore::SupplyDeployInst(ctx));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class SupplyDeployInstTest : public ::testing::Test {
 protected:

  virtual void SetUp() {}

  virtual void TearDown() {}
};


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
INSTANTIATE_TEST_CASE_P(SupplyDeployInst, InstModelTests,
                        Values(&SupplyDeployInstConstructor));
INSTANTIATE_TEST_CASE_P(SupplyDeployInst, ModelTests,
                        Values(&SupplyDeployInstModelConstructor));

