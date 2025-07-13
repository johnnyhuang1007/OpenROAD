#include "gpl/Replace.h"
#include "../clusterBase.h"
#include "../fft.h"
#include "../nesterovPlace.h"
#include "odb/db.h"
#include "../placerBase.h"
#include "utl/Logger.h"

namespace gpl {

ClusterBase::ClusterBase()
{
    std::cout<<"ClusterBase constructor called"<<std::endl;
}

ClusterBase::~ClusterBase()
{
    std::cout<<"ClusterBase destructor called"<<std::endl;
}

ClusterBase::ClusterBase(NesterovBaseVars nbVars,
                        std::shared_ptr<PlacerBase> pb,
                        std::shared_ptr<NesterovBaseCommon> nbc,
                        utl::Logger* log)
{
    nbVars_ = nbVars;
	pb_ = std::move(pb);
	nbc_ = std::move(nbc);
	log_ = log;

	// Set a fixed seed
	srand(42);
	// area update from pb
    // To check if the cell is sequential
    // Find the dbmaster first then find isSequential inside dbmaster
    std::vector<Instance*> instVec_ = pb_->insts();
    for(size_t i = 0 ; i < instVec_.size() ; i++)
    {
        Instance* inst = instVec_[i];
        if(inst->dbInst()->getMaster()->isSequential())
        {
            ffInstArea_ += inst->area();
            SBFFCnt_ += inst->getBitCnt();
        }
    }

	

	int dbu_per_micron = pb_->db()->getChip()->getBlock()->getDbUnitsPerMicron();

	// update gFillerCells
	initPMBFFCells();
    
	nb_gcells_.reserve(pb_->insts().size() + PMBFFStor_.size());

	// add place instances
	for (auto& pb_inst : pb_->placeInsts()) {
		if(!pb_inst->isFF()) {
            continue; // Skip non-FF instances
        }

		GCell* gCell = nbc_->pbToNb(pb_inst);
		if (pb_inst != gCell->insts()[0]) {
			// Only process the first cluster once
			continue;
		}

		nb_gcells_.emplace_back(nbc_.get(), nbc_->getGCellIndex(gCell));
		size_t gcells_index = nb_gcells_.size() - 1;
		db_inst_to_nb_index_map_[pb_inst->dbInst()] = gcells_index;
	}





    
	for (size_t i = 0; i < PMBFFStor_.size(); ++i) {
		nb_gcells_.emplace_back(this, i);
	}
    /*Initialize self-implemented bingrid here*/

	/*
	// initialize bin grid structure
	// send param into binGrid structure
	if (nbVars_.isSetBinCnt) {
		bg_.setBinCnt(nbVars_.binCntX, nbVars_.binCntY);
	}

	bg_.setPlacerBase(pb_);
	bg_.setLogger(log_);
	bg_.setCorePoints(&(pb_->die()));
	bg_.setTargetDensity(targetDensity_);

	// update binGrid info
	bg_.initBins();

	// initialize fft structrue based on bins
	std::unique_ptr<FFT> fft(
			new FFT(bg_.binCntX(), bg_.binCntY(), bg_.binSizeX(), bg_.binSizeY()));

	fft_ = std::move(fft);

	// update densitySize and densityScale in each gCell
	updateDensitySize();
    */
}

}