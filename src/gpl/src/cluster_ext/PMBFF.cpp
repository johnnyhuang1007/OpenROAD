#include "gpl/Replace.h"
#include "../clusterBase.h"
#include "../fft.h"
#include "../nesterovPlace.h"
#include "odb/db.h"
#include "../placerBase.h"
#include "utl/Logger.h"


namespace gpl
{

// virtual filler GCells
void ClusterBase::initPMBFFCells()
{
	// extract average dx/dy in range (10%, 90%)
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

	// the avgDx and avgDy will be used as filler cells'
	// width and height
	fillerDx_ = static_cast<int>(dxSum / (maxIdx - minIdx));
	fillerDy_ = static_cast<int>(dySum / (maxIdx - minIdx));

	int64_t coreArea = pb_->die().coreArea();
	whiteSpaceArea_ = coreArea - static_cast<int64_t>(pb_->nonPlaceInstsArea());

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

	float tmp_targetDensity
			= static_cast<float>(stdInstsArea_)
						/ static_cast<float>(whiteSpaceArea_ - macroInstsArea_)
				+ 0.01;
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

	const int fillerCnt = static_cast<int>(
			totalFillerArea_ / static_cast<int64_t>(fillerDx_ * fillerDy_));

	debugPrint(log_, GPL, "FillerInit", 1, "CoreArea {}", coreArea);
	debugPrint(
			log_, GPL, "FillerInit", 1, "nesterovInstsArea {}", nesterovInstanceArea);
	debugPrint(log_, GPL, "FillerInit", 1, "WhiteSpaceArea {}", whiteSpaceArea_);
	debugPrint(log_, GPL, "FillerInit", 1, "MovableArea {}", movableArea_);
	debugPrint(
			log_, GPL, "FillerInit", 1, "TotalFillerArea {}", totalFillerArea_);
	debugPrint(log_, GPL, "FillerInit", 1, "NumFillerCells {}", fillerCnt);
	debugPrint(log_, GPL, "FillerInit", 1, "FillerCellArea {}", fillerCellArea());
	debugPrint(
			log_, GPL, "FillerInit", 1, "FillerCellSize {} {}", fillerDx_, fillerDy_);

	//
	// mt19937 supports huge range of random values.
	// rand()'s RAND_MAX is only 32767.
	//

    PMBFFCnt_ = SBFFCnt_/4+1;

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

		fillerStor_.push_back(myGCell);
	}
}

}
