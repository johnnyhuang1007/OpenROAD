#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "gpl/Replace.h"
#include "odb/db.h"
#include "placerBase.h"
#include "nesterovBase.h"
#include "point.h"
#include "routeBase.h"

namespace gpl {

class ClusterBase
{
 public:
	ClusterBase();
	ClusterBase(NesterovBaseVars nbVars,
               std::shared_ptr<PlacerBase> pb,
               std::shared_ptr<NesterovBaseCommon> nbc,
               utl::Logger* log);
	~ClusterBase();
	GCell& getPMBFF(size_t index) {return PMBFFStor_[index];}

	//PMBFF util
	void initPMBFFCells();

	const std::vector<GCellHandle>& getGCells() const { return nb_gcells_; }

	//Pseudo MBFF 
	//used when activate in-placement clustering
	int getPMBFF_Cnt();
	int64_t PMBFF_Area();
	int64_t PMBFF_totCharge();
//	int64_t movableArea() const;

 private:
	NesterovBaseVars nbVars_;
	std::shared_ptr<PlacerBase> pb_;
	std::shared_ptr<NesterovBaseCommon> nbc_;
	utl::Logger* log_ = nullptr;

	//BinGrid bg_;
	//std::unique_ptr<FFT> fft_;

	//int fillerDx_ = 0;
	//int fillerDy_ = 0;
	int64_t whiteSpaceArea_ = 0;
	int64_t movableArea_ = 0;
	int64_t totalFillerArea_ = 0;

	//int64_t stdInstsArea_ = 0;
	//int64_t macroInstsArea_ = 0;
	int64_t ffInstArea_ = 0;
	size_t SBFFCnt_ = 0;

	std::vector<GCell> PMBFFStor_;
	std::vector<GCellHandle> nb_gcells_;

	std::unordered_map<odb::dbInst*, size_t> db_inst_to_nb_index_map_;

	// used to update gcell states after fixPointers() is called
	std::vector<odb::dbInst*> new_instances;

	// Nesterov loop data for each region, using parallel vectors
	// SLP is Step Length Prediction.
	//
	// y_st, y_dst, y_wdst, w_pdst
	std::vector<FloatPoint> curSLPCoordi_;
	std::vector<FloatPoint> curSLPClusterGrads_;

	// y0_st, y0_dst, y0_wdst, y0_pdst
	std::vector<FloatPoint> nextSLPCoordi_;
	std::vector<FloatPoint> nextSLPClusterGrads_;

	// z_st, z_dst, z_wdst, z_pdst
	std::vector<FloatPoint> prevSLPCoordi_;
	std::vector<FloatPoint> prevSLPWireLengthGrads_;
	

	// x_st and x0_st
	std::vector<FloatPoint> curCoordi_;
	std::vector<FloatPoint> nextCoordi_;

	// save initial coordinates -- needed for RD
	std::vector<FloatPoint> initCoordi_;

	float ffClusterGradSum_ = 0;
	// alpha
	float stepLength_ = 0;
    // clustering_cof
    float clusterCoef = 0;

	NesterovPlaceVars* npVars_;

	float minSumOverflow_ = 1e30;
	float hpwlWithMinSumOverflow_ = 1e30;
	int iter_ = 0;
};

}