#ifndef BINGRID_H
#define BINGRID_H
#include "cBin.h"
#include <vector>
#include <complex>
#include "gpl/Replace.h"
#include "odb/db.h"
#include "./placerBase.h"
#include "point.h"
#include "./routeBase.h"
#include "nesterovBase.h"


using namespace std;
#define cd std::complex<double>
#define PI 3.14159265358979323846

namespace gpl {

class clusterBinGrid
{
    public:
    vector<vector<cBin>> _Bin2D;
    vector<GCell*> _pModules;
    vector<FloatPoint> _field_cache;
    int _num_bins_x;
    int _num_bins_y;
    double len_x;
    double len_y; 
    double lower_x;
    double lower_y;
    float ovfl;
    float target_density;
    double maximal_weight = 0.0;
    bool init = 0;

    // interface with FFT LIBRARY
    float** density_map;
    float** electroPhi_;
    float** field_x;
    float** field_y;
    vector<float> _cos_table;
    vector<int> ip_table;

    clusterBinGrid(double x0, double x1, double y0, double y1, vector<GCell*> GCells);
    ~clusterBinGrid();
    void rescale(int num_bin_x, int num_bin_y);
    int get_num_bins_x() {return _num_bins_x;}
    int get_num_bins_y() {return _num_bins_y;}
    clusterBinGrid(){}

    pair<int,int> getBinIdx(double x, double y);
    pair<pair<int,int>, pair<int,int>> getBinIdx(GCell* module);


    void initBin2D();
    void updateBin2D();
    void updateBinField();
    void updateBinPhi();
    void normalizeBinField();
    void update_ovfl();
    double get_energy();
    vector<FloatPoint> getBinField();
    vector<FloatPoint> getBinField_cache(){return _field_cache;}

};

/// 1D FFT ////////////////////////////////////////////////////////////////
void cdft(int n, int isgn, float *a, int *ip, float *w);
void ddct(int n, int isgn, float *a, int *ip, float *w);
void ddst(int n, int isgn, float *a, int *ip, float *w);

/// 2D FFT ////////////////////////////////////////////////////////////////
void cdft2d(int, int, int, float **, float *, int *, float *);
void rdft2d(int, int, int, float **, float *, int *, float *);
void ddct2d(int, int, int, float **, float *, int *, float *);
void ddst2d(int, int, int, float **, float *, int *, float *);
void ddsct2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w);
void ddcst2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w);

}

#endif // BINGRID_H