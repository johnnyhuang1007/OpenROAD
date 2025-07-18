// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

%{
#include "ord/OpenRoad.hh"
#include "gpl/Replace.h"
#include "odb/db.h"

namespace ord {
OpenRoad*
getOpenRoad();

gpl::Replace*
getReplace();

}

using ord::getOpenRoad;
using ord::getReplace;
using gpl::Replace;

%}

%import <std_vector.i>
%import "dbtypes.i"
%import "dbenums.i"
%include "../../Exception.i"

%inline %{

void
placement_cluster_cmd(const std::vector<odb::dbInst*>& cluster)
{
  Replace* replace = getReplace();
  replace->addPlacementCluster(cluster);
}

void 
replace_reset_cmd() 
{
  Replace* replace = getReplace();  
  replace->reset();
}

void 
replace_initial_place_cmd()
{
  Replace* replace = getReplace();
  int threads = ord::OpenRoad::openRoad()->getThreadCount();
  replace->doInitialPlace(threads);
}

void 
replace_nesterov_place_cmd()
{
  Replace* replace = getReplace();
  int threads = ord::OpenRoad::openRoad()->getThreadCount();
  std::cout<<"replace_nesterov_place_cmd()" << std::endl;
  replace->doNesterovPlace(threads);
}


void
replace_run_mbff_cmd(int max_sz, float alpha, float beta, int num_paths) 
{
  Replace* replace = getReplace();
  int threads = ord::OpenRoad::openRoad()->getThreadCount();
  replace->runMBFF(max_sz, alpha, beta, threads, num_paths);   
}


void
set_density_cmd(float density)
{
  Replace* replace = getReplace();
  replace->setTargetDensity(density);
}

void
set_uniform_target_density_mode_cmd(bool uniform)
{
  Replace* replace = getReplace();
  replace->setUniformTargetDensityMode(uniform);
}

void
set_initial_place_max_iter_cmd(int iter)
{
  Replace* replace = getReplace();
  replace->setInitialPlaceMaxIter(iter); 
}

void
set_initial_place_max_fanout_cmd(int fanout)
{
  Replace* replace = getReplace();
  replace->setInitialPlaceMaxFanout(fanout);
}

void
set_nesv_place_iter_cmd(int iter)
{
  Replace* replace = getReplace();
  replace->setNesterovPlaceMaxIter(iter);
}

void
set_bin_grid_cnt_cmd(int cnt_x, int cnt_y)
{
  Replace* replace = getReplace();
  replace->setBinGridCnt(cnt_x, cnt_y);
}

void
set_overflow_cmd(float overflow)
{
  Replace* replace = getReplace();
  replace->setTargetOverflow(overflow);
}

void
set_min_phi_coef_cmd(float min_phi_coef)
{
  Replace* replace = getReplace();
  replace->setMinPhiCoef(min_phi_coef);
}

void
set_max_phi_coef_cmd(float max_phi_coef) 
{
  Replace* replace = getReplace();
  replace->setMaxPhiCoef(max_phi_coef);
}

void
set_reference_hpwl_cmd(float reference_hpwl)
{
  Replace* replace = getReplace();
  replace->setReferenceHpwl(reference_hpwl);
}

void
set_init_density_penalty_factor_cmd(float penaltyFactor)
{
  Replace* replace = getReplace();
  replace->setInitDensityPenalityFactor(penaltyFactor);
}

void
set_init_wirelength_coef_cmd(float coef)
{
  Replace* replace = getReplace();
  replace->setInitWireLengthCoef(coef);
}

void
replace_incremental_place_cmd()
{
  Replace* replace = getReplace();
  int threads = ord::OpenRoad::openRoad()->getThreadCount();
  replace->doIncrementalPlace(threads);
}


void set_timing_driven_mode(bool timing_driven)
{
  std::cout<<"set_timing_driven_mode: " << timing_driven << std::endl;
  Replace* replace = getReplace();
  replace->setTimingDrivenMode(timing_driven);
}

void set_ff_clustering_mode(bool ff_clustering)
{
  std::cout<<"set_ff_clustering_mode: " << ff_clustering << std::endl;
  Replace* replace = getReplace();
  replace->setFFClusteringMode(ff_clustering);
}


void
set_keep_resize_below_overflow_cmd(float overflow) 
{
  Replace* replace = getReplace();
  replace->setKeepResizeBelowOverflow(overflow);
}

void
set_routability_driven_mode(bool routability_driven)
{
  Replace* replace = getReplace();
  replace->setRoutabilityDrivenMode(routability_driven);
}

void
set_routability_use_grt(bool use_grt)
{
  Replace* replace = getReplace();
  replace->setRoutabilityUseGrt(use_grt);
}

void
set_routability_check_overflow_cmd(float overflow) 
{
  Replace* replace = getReplace();
  replace->setRoutabilityCheckOverflow(overflow);
}
 
void
set_routability_max_density_cmd(float density) 
{
  Replace* replace = getReplace();
  replace->setRoutabilityMaxDensity(density);
}

void
set_routability_max_inflation_iter_cmd(int iter) 
{
  Replace* replace = getReplace();
  replace->setRoutabilityMaxInflationIter(iter);
}

void
set_routability_target_rc_metric_cmd(float rc)
{
  Replace* replace = getReplace();
  replace->setRoutabilityTargetRcMetric(rc);
}

void
set_routability_inflation_ratio_coef_cmd(float coef)
{
  Replace* replace = getReplace();
  replace->setRoutabilityInflationRatioCoef(coef);
}

void
set_routability_max_inflation_ratio_cmd(float ratio) 
{
  Replace* replace = getReplace();
  replace->setRoutabilityMaxInflationRatio(ratio);
}

void
set_routability_rc_coefficients_cmd(float k1,
                                    float k2,
                                    float k3,
                                    float k4)
{
  Replace* replace = getReplace();
  replace->setRoutabilityRcCoefficients(k1, k2, k3, k4);
}


void
set_pad_left_cmd(int pad) 
{
  Replace* replace = getReplace();
  replace->setPadLeft(pad);
}

void
set_pad_right_cmd(int pad) 
{
  Replace* replace = getReplace();
  replace->setPadRight(pad);
}

void
set_skip_io_mode_cmd(bool mode) 
{
  Replace* replace = getReplace();
  replace->setSkipIoMode(mode);
}

void
set_disable_revert_if_diverge(bool disable_revert_if_diverge)
{
  Replace* replace = getReplace();
  replace->setDisableRevertIfDiverge(disable_revert_if_diverge);
}

void
set_enable_routing_congestion(bool enable_routing_congestion)
{
  Replace* replace = getReplace();
  replace->setEnableRoutingCongestion(enable_routing_congestion);
}

float
get_global_placement_uniform_density_cmd() 
{
  Replace* replace = getReplace();
  int threads = ord::OpenRoad::openRoad()->getThreadCount();
  return replace->getUniformTargetDensity(threads);
}

void 
add_timing_net_reweight_overflow_cmd(int overflow)
{
  Replace* replace = getReplace();
  return replace->addTimingNetWeightOverflow(overflow);
}

void
set_timing_driven_net_weight_max_cmd(float max)
{
  Replace* replace = getReplace();
  return replace->setTimingNetWeightMax(max);
}



void
set_debug_cmd(int pause_iterations,
              int update_iterations,
              bool draw_bins,
              bool initial,
              const char* inst_name,
              int start_iter,
              bool generate_images,
              const char* images_path)
{
  Replace* replace = getReplace();
  odb::dbInst* inst = nullptr;
  if (inst_name) {
    auto block = ord::OpenRoad::openRoad()->getDb()->getChip()->getBlock();
    inst = block->findInst(inst_name);
  }

  std::string resolved_path = (images_path && *images_path)
                                  ? images_path
                                  : "REPORTS_DIR";

  replace->setDebug(pause_iterations, update_iterations, draw_bins,
                    initial, inst, start_iter, generate_images,
                    resolved_path);
}

%} // inline
