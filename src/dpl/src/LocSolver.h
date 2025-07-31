#include <vector>
#include <list>
#include "cad2025_blockage/blockage.h"

namespace dpl
{

class Node;

struct LocGrid
{
    DbuX left_;
    DbuY buttom_;
    DbuX width;
    DbuY height;
};

#define stripe std::pair<int,bool>

class LocSolver
{
    public:
    LocSolver() = default;
    private:
    std::vector<Node*>  combCells;
    std::vector<Node*>  ffCells;
    std::vector<blockage> ffBlockages;
    std::vector<>

    std::vector<std::vector<LocGrid>> placementTable;
    std::list<stripe> occupied_Map;
    int target_rowHeight = 1; 
    std::pair<int,int> min_max_CellHeight;

    DbuX left_;
    DbuX right_;
    DbuY bottom_;
    DbuX top_;

    void initFromMgr();
    void blockageGeneration();
    void plRowSetup();
    void plMapSetup();
    void updateplRow(std::vector<>);



    int totalQ;
    int totalQN;
    int totalQpQN; //this is not equal to Q+QN, some FF might have Q+QN outpin

};
}