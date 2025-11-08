#ifndef DIFFGB_H
#define DIFFGB_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>
#include <memory>

#include "weightedInst.h"

namespace est {
class EstimateParasitics;
}

namespace rsz {

class diffSizer
{
    public:
        std::vector<std::unique_ptr<weightedInst>> diffweightedInsts;
        diffSizer(Resizer* resizer_) : resizer_(resizer_) {}
        Network* network_; //sta network
        Resizer* resizer_;

        void initweightedInsts();
        weightedInst makeweightedInst(Instance* inst);


};

}

#endif