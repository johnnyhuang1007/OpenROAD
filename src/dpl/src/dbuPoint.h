#ifndef DPL_SRC_DBU_POINT_H
#define DPL_SRC_DBU_POINT_H

#include "dpl/Opendp.h"
#include "odb/db.h"
#include "infrastructure/Coordinates.h"

namespace dpl {

struct dbuPoint
{
  DbuX x;
  DbuY y;

  dbuPoint(int x_, int y_) : x(x_), y(y_) {}
  dbuPoint(const DbuX& x_, const DbuY& y_) : x(x_), y(y_) {}

  dbuPoint() = default;
};

}

#endif  // DPL_SRC_DBU_POINT_H