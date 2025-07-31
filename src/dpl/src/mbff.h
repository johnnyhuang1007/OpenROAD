#ifndef DPL_SRC_MBF_H
#define DPL_SRC_MBF_H

#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

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

#include "GridMap.h"

namespace utl {
class Logger;
}

namespace sta {
class dbNetwork;
class dbSta;
class FuncExpr;
class LibertyCell;
class LibertyPort;
}  // namespace sta

namespace dpl {

// ---------- 1. 用 enum class 定義類型 ----------
enum class MBFFType : std::uint8_t {
  QFF = 0,
  QNFF = 1,
  Q_QNFF = 2,
  ScanQFF = 3,
  ScanQNFF = 4,
  ScanQ_QNFF = 5,
  Count = 6      // ★ 方便計算 array 大小
};

#define MIN 0
#define MAX 1
// ---------- 2. 型別別名，閱讀更清楚 ----------
using MasterVec  = std::vector<odb::dbMaster*>;           // 內層
using Master2D   = std::vector<std::vector<odb::dbMaster*>>;  // 前置宣告，讓 operator[] 可以使用



using Master6Type = std::array<Master2D, static_cast<size_t>(dpl::MBFFType::Count)>;

// ---------- 3. Class 設計 ----------
class FF_LibArray
{
public:
  /* 取用：可用 [] 也可用 at()，兩者都吃 MBFFType ---------- */
    Master2D& operator[](MBFFType type)
    {
        return libs_[idx(type)];
    }
    const Master2D& operator[](MBFFType type) const
    {
        return libs_[idx(type)];
    }

    Master2D& at(MBFFType type)
    {
        return libs_.at(idx(type));           // std::array::at() 會做界限檢查
    }

    odb::dbMaster* getMinAreaFF(MBFFType type, int bits);

private:
    static constexpr size_t idx(MBFFType t)
    {
        return static_cast<size_t>(t);
    }

    Master6Type libs_;   
};



class MBFF_solver
{
    public:
    MBFF_solver(odb::dbDatabase* db,
			 sta::dbSta* sta,
			 utl::Logger* log,
			 int threads,
			 int multistart,
			 int num_paths);
    odb::dbDatabase* db_;
	odb::dbBlock* block_;
	sta::dbSta* sta_;
	sta::dbNetwork* network_;
	sta::Corner* corner_;
	utl::Logger* log_;
    
    //FF lib
    void ReadLibs();    //FF_lib init;
    FF_LibArray FF_lib;
    std::vector<odb::dbMaster*> ffVec;  //flatten FF_libs
    std::vector<odb::dbMaster*> getFlattenFFLibs();
    std::vector<odb::dbMaster*> get_nBit_FFs(int n);
    odb::dbMaster* getAreaFF(dpl::MBFFType, int, bool);

    //physical location information
    std::list<Tile> combTiles;
    Architecture* arch_;
    GridMap gMap_;
    int siteWidth = 0;
    int siteHeight = 0;
    int getSiteWidth() const { return siteWidth; }
    int getSiteHeight() const { return siteHeight; }
    void setSiteWidth(DbuX width);
    void setSiteHeight(DbuY height);
    void buildTileList(std::vector<Node*>& combCells,Architecture* arch);
    void buildGridMap();
    std::vector<std::pair<dpl::dbuPoint, int>>  getInsertable(dbMaster* ff);
    std::vector<insertable> insertableCache;
    void TopDownSplit();
    

    //FlipFlop
    void initFFCells(std::vector<dpl::Node*>& FFCells) {this->FFCells = FFCells;}
    void setFFPins();
    std::vector<dpl::Node*> FFCells;
    std::unordered_map<dpl::Pin*, std::string> PinsMap;

    void generatePinMapping(const std::string& output_filename);
    void printPinMapping();
    

};

}

#endif  // DPL_SRC_MBF_H