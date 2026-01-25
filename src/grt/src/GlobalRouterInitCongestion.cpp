// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "grt/GlobalRouter.h"

#include "utl/Logger.h"

namespace grt {

void GlobalRouter::initCongestionMap()
{
  if (db_ == nullptr || db_->getTech() == nullptr) {
    logger_->error(utl::GRT, 58, "Missing dbTech.");
  }

  auto* chip = db_->getChip();
  if (chip == nullptr || chip->getBlock() == nullptr) {
    logger_->error(utl::GRT, 59, "Missing dbBlock.");
  }

  clear();
  block_ = chip->getBlock();

  int min_layer, max_layer;
  getMinMaxLayer(min_layer, max_layer);

  initFastRoute(min_layer, max_layer);

  const bool saved_use_cugr = use_cugr_;
  use_cugr_ = false;
  updateDbCongestion();
  use_cugr_ = saved_use_cugr;
}

}  // namespace grt
