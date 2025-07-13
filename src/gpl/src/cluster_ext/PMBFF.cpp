#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <random>

#include "gpl/Replace.h"
#include "../clusterBase.h"
#include "../fft.h"
#include "../nesterovPlace.h"
#include "odb/db.h"
#include "../placerBase.h"
#include "utl/Logger.h"


namespace gpl
{

using odb::dbBlock;
using utl::GPL;

void ClusterBase::initPMBFFCells()
{

	std::vector<int> dxStor;
	std::vector<int> dyStor;

	dxStor.reserve(pb_->placeInsts().size());
	dyStor.reserve(pb_->placeInsts().size());
	for (auto& placeInst : pb_->placeInsts()) {
		dxStor.push_back(placeInst->dx());
		dyStor.push_back(placeInst->dy());
	}

	// sort
	std::sort(dxStor.begin(), dxStor.end());
	std::sort(dyStor.begin(), dyStor.end());

	// average from (10 - 90%) .
	int64_t dxSum = 0, dySum = 0;

	int minIdx = dxStor.size() * 0.05;
	int maxIdx = dxStor.size() * 0.95;

	// when #instances are too small,
	// extracts average values in whole ranges.
	if (minIdx == maxIdx) {
		minIdx = 0;
		maxIdx = dxStor.size();
	}

	// This should never happen (implies no placeable insts) but it
	// quiets clang-tidy.
	if (maxIdx == minIdx) {
		return;
	}

	for (int i = minIdx; i < maxIdx; i++) {
		dxSum += dxStor[i];
		dySum += dyStor[i];
	}


	int64_t coreArea = pb_->die().coreArea();


	// if(pb_->group() == nullptr) {
	//   // nonPlaceInstsArea should not have density downscaling!!!
	//   whiteSpaceArea_ = coreArea - pb_->nonPlaceInstsArea();
	// } else {
	//   int64_t domainArea = 0;
	//   for(auto boundary: pb_->group()->getRegion()->getBoundaries()) {
	//     domainArea += boundary->getBox().area();
	//   }
	//   whiteSpaceArea_ = domainArea - pb_->nonPlaceInstsArea();
	// }

	/*
	// targetDensity initialize
	if (nbVars_.useUniformTargetDensity) {
		targetDensity_ = tmp_targetDensity;
	} else {
		targetDensity_ = nbVars_.targetDensity;
	}

	const int64_t nesterovInstanceArea = nesterovInstsArea();

	
	// TODO density screening
	movableArea_ = whiteSpaceArea_ * targetDensity_;

	totalFillerArea_ = movableArea_ - nesterovInstanceArea;
	uniformTargetDensity_ = static_cast<float>(nesterovInstanceArea)
													/ static_cast<float>(whiteSpaceArea_);
	uniformTargetDensity_ = ceilf(uniformTargetDensity_ * 100) / 100;

	if (totalFillerArea_ < 0) {
		log_->warn(GPL,
							 312,
							 "Target density {:.4f} is too low for the available free area.\n"
							 "Automatically adjusting to uniform density {:.4f}.",
							 targetDensity_,
							 uniformTargetDensity_);
		targetDensity_ = uniformTargetDensity_;
		movableArea_ = whiteSpaceArea_ * targetDensity_;
		totalFillerArea_ = movableArea_ - nesterovInstanceArea;
	}
	*/

	/*
	debugPrint(log_, GPL, "FillerInit", 1, "CoreArea {}", coreArea);
	debugPrint(
			log_, GPL, "FillerInit", 1, "nesterovInstsArea {}", nesterovInstanceArea);
	debugPrint(log_, GPL, "FillerInit", 1, "NumFillerCells {}", fillerCnt);
	debugPrint(log_, GPL, "FillerInit", 1, "FillerCellArea {}", fillerCellArea());
	*/

    int PMBFFCnt_ = SBFFCnt_/4+1;

	std::mt19937 randVal(0);
	for (int i = 0; i < PMBFFCnt_; i++) {
		// instability problem between g++ and clang++!
		auto randX = randVal();
		auto randY = randVal();

		// place filler cells on random coordi and
		// set size as avgDx and avgDy
		GCell myGCell(randX % pb_->die().coreDx() + pb_->die().coreLx(),
									randY % pb_->die().coreDy() + pb_->die().coreLy(),
									1,
									1,
                                    -4); //TODO find the largest bit ff in lib

		PMBFFStor_.push_back(myGCell);
	}
}

}
