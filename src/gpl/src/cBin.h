#ifndef BIN_H
#define BIN_H
#include "Point.h"
#include <complex>
using namespace std;


class cBin
{
public:
    double l_x;
    double l_y;
    double u_x;
    double u_y;
    double ovlp_area;
    double target_density;

    cBin(){}

    double len_x(){return u_x - l_x;}
    double len_y(){return u_y - l_y;}
    double area(){return len_x() * len_y();}
    double ovlp_area_ratio(){return ovlp_area / area();}
};

#endif 