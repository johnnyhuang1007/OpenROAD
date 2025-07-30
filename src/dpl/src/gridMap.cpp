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

int c = 0;
void GridMap::TopDownSplit(tileGrid* grid,
                  std::vector<Node*> FF_x,
                  std::vector<Node*> FF_y,
                  std::vector<insertable> insert_x,
                  std::vector<insertable> insert_y)
{
    if(FF_x.size() < 32)
    {
        grid->FF_nodes.insert(grid->FF_nodes.end(), FF_x.begin(), FF_x.end());
        grid->valid_spaces.insert(grid->valid_spaces.end(),
                                  insert_x.begin(),
                                  insert_x.end());
        c++;
        std::cout<<"Count: "<<c<<", FF_x size: "<<FF_x.size()<<", Insert_x size: "
                 <<insert_x.size()<<std::endl;
        return;
    }
    
    int half_x_idx = FF_x.size() / 2;
    int half_y_idx = FF_y.size() / 2;
    double half_x = FF_x[half_x_idx]->getLeft().v /2.0 + FF_x[half_x_idx+1]->getLeft().v / 2.0;
    double half_y = FF_y[half_y_idx]->getBottom().v /2.0 + FF_y[half_y_idx+1]->getBottom().v / 2.0;
    double boundLeft = grid->x_;
    double boundBottom = grid->y_;
    double boundRight = grid->x_ + grid->width_;
    double boundTop = grid->y_ + grid->height_;

    std::cout<<"Splitting TileGrid: ("<<grid->x_<<", "<<grid->y_<<") to ("
              <<grid->x_ + grid->width_<<", "<<grid->y_ + grid->height_<<")"<<std::endl;
    std::cout<<"Half X: "<<half_x<<", Half Y: "<<half_y<<std::endl;
    std::cout<<"Bound: ("<<boundLeft<<", "<<boundBottom<<") to ("
              <<boundRight<<", "<<boundTop<<")"<<std::endl;

    std::cout<<"FF_x size: "<<FF_x.size()<<", FF_y size: "<<FF_y.size()<<std::endl;
    std::cout<<"Insert_x size: "<<insert_x.size()<<", Insert_y size: "<<insert_y.size()<<std::endl;

    grid->descendants[0][0] = std::unique_ptr<tileGrid>(new tileGrid);
    grid->descendants[0][1] = std::unique_ptr<tileGrid>(new tileGrid);
    grid->descendants[1][0] = std::unique_ptr<tileGrid>(new tileGrid);
    grid->descendants[1][1] = std::unique_ptr<tileGrid>(new tileGrid);
    grid->descendants[0][0]->parent = grid;
    grid->descendants[0][1]->parent = grid;
    grid->descendants[1][0]->parent = grid;
    grid->descendants[1][1]->parent = grid;

    grid->descendants[0][0]->x_ = boundLeft;
    grid->descendants[0][0]->y_ = boundBottom;
    grid->descendants[0][0]->width_ = grid->width_/2.0;
    grid->descendants[0][0]->height_ = grid->height_/2.0;

    grid->descendants[0][1]->x_ = boundLeft;
    grid->descendants[0][1]->y_ = half_y;
    grid->descendants[0][1]->width_ = grid->width_/2.0;
    grid->descendants[0][1]->height_ = grid->height_/2.0;

    grid->descendants[1][0]->x_ = half_x;
    grid->descendants[1][0]->y_ = boundBottom;
    grid->descendants[1][0]->width_ = grid->width_/2.0;
    grid->descendants[1][0]->height_ = grid->height_/2.0;

    grid->descendants[1][1]->x_ = half_x;
    grid->descendants[1][1]->y_ = half_y;
    grid->descendants[1][1]->width_ = grid->width_/2.0;
    grid->descendants[1][1]->height_ = grid->height_/2.0;


    TopDownSplit(grid->descendants[0][0].get(), {FF_x.begin(), FF_x.begin() + half_x_idx},
                 {FF_y.begin(), FF_y.begin() + half_y_idx},
                 {insert_x.begin(), insert_x.begin() + half_x_idx},
                 {insert_y.begin(), insert_y.begin() + half_y_idx}); 
    
    TopDownSplit(grid->descendants[0][1].get(), {FF_x.begin() + half_x_idx, FF_x.end()},
                 {FF_y.begin(), FF_y.begin() + half_y_idx},
                 {insert_x.begin() + half_x_idx, insert_x.end()},   
                 {insert_y.begin(), insert_y.begin() + half_y_idx});
    
    TopDownSplit(grid->descendants[1][0].get(), {FF_x.begin(), FF_x.begin() + half_x_idx},
                {FF_y.begin() + half_y_idx, FF_y.end()},
                {insert_x.begin(), insert_x.begin() + half_x_idx},
                {insert_y.begin() + half_y_idx, insert_y.end()});

    TopDownSplit(grid->descendants[1][1].get(), {FF_x.begin() + half_x_idx, FF_x.end()},
                {FF_y.begin() + half_y_idx, FF_y.end()},
                {insert_x.begin() + half_x_idx, insert_x.end()},
                {insert_y.begin() + half_y_idx, insert_y.end()});


}

void bottomUpMerging()
{
    
}
}  // namespace dpl