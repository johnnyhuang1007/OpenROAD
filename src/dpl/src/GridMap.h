
#ifndef GRID_MAP_H
#define GRID_MAP_H

#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "dbuPoint.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "sta/Corner.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/ClkNetwork.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/ExceptionPath.hh"
#include "sta/FuncExpr.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InputDrive.hh"
#include "sta/Liberty.hh"
#include "sta/Parasitics.hh"
#include "sta/PathAnalysisPt.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Sequential.hh"
#include "sta/TimingArc.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

#include "dpl/Opendp.h"
#include "graphics/DplObserver.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Tile.h"
#include "infrastructure/Objects.h"
#include "infrastructure/Padding.h"
#include "infrastructure/network.h"


namespace dpl
{

struct tileGrid
{
    std::vector<Tile*> fixedtiles;
    std::list<dpl::Node*> FF_nodes;
    std::list<insertable> valid_spaces;
    std::unique_ptr<tileGrid> descendants[2][2];
    tileGrid* parent;
    double x_;
    double y_;
    double width_;
    double height_;
};

struct GridMap
{
    
    std::vector<std::vector<tileGrid>> tbl; //uniform
    std::unique_ptr<tileGrid> header;   //tree based
    int siteWidth = 0;
    int siteHeight = 0;

    int getSiteWidth() const { return siteWidth; }
    int getSiteHeight() const { return siteHeight; }
    void setSiteWidth(DbuX width) { siteWidth = width.v; }
    void setSiteHeight(DbuY height) { siteHeight = height.v; }
    std::pair<int, int> getGridIndex(const dpl::dbuPoint& p) const;
    void setInsertableTile(std::vector<insertable>&);
    void addInsertableTile(std::vector<insertable>&);
    void addFFNodes(std::vector<dpl::Node*>& FF_nodes);
    void setFFNodes(std::vector<dpl::Node*>& FF_nodes);

    void TopDownSplit(tileGrid* grid,
                  std::vector<Node*> FFs,
                  std::vector<insertable> inserts);

};

void call_tree(tileGrid*);

dpl::dbuPoint median(std::vector<dpl::Node*>&, bool(*)(Node*,Node*));
std::vector<insertable> splitByCoordX(insertable, double);

}

#endif