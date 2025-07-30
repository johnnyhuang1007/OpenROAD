#ifndef DPL_SRC_INFRASTRUCTURE_TILE_H
#define DPL_SRC_INFRASTRUCTURE_TILE_H

#include <string>
#include <vector>
#include <list>

#include "Coordinates.h"
#include "Objects.h"
#include "dpl/Opendp.h"

namespace dpl
{

struct Tile
{
    Tile(DbuX x, DbuY y, DbuX width, DbuY height)
        : x_(x), y_(y), width_(width), height_(height)
    {
    }
    Tile()
        : x_(0), y_(0), width_(0), height_(0)
    {
    }
    DbuX x_;
    DbuY y_;
    DbuX width_;
    DbuY height_;
    std::list<Tile*>::iterator list_ptr;
    DbuX getRight()const{return x_+width_;}
    DbuY getTop()const{return y_+height_;}
    DbuX getLeft()const{return x_;}
    DbuY getBottom()const{return y_;}
    DbuX getWidth()const{return width_;}
    DbuY getHeight()const{return height_;} 
    bool operator==(const Tile& rhs);
};

struct insertable
{
    dbMaster* type;
    int count;
    Tile tile;
};


}

#endif