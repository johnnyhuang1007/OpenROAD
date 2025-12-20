// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "ord/Timing.h"

#include <tcl.h>

#include <algorithm>
#include <array>
#include <limits>
#include <cstring>
#include <set>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "ord/Design.h"
#include "ord/OpenRoad.hh"
#include "ord/Tech.h"
#include "rsz/Resizer.hh"
#include "sta/Clock.hh"
#include "sta/Corner.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/PowerClass.hh"
#include "sta/Search.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "sta/TableModel.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Graph.hh"
#include "sta/Delay.hh"
#include "sta/Debug.hh"
#include "utl/Logger.h"

namespace {

std::vector<float> axisValues(const sta::TableModel* model, int axis_index)
{
  std::vector<float> values;
  if (model == nullptr) {
    return values;
  }

  const sta::TableAxis* axis = nullptr;
  if (axis_index == 0) {
    axis = model->axis1();
  }
  else if (axis_index == 1) {
    axis = model->axis2();
  }

  if (axis == nullptr) {
    return values;
  }

  sta::FloatSeq* seq = axis->values();
  if (seq == nullptr) {
    return values;
  }

  values.reserve(seq->size());
  for (size_t i = 0; i < seq->size(); ++i) {
    values.push_back((*seq)[i]);
  }
  return values;
}

std::vector<std::vector<float>> tableValues(const sta::TableModel* model)
{
  std::vector<std::vector<float>> values;
  if (model == nullptr) {
    return values;
  }

  size_t rows = 1;
  if (const sta::TableAxis* axis = model->axis1()) {
    rows = std::max<size_t>(axis->size(), 1);
  }

  size_t cols = 1;
  if (const sta::TableAxis* axis = model->axis2()) {
    cols = std::max<size_t>(axis->size(), 1);
  }

  values.assign(rows, std::vector<float>(cols, 0.0f));
  for (size_t row = 0; row < rows; ++row) {
    for (size_t col = 0; col < cols; ++col) {
      values[row][col] = model->value(row, col, 0);
    }
  }
  return values;
}

struct AxisInterpolation
{
  size_t low;
  size_t high;
  float fraction;
};

AxisInterpolation locateAxisPosition(const std::vector<float>& axis,
                                     float value,
                                     size_t table_size)
{
  AxisInterpolation pos{0, 0, 0.0f};
  if (table_size == 0) {
    return pos;
  }

  if (axis.empty() || axis.size() == 1) {
    pos.low = pos.high = 0;
    pos.fraction = 0.0f;
    return pos;
  }

  if (value <= axis.front()) {
    pos.low = 0;
    pos.high = 1;
    pos.fraction = 0.0f;
    return pos;
  }

  if (value >= axis.back()) {
    pos.high = axis.size() - 1;
    pos.low = pos.high - 1;
    pos.fraction = 1.0f;
    return pos;
  }

  for (size_t i = 0; i < axis.size() - 1; ++i) {
    const float lower = axis[i];
    const float upper = axis[i + 1];
    if (value <= upper) {
      float fraction = 0.0f;
      if (upper > lower) {
        fraction = std::clamp((value - lower) / (upper - lower), 0.0f, 1.0f);
      }
      pos.low = i;
      pos.high = i + 1;
      pos.fraction = fraction;
      return pos;
    }
  }

  pos.high = axis.size() - 1;
  pos.low = pos.high - 1;
  pos.fraction = 1.0f;
  return pos;
}

AxisInterpolation clampAxisPosition(const AxisInterpolation& pos, size_t size)
{
  AxisInterpolation result = pos;
  if (size == 0) {
    result.low = result.high = 0;
    result.fraction = 0.0f;
    return result;
  }

  result.low = std::min(result.low, size - 1);
  result.high = std::min(result.high, size - 1);
  if (result.low == result.high) {
    result.fraction = 0.0f;
  }
  return result;
}

float interpolateTable(const std::vector<float>& axis0,
                       const std::vector<float>& axis1,
                       const std::vector<std::vector<float>>& table,
                       float axis0_value,
                       float axis1_value)
{
  
  if (table.empty() || table.front().empty()) {
    return 0.0f;
  }

  AxisInterpolation axis0_pos = clampAxisPosition(
      locateAxisPosition(axis0, axis0_value, table.size()), table.size());
  AxisInterpolation axis1_pos = clampAxisPosition(
      locateAxisPosition(axis1, axis1_value, table.front().size()),
      table.front().size());

  const float v00 = table[axis0_pos.low][axis1_pos.low];
  const float v01 = table[axis0_pos.low][axis1_pos.high];
  const float v10 = table[axis0_pos.high][axis1_pos.low];
  const float v11 = table[axis0_pos.high][axis1_pos.high];

  if (axis0_pos.low == axis0_pos.high
      && axis1_pos.low == axis1_pos.high) {
    return v00;
  }

  if (axis0_pos.low == axis0_pos.high) {
    return v00 + (v01 - v00) * axis1_pos.fraction;
  }

  if (axis1_pos.low == axis1_pos.high) {
    return v00 + (v10 - v00) * axis0_pos.fraction;
  }
  float dx = (axis0_value - axis0[axis0_pos.low])/(axis0[axis0_pos.high]-axis0[axis0_pos.low]);
  float dy = (axis1_value - axis1[axis1_pos.low])/(axis1[axis1_pos.high]-axis1[axis1_pos.low]);
  float tbl_value = v00*(1-dx)*(1-dy) + v10*dx*(1-dy) + v11*dx*dy + v01*(1-dx)*dy;

  const float v0 = v00 + (v01 - v00) * axis1_pos.fraction;
  const float v1 = v10 + (v11 - v10) * axis1_pos.fraction;
  return tbl_value;
}

void appendArcTableModels(const sta::TimingArcSetSeq& arc_sets,
                          std::vector<ord::TimingArcTableModel>& tables)
{
  for (const sta::TimingArcSet* arc_set : arc_sets) {
    if (arc_set == nullptr) {
      continue;
    }
    for (const sta::TimingArc* arc : arc_set->arcs()) {
      if (arc == nullptr) {
        continue;
      }
      sta::GateTableModel* gate_model = arc->gateTableModel();
      if (gate_model == nullptr) {
        continue;
      }

      ord::TimingArcTableModel entry;
      entry.arc_description = arc->to_string();
      entry.in_pin_name = arc->from()->name();
      entry.out_pin_name = arc->to()->name();

      entry.in_rf = arc->fromEdge()->to_string();
      entry.out_rf = arc->toEdge()->to_string(); //^ or v

      const sta::TableModel* delay_model = gate_model->delayModel();
      const sta::TableModel* slew_model = gate_model->slewModel();

      entry.slew_model = slew_model;
      entry.delay_model = delay_model;

      entry.table_axis0 = axisValues(delay_model, 0);
      if (entry.table_axis0.empty()) {
        entry.table_axis0 = axisValues(slew_model, 0);
      }

      entry.table_axis1 = axisValues(delay_model, 1);
      if (entry.table_axis1.empty()) {
        entry.table_axis1 = axisValues(slew_model, 1);
      }

      entry.delay_table = tableValues(delay_model);
      entry.slew_table = tableValues(slew_model);

      tables.push_back(std::move(entry));
    }
  }
}

}  // namespace

namespace ord {


float TimingArcTableModel::findOutputSlewValue(float axis0_value,
                                               float axis1_value) const
{
  return interpolateTable(table_axis0, table_axis1, slew_table, axis0_value,
                          axis1_value);
}


float TimingArcTableModel::findOutputDelayValue(float axis0_value,
                                                float axis1_value) const
{
  float actual = delay_model->findValue(axis0_value, axis1_value, 0); 
  float cal = interpolateTable(table_axis0, table_axis1, delay_table, axis0_value,
                          axis1_value);
  
  std::cout<<"actual delay: "<< actual <<", cal delay: "<< cal << std::endl;
  return cal;
}

std::vector<float> TimingArcTableModel::findOutputDelayGradients(float axis0_value,
                                   float axis1_value) const
{
  std::vector<float> gradients;
  gradients.resize(2, 0.0f);

  if (delay_table.empty() || delay_table.front().empty()) {
    return gradients;
  }

  AxisInterpolation axis0_pos = clampAxisPosition(
      locateAxisPosition(table_axis0, axis0_value, delay_table.size()), delay_table.size());
  AxisInterpolation axis1_pos = clampAxisPosition(
      locateAxisPosition(table_axis1, axis1_value, delay_table.front().size()),
      delay_table.front().size());

  if (axis0_pos.low == axis0_pos.high) axis0_pos.high++;
  if (axis1_pos.low == axis1_pos.high) axis1_pos.high++;
  const float v00 = delay_table[axis0_pos.low][axis1_pos.low];
  const float v01 = delay_table[axis0_pos.low][axis1_pos.high];
  const float v10 = delay_table[axis0_pos.high][axis1_pos.low];
  const float v11 = delay_table[axis0_pos.high][axis1_pos.high];
  
  gradients[0] = ((v11 - v01) + (v10 - v00)) / (2*(table_axis0[axis0_pos.high] - table_axis0[axis0_pos.low]));
  gradients[1] = ((v11 - v10) + (v01 - v00)) / (2*(table_axis1[axis1_pos.high] - table_axis1[axis1_pos.low]));


  return gradients;
}

std::vector<float> TimingArcTableModel::findOutputSlewGradients(float axis0_value,
                                   float axis1_value) const
{
  std::vector<float> gradients;
  gradients.resize(2, 0.0f);

  if (slew_table.empty() || slew_table.front().empty()) {
    return gradients;
  }

  AxisInterpolation axis0_pos = clampAxisPosition(
      locateAxisPosition(table_axis0, axis0_value, slew_table.size()), slew_table.size());
  AxisInterpolation axis1_pos = clampAxisPosition(
      locateAxisPosition(table_axis1, axis1_value, slew_table.front().size()),
      slew_table.front().size());

  if (axis0_pos.low == axis0_pos.high) axis0_pos.high++;
  if (axis1_pos.low == axis1_pos.high) axis1_pos.high++;
  const float v00 = slew_table[axis0_pos.low][axis1_pos.low];
  const float v01 = slew_table[axis0_pos.low][axis1_pos.high];
  const float v10 = slew_table[axis0_pos.high][axis1_pos.low];
  const float v11 = slew_table[axis0_pos.high][axis1_pos.high];
  
  gradients[0] = ((v11 - v01) + (v10 - v00)) / (2*(table_axis0[axis0_pos.high] - table_axis0[axis0_pos.low]));
  gradients[1] = ((v11 - v10) + (v01 - v00)) / (2*(table_axis1[axis1_pos.high] - table_axis1[axis1_pos.low]));


  return gradients;
}

Timing::Timing(Design* design) : design_(design)
{
}

sta::dbSta* Timing::getSta()
{
  return design_->getTech()->getSta();
}

std::pair<odb::dbITerm*, odb::dbBTerm*> Timing::staToDBPin(const sta::Pin* pin)
{
  sta::dbNetwork* db_network = getSta()->getDbNetwork();
  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  odb::dbModITerm* moditerm;
  db_network->staToDb(pin, iterm, bterm, moditerm);
  return std::make_pair(iterm, bterm);
}

bool Timing::isEndpoint(odb::dbITerm* db_pin)
{
  sta::Pin* sta_pin = getSta()->getDbNetwork()->dbToSta(db_pin);
  return isEndpoint(sta_pin);
}

bool Timing::isEndpoint(odb::dbBTerm* db_pin)
{
  sta::Pin* sta_pin = getSta()->getDbNetwork()->dbToSta(db_pin);
  return isEndpoint(sta_pin);
}

bool Timing::isEndpoint(sta::Pin* sta_pin)
{
  auto search = getSta()->search();
  auto vertex_array = vertices(sta_pin);
  for (auto vertex : vertex_array) {
    if (vertex != nullptr && search->isEndpoint(vertex)) {
      return true;
    }
  }
  return false;
}

float Timing::slewAllCorners(sta::Vertex* vertex, const sta::MinMax* minmax)
{
  auto sta = getSta();
  bool max = (minmax == sta::MinMax::max());
  float slew = (max) ? -sta::INF : sta::INF;
  float slew_corner;
  for (auto corner : getCorners()) {
    slew_corner = sta::delayAsFloat(
        sta->vertexSlew(vertex, sta::RiseFall::rise(), corner, minmax));
    slew = (max) ? std::max(slew, slew_corner) : std::min(slew, slew_corner);
  }
  return slew;
}

float Timing::slewAllCorners(sta::Vertex* vertex,
                             const sta::RiseFall* rf,
                             const sta::MinMax* minmax)
{
  auto sta = getSta();
  const bool take_max = (minmax == sta::MinMax::max());
  float slew = take_max ? -sta::INF : sta::INF;
  for (auto corner : getCorners()) {
    const float slew_corner = sta::delayAsFloat(sta->vertexSlew(vertex, rf, corner, minmax));
    slew = take_max ? std::max(slew, slew_corner) : std::min(slew, slew_corner);
  }
  return slew;
}

float Timing::getPinSlew(odb::dbITerm* db_pin, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinSlew(sta_pin, minmax);
}

float Timing::getPinSlew(odb::dbBTerm* db_pin, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinSlew(sta_pin, minmax);
}

float Timing::getPinSlew(sta::Pin* sta_pin, MinMax minmax)
{
  auto vertex_array = vertices(sta_pin);
  float pin_slew = (minmax == Max) ? -sta::INF : sta::INF;
  for (auto vertex : vertex_array) {
    if (vertex != nullptr) {
      const float pin_slew_temp = slewAllCorners(vertex, getMinMax(minmax));
      pin_slew = (minmax == Max) ? std::max(pin_slew, pin_slew_temp)
                                 : std::min(pin_slew, pin_slew_temp);
    }
  }
  return pin_slew;
}

float Timing::getPinSlew(odb::dbITerm* db_pin, RiseFall rf, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinSlew(sta_pin, rf == Rise ? sta::RiseFall::rise() : sta::RiseFall::fall(), minmax);
}

float Timing::getPinSlew(odb::dbBTerm* db_pin, RiseFall rf, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinSlew(sta_pin, rf == Rise ? sta::RiseFall::rise() : sta::RiseFall::fall(), minmax);
}

float Timing::getPinSlew(sta::Pin* sta_pin, const sta::RiseFall* rf, MinMax minmax)
{
  auto vertex_array = vertices(sta_pin);
  float pin_slew = (minmax == Max) ? -sta::INF : sta::INF;
  for (auto vertex : vertex_array) {
    if (vertex != nullptr) {
      const float pin_slew_temp = slewAllCorners(vertex, rf, getMinMax(minmax));
      pin_slew = (minmax == Max) ? std::max(pin_slew, pin_slew_temp)
                                 : std::min(pin_slew, pin_slew_temp);
    }
  }
  return pin_slew;
}

sta::Network* Timing::cmdLinkedNetwork()
{
  sta::Network* network = getSta()->cmdNetwork();
  if (network->isLinked()) {
    return network;
  }

  design_->getLogger()->error(utl::ORD, 104, "STA network is not linked.");
}

sta::Graph* Timing::cmdGraph()
{
  cmdLinkedNetwork();
  return getSta()->ensureGraph();
}

std::array<sta::Vertex*, 2> Timing::vertices(const sta::Pin* pin)
{
  sta::Vertex *vertex, *vertex_bidirect_drvr;
  std::array<sta::Vertex*, 2> vertices;

  cmdGraph()->pinVertices(pin, vertex, vertex_bidirect_drvr);
  vertices[0] = vertex;
  vertices[1] = vertex_bidirect_drvr;
  return vertices;
}

std::vector<float> Timing::arrivalsClk(const sta::RiseFall* rf,
                                       sta::Clock* clk,
                                       const sta::RiseFall* clk_rf,
                                       sta::Vertex* vertex)
{
  auto sta = getSta();
  std::vector<float> arrivals;
  const sta::ClockEdge* clk_edge = nullptr;
  if (clk) {
    clk_edge = clk->edge(clk_rf);
  }
  for (auto path_ap : sta->corners()->pathAnalysisPts()) {
    arrivals.push_back(sta::delayAsFloat(
        sta->vertexArrival(vertex, rf, clk_edge, path_ap, nullptr)));
  }
  return arrivals;
}

bool Timing::isTimeInf(float time)
{
  return (time > 1e+10 || time < -1e+10);
}

float Timing::getPinArrivalTime(sta::Clock* clk,
                                const sta::RiseFall* clk_rf,
                                sta::Vertex* vertex,
                                const sta::RiseFall* arrive_hold)
{
  std::vector<float> times = arrivalsClk(arrive_hold, clk, clk_rf, vertex);
  float delay = -sta::INF;
  for (float delay_time : times) {
    if (!isTimeInf(delay_time)) {
      delay = std::max(delay, delay_time);
    }
  }
  return delay;
}

sta::ClockSeq Timing::findClocksMatching(const char* pattern,
                                         bool regexp,
                                         bool nocase)
{
  auto sta = getSta();
  cmdLinkedNetwork();
  sta::PatternMatch matcher(pattern, regexp, nocase, sta->tclInterp());
  return sta->sdc()->findClocksMatching(&matcher);
}

float Timing::getPinArrival(odb::dbITerm* db_pin, RiseFall rf, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinArrival(sta_pin, rf, minmax);
}

float Timing::getPinArrival(odb::dbBTerm* db_pin, RiseFall rf, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinArrival(sta_pin, rf, minmax);
}

float Timing::getPinArrival(sta::Pin* sta_pin, RiseFall rf, MinMax minmax)
{
  auto vertex_array = vertices(sta_pin);
  float delay = (minmax == Max) ? -sta::INF : sta::INF;
  float d1, d2;
  sta::Clock* default_arrival_clock = getSta()->sdc()->defaultArrivalClock();
  for (auto vertex : vertex_array) {
    if (vertex == nullptr) {
      continue;
    }
    const sta::RiseFall* clk_r = sta::RiseFall::rise();
    const sta::RiseFall* clk_f = sta::RiseFall::fall();
    const sta::RiseFall* arrive_hold = (rf == Rise) ? clk_r : clk_f;
    d1 = getPinArrivalTime(nullptr, clk_r, vertex, arrive_hold);
    d2 = getPinArrivalTime(default_arrival_clock, clk_r, vertex, arrive_hold);
    delay = (minmax == Max) ? std::max({d1, d2, delay})
                            : std::min({d1, d2, delay});
    for (auto clk : findClocksMatching("*", false, false)) {
      d1 = getPinArrivalTime(clk, clk_r, vertex, arrive_hold);
      d2 = getPinArrivalTime(clk, clk_f, vertex, arrive_hold);
      delay = (minmax == Max) ? std::max({d1, d2, delay})
                              : std::min({d1, d2, delay});
    }
  }
  return delay;
}

std::vector<sta::Corner*> Timing::getCorners()
{
  sta::Corners* corners = getSta()->corners();
  return {corners->begin(), corners->end()};
}

sta::Corner* Timing::cmdCorner()
{
  return getSta()->cmdCorner();
}

sta::Corner* Timing::findCorner(const char* name)
{
  for (auto* corner : getCorners()) {
    if (strcmp(corner->name(), name) == 0) {
      return corner;
    }
  }

  return nullptr;
}

std::vector<TimingArcTableModel> Timing::getTimingArcTableModels(
    odb::dbMTerm* from_pin,
    odb::dbMTerm* to_pin)
{
  std::vector<TimingArcTableModel> tables;
  if (from_pin == nullptr || to_pin == nullptr) {
    return tables;
  }

  if (from_pin->getMaster() != to_pin->getMaster()) {
    return tables;
  }

  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Port* from_port = network->dbToSta(from_pin);
  sta::Port* to_port = network->dbToSta(to_pin);
  if (from_port == nullptr || to_port == nullptr) {
    return tables;
  }

  sta::Cell* sta_cell = network->dbToSta(from_pin->getMaster());
  if (sta_cell == nullptr) {
    return tables;
  }

  sta::LibertyCell* lib_cell = network->libertyCell(sta_cell);
  if (lib_cell == nullptr) {
    return tables;
  }

  sta::LibertyPort* from_lib_port = network->libertyPort(from_port);
  sta::LibertyPort* to_lib_port = network->libertyPort(to_port);
  if (from_lib_port == nullptr || to_lib_port == nullptr) {
    return tables;
  }

  appendArcTableModels(lib_cell->timingArcSets(from_lib_port, to_lib_port),
                       tables);
  return tables;
}

std::vector<TimingArcTableModel> Timing::getCellCurrentTableModels(
    odb::dbInst* inst)
{
  std::vector<TimingArcTableModel> tables;
  if (inst == nullptr) {
    return tables;
  }

  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Instance* sta_inst = network->dbToSta(inst);
  if (sta_inst == nullptr) {
    return tables;
  }

  sta::LibertyCell* lib_cell = network->libertyCell(sta_inst);
  if (lib_cell == nullptr) {
    return tables;
  }

  appendArcTableModels(lib_cell->timingArcSets(), tables);
  return tables;
}


std::vector<TimingArcTableModel> Timing::getLibertyCellTableModels(odb::dbMaster* master)
{
  std::vector<TimingArcTableModel> tables;
  if (master == nullptr) {
    return tables;
  }

  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Cell* sta_cell = network->dbToSta(master);
  
  if (sta_cell == nullptr) {
    return tables;
  }
  sta::LibertyCell* lib_cell = network->libertyCell(sta_cell);
  if (lib_cell == nullptr) {
    return tables;
  }
  appendArcTableModels(lib_cell->timingArcSets(), tables);
  return tables;
}

float Timing::gateScaleFactor(odb::dbInst* inst)
{
  if (inst == nullptr) {
    return 1.0f;
  }

  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Instance* sta_inst = network->dbToSta(inst);
  if (sta_inst == nullptr) {
    return 1.0f;
  }

  sta::LibertyCell* lib_cell = network->libertyCell(sta_inst);
  if (lib_cell == nullptr) {
    return 1.0f;
  }

  const sta::MinMax* min_max = getMinMax(Max);
  const sta::Pvt* pvt = sta->pvt(sta_inst, min_max);
  if (pvt == nullptr) {
    return 1.0f;
  }

  const sta::LibertyLibrary* lib = lib_cell->libertyLibrary();
  if (lib == nullptr) {
    return 1.0f;
  }

  return lib->scaleFactor(sta::ScaleFactorType::cell, lib_cell, pvt);
}

float Timing::getPinActivityDensity(odb::dbITerm* db_pin)
{
  if (db_pin == nullptr) {
    return 0.0f;
  }

  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Pin* sta_pin = network->dbToSta(db_pin);
  if (sta_pin == nullptr) {
    return 0.0f;
  }

  return sta->activity(sta_pin).density();
}

float Timing::getVoltage()
{
  sta::dbSta* sta = getSta();
  const sta::MinMax* mm = getMinMax(Max);

  // 確保 OC 存在：用預設 liberty 的 default operating_conditions
  if (sta->operatingConditions(mm) == nullptr) {
    sta::LibertyLibrary* lib = sta->network()->defaultLibertyLibrary();
    if (lib) {
      sta::OperatingConditions* oc = lib->defaultOperatingConditions();
      if (oc) {
        sta->setOperatingConditions(oc, sta::MinMaxAll::all());
      }
    }
  }

  sta::Corner* corner = getCorners()[0];
  if (!corner) {
    std::cout<<"No current corner."<<std::endl;
    return 0.0f;
  }

  const sta::DcalcAnalysisPt* dcalc_ap = corner->findDcalcAnalysisPt(mm);
  if (!dcalc_ap || dcalc_ap->operatingConditions() == nullptr) {
    std::cout<<"Operating conditions not set for max."<<std::endl;
    return 0.0f;
  }

  return dcalc_ap->operatingConditions()->voltage();
}

//dmp_ceff_elmore
float Timing::ScaleFactor(odb::dbInst* inst, TimingArcTableModel& _arcTableModel, std::string type)
{
  const sta::TableModel* _model = nullptr;
  if (type == "delay") 
    _model = _arcTableModel.delay_model;
  else if (type == "slew") 
    _model = _arcTableModel.slew_model;
  else 
    return 1.0f;
  
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Instance* sta_inst = network->dbToSta(inst);
  if (sta_inst == nullptr) {
    return 1.0f;
  }

  sta::LibertyCell* lib_cell = network->libertyCell(sta_inst);
  if (lib_cell == nullptr) {
    return 1.0f;
  }

  const sta::MinMax* min_max = getMinMax(Max);
  const sta::Pvt* pvt = sta->pvt(sta_inst, min_max);
  if (pvt == nullptr) {
    return 1.0f;
  }

  float value = _model->scaleFactor(lib_cell, pvt);
  return value;
}

float Timing::getPinSlack(odb::dbITerm* db_pin, RiseFall rf, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinSlack(sta_pin, rf, minmax);
}

float Timing::getPinSlack(odb::dbBTerm* db_pin, RiseFall rf, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinSlack(sta_pin, rf, minmax);
}

float Timing::getPinSlack(sta::Pin* sta_pin, RiseFall rf, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  auto sta_rf = (rf == Rise) ? sta::RiseFall::rise() : sta::RiseFall::fall();
  return sta->pinSlack(sta_pin, sta_rf, getMinMax(minmax));
}

void Timing::enableDebugPrinting()
{
  sta::dbSta* sta = getSta();
  if (sta == nullptr) {
    return;
  }
  sta::Debug* dbg = sta->debug();
  if (dbg == nullptr) {
    return;
  }
  // Enable a few useful STA categories at level 1.
  dbg->setLevel("delay_calc", 2);
  dbg->setLevel("parasitic_reduce", 2);
  
  dbg->setLevel("graph", 1);
  // Optionally enable stats summary prints.
  dbg->setLevel("stats", 1);
}

static const sta::RiseFall* toStaRiseFall(Timing::RiseFall rf)
{
  return (rf == Timing::Rise) ? sta::RiseFall::rise() : sta::RiseFall::fall();
}

float Timing::graphGateDelay(odb::dbITerm* from_pin,
                             odb::dbITerm* to_pin,
                             sta::Corner* corner,
                             MinMax minmax,
                             std::string from_rf_str,
                             std::string  to_rf_str)
{
  //std::cout<<getSta()->arcDelayCalc()->name()<<std::endl;
  if (from_pin == nullptr || to_pin == nullptr || corner == nullptr) {
    return std::numeric_limits<float>::quiet_NaN();
  }
  RiseFall from_rf, to_rf;
  if (from_rf_str == "^") {
    from_rf = Rise;
  } else if (from_rf_str == "v") {
    from_rf = Fall;
  } else {
    std::cout<<"invalid rise/fall string: " << to_rf_str << std::endl;
    return std::numeric_limits<float>::quiet_NaN();
  }

  if (to_rf_str == "^") {
    to_rf = Rise;
  } else if (to_rf_str == "v") {
    to_rf = Fall;
  } else {
    std::cout<<"invalid rise/fall string: " << to_rf_str << std::endl;
    return std::numeric_limits<float>::quiet_NaN();
  }

  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Pin* in_pin = network->dbToSta(from_pin);
  sta::Pin* drvr_pin = network->dbToSta(to_pin);
  if (in_pin == nullptr || drvr_pin == nullptr) {
    std::cout<<"invalid pins" << std::endl;
    return std::numeric_limits<float>::quiet_NaN();
  }

  sta::Graph* graph = cmdGraph();
  if (graph == nullptr) {
    std::cout<<"invalid graph" << std::endl;
    return std::numeric_limits<float>::quiet_NaN();
  }

  const sta::RiseFall* in_rf = toStaRiseFall(from_rf);
  const sta::RiseFall* out_rf = toStaRiseFall(to_rf);

  sta::Edge* edge = nullptr;
  const sta::TimingArc* arc = nullptr;
  graph->gateEdgeArc(in_pin, in_rf, drvr_pin, out_rf, edge, arc);
  if (edge == nullptr || arc == nullptr) {
    std::cout<<"invalid edge or arc" << std::endl;
    return std::numeric_limits<float>::quiet_NaN();
  }

  const sta::DcalcAnalysisPt* dcalc_ap =
      corner->findDcalcAnalysisPt(getMinMax(minmax));
  if (dcalc_ap == nullptr) {
    std::cout<<"invalid corner or minmax" << std::endl;
    return std::numeric_limits<float>::quiet_NaN();
  }
  sta::DcalcAPIndex ap_index = dcalc_ap->index();

  // If not annotated, don't recompute here; return NaN to indicate missing.
  /*
  if (!graph->arcDelayAnnotated(edge, arc, ap_index)) {
    std::cout<<"arc delay not annotated" << std::endl;
    return std::numeric_limits<float>::quiet_NaN();
  }
  */
  sta::ArcDelay delay = graph->arcDelay(edge, arc, ap_index);
  return sta::delayAsFloat(delay);
}

// I'd like to return a std::set but swig gave me way too much grief
// so I just copy the set to a vector.
std::vector<odb::dbMTerm*> Timing::getTimingFanoutFrom(odb::dbMTerm* input)
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();

  odb::dbMaster* master = input->getMaster();
  sta::Cell* cell = network->dbToSta(master);
  if (!cell) {
    return {};
  }

  sta::LibertyCell* lib_cell = network->libertyCell(cell);
  if (!lib_cell) {
    return {};
  }

  sta::Port* port = network->dbToSta(input);
  sta::LibertyPort* lib_port = network->libertyPort(port);

  std::set<odb::dbMTerm*> outputs;
  for (auto arc_set : lib_cell->timingArcSets(lib_port, /* to */ nullptr)) {
    const sta::TimingRole* role = arc_set->role();
    if (role->isTimingCheck() || role->isAsyncTimingCheck()
        || role->isNonSeqTimingCheck() || role->isDataCheck()) {
      continue;
    }
    sta::LibertyPort* to_port = arc_set->to();
    odb::dbMTerm* to_mterm = master->findMTerm(to_port->name());
    if (to_mterm) {
      outputs.insert(to_mterm);
    }
  }
  return {outputs.begin(), outputs.end()};
}

const sta::MinMax* Timing::getMinMax(MinMax type)
{
  return type == Max ? sta::MinMax::max() : sta::MinMax::min();
}

float Timing::getNetCap(odb::dbNet* net, sta::Corner* corner, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Net* sta_net = sta->getDbNetwork()->dbToSta(net);

  float pin_cap;
  float wire_cap;
  sta->connectedCap(sta_net, corner, getMinMax(minmax), pin_cap, wire_cap);
  return pin_cap + wire_cap;
}

float Timing::getPortCap(odb::dbITerm* pin, sta::Corner* corner, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Pin* sta_pin = network->dbToSta(pin);
  sta::LibertyPort* lib_port = network->libertyPort(sta_pin);
  return sta->capacitance(lib_port, corner, getMinMax(minmax));
}

float Timing::getPortCap(odb::dbMTerm* pin, sta::Corner* corner, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Port* port = network->dbToSta(pin);
  sta::LibertyPort* lib_port = network->libertyPort(port);

  return sta->capacitance(lib_port, corner, getMinMax(minmax));
}

float Timing::getMaxCapLimit(odb::dbMTerm* pin)
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Port* port = network->dbToSta(pin);
  sta::LibertyPort* lib_port = network->libertyPort(port);
  sta::LibertyLibrary* lib = network->defaultLibertyLibrary();
  float max_cap = 0.0;
  bool max_cap_exists = false;
  if (!pin->getSigType().isSupply()) {
    lib_port->capacitanceLimit(sta::MinMax::max(), max_cap, max_cap_exists);
    if (!max_cap_exists) {
      lib->defaultMaxCapacitance(max_cap, max_cap_exists);
    }
  }
  return max_cap;
}

float Timing::getMaxSlewLimit(odb::dbMTerm* pin)
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Port* port = network->dbToSta(pin);
  sta::LibertyPort* lib_port = network->libertyPort(port);
  sta::LibertyLibrary* lib = network->defaultLibertyLibrary();
  float max_slew = 0.0;
  bool max_slew_exists = false;
  if (!pin->getSigType().isSupply()) {
    lib_port->slewLimit(sta::MinMax::max(), max_slew, max_slew_exists);
    if (!max_slew_exists) {
      lib->defaultMaxSlew(max_slew, max_slew_exists);
    }
  }
  return max_slew;
}

float Timing::staticPower(odb::dbInst* inst, sta::Corner* corner)
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();

  sta::Instance* sta_inst = network->dbToSta(inst);
  if (!sta_inst) {
    return 0.0;
  }
  sta::PowerResult power = sta->power(sta_inst, corner);
  return power.leakage();
}

float Timing::dynamicPower(odb::dbInst* inst, sta::Corner* corner)
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();

  sta::Instance* sta_inst = network->dbToSta(inst);
  if (!sta_inst) {
    return 0.0;
  }
  sta::PowerResult power = sta->power(sta_inst, corner);
  return (power.internal() + power.switching());
}

void Timing::makeEquivCells()
{
  rsz::Resizer* resizer = design_->getResizer();
  resizer->makeEquivCells();
}

std::vector<odb::dbMaster*> Timing::equivCells(odb::dbMaster* master)
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Cell* cell = network->dbToSta(master);
  std::vector<odb::dbMaster*> master_seq;
  if (cell) {
    sta::LibertyCell* libcell = network->libertyCell(cell);
    sta::LibertyCellSeq* equiv_cells = sta->equivCells(libcell);
    if (equiv_cells) {
      for (sta::LibertyCell* equiv_cell : *equiv_cells) {
        odb::dbMaster* equiv_master = network->staToDb(equiv_cell);
        master_seq.emplace_back(equiv_master);
      }
    } else {
      master_seq.emplace_back(master);
    }
  }
  return master_seq;
}
}  // namespace ord
