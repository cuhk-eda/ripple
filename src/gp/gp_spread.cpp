#include "gp_global.h"
#include "gp_data.h"
#include "gp_spread.h"

class LGBin {
    public:
        double lx = -1;
        double ly = -1;
        double hx = -1;
        double hy = -1;
        list<int> cells;
        //usable area
        double uArea = 0;
        //free area
        double fArea = 0;
        //cell area
        double cArea = 0;

        LGBin(const double _lx = -1, const double _ly = -1, const double _hx = -1, const double _hy = -1) : lx(_lx), ly(_ly), hx(_hx), hy(_hy) {}
};

class OFBin {
    public:
        //  overfill center
        int x = -1;
        int y = -1;
        int region = -1;
        double ofArea = 0;
        double ofRatio = 0;
        double aofArea = 0;
        double aofRatio = 0;
        //  expanded region
        int lx = -1;
        int ly = -1;
        int hx = -1;
        int hy = -1;
        double exCArea = 0;
        double exUArea = 0;
        double exFArea = 0;

        OFBin(const int _r = -1, const int _x = -1, const int _y = -1) : x(_x), y(_y), region(_r), lx(_x), ly(_y), hx(_x), hy(_y) {}

        bool operator<(const OFBin& b) const { return ofRatio>b.ofRatio; }
};

class ExpandBox {
    public:
        int lx,ly,hx,hy;
        list<int> cells;
        char dir;
        int level;
        ExpandBox(){
            lx=ly=hx=hy=-1;
            dir='h';
            level=0;
        }
        ExpandBox(int _lx, int _ly, int _hx, int _hy, int _level){
            lx=_lx;
            ly=_ly;
            hx=_hx;
            hy=_hy;
            dir='h';
            level=_level;
        }
        ExpandBox(int _lx, int _ly, int _hx, int _hy, int _level, list<int> _cells){
            lx=_lx;
            ly=_ly;
            hx=_hx;
            hy=_hy;
            cells=_cells;
            dir='h';
            level=_level;
        }
        void clear(){
            lx=ly=hx=hy=-1;
            cells.clear();
        }
};

vector<vector<vector<LGBin> > > LGGrid; //[fence][binx][biny]
vector<list<OFBin> > OFBins; //[fence][bin]
vector<vector<vector<char> > > OFMap;
vector<double> spreadCellX;
vector<double> spreadCellY;
int LGNumX;
int LGNumY;
double LGBinW;
double LGBinH;
int LGBinSize; //in number of rows

bool cmpX(const int& a, const int& b){
    return cellX[a] < cellX[b];
}
bool cmpY(const int& a, const int& b){
    return cellY[a] < cellY[b];
}

void updateLGGrid(int lx, int ly, int hx, int hy){
    //update cell area information inside the box (lx,ly,hx,hy)
    //clear information
    for(int r=0; r<numFences; r++){
        for(int x=lx; x<=hx; x++){
            for(int y=ly; y<=hy; y++){
                LGGrid[r][x][y].cArea=0;
                LGGrid[r][x][y].cells.clear();
            }
        }
    }
    //add cell to bins
    for(int i=0; i<numCells; i++){
        int r=cellFence[i];
        int x=max(0, min(LGNumX-1, (int)((cellX[i]-coreLX)/LGBinW)));
        int y=max(0, min(LGNumY-1, (int)((cellY[i]-coreLY)/LGBinH)));
        if(x>=lx && x<=hx && y>=ly && y<=hy){
            LGGrid[r][x][y].cArea+=cellW[i]*cellH[i];
            LGGrid[r][x][y].cells.push_back(i);
        }
    }
}

void initSpreadCells(int binSize){
    if(binSize==LGBinSize){
        return;
    }
    LGBinSize = binSize;
    LGBinW = LGBinSize * siteH;
    LGBinH = LGBinSize * siteH;
    LGNumX = (int)RoundUp((coreHX-coreLX)/LGBinW);
    LGNumY = (int)RoundUp((coreHY-coreLY)/LGBinH);
    double siteArea = siteW * siteH;

    //resize data structures
    LGGrid.clear();
    OFMap.clear();
    OFBins.clear();

    LGGrid.resize(numFences, vector<vector<LGBin> >(LGNumX, vector<LGBin>(LGNumY)));
    OFMap.resize(numFences, vector<vector<char> >(LGNumX, vector<char>(LGNumY, 0)));
    OFBins.resize(numFences);

    //for each legalization bin of each fence

    for(int r=0; r<numFences; r++){
        for(int x=0; x<LGNumX; x++){
            for(int y=0; y<LGNumY; y++){
                double lx=coreLX+x*LGBinW;
                double ly=coreLY+y*LGBinH;
                double hx=lx+LGBinW;
                double hy=ly+LGBinH;
                LGGrid[r][x][y].lx=lx;
                LGGrid[r][x][y].ly=ly;
                LGGrid[r][x][y].hx=hx;
                LGGrid[r][x][y].hy=hy;
                //convert to unit of site map
                int blx = x * (int)round(LGBinW/siteW);
                int bhx = blx+(int)round(LGBinW/siteW)-1;
                int bly = y * (int)round(LGBinH/siteH);
                int bhy = bly+(int)round(LGBinH/siteH)-1;
                blx=max(0, blx);
                bly=max(0, bly);

                bhx=min(numSitesX-1, bhx);
                bhy=min(numSitesY-1, bhy);
                LGGrid[r][x][y].fArea=0;
                for(int sx=blx; sx<=bhx; sx++){
                    for(int sy=bly; sy<=bhy; sy++){
                        if(  database.siteMap.getRegion(sx,sy) == r &&
                            !database.siteMap.getSiteMap(sx,sy,db::SiteMap::SiteBlocked) &&
                            !database.siteMap.getSiteMap(sx,sy,db::SiteMap::SiteM2Blocked)){
                            LGGrid[r][x][y].fArea+=siteArea;
                        }
                    }
                }
            }
        }
    }
}

void updateBinUsableArea(const double td){
    //Calculate placable area for each bin
    //for each fence
    for(int r=0; r<numFences; r++){
        double totalCArea=0.0;
        double totalUArea=0.0;
        double scale=1.0;
        //calculate total cell area of the fence
        for(int i=0; i<numCells; i++){
            if(r==cellFence[i]){
                totalCArea+=cellW[i]*cellH[i];
            }
        }
        //calculate total usable area under the current target density
        for(int x=0; x<LGNumX; x++){
            for(int y=0; y<LGNumY; y++){
                totalUArea += LGGrid[r][x][y].fArea * td;
            }
        }
        if(totalUArea<=totalCArea){
            //reduce target density by scale
            scale=totalCArea/(totalUArea*0.95);
        }else{
            scale=1.0;
        }
        //calculate usable area for each bin
        for(int x=0; x<LGNumX; x++){
            for(int y=0; y<LGNumY; y++){
                LGGrid[r][x][y].uArea = LGGrid[r][x][y].fArea * td * scale;
            }
        }
    }
}

double totOF=0.0;
double avgOF=0.0;
double maxOF=0.0;
double maxOFR=0.0;
double maxAOF=0.0;
double maxAOFR=0.0;
int nOF=0;

void findOverfillBins(){
    totOF=0.0;
    avgOF=0.0;
    maxOF=0.0;
    maxOFR=0.0;
    maxAOF=0.0;
    maxAOFR=0.0;
    nOF=0;

    double siteArea = siteW * siteH;
    for(int r=0; r<numFences; r++){
        OFBins[r].clear();
        //for each legalization bin
        for(int x=0; x<LGNumX; x++){
            for(int y=0; y<LGNumY; y++){
                //cell area overfill
                if(LGGrid[r][x][y].cArea>LGGrid[r][x][y].uArea){
                    OFBin ofb(r, x, y);
                    ofb.ofArea=LGGrid[r][x][y].cArea-LGGrid[r][x][y].uArea;
                    ofb.aofArea=LGGrid[r][x][y].cArea-LGGrid[r][x][y].fArea;
                    ofb.exCArea=LGGrid[r][x][y].cArea;
                    ofb.exFArea=LGGrid[r][x][y].fArea;
                    ofb.exUArea=LGGrid[r][x][y].uArea;
                    OFMap[r][x][y]=1;
                    if(LGGrid[r][x][y].uArea<siteArea){
                        ofb.ofRatio=ofb.ofArea;
                        ofb.aofRatio=ofb.aofArea;
                    }else{
                        ofb.ofRatio=ofb.ofArea/LGGrid[r][x][y].uArea;
                        ofb.aofRatio=ofb.aofArea/LGGrid[r][x][y].fArea;
                    }
                    OFBins[r].push_back(ofb);
                    totOF+=ofb.ofArea;
                    maxOF=max(ofb.ofArea, maxOF);
                    maxOFR=max(ofb.ofRatio, maxOFR);
                    maxAOF=max(ofb.aofArea, maxAOF);
                    maxAOFR=max(ofb.aofRatio, maxAOFR);
                    nOF++;
                }else{
                    OFMap[r][x][y]=0;
                }
            }
        }
    }
}

void spreadCellsInBoxEven(int r, ExpandBox &box){
    int n = box.cells.size();
    list<int>::iterator ci;
    list<int>::iterator ce;
    double lx=coreLX+box.lx*LGBinW;
    double ly=coreLY+box.ly*LGBinH;
    double hx=coreLX+(box.hx+1)*LGBinW;
    double hy=coreLY+(box.hy+1)*LGBinH;
    double stepx=(hx-lx)/n;
    double stepy=(hy-ly)/n;
    lx+=stepx/2;
    ly+=stepy/2;
    hx-=stepx/2;
    hy-=stepy/2;

    //linear spread
    box.cells.sort(cmpX);
    ci=box.cells.begin();
    ce=box.cells.end();
    for(int i=0; ci!=ce; ++ci,i++){
        cellX[*ci]=lx+i*stepx;
    }
    box.cells.sort(cmpY);
    ci=box.cells.begin();
    ce=box.cells.end();
    for(int i=0; ci!=ce; ++ci,i++){
        cellY[*ci]=ly+i*stepy;
    }
}

void spreadCellsInBoxOverlapFree(int r, ExpandBox &box){
    box.cells.sort(cmpY);
    list<int>::iterator ci;
    list<int>::iterator ce;
    ci = box.cells.begin();
    ce = box.cells.end();

    // calculate the average width of each row.
    double total_width = 0.0;
    for(; ci != ce; ci++) {
        total_width += cellW[*ci];
    }
    int nrows = LGBinSize;
    double aver_width = total_width / nrows;
    
    // to divide the cells in nrows roughly equally
    ci = box.cells.begin();
    ce = box.cells.end();
    double now_width=0.0;
    vector<vector<int> > cells_in_row; // to store the index of all the cells in each row
    vector<double> cell_width; // the total cell width in each row
    for(int r = 0; r < nrows; r++) {
        double cell_width_in_row = 0;
        double tmpd = aver_width * (r + 1);
        vector<int> tmpv;
        while (now_width < tmpd && ci != ce) {
            tmpv.push_back(*ci);
            now_width += cellW[*ci];
            cell_width_in_row += cellW[*ci];
            ci++;
        }
        cell_width.push_back(cell_width_in_row);
        cells_in_row.push_back(tmpv);
    }
    
    // to place the cells
    double lx=coreLX+box.lx*LGBinW;
    double hx=coreLX+(box.hx+1)*LGBinW;
    hx = min(hx, coreHX);
    double ly=coreLY+box.ly*LGBinH;
    double hy=coreLY+(box.hy+1)*LGBinH;
    hy = min(hy, coreHY);
    double binH = hy - ly;
    double binW = hx - lx;

    double row_width = binW;
    double row_height = binH / nrows;

    for (int r = 0; r < nrows; r++) {
        if (cells_in_row[r].empty()) break;
        
        sort(cells_in_row[r].begin(), cells_in_row[r].end(), cmpX);
        // (cells_in_row[r]).sort(cmpX);
        int cell_num = cells_in_row[r].size();
        double aver_space = (row_width - cell_width[r]) / (cell_num - 1);
        double xpos = lx, ypos =  ly + row_height * (r + 0.5);

        if (cell_num == 1) {
            int tmp = cells_in_row[r][0];
            cellX[tmp] = xpos + row_width / 2;
            cellY[tmp] = ypos;
            continue;
        }
        for (int c = 0; c < cell_num; c++) {
            if (c == 0) {
                int tmp = cells_in_row[r][c];
                xpos = lx + cellW[tmp];
                cellX[tmp] = xpos - cellW[tmp] / 2;
                cellY[tmp] = ypos;
                continue;
            }
            int tmp = cells_in_row[r][c];
            xpos += aver_space + cellW[tmp];
            cellX[tmp] = xpos  - cellW[tmp] / 2;
            cellY[tmp] = ypos;
        }
    }

    // deal with the case where the width of certain cell is bigger than the width of 
}

void spreadCellsInBoxFullLegal(int r, ExpandBox &box){
    int nrows=LGBinSize;
    vector<list<int> > rowcells(nrows);
    vector<double> rowspace(nrows, 0);
    vector<double> rowwidth(nrows, 0);

    double totalWidth=0.0;
    double totalSpace=0.0;

    box.cells.sort(cmpY);
    list<int>::iterator ci;
    list<int>::iterator ce;

    ci=box.cells.begin();
    ce=box.cells.end();
    for(int i=0; ci!=ce; ++ci,i++){
        totalWidth+=cellW[*ci];
    }
    
    double lx=coreLX+box.lx*LGBinW;
    double ly=coreLY+box.ly*LGBinH;
    //double hx=coreLX+(box.hx+1)*LGBinW;
    //double hy=coreLY+(box.hy+1)*LGBinH;

    int blx = box.lx*(int)round(LGBinW/siteW);
    int bhx = blx + (int)round(LGBinW/siteW)-1;
    int bly = box.ly*(int)round(LGBinH/siteH);
    int bhy = bly + (int)round(LGBinH/siteH)-1;
    blx = max(0, blx);
    bly = max(0, bly);
    bhx = min(numSitesX-1, bhx);
    bhy = min(numSitesY-1, bhy);

    for(int sy=bly; sy<=bhy; sy++){
        for(int sx=blx; sx<=bhx; sx++){
            if(  database.siteMap.getRegion(sx,sy) == r &&
                !database.siteMap.getSiteMap(sx,sy,db::SiteMap::SiteBlocked)){
                rowspace[sy-bly] += siteW;
            }
        }
    }

    for(int i=0; i<nrows; i++){
        totalSpace+=rowspace[i];
    }

    double nowWidth=0.0;
    double targetWidth=0.0;
    ci=box.cells.begin();
    ce=box.cells.end();
    for(int i=0; i<nrows; i++){
        targetWidth+=totalWidth*rowspace[i]/totalSpace;
        for(; ci!=ce && nowWidth+cellW[*ci]<targetWidth; ++ci){
            rowcells[i].push_back(*ci);
            rowwidth[i]+=cellW[*ci];
            nowWidth+=cellW[*ci];
        }
        rowcells[i].sort(cmpX);
    }

    for(int i=0; i<nrows; i++){
        if(rowcells[i].size()==0){
            continue;
        }
        double cellSpace=(rowspace[i]-rowwidth[i])/(double)rowcells[i].size();
        double x = lx+cellSpace*0.5;
        double y = ly+siteH*(0.5+(double)i);
        list<int>::iterator ci=rowcells[i].begin();
        list<int>::iterator ce=rowcells[i].end();
        for(; ci!=ce; ++ci){
            cellX[*ci]=x+0.5*cellW[*ci];
            cellY[*ci]=y;
            x+=cellW[*ci]+cellSpace;
        }
    }
}

//#define EVENLY_SPREAD
bool spreadCellsH(int r, ExpandBox box, ExpandBox &lobox, ExpandBox &hibox){
    //assume the cell list has been sorted by increasing X
    //lobox.cells=box.cells;
    list<int>::iterator ci;
    list<int>::iterator ce;
    vector<double> uArea(box.hx-box.lx+1, 0);
    double loFArea=0;
    double hiFArea=0;
    double loCArea=0;
    double hiCArea=0;
    double totalFArea=0;
    double totalCArea=0;
    lobox.ly=hibox.ly=box.ly;
    lobox.hy=hibox.hy=box.hy;
    lobox.lx=box.lx;
    lobox.hx=box.lx;
    hibox.lx=box.hx;
    hibox.hx=box.hx;
    //calculate stripe free area
    for(int x=box.lx; x<=box.hx; x++){
        for(int y=box.ly; y<=box.hy; y++){
            double area=LGGrid[r][x][y].uArea;
            uArea[x-box.lx]+=area;
            totalFArea+=area;
        }
    }
    //shrink the box such that last stripe has free space
    for(int x=box.lx; x<=box.hx && lobox.hx<hibox.lx-1; x++){
        if(uArea[x-box.lx]>0){
            break;
        }
        lobox.lx++;
        lobox.hx++;
    }
    for(int x=box.hx; x>=box.lx && hibox.lx>lobox.hx+1; x--){
        if(uArea[x-box.lx]>0){
            break;
        }
        hibox.lx--;
        hibox.hx--;
    }
    if(lobox.hx==hibox.lx){
        //TODO: how to handle this?
        //box.lx=lobox.hx;
        //box.hx=lobox.hx;
        //spreadCellsInBoxEven(r, box);
        return false;
    }
    //calculate cell area
    ci=box.cells.begin();
    ce=box.cells.end();
    for(; ci!=ce; ++ci){
        totalCArea+=cellW[*ci]*cellH[*ci];
    }

    //search for cutting line, such that loArea = hiArea
    loFArea=uArea[lobox.hx-box.lx];
    hiFArea=uArea[hibox.lx-box.lx];
    while(hibox.lx-lobox.hx>1){
        if(loFArea<=hiFArea){
            lobox.hx++;
            loFArea+=uArea[lobox.hx-box.lx];
        }else{
            hibox.lx--;
            hiFArea+=uArea[hibox.lx-box.lx];
        }
    }

    double halfCArea=loFArea/totalFArea*totalCArea;
    /*
    ci=box.cells.begin();
    ce=box.cells.end();
    for(; ci!=ce; ++ci){
        double cellarea=cellW[*ci]*cellH[*ci];
        if(loCArea+cellarea<=halfCArea){
            loCArea+=cellarea;
            lobox.cells.push_front(*ci);
        }else{
            hiCArea+=cellarea;;
            hibox.cells.push_back(*ci);
        }
    }
    box.cells.clear();
    */
    
    ci=box.cells.begin();
    ce=box.cells.end();
    for(; ci!=ce; ++ci){
        double cellarea=cellW[*ci]*cellH[*ci];
        if(loCArea+cellarea<=halfCArea){
            loCArea+=cellarea;
        }else{
            break;
        }
    }
    lobox.cells.splice(lobox.cells.begin(), box.cells, box.cells.begin(), ci);
    hibox.cells.splice(hibox.cells.begin(), box.cells, box.cells.begin(), box.cells.end());
    lobox.cells.reverse();
    ci=hibox.cells.begin();
    ce=hibox.cells.end();
    for(; ci!=ce; ++ci){
        hiCArea+=cellW[*ci]*cellH[*ci];
    }

    list<int>::iterator cellFm;
    list<int>::iterator cellTo;

    ce=lobox.cells.end();
    cellFm=lobox.cells.begin();
    cellTo=lobox.cells.begin();
    //for each column of bins
    for(int x=lobox.hx; x>=lobox.lx; x--){
        if(cellFm==ce){
            break;
        }
#ifdef EVENLY_SPREAD
        double cArea=0;
        int num=0;
        for(cellTo=cellFm; cellTo!=ce && (x==lobox.lx || cArea<uArea[x-box.lx]); ++cellTo){
            cArea+=cellW[*cellTo]*cellH[*cellTo];
            num++;
        }
        if(num>0){
            double step=LGBinW/(double)num;
            double nowx=coreLX+(x+1)*LGBinW-step/2.0;
            for(ci=cellFm; ci!=cellTo; ++ci){
                cellX[*ci]=nowx;
                nowx-=step;
            }
        }
#else
        double cArea=0;
        int num=0;
        double oldLoX=cellX[*cellFm];
        double oldHiX=oldLoX;
        for(cellTo=cellFm; cellTo!=ce && (x==lobox.lx || cArea<uArea[x-box.lx]); ++cellTo){
            cArea+=cellW[*cellTo]*cellH[*cellTo];
            oldLoX=cellX[*cellTo]; //decreasing X
            num++;
        }
        if(num>0){
            double newLoX=coreLX+x*LGBinW+LGBinW/(double)(num*2);
            double newHiX=newLoX+LGBinW-LGBinW/(double)num;
            double rangeold=max(1.0, oldHiX-oldLoX);
            double rangenew=newHiX-newLoX;
            double scale=rangenew/rangeold;
            for(ci=cellFm; ci!=cellTo; ++ci){
                cellX[*ci]=newLoX+(cellX[*ci]-oldLoX)*scale;
            }
        }
#endif
        cellFm=cellTo;
    }
    ce=hibox.cells.end();
    cellFm=hibox.cells.begin();
    cellTo=hibox.cells.begin();
    for(int x=hibox.lx; x<=hibox.hx; x++){
        if(cellFm==ce){
            break;
        }
#ifdef EVENLY_SPREAD
        double cArea=0;
        int num=0;
        for(cellTo=cellFm; cellTo!=ce && (x==hibox.hx || cArea<uArea[x-box.lx]); ++cellTo){
            cArea+=cellW[*cellTo]*cellH[*cellTo];
            num++;
        }
        if(num>0){
            double step=LGBinW/(double)num;
            double nowx=coreLX+x*LGBinW+step/2.0;
            for(ci=cellFm; ci!=cellTo; ++ci){
                cellX[*ci]=nowx;
                nowx+=step;
            }
        }
#else
        double cArea=0;
        int num=0;
        double oldLoX=cellX[*cellFm];
        double oldHiX=oldLoX;
        for(cellTo=cellFm; cellTo!=ce && (x==hibox.hx || cArea<uArea[x-box.lx]); ++cellTo){
            cArea+=cellW[*cellTo]*cellH[*cellTo];
            oldHiX=cellX[*cellTo]; //increasing X
            num++;
        }
        if(num>0){
            double newLoX=coreLX+x*LGBinW+LGBinW/(double)(num*2);
            double newHiX=newLoX+LGBinW-LGBinW/(double)num;
            double rangeold=max(1.0, oldHiX-oldLoX);
            double rangenew=newHiX-newLoX;
            double scale=rangenew/rangeold;
            for(ci=cellFm; ci!=cellTo; ++ci){
                cellX[*ci]=newLoX+(cellX[*ci]-oldLoX)*scale;
            }
        }
#endif
        cellFm=cellTo;
    }
    return true;
}
bool spreadCellsV(int r, ExpandBox box, ExpandBox &lobox, ExpandBox &hibox){
    //assume the cell list has been sorted by increasing Y
    //lobox.cells=box.cells;
    list<int>::iterator ci;
    list<int>::iterator ce;
    vector<double> uArea(box.hy-box.ly+1, 0);
    double loFArea=0;
    double hiFArea=0;
    double loCArea=0;
    double hiCArea=0;
    double totalFArea=0;
    double totalCArea=0;
    lobox.lx=hibox.lx=box.lx;
    lobox.hx=hibox.hx=box.hx;
    lobox.ly=box.ly;
    lobox.hy=box.ly;
    hibox.ly=box.hy;
    hibox.hy=box.hy;
    //calculate stripe free area
    for(int x=box.lx; x<=box.hx; x++){
        for(int y=box.ly; y<=box.hy; y++){
            double area=LGGrid[r][x][y].uArea;
            uArea[y-box.ly]+=area;
            totalFArea+=area;
        }
    }
    //shrink the box such that last stripe has free space
    for(int y=box.ly; y<=box.hy && lobox.hy<hibox.ly-1; y++){
        if(uArea[y-box.ly]>0){
            break;
        }
        lobox.ly++;
        lobox.hy++;
    }
    for(int y=box.hy; y>=box.ly && hibox.ly>lobox.hx+1; y--){
        if(uArea[y-box.ly]>0){
            break;
        }
        hibox.ly--;
        hibox.hy--;
    }
    if(lobox.hy==hibox.ly){
        //TODO: how to handle this?
        //box.ly=lobox.hy;
        //box.hy=lobox.hy;
        //spreadCellsInBoxEven(r, box);
        return false;
    }
    //calculate cell area
    ci=box.cells.begin();
    ce=box.cells.end();
    for(; ci!=ce; ++ci){
        totalCArea+=cellW[*ci]*cellH[*ci];
    }

    //search for cutting line, such that loArea = hiArea
    loFArea=uArea[lobox.hy-box.ly];
    hiFArea=uArea[hibox.ly-box.ly];
    while(hibox.ly-lobox.hy>1){
        if(loFArea<=hiFArea){
            lobox.hy++;
            loFArea+=uArea[lobox.hy-box.ly];
        }else{
            hibox.ly--;
            hiFArea+=uArea[hibox.ly-box.ly];
        }
    }

    double halfCArea=loFArea/totalFArea*totalCArea;
    /*
    ci=box.cells.begin();
    ce=box.cells.end();
    for(; ci!=ce; ++ci){
        double cellarea=cellW[*ci]*cellH[*ci];
        if(loCArea+cellarea<=halfCArea){
            loCArea+=cellarea;
            lobox.cells.push_front(*ci);
        }else{
            hiCArea+=cellarea;
            hibox.cells.push_back(*ci);
        }
    }
    box.cells.clear();
    */
    
    ci=box.cells.begin();
    ce=box.cells.end();
    for(; ci!=ce; ++ci){
        double cellarea=cellW[*ci]*cellH[*ci];
        if(loCArea+cellarea<=halfCArea){
            loCArea+=cellarea;
        }else{
            break;
        }
    }
    lobox.cells.splice(lobox.cells.begin(), box.cells, box.cells.begin(), ci);
    hibox.cells.splice(hibox.cells.begin(), box.cells, box.cells.begin(), box.cells.end());
    lobox.cells.reverse();
    ci=hibox.cells.begin();
    ce=hibox.cells.end();
    for(; ci!=ce; ++ci){
        hiCArea+=cellW[*ci]*cellH[*ci];
    }

    list<int>::iterator cellFm;
    list<int>::iterator cellTo;

    ce=lobox.cells.end();
    cellFm=lobox.cells.begin();
    cellTo=lobox.cells.begin();
    //for each row of bins
    for(int y=lobox.hy; y>=lobox.ly; y--){
        if(cellFm==ce){
            break;
        }
#ifdef EVENLY_SPREAD
        double cArea=0;
        int num=0;
        for(cellTo=cellFm; cellTo!=ce && (y==lobox.ly || cArea<uArea[y-box.ly]); ++cellTo){
            cArea+=cellW[*cellTo]*cellH[*cellTo];
            num++;
        }
        if(num>0){
            double step=LGBinH/(double)num;
            double nowy=coreLY+(y+1)*LGBinH-step/2.0;
            for(ci=cellFm; ci!=cellTo; ++ci){
                yCellCoord[*ci]=nowy;
                nowy-=step;
            }
        }
#else
        double cArea=0;
        int num=0;
        double oldLoY=cellY[*cellFm];
        double oldHiY=oldLoY;
        for(cellTo=cellFm; cellTo!=ce && (y==lobox.ly || cArea<uArea[y-box.ly]); ++cellTo){
            cArea+=cellW[*cellTo]*cellH[*cellTo];
            oldLoY=cellY[*cellTo]; //decreasing Y
            num++;
        }
        if(num>0){
            double newLoY=coreLY+y*LGBinH+LGBinH/(double)(num*2);
            double newHiY=newLoY+LGBinH-LGBinH/(double)num;
            double rangeold=max(1.0, oldHiY-oldLoY);
            double rangenew=newHiY-newLoY;
            double scale=rangenew/rangeold;
            for(ci=cellFm; ci!=cellTo; ++ci){
                cellY[*ci]=newLoY+(cellY[*ci]-oldLoY)*scale;
            }
        }
#endif
        cellFm=cellTo;
    }
    ce=hibox.cells.end();
    cellFm=hibox.cells.begin();
    cellTo=hibox.cells.begin();
    for(int y=hibox.ly; y<=hibox.hy; y++){
        if(cellFm==ce){
            break;
        }
#ifdef EVENLY_SPREAD
        double cArea=0;
        int num=0;
        for(cellTo=cellFm; cellTo!=ce && (y==hibox.hy || cArea<uArea[y-box.ly]); ++cellTo){
            cArea+=cellW[*cellTo]*cellH[*cellTo];
            num++;
        }
        if(num>0){
            double step=LGBinH/(double)num;
            double nowy=coreLY+y*LGBinH+step/2.0;
            for(ci=cellFm; ci!=cellTo; ++ci){
                yCellCoord[*ci]=nowy;
                nowy+=step;
            }
        }
#else
        double cArea=0;
        int num=0;
        double oldLoY=cellY[*cellFm];
        double oldHiY=oldLoY;
        for(cellTo=cellFm; cellTo!=ce && (y==hibox.hy || cArea<uArea[y-box.ly]); ++cellTo){
            cArea+=cellW[*cellTo]*cellH[*cellTo];
            oldHiY=cellY[*cellTo]; //increasing Y
            num++;
        }
        if(num>0){
            double newLoY=coreLY+y*LGBinH+LGBinH/(double)(num*2);
            double newHiY=newLoY+LGBinH-LGBinH/(double)num;
            double rangeold=max(1.0, oldHiY-oldLoY);
            double rangenew=newHiY-newLoY;
            double scale=rangenew/rangeold;
            for(ci=cellFm; ci!=cellTo; ++ci){
                cellY[*ci]=newLoY+(cellY[*ci]-oldLoY)*scale;
            }
        }
#endif
        cellFm=cellTo;
    }
    return true;
}

void expandLGRegion(OFBin &ofBin) {
    ofBin.lx=ofBin.hx=ofBin.x;
    ofBin.ly=ofBin.hy=ofBin.y;
    ofBin.exFArea=LGGrid[ofBin.region][ofBin.x][ofBin.y].fArea;
    ofBin.exUArea=LGGrid[ofBin.region][ofBin.x][ofBin.y].uArea;
    ofBin.exCArea=LGGrid[ofBin.region][ofBin.x][ofBin.y].cArea;
    unsigned char direction{'\0'};
    for(; ofBin.exCArea > ofBin.exUArea; direction = (direction + 1) % 4){
        if (direction == 0 && ofBin.lx > 0) {
            //expand to left
            ofBin.lx--;
            for(int y=ofBin.ly; y<=ofBin.hy; y++){
                ofBin.exFArea+=LGGrid[ofBin.region][ofBin.lx][y].fArea;
                ofBin.exUArea+=LGGrid[ofBin.region][ofBin.lx][y].uArea;
                ofBin.exCArea+=LGGrid[ofBin.region][ofBin.lx][y].cArea;
            }
        } else if (direction == 1 && ofBin.ly > 0) {
            //expand to bottom
            ofBin.ly--;
            for(int x=ofBin.lx; x<=ofBin.hx; x++){
                ofBin.exFArea+=LGGrid[ofBin.region][x][ofBin.ly].fArea;
                ofBin.exUArea+=LGGrid[ofBin.region][x][ofBin.ly].uArea;
                ofBin.exCArea+=LGGrid[ofBin.region][x][ofBin.ly].cArea;
            }
        } else if (direction == 2 && ofBin.hx < LGNumX - 1) {
            //expand to right
            ofBin.hx++;
            for(int y=ofBin.ly; y<=ofBin.hy; y++){
                ofBin.exFArea+=LGGrid[ofBin.region][ofBin.hx][y].fArea;
                ofBin.exUArea+=LGGrid[ofBin.region][ofBin.hx][y].uArea;
                ofBin.exCArea+=LGGrid[ofBin.region][ofBin.hx][y].cArea;
            }
        } else if (direction == 3 && ofBin.hy < LGNumY - 1) {
            //expand to top
            ofBin.hy++;
            for(int x=ofBin.lx; x<=ofBin.hx; x++){
                ofBin.exFArea+=LGGrid[ofBin.region][x][ofBin.hy].fArea;
                ofBin.exUArea+=LGGrid[ofBin.region][x][ofBin.hy].uArea;
                ofBin.exCArea+=LGGrid[ofBin.region][x][ofBin.hy].cArea;
            }
        }
        if ((ofBin.lx == 0 && ofBin.hx == LGNumX - 1) && (ofBin.ly == 0 && ofBin.hy == LGNumY - 1)) {
            break;
        }
    }
    if(ofBin.exUArea<ofBin.exCArea){
        cout<<"ERROR: expand fail"<<(int)ofBin.exUArea<<" < "<<(int)ofBin.exCArea<<endl;
    }
    if(ofBin.lx==0 && ofBin.ly==0 && ofBin.hx==LGNumX-1 && ofBin.hy==LGNumY-1){
        cout<<"WARN: expand to chip size"<<endl;
    }
}
#define FAST_LG_UPDATE
void spreadCells(const int binSize, const int times, const double td) {
    initSpreadCells(binSize);
    updateBinUsableArea(td);

    vector<OFBin> maxBin(numFences);
    int minExpandSize=2;
    int maxLevel=100;
    
    spreadCellX.resize(numNodes);
    spreadCellY.resize(numNodes);

    for(int iter=0; iter<times; iter++){
        //int timerid=start_timer();
        updateLGGrid(0, 0, LGNumX-1, LGNumY-1);
        findOverfillBins();
        
        copy(cellX.begin(), cellX.end(), spreadCellX.begin());
        copy(cellY.begin(), cellY.end(), spreadCellY.begin());

        if(nOF>0){
            avgOF=totOF/nOF;
            //cout<<"rough legalize iter "<<iter<<" avg-of="<<avgOF<<" max-of="<<maxOF<<" max-ofr="<<maxOFR<<" max-aof="<<maxAOF<<" max-aofr="<<maxAOFR<<" n-of="<<nOF<<endl;
        }else{
            //cout<<"rough legalize iter "<<iter<<" no overflow"<<endl;
        }

        for(int r=0; r<numFences; r++){
            if(OFBins[r].size()==0){
                continue;
            }
            OFBins[r].sort();

            //legalize all overfilled bins
            while(OFBins[r].size()>0){
                maxBin[r]=OFBins[r].front();
                OFBins[r].pop_front();
                if(OFMap[r][maxBin[r].x][maxBin[r].y]==0){
                    //overfill have been resolved
                    continue;
                }

                //  find expand region
                expandLGRegion(maxBin[r]);
                
                queue<ExpandBox> boxes;

                ExpandBox box(maxBin[r].lx, maxBin[r].ly, maxBin[r].hx, maxBin[r].hy, maxLevel);
                //get list of cells in expanded region
                for(int x=maxBin[r].lx; x<=maxBin[r].hx; x++){
                    for(int y=maxBin[r].ly; y<=maxBin[r].hy; y++){
                        OFMap[r][x][y]=0;
                        box.cells.splice(box.cells.end(), LGGrid[r][x][y].cells);
                    }
                }
                boxes.push(box);
                
                while(!boxes.empty()){
                    ExpandBox box=boxes.front();
                    boxes.pop();

                    ExpandBox lobox;
                    ExpandBox hibox;
                    lobox.level=box.level-1;
                    hibox.level=box.level-1;

                    if(box.hx-box.lx<minExpandSize-1 && box.hy-box.ly<minExpandSize-1){
                        continue;
                    }

                    bool success=false;
                    if(box.dir=='h'){
                        if(box.hx-box.lx>=minExpandSize-1){
                            box.cells.sort(cmpX);
                            success=spreadCellsH(r, box, lobox, hibox);
                            lobox.dir='v';
                            hibox.dir='v';
                        }
                        if(!success && box.hy-box.ly>=minExpandSize-1){
                            box.cells.sort(cmpY);
                            success=spreadCellsV(r, box, lobox, hibox);
                            lobox.dir='h';
                            hibox.dir='h';
                        }
                    }else{
                        if(box.hy-box.ly>=minExpandSize-1){
                            box.cells.sort(cmpY);
                            success=spreadCellsV(r, box, lobox, hibox);
                            lobox.dir='h';
                            hibox.dir='h';
                        }
                        if(!success && box.hx-box.lx>=minExpandSize-1){
                            box.cells.sort(cmpX);
                            success=spreadCellsH(r, box, lobox, hibox);
                            lobox.dir='v';
                            hibox.dir='v';
                        }
                    }
                    if(success){
                        if(lobox.cells.size()>0 && lobox.level>0){
                            boxes.push(lobox);
                        }
                        if(hibox.cells.size()>0 && hibox.level>0){
                            boxes.push(hibox);
                        }
                    }
                }
#ifdef FAST_LG_UPDATE
                updateLGGrid(box.lx, box.ly, box.hx, box.hy);
#else
                updateLGGrid(0, 0, LGNumX-1, LGNumY-1);
#endif
            }
        }
        double dispx=0.0;
        double dispy=0.0;
        for(int i=0; i<numCells; i++){
            dispx+=std::abs(cellX[i]-spreadCellX[i]);
            dispy+=std::abs(cellY[i]-spreadCellY[i]);
        }
        //cout<<"disp="<<(int)dispx<<"+"<<(int)dispy<<"="<<(int)(dispx+dispy)<<std::endl;
        //cout<<"rough legalization time="<<stop_timer(timerid)<<endl;
        if((int)(dispx+dispy)==0){
            break;
        }
    }

    //int timerid=start_timer();
    updateLGGrid(0, 0, LGNumX-1, LGNumY-1);
    for(int r=0; r<numFences; r++){
        for(int x=0; x<LGNumX; x++){
            for(int y=0; y<LGNumY; y++){
                LGBin &bin=LGGrid[r][x][y];
                ExpandBox box(x, y, x, y, 0, bin.cells);
                //spreadCellsInBoxEven(r, box);
                //spreadCellsInBoxOverlapFree(r, box);
                spreadCellsInBoxFullLegal(r, box);
            }
        }
    }
    /*
    gp_copy_out();
    legalize(LG_SNAP);
    gp_copy_in(false);
    */

    //cout<<"in-bin spread time="<<stop_timer(timerid)<<endl;
}


