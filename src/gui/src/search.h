// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <QObject>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <mutex>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"

namespace gui {

namespace bgi = boost::geometry::index;

// This is a geometric search structure.  It wraps up Boost's
// rtree.  OpenDB also has some code for this purpose but I
// find it confusing so just made a simpler solution for now.
//
// Currently this class is static once built and doesn't follow
// db changes.  TODO: this should be into an observer of OpenDB.
class Search : public QObject, public odb::dbBlockCallBackObj
{
  Q_OBJECT

  template <typename T>
  class MinSizePredicate;

  template <typename T>
  class MinHeightPredicate;

  template <typename T>
  class PolygonIntersectPredicate;

 public:
  template <typename T>
  using LayerMap = std::map<odb::dbTechLayer*, T>;

  template <typename T>
  using RectValue = std::pair<odb::Rect, T>;
  template <typename T>
  using RouteBoxValue = std::tuple<odb::Rect, bool, T>;
  template <typename T>
  using SNetValue = std::tuple<odb::dbSBox*, odb::Polygon, T>;
  template <typename T>
  using SNetDBoxValue = std::pair<odb::dbSBox*, T>;
  ;

  template <typename T>
  struct BBoxIndexableGetter
  {
    using result_type = odb::Rect;
    odb::Rect operator()(T t) const { return t->getBBox()->getBox(); }
    odb::Rect operator()(const SNetValue<T>& t) const
    {
      return std::get<0>(t)->getBox();
    }
    odb::Rect operator()(const SNetDBoxValue<T>& t) const
    {
      return std::get<0>(t)->getBox();
    }
  };

  struct FillIndexableGetter
  {
    using result_type = odb::Rect;
    odb::Rect operator()(odb::dbFill* t) const
    {
      odb::Rect fill;
      t->getRect(fill);
      return fill;
    }
  };

  template <typename T>
  using RtreeRect = bgi::rtree<RectValue<T>, bgi::quadratic<16>>;
  template <typename T>
  using RtreeDBox = bgi::rtree<T, bgi::quadratic<16>, BBoxIndexableGetter<T>>;
  template <typename T>
  using RtreeRoutingShapes = bgi::rtree<RouteBoxValue<T>, bgi::quadratic<16>>;
  template <typename T>
  using RtreeSNetShapes
      = bgi::rtree<SNetValue<T>, bgi::quadratic<16>, BBoxIndexableGetter<T>>;
  template <typename T>
  using RtreeSNetDBoxShapes = bgi::
      rtree<SNetDBoxValue<T>, bgi::quadratic<16>, BBoxIndexableGetter<T>>;
  using RtreeFill
      = bgi::rtree<odb::dbFill*, bgi::quadratic<16>, FillIndexableGetter>;

  // This is an iterator range for return values
  template <typename Tree>
  class Range
  {
   public:
    using Iterator = typename Tree::const_query_iterator;

    Range() = default;
    Range(const Iterator& begin, const Iterator& end) : begin_(begin), end_(end)
    {
    }

    Iterator begin() { return begin_; }
    Iterator end() { return end_; }

   private:
    Iterator begin_;
    Iterator end_;
  };
  using InstRange = Range<RtreeDBox<odb::dbInst*>>;
  using RoutingRange = Range<RtreeRoutingShapes<odb::dbNet*>>;
  using SNetSBoxRange = Range<RtreeSNetDBoxShapes<odb::dbNet*>>;
  using SNetShapeRange = Range<RtreeSNetShapes<odb::dbNet*>>;
  using FillRange = Range<RtreeFill>;
  using ObstructionRange = Range<RtreeDBox<odb::dbObstruction*>>;
  using BlockageRange = Range<RtreeDBox<odb::dbBlockage*>>;
  using RowRange = Range<RtreeRect<odb::dbRow*>>;

  ~Search() override;

  // Build the structure for the given block.
  void setTopBlock(odb::dbBlock* block);

  // Find all box shapes in the given bounds on the given layer which
  // are at least min_size in either dimension.
  RoutingRange searchBoxShapes(odb::dbBlock* block,
                               odb::dbTechLayer* layer,
                               int x_lo,
                               int y_lo,
                               int x_hi,
                               int y_hi,
                               int min_size = 0);

  // Find all via sbox shapes in the given bounds on the given layer which
  // are at least min_size in either dimension.
  SNetSBoxRange searchSNetViaShapes(odb::dbBlock* block,
                                    odb::dbTechLayer* layer,
                                    int x_lo,
                                    int y_lo,
                                    int x_hi,
                                    int y_hi,
                                    int min_size = 0);

  // Find all polgyon shapes in the given bounds on the given layer which
  // are at least min_size in either dimension.
  SNetShapeRange searchSNetShapes(odb::dbBlock* block,
                                  odb::dbTechLayer* layer,
                                  int x_lo,
                                  int y_lo,
                                  int x_hi,
                                  int y_hi,
                                  int min_size = 0);

  // Find all fills in the given bounds on the given layer which
  // are at least min_size in either dimension.
  FillRange searchFills(odb::dbBlock* block,
                        odb::dbTechLayer* layer,
                        int x_lo,
                        int y_lo,
                        int x_hi,
                        int y_hi,
                        int min_size = 0);

  // Find all instances in the given bounds with height of at least min_height
  InstRange searchInsts(odb::dbBlock* block,
                        int x_lo,
                        int y_lo,
                        int x_hi,
                        int y_hi,
                        int min_height = 0);

  // Find all blockages in the given bounds with height of at least min_height
  BlockageRange searchBlockages(odb::dbBlock* block,
                                int x_lo,
                                int y_lo,
                                int x_hi,
                                int y_hi,
                                int min_height = 0);

  // Find all obstructions in the given bounds on the given layer which
  // are at least min_size in either dimension.
  ObstructionRange searchObstructions(odb::dbBlock* block,
                                      odb::dbTechLayer* layer,
                                      int x_lo,
                                      int y_lo,
                                      int x_hi,
                                      int y_hi,
                                      int min_size = 0);

  // Find all rows in the given bounds with height of at least min_height.
  RowRange searchRows(odb::dbBlock* block,
                      int x_lo,
                      int y_lo,
                      int x_hi,
                      int y_hi,
                      int min_height = 0);

  void clearShapes();
  void clearFills();
  void clearInsts();
  void clearBlockages();
  void clearObstructions();
  void clearRows();

  // From dbBlockCallBackObj
  void inDbNetDestroy(odb::dbNet* net) override;
  void inDbInstDestroy(odb::dbInst* inst) override;
  void inDbInstSwapMasterAfter(odb::dbInst* inst) override;
  void inDbInstPlacementStatusBefore(
      odb::dbInst* inst,
      const odb::dbPlacementStatus& status) override;
  void inDbPostMoveInst(odb::dbInst* inst) override;
  void inDbBPinCreate(odb::dbBPin* pin) override;
  void inDbBPinDestroy(odb::dbBPin* pin) override;
  void inDbFillCreate(odb::dbFill* fill) override;
  void inDbWireCreate(odb::dbWire* wire) override;
  void inDbWireDestroy(odb::dbWire* wire) override;
  void inDbSWireCreate(odb::dbSWire* wire) override;
  void inDbSWireDestroy(odb::dbSWire* wire) override;
  void inDbSWireAddSBox(odb::dbSBox* box) override;
  void inDbSWireRemoveSBox(odb::dbSBox* box) override;
  void inDbBlockSetDieArea(odb::dbBlock* block) override;
  void inDbBlockageCreate(odb::dbBlockage* blockage) override;
  void inDbBlockageDestroy(odb::dbBlockage* blockage) override;
  void inDbObstructionCreate(odb::dbObstruction* obs) override;
  void inDbObstructionDestroy(odb::dbObstruction* obs) override;
  void inDbRegionAddBox(odb::dbRegion*, odb::dbBox*) override;
  void inDbRegionDestroy(odb::dbRegion* region) override;
  void inDbRowCreate(odb::dbRow* row) override;
  void inDbRowDestroy(odb::dbRow* row) override;
  void inDbWirePostModify(odb::dbWire* wire) override;

 signals:
  void modified();
  void newBlock(odb::dbBlock* block);

 private:
  struct BlockData;

  void addSNet(odb::dbNet* net,
               LayerMap<std::vector<SNetValue<odb::dbNet*>>>& net_shapes,
               LayerMap<std::vector<SNetDBoxValue<odb::dbNet*>>>& via_shapes);
  void addNet(odb::dbNet* net,
              LayerMap<std::vector<RouteBoxValue<odb::dbNet*>>>& tree_shapes);
  void addVia(odb::dbNet* net,
              odb::dbShape* shape,
              int x,
              int y,
              LayerMap<std::vector<RouteBoxValue<odb::dbNet*>>>& tree_shapes);

  void updateShapes(odb::dbBlock* block);
  void updateFills(odb::dbBlock* block);
  void updateInsts(odb::dbBlock* block);
  void updateBlockages(odb::dbBlock* block);
  void updateObstructions(odb::dbBlock* block);
  void updateRows(odb::dbBlock* block);

  void clear();

  void announceModified(std::atomic_bool& flag);
  BlockData& getData(odb::dbBlock* block);

  odb::dbBlock* top_block_{nullptr};

  struct BlockData
  {
    RtreeDBox<odb::dbInst*> insts;
    RtreeDBox<odb::dbBlockage*> blockages;
    RtreeRect<odb::dbRow*> rows;

    std::mutex shapes_init_mutex;
    std::mutex fills_init_mutex;
    std::mutex insts_init_mutex;
    std::mutex blockages_init_mutex;
    std::mutex obstructions_init_mutex;
    std::mutex rows_init_mutex;

    // The net is used for filter shapes by net type
    LayerMap<RtreeRoutingShapes<odb::dbNet*>> box_shapes;
    // Special net vias may be large multi-cut vias.  It is more efficient
    // to store the dbSBox (ie the via) than all the cuts.  This is
    // particularly true when you have parallel straps like m1 & m2 in asap7.
    LayerMap<RtreeSNetDBoxShapes<odb::dbNet*>> snet_via_shapes;
    LayerMap<RtreeSNetShapes<odb::dbNet*>> snet_shapes;
    LayerMap<RtreeFill> fills;
    LayerMap<RtreeDBox<odb::dbObstruction*>> obstructions;

    std::atomic_bool shapes_init{false};
    std::atomic_bool fills_init{false};
    std::atomic_bool insts_init{false};
    std::atomic_bool blockages_init{false};
    std::atomic_bool obstructions_init{false};
    std::atomic_bool rows_init{false};
  };
  std::map<odb::dbBlock*, BlockData> child_block_data_;
  BlockData top_block_data_;
};

}  // namespace gui
