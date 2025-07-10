#include "gpl/Replace.h"
#include "../clusterBase.h"
#include "../fft.h"
#include "../nesterovPlace.h"
#include "odb/db.h"
#include "../placerBase.h"
#include "utl/Logger.h"

namespace gpl {

ClusterBase::ClusterBase()
{
    std::cout<<"ClusterBase constructor called"<<std::endl;
}

ClusterBase::~ClusterBase()
{
    std::cout<<"ClusterBase destructor called"<<std::endl;
}



}