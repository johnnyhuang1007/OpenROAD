#include "diffGB.h"


namespace rsz {


void diffSizer::initweightedInsts()
{
    diffweightedInsts.clear();
    LeafInstanceIterator* it = network_->leafInstanceIterator();

    resizer_->makeEquivCells();

    //diffweightedInsts.reserve(network->numLeafInstances());
    while (it->hasNext()) 
    {
        Instance* inst = it->next();
        weightedInst new_wc = makeweightedInst(inst);
        //use move to avoid inner uni_ptr copy
    }
    delete it;
}

weightedInst diffSizer::makeweightedInst(Instance* inst)
{
    weightedInst wc;
    
    LibertyCell* base = network_->libertyCell(inst);
    LibertyCellSeq eqvs = resizer_->getSwappableCells(base);    //std::vector<LibertyCell*>

    wc.inst_ = inst;
    wc.eqvCells = eqvs;
    for(auto cell : eqvs)
    {
        if (cell == base) wc.weights.push_back(1.0);
        else wc.weights.push_back(0.0);
    }

    wc.generateWeightedLibCell();

    return wc;
}

}