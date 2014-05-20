// supply_deploy_inst.cc
// Implements the SupplyDeployInst class

#include "supply_deploy_inst.h"
#include "supply_demand_manager.h"

#include "error.h"

namespace cycamore {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SupplyDeployInst::SupplyDeployInst(cyclus::Context* ctx)
    : cyclus::InstModel(ctx),
      cyclus::Model(ctx) {}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SupplyDeployInst::~SupplyDeployInst() {}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string SupplyDeployInst::schema() {
  return
    "<element name=\"decomissionrule\">       \n"
    "  <element name=\"prototype\">           \n"
    "    <data type=\"string\"/>              \n"
    "  </element>                             \n"
    "  <element name=\"commodity\">           \n"
    "    <data type=\"string\"/>              \n"
    "  </element>                             \n"
    "  <element name=\"quantity\">            \n"
    "    <data type=\"double\"/>              \n"
    "  </element>                             \n"
    "  <optional>                             \n"
    "  <element name=\"replacement\">         \n"
    "    <data type=\"string\"/>              \n"
    "  </element>                             \n"
    "  </optional>                            \n"
    "  <optional>                             \n"
    "  <element name=\"repl_rate\">           \n"
    "    <data type=\"nonNegativeInteger\"/>  \n"
    "  </element>                             \n"
    "  </optional>                            \n"
    "</element>                               \n";
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SupplyDeployInst::InitFrom(cyclus::QueryEngine* qe) {
  cyclus::InstModel::InitFrom(qe);
  qe = qe->QueryElement("model/" + ModelImpl());

  using std::string;
  using cyclus::GetOptionalQuery;

  string query = "decomissionrule";

  cyclus::QueryEngine* rule = qe->QueryElement(query);

  // required facility data
  string prototype = rule->GetElementContent("prototype");
  to_decomm(prototype);
  string commod = rule->GetElementContent("commodity");
  rule_commod(commod);
  double quantity = atof(rule->GetElementContent("quantity").c_str());
  rule_quantity(quantity);

  // optional facility data
  string repl = GetOptionalQuery<string>(rule, "replacement", replacement());
  replacement(repl);
  int rate = GetOptionalQuery<int>(rule, "repl_rate", repl_rate());
  repl_rate(rate);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SupplyDeployInst::InitFrom(SupplyDeployInst* m){
  cyclus::InstModel::InitFrom(m);
  to_decomm_ = m->to_decomm_;
  replacement_ = m->replacement_;
  rule_commod_ = m->rule_commod_;
  rule_quantity_ = m->rule_quantity_;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SupplyDeployInst::Tick(int time) {
  int n = NumToDecommission(time);
  // decommission as many as required
  for (int i = 0; i < n; i++) {
    //Decommission(to_decomm());
    // replace decomissioned facs at the replacement rate
    for (int j = 0; j < repl_rate(); j++) {
      Build(replacement());
    }
  }
  InstModel::Tick(time);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SupplyDeployInst::NumToDecommission(int time) {
  using std::string;

  int to_ret = 0;
  double avail = QuantityAvailable(rule_commod());

  if( avail > rule_quantity() ) {
    to_ret = int(floor(avail/rule_quantity()));
  }

  return to_ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SupplyDeployInst::QuantityAvailable(std::string commod){
  // ask the market how much was offered last month.
  //return cyclus::Market(commod)->avail();
  return 100.0; // @TODO this is obviously a placeholder
}

/* ------------------- */


/* --------------------
 * all MODEL classes have these members
 * --------------------
 */

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Model* ConstructSupplyDeployInst(cyclus::Context* ctx) {
  return new SupplyDeployInst(ctx);
}
/* ------------------- */

} // namespace cycamore
