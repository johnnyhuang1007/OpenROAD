#include "../clusterBinGrid.h"

namespace gpl {


pair<int,int> clusterBinGrid::getBinIdx(double x, double y)
{
    int idx_x = (x - _Bin2D[0][0].l_x) / _Bin2D[0][0].len_x();
    int idx_y = (y - _Bin2D[0][0].l_y) / _Bin2D[0][0].len_y();
    return make_pair(idx_x, idx_y);
}
pair<pair<int,int>, pair<int,int>> clusterBinGrid::getBinIdx(GCell* module)
{
    int l_idx_x = (module->lx() - _Bin2D[0][0].l_x) / _Bin2D[0][0].len_x();
    int l_idx_y = (module->ly() - _Bin2D[0][0].l_y) / _Bin2D[0][0].len_y();
    int u_idx_x = (module->lx() + module->width() - _Bin2D[0][0].l_x) / _Bin2D[0][0].len_x();
    int u_idx_y = (module->ly() + module->height() - _Bin2D[0][0].l_y) / _Bin2D[0][0].len_y();
    if(l_idx_x < 0) l_idx_x = 0;
    if(l_idx_y < 0) l_idx_y = 0;
    if(u_idx_x >= _num_bins_x) u_idx_x = _num_bins_x - 1;
    if(u_idx_y >= _num_bins_y) u_idx_y = _num_bins_y - 1;
    return make_pair(make_pair(l_idx_x, l_idx_y), make_pair(u_idx_x, u_idx_y));
}

double ovlp_area(GCell &gcell, cBin &bin) {
    double x1 = max(double(gcell.lx()), double(bin.l_x));
    double y1 = max(double(gcell.ly()), double(bin.l_y));
    double x2 = min(double(gcell.lx()) + gcell.width(), double(bin.u_x));
    double y2 = min(double(gcell.ly()) + gcell.height(), double(bin.u_y));
    return max(0.0, x2 - x1) * max(0.0, y2 - y1);
}

clusterBinGrid::clusterBinGrid(double x0, double x1, double y0, double y1, vector<GCell*> gcells)
{
    cout<<"clusterBinGrid constructor called"<<endl;
    _pModules.resize(gcells.size());
    for(int i = 0 ; i < gcells.size() ; i++)
    {
        _pModules[i] = gcells[i];
    }

    len_x = (x1 - x0);
    len_y = (y1 - y0);
    lower_x = x0;
    lower_y = y0;

    _num_bins_x = 256;
    _num_bins_y = 256;


    _Bin2D.resize(_num_bins_x, vector<cBin>(_num_bins_y));

    
    _field_cache.resize(gcells.size(), FloatPoint(0,0));

    // interface with FFT LIBRARY UPSCALE THE 2D FACTOR INITIALLY
    density_map = new float*[_num_bins_x];
    for(int i = 0 ; i < _num_bins_x ; i++)
        density_map[i] = new float[_num_bins_y];
    electroPhi_ = new float*[_num_bins_x];
    for(int i = 0 ; i < _num_bins_x ; i++)
        electroPhi_[i] = new float[_num_bins_y];
    field_x = new float*[_num_bins_x];
    for(int i = 0 ; i < _num_bins_x ; i++)
        field_x[i] = new float[_num_bins_y];
    field_y = new float*[_num_bins_x];
    for(int i = 0 ; i < _num_bins_x ; i++)
        field_y[i] = new float[_num_bins_y];


    double die_area = len_x * len_y;
    target_density = 0.9;
    

    
    initBin2D();
}

clusterBinGrid::~clusterBinGrid()
{
    cout<<"clusterBinGrid destructor called"<<endl;
    for(int i = 0 ; i < _Bin2D.size() ; i++)
    {
        delete[] density_map[i];
        delete[] electroPhi_[i];
        delete[] field_x[i];
        delete[] field_y[i];
    }
    if(_Bin2D.size() > 0)
    {
        delete[] density_map;
        delete[] electroPhi_;
        delete[] field_x;
        delete[] field_y;
    }
    density_map = nullptr;
    electroPhi_ = nullptr;
    field_x = nullptr;
    field_y = nullptr;
    cout<<"clusterBinGrid destructor called"<<endl;
}

void clusterBinGrid::rescale(int x_in, int y_in)
{
    _num_bins_x = x_in;
    _num_bins_y = y_in;
    initBin2D();
}

void clusterBinGrid::initBin2D()
{
    //TODO change the value 1 to score
    maximal_weight = 0;
    for(int i = 0 ; i < _pModules.size() ; i++)
    {
        maximal_weight = max(maximal_weight, 1.0);
    }
    _cos_table.resize( max(_num_bins_x, _num_bins_y) * 3 / 2, 0 );
    ip_table.resize( round(sqrt(max(_num_bins_x, _num_bins_y))) + 2, 0 );
    
    for(int i = 0 ; i < _num_bins_x ; i++)
    {
        for(int j = 0 ; j < _num_bins_y ; j++)
        {
            _Bin2D[i][j].l_x = (i * len_x) / _num_bins_x + lower_x;
            _Bin2D[i][j].l_y = (j * len_y) / _num_bins_y + lower_y;
            _Bin2D[i][j].u_x = ((i+1) * len_x) / _num_bins_x + lower_x;
            _Bin2D[i][j].u_y = ((j+1) * len_y) / _num_bins_y + lower_y;
        }
    }
    for(int i = 0 ; i < _num_bins_x ; i++)
    {
        for(int j = 0 ; j < _num_bins_y ; j++)
        {
            _Bin2D[i][j].ovlp_area = 0;
        }
    }
    
}

void clusterBinGrid::updateBin2D() {

    // Update the 2D bin grid
    for(int i = 0 ; i < _Bin2D.size() ; i++)
    {
        for(int j = 0 ; j < _Bin2D[0].size() ; j++)
        {
            _Bin2D[i][j].ovlp_area = 0;
        }
    }


    for (unsigned i = 0; i < _pModules.size(); i++) {
        GCell gcell = *_pModules[i];
        int l_idx_x = (gcell.lx() - lower_x) / _Bin2D[0][0].len_x();
        int l_idx_y = (gcell.ly() - lower_y) / _Bin2D[0][0].len_y();
        int u_idx_x = (gcell.lx() + gcell.width() - lower_x) / _Bin2D[0][0].len_x();
        int u_idx_y = (gcell.ly() + gcell.height() - lower_y) / _Bin2D[0][0].len_y();
        if(l_idx_x < 0) l_idx_x = 0;
        if(l_idx_y < 0) l_idx_y = 0;
        if(u_idx_x >= _num_bins_x) u_idx_x = _num_bins_x - 1;
        if(u_idx_y >= _num_bins_y) u_idx_y = _num_bins_y - 1;
        for (int j = l_idx_x; j <= u_idx_x; j++) {
            for (int k = l_idx_y; k <= u_idx_y; k++) {
                cBin &bin = _Bin2D[j][k];
                //TODO change the value 1 to charge_density
                bin.ovlp_area += ovlp_area(gcell, bin)* 1.0 ;
            }
        }
    }
    double overall_area = 0;
    for(int i = 0 ; i < _Bin2D.size() ; i++)
    {
        for(int j = 0 ; j < _Bin2D[0].size() ; j++)
        {
            overall_area += _Bin2D[i][j].area();
        }
    }
    double avg_area = overall_area / (_Bin2D.size() * _Bin2D[0].size());
    for(int i = 0 ; i < _Bin2D.size() ; i++)
    {
        for(int j = 0 ; j < _Bin2D[0].size() ; j++)
        {
            _Bin2D[i][j].ovlp_area = (_Bin2D[i][j].ovlp_area - avg_area)/ _Bin2D[i][j].area();
            if(_Bin2D[i][j].ovlp_area < 0.5) _Bin2D[i][j].ovlp_area -= 1;
        }
    }
    

        
}

void clusterBinGrid::updateBinPhi()
{

    /*
        for base func exp(i2\pi * (k_x *x + k_y * y))
        D^2 = -(k_x^2 + k_y^2)
        First use DCT to get the weight of each R(kx,ky)
        -(Kx^2 + Ky^2) * phi(kx,ky) = 2D-FFT(rho(kx,ky))
        phi(kx,ky) = -2D-FFT(rho(kx,ky)) / (Kx^2 + Ky^2)
    */
    #pragma omp parallel for
    for(int i = 0 ; i < _Bin2D.size() ; i++)
        for(int j = 0 ; j < _Bin2D[0].size() ; j++)
        {
            density_map[i][j] = _Bin2D[i][j].ovlp_area;
        }
    
    ddct2d(_num_bins_x, _num_bins_y, -1, density_map, 
        NULL, (int*) &ip_table[0], (float*)&_cos_table[0]);
    

    for(int i = 0; i < _num_bins_x; i++) {
        density_map[i][0] *= 0.5;
    }
        
    for(int i = 0; i < _num_bins_y; i++) {
        density_map[0][i] *= 0.5;
    }
    for(int i = 0; i < _num_bins_x; i++) {
        for(int j = 0; j < _num_bins_y; j++) {
            density_map[i][j] *= 4.0 / _num_bins_x / _num_bins_y;
        }
    }

}

void clusterBinGrid::updateBinField()
{
    const int Ny = _Bin2D.size();       // rows
    const int Nx = _Bin2D[0].size();    // cols


    #pragma omp parallel for
    for(int i = 0; i < _num_bins_x; i++) {
        float wx_i = PI * float(i) / float(_num_bins_x);
        float wx_i2 = wx_i * wx_i;
    
        for(int j = 0; j < _num_bins_y; j++) {
          float wy_i = PI * float(j) / float(_num_bins_y);
          float wy_i2 = wy_i*wy_i;
    
          float density = density_map[i][j];
          float phi = 0;
          float electroX = 0, electroY = 0;
    
          if(i == 0 && j == 0) {
            phi = electroX = electroY = 0.0f;
          }
          else {
            phi = density / (wx_i2 + wy_i2);
            electroX = phi * wx_i;
            electroY = phi * wy_i;
          }
          electroPhi_[i][j] = phi;
          field_x[i][j] = -electroX;
          field_y[i][j] = -electroY;        //D \phi = -E
        }
      }



    //ddct2d(_num_bins_x, _num_bins_y, 1, 
    //    electroPhi_, NULL, 
    //    (int*) &ip_table[0], (float*) &_cos_table[0]);
    #pragma omp parallel sections
    {
        #pragma omp section
            ddsct2d(_num_bins_x, _num_bins_y, 1, 
                field_x, NULL, 
                (int*) &ip_table[0], (float*) &_cos_table[0]);
        #pragma omp section
            ddcst2d(_num_bins_x, _num_bins_y, 1, 
                field_y, NULL, 
                (int*) &ip_table[0], (float*) &_cos_table[0]);
    }


}

void clusterBinGrid::normalizeBinField()
{
    double max_field = 1;   //in case that the field is nearly convergent
    float len_coeff_x = 1;
    float len_coeff_y = 1;
    /*
    if(_Bin2D[0][0].len_x() > _Bin2D[0][0].len_y())
    {
        len_coeff_x = 1;
        len_coeff_y = _Bin2D[0][0].len_x()/_Bin2D[0][0].len_y();
        float norm = sqrt(len_coeff_x * len_coeff_x + len_coeff_y * len_coeff_y);
        len_coeff_x /= norm;
        len_coeff_y /= norm;
    }
    else
    {
        len_coeff_x = _Bin2D[0][0].len_y() / _Bin2D[0][0].len_x();
        len_coeff_y = 1;
        float norm = sqrt(len_coeff_x * len_coeff_x + len_coeff_y * len_coeff_y);
        len_coeff_x /= norm;
        len_coeff_y /= norm;
    }
    */
    #pragma omp parallel for
    for(int i = 0 ; i < _pModules.size() ; i++)
    {
        _field_cache[i].x = 0;
        _field_cache[i].y = 0;
        pair<pair<int,int>, pair<int,int>> bin_idx = getBinIdx(_pModules[i]);
        for(int k = bin_idx.first.first ; k <= bin_idx.second.first ; k++)
        {
            for(int l = bin_idx.first.second ; l <= bin_idx.second.second ; l++)
            {
                //TODO change the value 1 to score
                double score_fac = 1;
                _field_cache[i].x += field_x[k][l] * score_fac * ovlp_area(*_pModules[i], _Bin2D[k][l]) / _Bin2D[k][l].area()*len_coeff_x;
                _field_cache[i].y += field_y[k][l] * score_fac * ovlp_area(*_pModules[i], _Bin2D[k][l]) / _Bin2D[k][l].area()*len_coeff_y;
            }
        }
    }

    for(int i = 0 ; i < _pModules.size() ; i++)
        _field_cache[i] /= _pModules[i]->area();    //precondition
    
}

vector<FloatPoint> clusterBinGrid::getBinField()
{

    updateBin2D();

    updateBinPhi();

    updateBinField();

    normalizeBinField();

    update_ovfl();

    return _field_cache;
}   

void clusterBinGrid::update_ovfl()
{
    ovfl = 0;
    #pragma omp parallel for
    for(int i = 0 ; i < _num_bins_x/2 ; i++)
    {
        for(int j = 0 ; j < _num_bins_y/2 ; j++)
        {
            double area = _Bin2D[2*i][2*j].ovlp_area + 
                _Bin2D[2*i+1][2*j].ovlp_area +
                _Bin2D[2*i][2*j+1].ovlp_area +
                _Bin2D[2*i+1][2*j+1].ovlp_area;
            double new_ovfl = (_Bin2D[i][j].ovlp_area - target_density) / target_density;
            
            if(new_ovfl > ovfl)
                ovfl = new_ovfl;  
        }
    }
}

double clusterBinGrid::get_energy()
{
    double energy = 0;
    for(int i = 0 ; i < _pModules.size() ; i++)
    {
        pair<pair<int,int>, pair<int,int>> bin_idx = getBinIdx(_pModules[i]);
        for(int k = bin_idx.first.first ; k <= bin_idx.second.first ; k++)
        {
            for(int l = bin_idx.first.second ; l <= bin_idx.second.second ; l++)
            {
                energy += electroPhi_[k][l] * ovlp_area(*_pModules[i], _Bin2D[k][l]) / _Bin2D[k][l].area();
            }
        }
    }
    return energy;
}

}