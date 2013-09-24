// growth_region_tests.h
#ifndef GROWTHREGIONTESTS_H
#define GROWTHREGIONTESTS_H

#include <gtest/gtest.h>

#include "context.h"
#include "growth_region.h"
#include "event_manager.h"
#include "timer.h"

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class GrowthRegionTests : public ::testing::Test {
 protected:
  cyclus::Context* ctx;
  cyclus::Timer ti;
  cyclus::EventManager em;
  cycamore::GrowthRegion* region;
  std::string commodity_name, demand_type, demand_params, demand_start;

  virtual void SetUp();
  virtual void TearDown();
  void InitRegion();
  bool ManagesCommodity(cyclus::Commodity& commodity);
};

#endif