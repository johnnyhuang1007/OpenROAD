// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbMaster.h"

#include <string>

#include "dbBlock.h"
#include "dbBox.h"
#include "dbBoxItr.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbLib.h"
#include "dbMPin.h"
#include "dbMPinItr.h"
#include "dbMTerm.h"
#include "dbMasterEdgeType.h"
#include "dbPolygon.h"
#include "dbPolygonItr.h"
#include "dbSite.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayerAntennaRule.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "utl/Logger.h"

namespace odb {

class _dbInstHdr;

template class dbHashTable<_dbMTerm>;
template class dbTable<_dbMaster>;

bool _dbMaster::operator==(const _dbMaster& rhs) const
{
  if (_flags._frozen != rhs._flags._frozen) {
    return false;
  }

  if (_flags._x_symmetry != rhs._flags._x_symmetry) {
    return false;
  }

  if (_flags._y_symmetry != rhs._flags._y_symmetry) {
    return false;
  }

  if (_flags._R90_symmetry != rhs._flags._R90_symmetry) {
    return false;
  }

  if (_flags._type != rhs._flags._type) {
    return false;
  }

  if (_x != rhs._x) {
    return false;
  }

  if (_y != rhs._y) {
    return false;
  }

  if (_height != rhs._height) {
    return false;
  }

  if (_width != rhs._width) {
    return false;
  }

  if (_mterm_cnt != rhs._mterm_cnt) {
    return false;
  }

  if (_id != rhs._id) {
    return false;
  }

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0) {
      return false;
    }
  } else if (_name || rhs._name) {
    return false;
  }

  if (_next_entry != rhs._next_entry) {
    return false;
  }

  if (_leq != rhs._leq) {
    return false;
  }

  if (_eeq != rhs._eeq) {
    return false;
  }

  if (_obstructions != rhs._obstructions) {
    return false;
  }

  if (_poly_obstructions != rhs._poly_obstructions) {
    return false;
  }

  if (_lib_for_site != rhs._lib_for_site) {
    return false;
  }

  if (_site != rhs._site) {
    return false;
  }

  if (_mterm_hash != rhs._mterm_hash) {
    return false;
  }

  if (*_mterm_tbl != *rhs._mterm_tbl) {
    return false;
  }

  if (*_mpin_tbl != *rhs._mpin_tbl) {
    return false;
  }

  if (*_box_tbl != *rhs._box_tbl) {
    return false;
  }

  if (*_poly_box_tbl != *rhs._poly_box_tbl) {
    return false;
  }

  if (*_antenna_pin_model_tbl != *rhs._antenna_pin_model_tbl) {
    return false;
  }

  if (*edge_types_tbl_ != *rhs.edge_types_tbl_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// _dbMaster - Methods
//
////////////////////////////////////////////////////////////////////
_dbMaster::_dbMaster(_dbDatabase* db)
{
  _flags._x_symmetry = 0;
  _flags._y_symmetry = 0;
  _flags._R90_symmetry = 0;
  _flags._type = dbMasterType::CORE;
  _flags._frozen = 0;
  _flags._mark = 0;
  _flags._special_power = 0;
  _flags._sequential = 0;
  _flags._spare_bits_19 = 0;

  _x = 0;
  _y = 0;
  _height = 0;
  _width = 0;
  _mterm_cnt = 0;
  _id = 0;
  _name = nullptr;

  _mterm_tbl = new dbTable<_dbMTerm, 4>(
      db, this, (GetObjTbl_t) &_dbMaster::getObjectTable, dbMTermObj);

  _mpin_tbl = new dbTable<_dbMPin, 4>(
      db, this, (GetObjTbl_t) &_dbMaster::getObjectTable, dbMPinObj);

  _box_tbl = new dbTable<_dbBox, 8>(
      db, this, (GetObjTbl_t) &_dbMaster::getObjectTable, dbBoxObj);

  _poly_box_tbl = new dbTable<_dbPolygon, 8>(
      db, this, (GetObjTbl_t) &_dbMaster::getObjectTable, dbPolygonObj);

  _antenna_pin_model_tbl = new dbTable<_dbTechAntennaPinModel, 8>(
      db,
      this,
      (GetObjTbl_t) &_dbMaster::getObjectTable,
      dbTechAntennaPinModelObj);
  edge_types_tbl_ = new dbTable<_dbMasterEdgeType, 8>(
      db, this, (GetObjTbl_t) &_dbMaster::getObjectTable, dbMasterEdgeTypeObj);

  _box_itr = new dbBoxItr<8>(_box_tbl, _poly_box_tbl, true);

  _pbox_itr = new dbPolygonItr(_poly_box_tbl);

  _pbox_box_itr = new dbBoxItr<8>(_box_tbl, _poly_box_tbl, false);

  _mpin_itr = new dbMPinItr(_mpin_tbl);

  _mterm_hash.setTable(_mterm_tbl);

  _sta_cell = nullptr;
}

_dbMaster::~_dbMaster()
{
  delete _mterm_tbl;
  delete _mpin_tbl;
  delete _box_tbl;
  delete _poly_box_tbl;
  delete _antenna_pin_model_tbl;
  delete edge_types_tbl_;
  delete _box_itr;
  delete _pbox_itr;
  delete _pbox_box_itr;
  delete _mpin_itr;

  if (_name) {
    free((void*) _name);
  }
}

dbOStream& operator<<(dbOStream& stream, const _dbMaster& master)
{
  uint* bit_field = (uint*) &master._flags;
  stream << *bit_field;
  stream << master._x;
  stream << master._y;
  stream << master._height;
  stream << master._width;
  stream << master._mterm_cnt;
  stream << master._id;
  stream << master._name;
  stream << master._next_entry;
  stream << master._leq;
  stream << master._eeq;
  stream << master._obstructions;
  stream << master._poly_obstructions;
  stream << master._lib_for_site;
  stream << master._site;
  stream << master._mterm_hash;
  stream << *master._mterm_tbl;
  stream << *master._mpin_tbl;
  stream << *master._box_tbl;
  stream << *master._poly_box_tbl;
  stream << *master._antenna_pin_model_tbl;
  stream << *master.edge_types_tbl_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbMaster& master)
{
  _dbDatabase* db = master.getImpl()->getDatabase();
  uint* bit_field = (uint*) &master._flags;
  stream >> *bit_field;
  stream >> master._x;
  stream >> master._y;
  stream >> master._height;
  stream >> master._width;
  stream >> master._mterm_cnt;
  stream >> master._id;
  stream >> master._name;
  stream >> master._next_entry;
  stream >> master._leq;
  stream >> master._eeq;
  stream >> master._obstructions;
  if (db->isSchema(db_schema_polygon)) {
    stream >> master._poly_obstructions;
  }
  if (db->isSchema(db_schema_dbmaster_lib_for_site)) {
    stream >> master._lib_for_site;
  } else {
    // The site was copied into the same dbLib previously
    master._lib_for_site = master.getOwner()->getId();
  }
  stream >> master._site;
  stream >> master._mterm_hash;
  stream >> *master._mterm_tbl;
  stream >> *master._mpin_tbl;
  if (!db->isSchema(db_rm_target)) {
    // obsolete table is always unpopulated so type/values unimportant
    dbTable<_dbMaster, 4> dummy(nullptr, nullptr, nullptr, dbDatabaseObj);
    stream >> dummy;
  }
  stream >> *master._box_tbl;
  if (db->isSchema(db_schema_polygon)) {
    stream >> *master._poly_box_tbl;
  }
  stream >> *master._antenna_pin_model_tbl;
  if (db->isSchema(db_schema_master_edge_type)) {
    stream >> *master.edge_types_tbl_;
  }
  return stream;
}

dbObjectTable* _dbMaster::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbMTermObj:
      return _mterm_tbl;
    case dbMPinObj:
      return _mpin_tbl;
    case dbBoxObj:
      return _box_tbl;
    case dbPolygonObj:
      return _poly_box_tbl;
    case dbTechAntennaPinModelObj:
      return _antenna_pin_model_tbl;
    case dbMasterEdgeTypeObj:
      return edge_types_tbl_;
    default:
      break;  // DIMITRIS_COMP_WARN
  }

  return getTable()->getObjectTable(type);
}

////////////////////////////////////////////////////////////////////
//
// dbMaster - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbMaster::getName() const
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_name;
}

const char* dbMaster::getConstName()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_name;
}

Point dbMaster::getOrigin()
{
  _dbMaster* master = (_dbMaster*) this;
  return {master->_x, master->_y};
}

void dbMaster::setOrigin(int x, int y)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_x = x;
  master->_y = y;
}

void* dbMaster::staCell()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_sta_cell;
}

void dbMaster::staSetCell(void* cell)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_sta_cell = cell;
}

uint dbMaster::getWidth() const
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_width;
}

void dbMaster::setWidth(uint w)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_width = w;
}

uint dbMaster::getHeight() const
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_height;
}

void dbMaster::setHeight(uint h)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_height = h;
}

int64_t dbMaster::getArea() const
{
  return getWidth() * static_cast<int64_t>(getHeight());
}

dbMasterType dbMaster::getType() const
{
  _dbMaster* master = (_dbMaster*) this;
  return dbMasterType(master->_flags._type);
}

void dbMaster::setType(dbMasterType type)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_flags._type = type.getValue();
}

dbMaster* dbMaster::getLEQ()
{
  _dbMaster* master = (_dbMaster*) this;

  if (master->_leq == 0) {
    return nullptr;
  }

  _dbLib* lib = (_dbLib*) master->getOwner();
  return (dbMaster*) lib->_master_tbl->getPtr(master->_leq);
}

void dbMaster::setLEQ(dbMaster* leq)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_leq = leq->getImpl()->getOID();
}

dbMaster* dbMaster::getEEQ()
{
  _dbMaster* master = (_dbMaster*) this;

  if (master->_eeq == 0) {
    return nullptr;
  }

  _dbLib* lib = (_dbLib*) master->getOwner();
  return (dbMaster*) lib->_master_tbl->getPtr(master->_eeq);
}

void dbMaster::setEEQ(dbMaster* eeq)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_eeq = eeq->getImpl()->getOID();
}

dbSet<dbMTerm> dbMaster::getMTerms()
{
  _dbMaster* master = (_dbMaster*) this;
  return dbSet<dbMTerm>(master, master->_mterm_tbl);
}

dbSet<dbMasterEdgeType> dbMaster::getEdgeTypes()
{
  _dbMaster* master = (_dbMaster*) this;
  return dbSet<dbMasterEdgeType>(master, master->edge_types_tbl_);
}

dbMTerm* dbMaster::findMTerm(const char* name)
{
  _dbMaster* master = (_dbMaster*) this;
  return (dbMTerm*) master->_mterm_hash.find(name);
}

dbMTerm* dbMaster::findMTerm(dbBlock* block, const char* name)
{
  dbMTerm* mterm = findMTerm(name);
  if (mterm) {
    return mterm;
  }
  char blk_left_bus_del, blk_right_bus_del;
  block->getBusDelimiters(blk_left_bus_del, blk_right_bus_del);

  char lib_left_bus_del, lib_right_bus_del;
  ;
  getLib()->getBusDelimiters(lib_left_bus_del, lib_right_bus_del);

  if (lib_left_bus_del == '\0' || lib_right_bus_del == '\0') {
    return mterm;
  }

  uint ii = 0;
  std::string ttname(name);
  if (lib_left_bus_del != blk_left_bus_del
      || lib_right_bus_del != blk_right_bus_del) {
    while (name[ii] != '\0') {
      if (name[ii] == blk_left_bus_del) {
        ttname[ii] = lib_left_bus_del;
      } else if (name[ii] == blk_right_bus_del) {
        ttname[ii] = lib_right_bus_del;
      }
      ii++;
    }
    mterm = findMTerm(ttname.c_str());
  }
  return mterm;
}

dbLib* dbMaster::getLib()
{
  return (dbLib*) getImpl()->getOwner();
}

dbSet<dbBox> dbMaster::getObstructions(bool include_decomposed_polygons)
{
  _dbMaster* master = (_dbMaster*) this;
  if (include_decomposed_polygons) {
    return dbSet<dbBox>(master, master->_box_itr);
  }
  return dbSet<dbBox>(master, master->_pbox_box_itr);
}

dbSet<dbPolygon> dbMaster::getPolygonObstructions()
{
  _dbMaster* master = (_dbMaster*) this;
  return dbSet<dbPolygon>(master, master->_pbox_itr);
}

bool dbMaster::isFrozen()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_flags._frozen != 0;
}

int dbMaster::getMTermCount()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_mterm_cnt;
}

void dbMaster::setSite(dbSite* site)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_lib_for_site = site->getLib()->getImpl()->getOID();
  master->_site = site->getImpl()->getOID();
}

dbSite* dbMaster::getSite()
{
  _dbMaster* master = (_dbMaster*) this;

  if (master->_site == 0) {
    return nullptr;
  }

  _dbDatabase* db = (_dbDatabase*) getDb();
  _dbLib* lib = (_dbLib*) db->_lib_tbl->getPtr(master->_lib_for_site);

  return (dbSite*) lib->_site_tbl->getPtr(master->_site);
}

void dbMaster::setSymmetryX()
{
  _dbMaster* master = (_dbMaster*) this;
  master->_flags._x_symmetry = 1;
}

bool dbMaster::getSymmetryX()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_flags._x_symmetry != 0;
}

void dbMaster::setSymmetryY()
{
  _dbMaster* master = (_dbMaster*) this;
  master->_flags._y_symmetry = 1;
}

bool dbMaster::getSymmetryY()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_flags._y_symmetry != 0;
}

void dbMaster::setSymmetryR90()
{
  _dbMaster* master = (_dbMaster*) this;
  master->_flags._R90_symmetry = 1;
}

bool dbMaster::getSymmetryR90()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_flags._R90_symmetry != 0;
}

void dbMaster::setFrozen()
{
  _dbMaster* master = (_dbMaster*) this;

  if (master->_flags._frozen == 1) {
    return;
  }

  master->_flags._frozen = 1;

  // set the order id on the mterm.
  // this id is used to index mterms on a inst-hdr
  int i = 0;
  for (dbMTerm* mterm : getMTerms()) {
    ((_dbMTerm*) mterm)->_order_id = i++;
  }
}

void dbMaster::setSequential(bool v)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_flags._sequential = v;
}

bool dbMaster::isSequential()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_flags._sequential;
}
void dbMaster::setMark(uint mark)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_flags._mark = mark;
}

uint dbMaster::isMarked()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_flags._mark;
}

void dbMaster::setSpecialPower(bool value)
{
  _dbMaster* master = (_dbMaster*) this;
  master->_flags._special_power = (value == true) ? 1 : 0;
}

bool dbMaster::isSpecialPower()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_flags._special_power == 1;
}

void dbMaster::getPlacementBoundary(Rect& r)
{
  _dbMaster* master = (_dbMaster*) this;
  r = Rect(0, 0, master->_width, master->_height);
  dbTransform t(Point(-master->_x, -master->_y));
  t.apply(r);
}

void dbMaster::transform(dbTransform& t)
{
  for (dbBox* box : getObstructions()) {
    t.apply(((_dbBox*) box)->_shape._rect);
  }

  for (dbMTerm* mterm : getMTerms()) {
    for (dbMPin* mpin : mterm->getMPins()) {
      for (dbBox* box : mpin->getGeometry()) {
        t.apply(((_dbBox*) box)->_shape._rect);
      }
    }
  }
}

int dbMaster::getMasterId()
{
  _dbMaster* master = (_dbMaster*) this;
  return master->_id;
}

dbMaster* dbMaster::create(dbLib* lib_, const char* name_)
{
  if (lib_->findMaster(name_)) {
    return nullptr;
  }

  _dbLib* lib = (_dbLib*) lib_;
  _dbDatabase* db = lib->getDatabase();
  _dbMaster* master = lib->_master_tbl->create();
  master->_name = safe_strdup(name_);
  master->_id = db->_master_id++;
  lib->_master_hash.insert(master);
  return (dbMaster*) master;
}

void dbMaster::destroy(dbMaster* master)
{
  auto db = master->getDb();
  _dbMaster* master_impl = (_dbMaster*) master;
  if (db->getChip() && db->getChip()->getBlock()) {
    _dbBlock* block = (_dbBlock*) db->getChip()->getBlock();
    _dbInstHdr* inst_hdr = block->_inst_hdr_hash.find(master_impl->_id);
    if (inst_hdr) {
      master->getImpl()->getLogger()->error(
          utl::ODB,
          431,
          "Can't delete master {} which still has instances",
          master->getName());
    }
  }

  _dbLib* lib = (_dbLib*) master->getLib();
  lib->_master_hash.remove(master_impl);
  lib->_master_tbl->destroy(master_impl);
}

dbMaster* dbMaster::getMaster(dbLib* lib_, uint dbid_)
{
  _dbLib* lib = (_dbLib*) lib_;
  return (dbMaster*) lib->_master_tbl->getPtr(dbid_);
}

int dbMaster::getFFType()
{
  //if (ff_type_ != -1) {
  //  return ff_type_;
  //}
  if(!isFF()) return -1;


  _dbMaster* master = (_dbMaster*) this;
  int ff_type_ = -1; // -1: not FF, 0: DFF, 1: QNFF, 2: DFF and QNFF, 3: QFF, 4: QNFF, 5: Q and QN
  bool has_Q = false;
  bool hasQN = false;
  bool hasScan = false;
  for (dbMTerm* mterm : getMTerms()) {
    std::string n = mterm->getConstName();
    if (n.rfind("QN") == 0) {               // 先看 QN
      hasQN = true;
    }
    else if (n.rfind("Q") == 0) {           // 才看 Q
      has_Q = true;
    }

    // --- Check Scan In (SI) ---
    // 僅接受「恰好是 SI」或「SI0、SI1…」這類掃描腳
    if ((n.rfind("SI") == 0 && std::isdigit(n[2]))) {
      hasScan = true;
    }
  }
  if(!hasScan)
  {
    if(has_Q && !hasQN) ff_type_ = 0;
    if(!has_Q && hasQN) ff_type_ = 1;
    if(has_Q && hasQN) ff_type_ = 2;
  }
  else
  {
    if(has_Q && !hasQN) ff_type_ = 3; // Q
    if(!has_Q && hasQN) ff_type_ = 4; // QN
    if(has_Q && hasQN) ff_type_ = 5; // Q and QN
  }
  return ff_type_;
}


int dbMaster::getBitCount()
{
  //if(bit_count_ != -1) {
  //  return bit_count_;
  //}
  if(!isFF()) {
    return 0;
  }
  _dbMaster* master = (_dbMaster*) this;
  int bit_count_ = 0;
  for (dbMTerm* mterm : getMTerms()) {
    std::string mterm_name = mterm->getConstName();
    if (mterm_name.rfind('D', 0) == 0) {
      bit_count_++;
    }
  }
  return bit_count_;
}

bool dbMaster::isFF()
{
  //if(is_ff_ != -1) {
   // return is_ff_;
  //}
  _dbMaster* master = (_dbMaster*) this;
  //if(!master->_flags._sequential)
  //{
  //   std::cout<< "Not FF " << std::endl;
    //return false;
  //}
  bool has_Q = false;
  bool has_D = false;
  for (dbMTerm* mterm : getMTerms()) {
    dbSigType sig_type = mterm->getSigType();
    if (sig_type != dbSigType::SIGNAL && sig_type != dbSigType::CLOCK)
      continue;
    // 2. 只要名稱是 D*/Q* 就標記
    std::string name = mterm->getConstName();
    if (name.rfind('Q', 0) == 0)   // 以 'Q' 開頭
      has_Q = true;
    if (name.rfind('D', 0) == 0)   // 以 'D' 開頭
      has_D = true;

    if (has_Q && has_D) {

      return true;
    }
  }

  return false;

}

bool dbMaster::isFiller()
{
  _dbMaster* master = (_dbMaster*) this;

  switch (master->_flags._type) {
    case dbMasterType::CORE_SPACER:
      return true;
    case dbMasterType::CORE:
    case dbMasterType::CORE_FEEDTHRU:
    case dbMasterType::CORE_TIEHIGH:
    case dbMasterType::CORE_TIELOW:
    case dbMasterType::CORE_WELLTAP:
    case dbMasterType::CORE_ANTENNACELL:
    case dbMasterType::BLOCK:
    case dbMasterType::BLOCK_BLACKBOX:
    case dbMasterType::BLOCK_SOFT:
    case dbMasterType::ENDCAP:
    case dbMasterType::ENDCAP_PRE:
    case dbMasterType::ENDCAP_POST:
    case dbMasterType::ENDCAP_TOPLEFT:
    case dbMasterType::ENDCAP_TOPRIGHT:
    case dbMasterType::ENDCAP_BOTTOMLEFT:
    case dbMasterType::ENDCAP_BOTTOMRIGHT:
    case dbMasterType::ENDCAP_LEF58_BOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_TOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER:
    case dbMasterType::COVER:
    case dbMasterType::COVER_BUMP:
    case dbMasterType::RING:
    case dbMasterType::PAD:
    case dbMasterType::PAD_AREAIO:
    case dbMasterType::PAD_INPUT:
    case dbMasterType::PAD_OUTPUT:
    case dbMasterType::PAD_INOUT:
    case dbMasterType::PAD_POWER:
    case dbMasterType::PAD_SPACER:
      return false;
  }
  // gcc warning
  return false;
}

bool dbMaster::isCoreAutoPlaceable()
{
  // Use switch so if new types are added we get a compiler warning.
  switch (getType()) {
    case dbMasterType::CORE:
    case dbMasterType::CORE_FEEDTHRU:
    case dbMasterType::CORE_TIEHIGH:
    case dbMasterType::CORE_TIELOW:
    case dbMasterType::CORE_SPACER:
    case dbMasterType::CORE_WELLTAP:
    case dbMasterType::CORE_ANTENNACELL:
    case dbMasterType::BLOCK:
    case dbMasterType::BLOCK_BLACKBOX:
    case dbMasterType::BLOCK_SOFT:
    case dbMasterType::ENDCAP:
    case dbMasterType::ENDCAP_PRE:
    case dbMasterType::ENDCAP_POST:
    case dbMasterType::ENDCAP_LEF58_BOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_TOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER:
      return true;
      // These classes are completely ignored by the placer.
    case dbMasterType::ENDCAP_TOPLEFT:
    case dbMasterType::ENDCAP_TOPRIGHT:
    case dbMasterType::ENDCAP_BOTTOMLEFT:
    case dbMasterType::ENDCAP_BOTTOMRIGHT:
    case dbMasterType::COVER:
    case dbMasterType::COVER_BUMP:
    case dbMasterType::RING:
    case dbMasterType::PAD:
    case dbMasterType::PAD_AREAIO:
    case dbMasterType::PAD_INPUT:
    case dbMasterType::PAD_OUTPUT:
    case dbMasterType::PAD_INOUT:
    case dbMasterType::PAD_POWER:
    case dbMasterType::PAD_SPACER:
      return false;
  }
  // gcc warning
  return false;
}

void _dbMaster::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["name"].add(_name);
  info.children_["mterm_hash"].add(_mterm_hash);
  _mterm_tbl->collectMemInfo(info.children_["mterm"]);
  _mpin_tbl->collectMemInfo(info.children_["mpin"]);
  _box_tbl->collectMemInfo(info.children_["box"]);
  _poly_box_tbl->collectMemInfo(info.children_["poly_box"]);
  _antenna_pin_model_tbl->collectMemInfo(info.children_["antenna_pin_model"]);
  edge_types_tbl_->collectMemInfo(info.children_["edge_types"]);
}

}  // namespace odb
