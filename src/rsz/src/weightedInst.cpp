#include "weightedInst.h"

namespace rsz {

LibertyCell* weightedInst::generateWeightedLibCell()
{
    if(!weightedLibCell)
    {

        float highestWeight = 0.0;
        LibertyCell* base = eqvCells[0];
        for(auto weight : weights)
        {
            if(weight > highestWeight)
            {
                highestWeight = weight;
                base = eqvCells[&weight - &weights[0]];
            }
        }
        //weightedLibCell = new LibertyCell(*base);
    }



    return weightedLibCell;
}

} // namespace rsz
