// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "initialPlace.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "placerBase.h"
#include "solver.h"

namespace gpl {

using T = Eigen::Triplet<float>;

InitialPlaceVars::InitialPlaceVars()
{
  reset();
}

void InitialPlaceVars::reset()
{
  maxIter = 20;
  minDiffLength = 1500;
  maxSolverIter = 100;
  maxFanout = 200;
  netWeightScale = 800.0;
  debug = false;
}

InitialPlace::InitialPlace(InitialPlaceVars ipVars,
                           std::shared_ptr<PlacerBaseCommon> pbc,
                           std::vector<std::shared_ptr<PlacerBase>>& pbVec,
                           utl::Logger* log)
    : ipVars_(ipVars), pbc_(std::move(pbc)), pbVec_(pbVec), log_(log)
{
  std::cout<<"InitialPlace::InitialPlace() called" << std::endl;
}

void InitialPlace::doBicgstabPlace(int threads)
{
  ResidualError error;

  std::unique_ptr<Graphics> graphics;
  if (ipVars_.debug && Graphics::guiActive()) {
    graphics = std::make_unique<Graphics>(log_, pbc_, pbVec_);
  }

  placeInstsCenter();

  // set ExtId for idx reference // easy recovery
  setPlaceInstExtId();

  if (graphics) {
    graphics->getGuiObjectFromGraphics()->gifStart("initPlacement.gif");
  }

  for (size_t iter = 1; iter <= ipVars_.maxIter; iter++) {
    updatePinInfo();
    createSparseMatrix();
    error = cpuSparseSolve(ipVars_.maxSolverIter,
                           iter,
                           placeInstForceMatrixX_,
                           fixedInstForceVecX_,
                           instLocVecX_,
                           placeInstForceMatrixY_,
                           fixedInstForceVecY_,
                           instLocVecY_,
                           log_,
                           threads);
    float error_max = std::max(error.x, error.y);
    log_->report(
        "[InitialPlace]  Iter: {} conjugate gradient residual: {:0.8f} HPWL: "
        "{}",
        iter,
        error_max,
        pbc_->hpwl());
    updateCoordi();

    if (graphics) {
      graphics->cellPlot(true);

      gui::Gui* gui = graphics->getGuiObjectFromGraphics();
      odb::Rect region;
      odb::Rect bbox = pbc_->db()->getChip()->getBlock()->getBBox()->getBox();
      int max_dim = std::max(bbox.dx(), bbox.dy());
      double dbu_per_pixel = static_cast<double>(max_dim) / 1000.0;
      gui->gifAddFrame(region, 500, dbu_per_pixel, 20);
    }

    if (error_max <= 1e-5 && iter >= 5) {
      break;
    }
  }

  if (graphics) {
    graphics->getGuiObjectFromGraphics()->gifEnd();
  }
}

// starting point of initial place is center.
void InitialPlace::placeInstsCenter()
{
  const int center_x = pbc_->die().coreCx();
  const int center_y = pbc_->die().coreCy();

  int count_region_center = 0;
  int count_db_location = 0;
  int count_core_center = 0;

  for (auto& inst : pbc_->placeInsts()) {
    if (inst->isLocked()) {
      continue;
    }

    const auto db_inst = inst->dbInst();
    const auto group = db_inst->getGroup();

    if (group && group->getType() == odb::dbGroupType::POWER_DOMAIN) {
      auto domain_region = group->getRegion();
      int domain_x_min = std::numeric_limits<int>::max();
      int domain_y_min = std::numeric_limits<int>::max();
      int domain_x_max = std::numeric_limits<int>::min();
      int domain_y_max = std::numeric_limits<int>::min();

      for (auto boundary : domain_region->getBoundaries()) {
        domain_x_min = std::min(domain_x_min, boundary->xMin());
        domain_y_min = std::min(domain_y_min, boundary->yMin());
        domain_x_max = std::max(domain_x_max, boundary->xMax());
        domain_y_max = std::max(domain_y_max, boundary->yMax());
      }

      inst->setCenterLocation(domain_x_max - (domain_x_max - domain_x_min) / 2,
                              domain_y_max - (domain_y_max - domain_y_min) / 2);
      ++count_region_center;
    } else if (pbc_->skipIoMode() && db_inst->isPlaced()) {
      // It is helpful to pick up the placement from mpl if available,
      // particularly when you are going to run skip_io.
      const auto bbox = db_inst->getBBox()->getBox();
      inst->setCenterLocation(bbox.xCenter(), bbox.yCenter());
      ++count_db_location;
    } else {
      inst->setCenterLocation(center_x, center_y);
      ++count_core_center;
    }
  }

  debugPrint(log_,
             GPL,
             "init",
             1,
             "[InitialPlace] origin position counters: region center = {}, db "
             "location = {}, core center = {}",
             count_region_center,
             count_db_location,
             count_core_center);
}

void InitialPlace::setPlaceInstExtId()
{
  // reset ExtId for all instances
  for (auto& inst : pbc_->insts()) {
    inst->setExtId(INT_MAX);
  }
  // set index only with place-able instances
  for (auto& inst : pbc_->placeInsts()) {
    inst->setExtId(&inst - pbc_->placeInsts().data());
  }
}

void InitialPlace::updatePinInfo()
{
  // reset all MinMax attributes
  for (auto& pin : pbc_->pins()) {
    pin->unsetMinPinX();
    pin->unsetMinPinY();
    pin->unsetMaxPinX();
    pin->unsetMaxPinY();
  }

  for (auto& net : pbc_->nets()) {
    Pin *pinMinX = nullptr, *pinMinY = nullptr;
    Pin *pinMaxX = nullptr, *pinMaxY = nullptr;
    int lx = INT_MAX, ly = INT_MAX;
    int ux = INT_MIN, uy = INT_MIN;

    // Mark B2B info on Pin structures
    for (auto& pin : net->pins()) {
      if (lx > pin->cx()) {
        if (pinMinX) {
          pinMinX->unsetMinPinX();
        }
        lx = pin->cx();
        pinMinX = pin;
        pinMinX->setMinPinX();
      }

      if (ux < pin->cx()) {
        if (pinMaxX) {
          pinMaxX->unsetMaxPinX();
        }
        ux = pin->cx();
        pinMaxX = pin;
        pinMaxX->setMaxPinX();
      }

      if (ly > pin->cy()) {
        if (pinMinY) {
          pinMinY->unsetMinPinY();
        }
        ly = pin->cy();
        pinMinY = pin;
        pinMinY->setMinPinY();
      }

      if (uy < pin->cy()) {
        if (pinMaxY) {
          pinMaxY->unsetMaxPinY();
        }
        uy = pin->cy();
        pinMaxY = pin;
        pinMaxY->setMaxPinY();
      }
    }
  }
}

// solve placeInstForceMatrixX_ * xcg_x_ = xcg_b_ and placeInstForceMatrixY_ *
// ycg_x_ = ycg_b_ eq.
void InitialPlace::createSparseMatrix()
{
  const int placeCnt = pbc_->placeInsts().size();
  instLocVecX_.resize(placeCnt);
  fixedInstForceVecX_.resize(placeCnt);
  instLocVecY_.resize(placeCnt);
  fixedInstForceVecY_.resize(placeCnt);

  placeInstForceMatrixX_.resize(placeCnt, placeCnt);
  placeInstForceMatrixY_.resize(placeCnt, placeCnt);

  //
  // listX and listY is a temporary vector that have tuples, (idx1, idx2, val)
  //
  // listX finally becomes placeInstForceMatrixX_
  // listY finally becomes placeInstForceMatrixY_
  //
  // The triplet vector is recommended usages
  // to fill in SparseMatrix from Eigen docs.
  //

  std::vector<T> listX, listY;
  listX.reserve(1000000);
  listY.reserve(1000000);

  // initialize vector
  for (auto& inst : pbc_->placeInsts()) {
    int idx = inst->extId();

    instLocVecX_(idx) = inst->cx();
    instLocVecY_(idx) = inst->cy();

    fixedInstForceVecX_(idx) = fixedInstForceVecY_(idx) = 0;
  }

  // for each net
  for (auto& net : pbc_->nets()) {
    // skip for small nets.
    if (net->pins().size() <= 1) {
      continue;
    }

    // escape long time cals on huge fanout.
    //
    if (net->pins().size() >= ipVars_.maxFanout) {
      continue;
    }

    float netWeight = ipVars_.netWeightScale / (net->pins().size() - 1);

    // foreach two pins in single nets.
    auto& pins = net->pins();
    for (int pinIdx1 = 1; pinIdx1 < pins.size(); ++pinIdx1) {
      Pin* pin1 = pins[pinIdx1];
      for (int pinIdx2 = 0; pinIdx2 < pinIdx1; ++pinIdx2) {
        Pin* pin2 = pins[pinIdx2];

        // no need to fill in when instance is same
        if (pin1->instance() == pin2->instance()) {
          continue;
        }

        // B2B modeling on min/maxX pins.
        if (pin1->isMinPinX() || pin1->isMaxPinX() || pin2->isMinPinX()
            || pin2->isMaxPinX()) {
          int diffX = abs(pin1->cx() - pin2->cx());
          float weightX = 0;
          if (diffX > ipVars_.minDiffLength) {
            weightX = netWeight / diffX;
          } else {
            weightX = netWeight / ipVars_.minDiffLength;
          }

          // both pin cames from instance
          if (pin1->isPlaceInstConnected() && pin2->isPlaceInstConnected()) {
            const int inst1 = pin1->instance()->extId();
            const int inst2 = pin2->instance()->extId();

            listX.emplace_back(inst1, inst1, weightX);
            listX.emplace_back(inst2, inst2, weightX);

            listX.emplace_back(inst1, inst2, -weightX);
            listX.emplace_back(inst2, inst1, -weightX);

            fixedInstForceVecX_(inst1)
                += -weightX
                   * ((pin1->cx() - pin1->instance()->cx())
                      - (pin2->cx() - pin2->instance()->cx()));

            fixedInstForceVecX_(inst2)
                += -weightX
                   * ((pin2->cx() - pin2->instance()->cx())
                      - (pin1->cx() - pin1->instance()->cx()));
          }
          // pin1 from IO port / pin2 from Instance
          else if (!pin1->isPlaceInstConnected()
                   && pin2->isPlaceInstConnected()) {
            const int inst2 = pin2->instance()->extId();
            listX.emplace_back(inst2, inst2, weightX);

            fixedInstForceVecX_(inst2)
                += weightX
                   * (pin1->cx() - (pin2->cx() - pin2->instance()->cx()));
          }
          // pin1 from Instance / pin2 from IO port
          else if (pin1->isPlaceInstConnected()
                   && !pin2->isPlaceInstConnected()) {
            const int inst1 = pin1->instance()->extId();
            listX.emplace_back(inst1, inst1, weightX);

            fixedInstForceVecX_(inst1)
                += weightX
                   * (pin2->cx() - (pin1->cx() - pin1->instance()->cx()));
          }
        }

        // B2B modeling on min/maxY pins.
        if (pin1->isMinPinY() || pin1->isMaxPinY() || pin2->isMinPinY()
            || pin2->isMaxPinY()) {
          int diffY = abs(pin1->cy() - pin2->cy());
          float weightY = 0;
          if (diffY > ipVars_.minDiffLength) {
            weightY = netWeight / diffY;
          } else {
            weightY = netWeight / ipVars_.minDiffLength;
          }

          // both pin cames from instance
          if (pin1->isPlaceInstConnected() && pin2->isPlaceInstConnected()) {
            const int inst1 = pin1->instance()->extId();
            const int inst2 = pin2->instance()->extId();

            listY.emplace_back(inst1, inst1, weightY);
            listY.emplace_back(inst2, inst2, weightY);

            listY.emplace_back(inst1, inst2, -weightY);
            listY.emplace_back(inst2, inst1, -weightY);

            fixedInstForceVecY_(inst1)
                += -weightY
                   * ((pin1->cy() - pin1->instance()->cy())
                      - (pin2->cy() - pin2->instance()->cy()));

            fixedInstForceVecY_(inst2)
                += -weightY
                   * ((pin2->cy() - pin2->instance()->cy())
                      - (pin1->cy() - pin1->instance()->cy()));
          }
          // pin1 from IO port / pin2 from Instance
          else if (!pin1->isPlaceInstConnected()
                   && pin2->isPlaceInstConnected()) {
            const int inst2 = pin2->instance()->extId();
            listY.emplace_back(inst2, inst2, weightY);

            fixedInstForceVecY_(inst2)
                += weightY
                   * (pin1->cy() - (pin2->cy() - pin2->instance()->cy()));
          }
          // pin1 from Instance / pin2 from IO port
          else if (pin1->isPlaceInstConnected()
                   && !pin2->isPlaceInstConnected()) {
            const int inst1 = pin1->instance()->extId();
            listY.emplace_back(inst1, inst1, weightY);

            fixedInstForceVecY_(inst1)
                += weightY
                   * (pin2->cy() - (pin1->cy() - pin1->instance()->cy()));
          }
        }
      }
    }
  }

  placeInstForceMatrixX_.setFromTriplets(listX.begin(), listX.end());
  placeInstForceMatrixY_.setFromTriplets(listY.begin(), listY.end());
}

void InitialPlace::updateCoordi()
{
  for (auto& inst : pbc_->placeInsts()) {
    int idx = inst->extId();
    if (!inst->isLocked()) {
      inst->dbSetCenterLocation(instLocVecX_(idx), instLocVecY_(idx));
      inst->dbSetPlaced();
    }
  }
}

}  // namespace gpl
