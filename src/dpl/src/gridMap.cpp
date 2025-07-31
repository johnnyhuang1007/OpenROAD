#include "mbff.h"


namespace dpl
{


std::pair<int, int> GridMap::getGridIndex(const dpl::dbuPoint& p) const
{
    return std::make_pair((p.x.v / tbl[0][0].width_), (p.y.v / tbl[0][0].height_));
}

void call_tree(tileGrid* grid)
{
    if(grid == nullptr)
        return;

    std::cout<<"TileGrid: ("<<grid->x_<<", "<<grid->y_<<") to ("
              <<grid->x_ + grid->width_<<", "<<grid->y_ + grid->height_<<")"<<std::endl;
    std::cout<<"Fixed Tiles: "<<grid->fixedtiles.size()<<std::endl;
    std::cout<<"FF Nodes: "<<grid->FF_nodes.size()<<std::endl;
    std::cout<<"Valid Spaces: "<<grid->valid_spaces.size()<<std::endl;
    for(int i = 0 ; i < 2; i++)
    {
        for(int j = 0; j < 2; j++)
        {
            if(grid->descendants[i][j])
            {
                call_tree(grid->descendants[i][j].get());
            }
        }
    }
}

void GridMap::setFFNodes(std::vector<dpl::Node*>& nodes)
{
    for(int i = 0; i < tbl.size(); i++) {
        for(int j = 0; j < tbl[i].size(); j++) {
            tbl[i][j].FF_nodes.clear();
        }
    }

    for(auto& node : nodes) {
        DbuX left = node->getLeft();
        DbuY bottom = node->getBottom();

        std::pair<int, int> idx1 = getGridIndex(dbuPoint(left, bottom));
        tbl[idx1.first][idx1.second].FF_nodes.push_back(node);
    }
    header->FF_nodes = std::list<Node*>{nodes.begin(), nodes.end()};
}

void GridMap::addFFNodes(std::vector<dpl::Node*>& nodes)
{
    for(auto& node : nodes) {
        DbuX left = node->getLeft();
        DbuY bottom = node->getBottom();

        std::pair<int, int> idx1 = getGridIndex(dbuPoint(left, bottom));
        tbl[idx1.first][idx1.second].FF_nodes.push_back(node);
    }
    header->FF_nodes.insert(header->FF_nodes.end(), nodes.begin(), nodes.end());
}

void GridMap::setInsertableTile(std::vector<insertable>& inserts)
{
    for(int i = 0 ; i < tbl.size(); i++)
    {
        for(int j = 0; j < tbl[i].size(); j++)
        {
            tbl[i][j].valid_spaces.clear();
        }
    }

    for (auto& insert : inserts) {
        DbuX left = insert.tile.getLeft();
        DbuY bottom = insert.tile.getBottom();
        DbuX right = insert.tile.getRight() - insert.tile.getWidth();
        DbuY top = insert.tile.getTop()-1;


        std::pair<int, int> idx1 = getGridIndex(dbuPoint(left, bottom));
        std::pair<int, int> idx2 = getGridIndex(dbuPoint(right, top));
        //split insertable into several sub-insertables
        /*
            TODO
        */

        for (int i = idx1.first; i <= idx2.first; ++i) {
            for (int j = idx1.second; j <= idx2.second; ++j) {
                tbl[i][j].valid_spaces.push_back(insert);
            }
        }
    }
    header->valid_spaces = std::list<insertable>{inserts.begin(), inserts.end()};
}

void GridMap::addInsertableTile(std::vector<insertable>& inserts)
{
    for (auto& insert : inserts) {
        DbuX left = insert.tile.getLeft();
        DbuY bottom = insert.tile.getBottom();
        DbuX right = insert.tile.getRight() - insert.tile.getWidth();

        std::pair<int, int> idx1 = getGridIndex(dbuPoint(left, bottom));
        std::pair<int, int> idx2 = getGridIndex(dbuPoint(right, bottom));
        //split insertable into several sub-insertables
        /*
            TODO
        */
        for (int i = idx1.first; i <= idx2.first; ++i) {
            for (int j = idx1.second; j <= idx2.second; ++j) {
                tbl[i][j].valid_spaces.push_back(insert);
            }
        }
    }
    header->valid_spaces.insert(
        header->valid_spaces.end(), inserts.begin(), inserts.end());
}

dbuPoint median(std::vector<Node*> inputs, bool(*compare)(Node* a, Node* b))
{
    sort(inputs.begin(), inputs.end(), compare);
    int half_index1 = inputs.size()/2;
    int half_index2 = inputs.size()2 + (inputs.size()+1)%2;

    dbuPoint ret;
    ret.x = (inputs[half_index1]->getLeft()/2.0 + inputs[half_index2]->getLeft()/2.0 );
    ret.y = (inputs[half_index1]->getTop()/2.0 + inputs[half_index2]->getTop()/2.0 );

    return ret;
}

std::vector<insertable> splitByCoordX(insertable inst, double splitPoint)
{
    int width = inst.tile.width_/inst.count;
    if(inst.count < 2) return std::vector<insertable>{inst};

    double leftRegionWidth = splitPoint - inst.tile.x_;
    if(inst.count - cell(leftRegionWidth / width) < 1) return std::vector<insertable>{inst};
    
    int leftCount = cell(leftRegionWidth / width);
    std::vector<insertable> ret(2);
    ret[0].count = leftCount;
    ret[1].count = inst.count - leftCount;

    ret[0].tile.x_ = inst.tile.x_;
    ret[0].tile.y_ = inst.tile.y_;
    ret[0].tile.width_ = leftCount*width;
    ret[0].tile.height_ = inst.tile.height_;
    ret[0].type = inst.type;

    ret[1].tile.x_ = inst.tile.x_ + ret[0].count * width;
    ret[1].tile.y_ = inst.tile.y_;
    ret[1].tile.width_ = ret[1].count*width;
    ret[1].tile.height_ = inst.tile.height_;
    ret[1].type = inst.type;

    return ret;
}

int c = 0;
void GridMap::TopDownSplit(tileGrid* grid,
                  std::vector<Node*> FFs,
                  std::vector<insertable> inserts)
{



    if(FF_x.size() < 32)
    {
        grid->FF_nodes.insert(grid->FF_nodes.end(), FFs.begin(), FFs.end());
        grid->valid_spaces.insert(grid->valid_spaces.end(),
                                  inserts.begin(),
                                  inserts.end());
        c++;
        std::cout<<"Count: "<<c<<", FF_x size: "<<FFs.size()<<", Insert_x size: "
                 <<inserts.size()<<std::endl;
        return;
    }

    std::vector<Node*> subFFs[2][2];
    std::vector<insertable> subInserts[2][2];
    
    int half_x_idx = FF_x.size() / 2;
    int half_y_idx = FF_y.size() / 2;

    double half_x = median(FFs,[](Node* a, Node* b)
                                {
                                    return a->getLeft() < b->getLeft();
                                }).x;
    double half_y = median(FFs,[](Node* a, Node* b)
                                {
                                    return a->getTop() < b.getTop();
                                }).y;
    
    double x_seq[3] = {grid->x_,half_x,grid->x_ + grid->width_};
    double y_seq[3] = {grid->y_,half_y,grid->y_ + grid->height_};

    for(int i = 0 ; i < FFs.size() ; i++)
    {
        int x_idx = FFs[i]->getLeft() >= half_x;
        int y_idx = FFs[j]->getBottom() >= half_y;
        subFFs[x_idx][y_idx].push_back(FFs[i]);
    }

    for(int i = 0 ; i < inserts.size() ; i++)
    {
        std::vector<insertable> splitInst = splitByCoordX(inserts[i],half_x);
        for(int j = 0 ; j < splitInst.size() ; j++)
        {
            int x_idx = splitInst[j]->getLeft() >= half_x;
            int y_idx = splitInst[j]->getBottom() >= half_y;
            subInserts[x_idx][y_idx].push_back(splitInst[j]);
        }
    }

    for(int i = 0 ; i < 2 ; i++)
    {
        for(int j = 0 ; j < 2 ; j++)
        {
            grid->descendants[i][j] = std::unique_ptr<tileGrid>(new tileGrid);
            grid->descendants[i][j]->parent = grid;
            grid->descendants[i][j]->x_ = x_seq[i];
            grid->descendants[i][j]->y_ = y_seq[j];
            grid->descendants[i][j]->width_ = x_seq[i+1] - x_seq[i];
            grid->descendants[i][j]->height_ = y_seq[j+1] - y_seq[j];  
            TopDownSplit(grid->descendants[i][j].get(),
                         subFFs[i][j],
                         subInserts[i][j]);          
        }
    }

}

void bottomUpMerging()
{
    
}
}  // namespace dpl