#include "Tile.h"

namespace dpl
{

bool Tile::operator==(const Tile& rhs)
{
  return (x_ == rhs.x_) && (y_ == rhs.y_) && (width_ == rhs.width_)
         && (height_ == rhs.height_);
}

}