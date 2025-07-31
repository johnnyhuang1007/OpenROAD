#include "mbff.h"

namespace dpl
{

using odb::dbInst;
using odb::dbITerm;
using odb::dbMaster;
using odb::dbMTerm;
using odb::dbNet;

MBFF_solver::MBFF_solver(odb::dbDatabase* db,
           sta::dbSta* sta,
           utl::Logger* log,
           const int threads,
           const int multistart,
           const int num_paths)
    : db_(db),
      block_(db_->getChip()->getBlock()),
      sta_(sta),
      network_(sta_->getDbNetwork()),
      corner_(sta_->cmdCorner()),
      log_(log)
{
    ReadLibs();
}


void MBFF_solver::buildTileList(std::vector<Node*>& combCells,Architecture* arch)
{
    arch_ = arch;
    std::vector<Architecture::Row*> Rows = arch_->getRows();

  	for(int i = 0 ; i < combCells.size() ; i++)
  	{
    	DbuY topY = combCells[i]->getBottom() + combCells[i]->getHeight();
    	DbuY botY = combCells[i]->getBottom();
    	int topIDX = arch_->find_closest_row(topY);
    	int botIDX = arch_->find_closest_row(botY);

    	if(Rows[topIDX]->getBottom() > topY) topIDX--;
    	if(Rows[botIDX]->getBottom() > botY) botIDX--;
    	for(int j = botIDX ; j <= topIDX ; j++)
    	{
    	  	DbuX leftX = (combCells[i]->getLeft() - Rows[j]->getLeft())/getSiteWidth();
      		leftX *= getSiteWidth(); // snap to site
      		DbuX rightX = (combCells[i]->getRight() - Rows[j]->getLeft())/getSiteWidth();
      
      		rightX *= getSiteWidth(); // snap to site
      		if(rightX != combCells[i]->getRight()) rightX += getSiteWidth();
      		combTiles.emplace_back(leftX,Rows[j]->getBottom(),\
                          rightX - leftX, Rows[j]->getHeight());
    	}
  	}

  	std::cout<<"Total Tiles: "<<combTiles.size()<<std::endl;
  

  	std::vector<Tile> sorted_Tiles{combTiles.begin(),combTiles.end()};
  	std::sort(sorted_Tiles.begin(),sorted_Tiles.end(),
    	        [](Tile a, Tile b) {
        	      if(a.y_ != b.y_) return a.y_ < b.y_;
            	  return a.x_ < b.x_;
            	});
  	combTiles = std::list<Tile>(sorted_Tiles.begin(),sorted_Tiles.end()); //putback
  
 

  	for(std::list<Tile>::iterator tilePtr = combTiles.begin(); tilePtr != combTiles.end(); tilePtr++)
  	{
    	//compare 2 tiles
    	//break if nexttile is end
    	if(tilePtr == std::prev(combTiles.end())) break;
    	std::list<Tile>::iterator nextTilePtr = std::next(tilePtr);
    	Tile cur = *tilePtr;
    	Tile next = *nextTilePtr;
    	//if not same row, skip
    	if(cur.getBottom() != next.getBottom()) continue;
    	if(cur.getRight() < next.getLeft()) continue;
		
    	//overlapping tiles merge it
    	tilePtr->width_ = next.getRight() - cur.getLeft();
    	combTiles.erase(nextTilePtr);
    	tilePtr = std::prev(tilePtr); //back to current tile
  	}


  	std::vector<odb::dbMaster*> ffLibs = getFlattenFFLibs();
  	int min_width = std::numeric_limits<int>::max();
  	for(auto& ff : ffLibs)
  	{
  	  if(min_width > ff->getWidth()) min_width = ff->getWidth();
  	}

  	for(std::list<Tile>::iterator tilePtr = combTiles.begin(); tilePtr != combTiles.end(); tilePtr++)
  	{
  	  	if(tilePtr == std::prev(combTiles.end())) break;
  	  	std::list<Tile>::iterator nextTilePtr = std::next(tilePtr);
  	  	Tile cur = *tilePtr;
  	  	Tile next = *nextTilePtr;
  	  	if(cur.getBottom() != next.getBottom()) continue;
  	  	if(cur.getRight() + min_width <= next.getLeft()) continue;

  	  	//overlapping tiles merge it
  	  	tilePtr->width_ = next.getRight() - cur.getLeft();
  	  	combTiles.erase(nextTilePtr);
  	  	tilePtr = std::prev(tilePtr); //back to current tile
  	}
}


std::vector<std::pair<dbuPoint,int>> MBFF_solver::getInsertable(dbMaster* ff)
{
    std::vector<Architecture::Row*> Rows = arch_->getRows();
    std::list<std::pair<dbuPoint,int>> inserts;
    std::vector<std::list<Tile>> tileRows(Rows.size());
    tileRows.resize(Rows.size());

    DbuY currentYbot = DbuY(combTiles.front().getBottom());
    int currentYIdx = 0;
    for(const Tile& tile : combTiles) {
    if(tile.getBottom() != currentYbot) {
        currentYIdx++;
        currentYbot = tile.getBottom();
        }
        tileRows[currentYIdx].push_back(tile);
    }


    DbuY targetHeight = DbuY(ff->getHeight());
    std::cout<<"Org Width: "<<ff->getWidth()<<std::endl;
    DbuX targetWidth = DbuX(ff->getWidth())/ siteWidth;
    std::cout<<"Target Width: "<<targetWidth<<std::endl;
    std::cout<<"Site Width: "<<siteWidth<<std::endl;
    targetWidth = targetWidth * siteWidth; // snap to site
    if(targetWidth != ff->getWidth()) {
        targetWidth += siteWidth; // snap to site
    }
    std::cout<<"Target Width: "<<targetWidth<<std::endl;
  

    for(int i = 0; i < Rows.size(); i++)
    {
        bool terminate = 0;
        int bottomIdx = i;
        int topIdx = i;
        while(1)
        {
            if(Rows[topIdx]->getTop() - Rows[bottomIdx]->getBottom() < targetHeight)
            {
                if(topIdx == Rows.size() - 1) 
                {
                    terminate = 1;
                    break;
                }
            topIdx++;
        }
        break;
        }
        if(terminate) break;
        i = topIdx; 
        //init
        std::vector<std::list<Tile>::iterator> tileRowPtrs(topIdx - bottomIdx + 1);
        for(int j = bottomIdx; j <= topIdx; j++)
            tileRowPtrs[j - bottomIdx] = tileRows[j].begin();
        DbuX leftest = DbuX(0);
        DbuX nextBound = DbuX(0);
        for(int j = bottomIdx; j <= topIdx; j++)
        {
            if(Rows[j]->getLeft() > leftest) {
                leftest = Rows[j]->getLeft();
            }
        }
        nextBound = tileRowPtrs[0]->getLeft();
        for(int j = bottomIdx  ; j <= topIdx ; j++ )
        {
            nextBound = std::min(nextBound,
                               tileRowPtrs[j - bottomIdx]->getLeft());
        }

        while(1)
        {

            DbuX count = (nextBound - leftest)/targetWidth;
            if(count!=0)
                inserts.emplace_back(
                    dbuPoint(leftest, Rows[bottomIdx]->getBottom()),
                    static_cast<int>(count));

            leftest = DbuX(__INT_MAX__);
            for(int j = bottomIdx ; j <= topIdx ; j++ )
            {
                if(tileRowPtrs[j - bottomIdx] == tileRows[j].end())
                    continue;
                leftest = std::min(leftest,
                                tileRowPtrs[j - bottomIdx]->getRight());
            }

            if(leftest == __INT_MAX__)
                break;

            for(int j = bottomIdx ; j <= topIdx ; j++)
            {
                if(tileRowPtrs[j - bottomIdx] == tileRows[j].end())
                {
                    continue;
                }
                if(tileRowPtrs[j - bottomIdx]->getLeft() < leftest)
                    tileRowPtrs[j - bottomIdx]++;
            }

            if(tileRowPtrs[0] != tileRows[bottomIdx].end())
                nextBound = tileRowPtrs[0]->getLeft();
            else
                nextBound = Rows[bottomIdx]->getRight();
            for(int j = bottomIdx+1; j <= topIdx; j++)
            {
                DbuX candidate = DbuX(0);
                if(tileRowPtrs[j - bottomIdx] != tileRows[j].end())
                    candidate = tileRowPtrs[j - bottomIdx]->getLeft();
                else
                    candidate = Rows[j]->getRight();
                nextBound = std::min(nextBound, candidate);
            }
        }
    }

    std::vector<std::pair<dbuPoint,int>> ret{inserts.begin(),
                                           inserts.end()};

    insertableCache.resize(ret.size());
    for(int i = 0 ; i < ret.size() ; i++)
    {
        insertableCache[i].type = ff;
        insertableCache[i].count = ret[i].second;
        insertableCache[i].tile = Tile(ret[i].first.x, ret[i].first.y,
                                       targetWidth*ret[i].second, targetHeight);
    }


    //dbg
    for(auto& p : ret) {
        std::cout<< "Insertable: (" << p.first.x << ", "
                  << p.first.y << "), " << p.second << std::endl;
    }
    int total_cnt = 0;
    for(auto& p : ret) {
       total_cnt += p.second;
    }
    std::cout<<"Total Insertable Count: "<<total_cnt<<std::endl;
    return ret;

}
void MBFF_solver::ReadLibs()
{
    std::cout<< "Reading libraries..." << std::endl;

    
    
    int max_bit_cnt = 0;
    for (odb::dbLib* lib : db_->getLibs()) {
        ffVec.reserve(lib->getMasters().size());
        for (dbMaster* master : lib->getMasters()) {
            if (master->isFF()) {
                ffVec.push_back(master);
                if(max_bit_cnt < master->getBitCount()) {
                    max_bit_cnt = master->getBitCount();
                }
            }
            
        }
    }

    std::cout << "Max bit count: " << max_bit_cnt << std::endl;
    std::cout<< "Total FFs: " << ffVec.size() << std::endl;

    for (int v = 0; v < static_cast<int>(MBFFType::Count); ++v) {
        MBFFType type = static_cast<MBFFType>(v);
        FF_lib[type].resize(max_bit_cnt);
        std::cout<<"Type: " << static_cast<int>(type) << std::endl;
        for(int i = 0 ; i < max_bit_cnt; i++) {
            std::cout<<"Bit: "<<i<<std::endl;
            std::cout<< "FF_lib[" << static_cast<int>(type) << "][" << i << "] size: " << FF_lib[type][i].size() << std::endl;
            FF_lib[type][i].reserve(ffVec.size()); 
        }
    }

    std::cout << "FF_lib initialized." << std::endl;
    for (size_t i = 0 ; i < ffVec.size() ; i++)
    {
        MBFFType type = MBFFType(ffVec[i]->getFFType());
        int bitCnt = ffVec[i]->getBitCount();
        FF_lib[type][bitCnt-1].push_back(ffVec[i]);
    }


    std::cout << "FF_lib filled." << std::endl;
    int min_width = 0;
    for(auto& FF: ffVec)
    {
        if(FF->getWidth() < min_width || min_width == 0)
        {
            min_width = FF->getWidth();
        }
    }

    std::cout << "Minimum width: " << min_width << std::endl;   

}

std::vector<odb::dbMaster*> MBFF_solver::getFlattenFFLibs()
{
    return ffVec;
}
 
void MBFF_solver::setSiteWidth(DbuX width) { 
    siteWidth = width.v; 
    gMap_.setSiteWidth(width);
}
void MBFF_solver::setSiteHeight(DbuY height) { 
    siteHeight = height.v; 
    gMap_.setSiteHeight(height);
}

odb::dbMaster* MBFF_solver::getAreaFF(MBFFType type, int bits, bool is_max)
{
  if (bits < 0 || bits >= static_cast<int>(FF_lib[type].size())) {
    return nullptr;  // 超出範圍
  }

  std::vector<odb::dbMaster*>& ff_lib = FF_lib[type][bits - 1];

  double Area = __DBL_MAX__;
  if(is_max) Area = 0;  // Initialize to max or min area based on is_max
  odb::dbMaster* min_ff = nullptr;
  for(odb::dbMaster* ff : ff_lib)
  {
    if((ff->getArea() > Area) && is_max)
    {
      Area = ff->getArea();
      min_ff = ff;
    }
    else if((ff->getArea() < Area) && !is_max)
    {
      Area = ff->getArea();
      min_ff = ff;
    }
  }

  std::cout<< "Selected FF: " << (min_ff ? min_ff->getName() : "None")
            << ", Area: " << (min_ff ? min_ff->getArea() : 0) << std::endl;
  return min_ff;
}

bool overlap(const Tile& a, const Tile& b)
{
    return !(a.getRight() <= b.getLeft() || a.getLeft() >= b.getRight() ||
             a.getTop() <= b.getBottom() || a.getBottom() >= b.getTop());
}

void MBFF_solver::buildGridMap()
{
    std::cout<<"Building grid map..."<<std::endl;
    std::cout<<"RightBOund"<<arch_->getRow(0)->getRight()<<std::endl;
    std::cout<<"LeftBound"<<arch_->getRow(0)->getLeft()<<std::endl;
    std::cout<<"TopBound"<<arch_->getRow(0)->getTop()<<std::endl;
    std::cout<<"BottomBound"<<arch_->getRow(0)->getBottom()<<std::endl;
    int tile_n_FF_cnt = combTiles.size() + FFCells.size();
    int Y = arch_->getNumRows();
    int X = tile_n_FF_cnt / Y;
    gMap_.tbl.resize(X);
    
    for(int i = 0 ; i < X ; i++)
        gMap_.tbl[i].resize(Y);

    double planeTop = arch_->getMaxY().v;
    double planeRight = arch_->getMaxX().v;
    double planeLeft = arch_->getMinX().v;
    double planeBottom = arch_->getMinY().v;

    std::cout<<"Plane: ("<<planeLeft<<", "<<planeBottom<<") to ("
              <<planeRight<<", "<<planeTop<<")"<<std::endl;
    double x_step = (planeRight - planeLeft) / X;
    double y_step = (planeTop - planeBottom) / Y;
    for(int i = 0 ; i < X ; i++)
        for(int j = 0 ; j < Y ; j++)
        {
            gMap_.tbl[i][j].width_ = x_step;
            gMap_.tbl[i][j].height_ = y_step;
            gMap_.tbl[i][j].x_ = i * x_step + planeLeft;
            gMap_.tbl[i][j].y_ = j * y_step + planeBottom;
        }

    std::cout<<"Grid map built with "<<Y<<" rows and "
              <<X<<" columns."<<std::endl; 
    for(auto& tile : combTiles)
    {
        std::pair<int, int> idx = gMap_.getGridIndex(
            dbuPoint(tile.getLeft(), tile.getBottom()));
        std::pair<int, int> idx2 = gMap_.getGridIndex(
            dbuPoint(tile.getRight()-1, tile.getTop()-1));

        std::cout<<"Tile: ("<<tile.getLeft()<<", "<<tile.getBottom()<<") to ("
                  <<tile.getRight()<<", "<<tile.getTop()<<") -> Grid Index: ("
                  <<idx.first<<", "<<idx.second<<") to ("
                  <<idx2.first<<", "<<idx2.second<<")"<<std::endl;
        for(int i = idx.first ; i <= idx2.first ; i++)
        {
            for(int j = idx.second ; j <= idx2.second ; j++)
            {
                gMap_.tbl[i][j].fixedtiles.push_back(&tile);   
            }
        }
    }

    gMap_.header = std::unique_ptr<tileGrid>(new tileGrid);
    gMap_.header->x_ = planeLeft;
    gMap_.header->y_ = planeBottom;
    gMap_.header->width_ = planeRight - planeLeft;
    gMap_.header->height_ = planeTop - planeBottom;
    gMap_.header->parent = nullptr;
    gMap_.header->descendants[0][0] = nullptr;
    gMap_.header->descendants[0][1] = nullptr;
    gMap_.header->descendants[1][0] = nullptr;
    gMap_.header->descendants[1][1] = nullptr;
    //header doesn't need to store tiles (matching based algorithm)
    
}


void MBFF_solver::TopDownSplit()
{
    gMap_.TopDownSplit(gMap_.header.get(),
                  FFCells,
                  insertableCache);
}

void MBFF_solver::setFFPins()
{
    for(auto& ff : FFCells)
    {
        std::string ffName = ff->name();
        std::vector<Pin*> pins = ff->getPins();
        for(auto& pin : pins)
        {
            std::string pinName = ffName + "/" + pin->getName();
            std::cout<<"Name: "<<pinName<<std::endl;
            PinsMap[pin] = pinName;
        }

    }
    std::cout<<"FF Pins set."<<std::endl;
}

    

}