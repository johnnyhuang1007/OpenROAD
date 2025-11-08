// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Corner.hh"
#include "sta/Delay.hh"
#include "sta/ExceptionPath.hh"
#include "sta/FuncExpr.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/StaState.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"
#include "sta/UnorderedMap.hh"
#include "utl/Logger.h"

namespace est {
class EstimateParasitics;
}

namespace rsz {

using odb::dbMaster;

using odb::dbMaster;
using odb::Point;

using utl::Logger;

using sta::ArcDelay;
using sta::Cell;
using sta::Corner;
using sta::dbDatabase;
using sta::dbNetwork;
using sta::dbSta;
using sta::DcalcAnalysisPt;

using sta::Instance;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::Net;

using sta::InstancePinIterator;
using sta::LeafInstanceIterator;
using sta::NetConnectedPinIterator;

using sta::InstanceSet;
using sta::LoadPinIndexMap;
using sta::Network;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::RiseFall;
using sta::Slack;
using sta::Slew;
using sta::Vertex;

using BufferedNetPtr = std::shared_ptr<BufferedNet>;
using BufferedNetSeq = std::vector<BufferedNetPtr>;
using InputSlews = std::array<Slew, RiseFall::index_count>;
using TgtSlews = std::array<Slew, RiseFall::index_count>;

class weightedInst
{
    public:
        Instance* inst_;
        std::vector<LibertyCell*>   eqvCells;
        std::vector<float>          weights;

        LibertyCell* generateWeightedLibCell(); //it doesn't make a new cell type, it modify weightedLibCell member
        LibertyCell* weightedLibCell;   //its unique Cell type

        weightedInst(Instance* inst); //it doesn't has its actual constructor since it depends on resizer to get eqvCells
        weightedInst();
        ~weightedInst(){delete weightedLibCell;}
        
};



}