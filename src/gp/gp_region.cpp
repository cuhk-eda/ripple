#include "gp_global.h"
#include "gp_data.h"
#include "gp_region.h"

void findCellRegionDist(){
    for(int i=0; i<numCells; i++){
        //for each cell, find nearest region block
        double x=cellX[i];
        double y=cellY[i];
        int region=cellFence[i];

        double dist=-1;
        double regionx=-1;
        double regiony=-1;
        int regionr=-1;
        int nRects = fenceRectLX[region].size();
        for(int r=0; r<nRects; r++){
            double lx = fenceRectLX[region][r];
            double ly = fenceRectLY[region][r];
            double hx = fenceRectHX[region][r];
            double hy = fenceRectHY[region][r];
            double distx = 0;
            double disty = 0;
            double rx = x;
            double ry = y;
            if(x < lx){
                distx = lx - x;
                rx = lx;
            }else if(x > hx){
                distx = x - hx;
                rx = hx;
            }
            if(y < ly){
                disty = ly - y;
                ry = ly;
            }else if(y > hy){
                disty = y - hy;
                ry = hy;
            }
            if(r == 0 || distx + disty < dist){
                dist = distx + disty;
                regionx = rx;
                regiony = ry;
                regionr = r;
                if(dist == 0){
                    break;
                }
            }
        }
        if(regionr<0){
            printlog(LOG_ERROR, "no rect defined for region %d", region);
        }
        cellFenceX[i]=regionx;
        cellFenceY[i]=regiony;
        cellFenceRect[i]=regionr;
        cellFenceDist[i]=dist;
    }
}
int legal_count=0;
void legalizeRegion(){
    //assume findCellRegionDist() is called already
    double epsilon = 0.1;
    for(int i=0; i<numCells; i++){
        double x=cellX[i];
        double y=cellY[i];

        x=max(coreLX, min(coreHX, x));
        y=max(coreLY, min(coreHY, y));

        int f=cellFence[i];
        int r=cellFenceRect[i];
        double lx = fenceRectLX[f][r];
        double ly = fenceRectLY[f][r];
        double hx = fenceRectHX[f][r];
        double hy = fenceRectHY[f][r];
        if(x < lx){
            cellX[i] = lx + (x - coreLX) / (lx - coreLX) * epsilon;
            //it is always x >= coreLX, so when x < lx, lx > coreLX , divided by zero avoided
        }else if(x > hx){
            cellX[i] = hx - epsilon + (x - hx) / (coreHX - hx) * epsilon;
            //it is always x <= coreHX, so when x > hx, hx < coreHX , divided by zero avoided
        }else{
            cellX[i] = lx + epsilon + (x - lx) / (hx - lx)*(hx - lx - epsilon - epsilon);
        }
        if(y < ly){
            cellY[i] = ly + (y - coreLY) / (ly - coreLY) * epsilon;
        }else if(y > hy){
            cellY[i] = hy - epsilon + (y - hy) / (coreHY - hy) * epsilon;
        }else{
            cellY[i] = ly + epsilon + (y - ly) / (hy - ly) * (hy - ly - epsilon -epsilon);
        }
    }
}

