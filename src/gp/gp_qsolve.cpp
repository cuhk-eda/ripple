#include "gp_global.h"
#include "gp_data.h"

#include "gp_region.h"
#include "gp_qsolve.h"

using namespace gp;

#define FAST_INDEX_LOOKUP

double MinCellDist=0.01; //to avoid zero distance between two cells

long Cxsize;
long Cysize;
vector<SparseMatrix> Cx;
vector<SparseMatrix> Cy;
vector<double> bx;
vector<double> by;
vector<double> Mx_inverse;
vector<double> My_inverse;
vector<double> lox;
vector<double> hix;
vector<double> loy;
vector<double> hiy;
vector<double> lastVarX;
vector<double> lastVarY;
vector<double> varX;
vector<double> varY;

int numMovables = 0;
int numConnects = 0;

NetModel netModel = NetModelNone;

#ifdef FAST_INDEX_LOOKUP
vector<vector<long> > indexMapX;
vector<vector<long> > indexMapY;
#else
map<long, int> indexMapX;
map<long, int> indexMapY;
#endif

void initLowerBound(){
    netModel = NetModelNone;
}

void initCG(NetModel model){
    if(model == netModel){
        return;
    }

    int nConnects = 0;
    int nMovables = 0;
    if(model == NetModelClique){
        if(numPins > 50000){
            printlog(LOG_WARN, "too many pins for clique model!");
        }
        for(int i=0; i<numNets; i++){
            int nPins = netCell[i].size();
            if(nPins>1){
                nConnects += nPins * (nPins-1) / 2;
            }
        }
        nMovables = numCells;
    }else if(model == NetModelStar){
        nConnects = numPins;
        nMovables = numCells + numNets;
    }else if(model == NetModelB2B){
        nConnects = 2 * numPins + numCells;
        nMovables = numCells;
    }else if(model == NetModelHybrid){
        nConnects = numPins;
        nMovables = numCells + numNets;
    }else if(model == NetModelMST){
        nConnects = numPins;
        nMovables = numCells;
    }else{
        nConnects = 0;
        nMovables = 0;
        printlog(LOG_ERROR, "invalid net model");
    }

    numMovables = nMovables;
    numConnects = nConnects;
    netModel = model;

    varX.resize(numMovables, 0.0);
    varY.resize(numMovables, 0.0);
    lastVarX = cellX;
    lastVarY = cellY;
    Cx.resize(numConnects);
    Cy.resize(numConnects);
    bx.resize(numMovables);
    by.resize(numMovables);
    Mx_inverse.resize(numMovables);
    My_inverse.resize(numMovables);
    lox.resize(numMovables);
    hix.resize(numMovables);
    loy.resize(numMovables);
    hiy.resize(numMovables);
    Cxsize=0;
    Cysize=0;
#ifdef FAST_INDEX_LOOKUP
    indexMapX.resize(numMovables);
    indexMapY.resize(numMovables);
#endif
}


void resetCG(){
    bx.assign(numMovables, 0);
    by.assign(numMovables, 0);
#ifdef FAST_INDEX_LOOKUP
    vector<vector<long> >::iterator itrx=indexMapX.begin();
    vector<vector<long> >::iterator itrxe=indexMapX.end();
    vector<vector<long> >::iterator itry=indexMapY.begin();
    vector<vector<long> >::iterator itrye=indexMapY.end();
    for(;itrx!=itrxe; ++itrx){
        itrx->clear();
    }
    for(;itry!=itrye; ++itry){
        itry->clear();
    }
#endif
    Cxsize=0;
    Cysize=0;
}
#ifdef FAST_INDEX_LOOKUP
int idxGetX(int i, int j){
    int size=indexMapX[i].size();
    for(int k=0; k<size; k++){
        if((int)(indexMapX[i][k]&0xffffffff)==j){
            return indexMapX[i][k]>>32;
        }
    }
    return -1;
}
int idxGetY(int i, int j){
    int size=indexMapY[i].size();
    for(int k=0; k<size; k++){
        if((int)(indexMapY[i][k]&0xffffffff)==j){
            return indexMapY[i][k]>>32;
        }
    }
    return -1;
}
void idxSetX(int i, int j, int x){
    long val=(((long)x)<<32)|(long)j;
    indexMapX[i].push_back(val);
}
void idxSetY(int i, int j, int x){
    long val=(((long)x)<<32)|(long)j;
    indexMapY[i].push_back(val);
}
#else
int idxGetX(int i, int j){
    long idx=(long)i*(long)numMovables+(long)j;
    map<long, int>::iterator itr=indexMapX.find(idx);
    if(itr==indexMapX.end()){
        return -1;
    }
    return itr->second;
}
int idxGetY(int i, int j){
    long idx=(long)i*(long)numMovables+(long)j;
    map<long, int>::iterator itr=indexMapY.find(idx);
    if(itr==indexMapY.end()){
        return -1;
    }
    return itr->second;
}
void idxSetX(int i, int j, int x){
    long idx=(long)i*(long)numMovables+(long)j;
    indexMapX[idx]=x;
}
void idxSetY(int i, int j, int x){
    long idx=(long)i*(long)numMovables+(long)j;
    indexMapY[idx]=x;
}
#endif

void addToC(
        vector<SparseMatrix> &C,
        int i, int j, double w,
        long *Csize,
        int (*idxGet)(int,int),
        void (*idxSet)(int,int,int)){
    //C[i][j]=C[i][j]+w
    int C_index=idxGet(i,j);
    if(C_index<0){
        //new value, add to C[Csize]
        C_index=*Csize;
        idxSet(i,j,C_index);
        C[C_index].row=i;
        C[C_index].col=j;
        C[C_index].num=w;

        *Csize=C_index+1;
    }else{
        //accumulate to old value
        C[C_index].num+=w;
    }
}

void B2BaddInC_b(
        int net,
        vector<SparseMatrix> &C,
        vector<double> &b,
        int idx1, int idx2,
        double x1, double x2,
        bool mov1, bool mov2,
        double w, long *Csize,
        int (*idxGet)(int,int),
        void (*idxSet)(int,int,int)){
    if(idx1 == idx2){
        return;
    }
    if(mov1 && mov2){
        if(idx1 < idx2){
            addToC(C, idx1, idx1, w, Csize, idxGet, idxSet);
            addToC(C, idx2, idx2, w, Csize, idxGet, idxSet);
            addToC(C, idx1, idx2, -w, Csize, idxGet, idxSet);
        }else{
            addToC(C, idx1, idx1, w, Csize, idxGet, idxSet);
            addToC(C, idx2, idx2, w, Csize, idxGet, idxSet);
            addToC(C, idx2, idx1, -w, Csize, idxGet, idxSet);
        }
    }else if(mov1){
        addToC(C, idx1, idx1, w, Csize, idxGet, idxSet);
        b[idx1] += w * x2;
    }else if(mov2){
        addToC(C, idx2, idx2, w, Csize, idxGet, idxSet);
        b[idx2] += w * x1;
    }
}


void B2BModel(){
    for(int net=0; net<numNets; net++){
        //find the net bounding box and the bounding cells
        int nPins = netCell[net].size();
        if(nPins < 2){
            continue;
        }
        double lx,hx,ly,hy;
        int lxp,hxp,lyp,hyp;
        int lxc,hxc,lyc,hyc;
        bool lxm,hxm,lym,hym;
        lxp = hxp = lyp = hyp = 0;
        lxc = hxc = lyc = hyc = netCell[net][0];
        if(lxc < numCells){
            lx = hx = varX[lxc];
            ly = hy = varY[lyc];
            lxm = hxm = lym = hym = true;
        }else{
            lx = hx = cellX[lxc] + netPinX[net][0];
            ly = hy = cellY[lyc] + netPinY[net][0];
            lxm = hxm = lym = hym = false;
        }
        for(int p=1; p<nPins; p++){
            int cell = netCell[net][p];
            double px, py;
            bool mov;
            if(cell < numCells){
                px = varX[cell];
                py = varY[cell];
                mov = true;
            }else{
                px = cellX[cell]+netPinX[net][p];
                py = cellY[cell]+netPinY[net][p];
                mov = false;
            }
            if(px <= lx && cell != hxc){
                lx = px;
                lxp = p;
                lxc = cell;
                lxm = mov;
            }else if(px >= hx && cell != lxc){
                hx = px;
                hxp = p;
                hxc = cell;
                hxm = mov;
            }
            if(py <= ly && cell != hyc){
                ly = py;
                lyp = p;
                lyc = cell;
                lym = mov;
            }else if(py >= hy && cell != lyc){
                hy = py;
                hyp = p;
                hyc = cell;
                hym = mov;
            }
        }

        if(lxc == hxc || lyc == hyc){
            continue;
            printlog(LOG_ERROR, "same bounding (%d) [%d,%d][%d,%d]", nPins, lxc, hxc, lyc, hyc);
            for(int i=0; i<nPins; i++){
                int cell = netCell[net][i];
                double px,py;
                bool movable;
                if(cell < numCells){
                    px = varX[cell];
                    py = varY[cell];
                    movable = true;
                }else{
                    px = cellX[cell]+netPinX[net][i];
                    py = cellY[cell]+netPinY[net][i];
                    movable = false;
                }
                if(movable){
                    printlog(LOG_ERROR, "+cell=%d:%d pin=(%lf,%lf)", cell, i, px, py);
                }else{
                    printlog(LOG_ERROR, "-cell=%d:%d pin=(%lf,%lf)", cell, i, px, py);
                }
            }
        }

        //enumerate B2B connections
        double w = 2.0 * netWeight[net] / (double) (nPins-1);
        double weightX = w / max(MinCellDist, hx-lx);
        double weightY = w / max(MinCellDist, hy-ly);
        B2BaddInC_b(net, Cx, bx, lxc, hxc, lx, hx, lxm, hxm, weightX, &Cxsize, idxGetX, idxSetX);
        B2BaddInC_b(net, Cy, by, lyc, hyc, ly, hy, lym, hym, weightY, &Cysize, idxGetY, idxSetY);
        for(int p=0; p<nPins; p++){
            int cell = netCell[net][p];
            double px, py;
            bool mov;
            if(cell < numCells){
                px = varX[cell];
                py = varY[cell];
                mov = true;
            }else{
                px = cellX[cell]+netPinX[net][p];
                py = cellY[cell]+netPinY[net][p];
                mov = false;
            }
            if(p != lxp && p != hxp){
                weightX = w / max(MinCellDist, px-lx);
                B2BaddInC_b(net, Cx, bx, cell, lxc, px, lx, mov, lxm, weightX, &Cxsize, idxGetX, idxSetX);
                weightX = w / max(MinCellDist, hx-px);
                B2BaddInC_b(net, Cx, bx, cell, hxc, px, hx, mov, hxm, weightX, &Cxsize, idxGetX, idxSetX);
            }
            if(p != lyp && p != hyp){
                weightY = w / max(MinCellDist, py-ly);
                B2BaddInC_b(net, Cy, by, cell, lyc, py, ly, mov, lym, weightY, &Cysize, idxGetY, idxSetY);
                weightY = w / max(MinCellDist, hy-py);
                B2BaddInC_b(net, Cy, by, cell, hyc, py, hy, mov, hym, weightY, &Cysize, idxGetY, idxSetY);
            }
        }
    }
}

void StarModel(){
    for(int net=0; net<numNets; net++){
        int nPins = netCell[net].size();
        if(nPins<2){
            continue;
        }
        double w = 1.0 * netWeight[net] / (double)nPins;
        int star = numCells+net;
        double starX = 0.0;
        double starY = 0.0;
        for(int i=0; i<nPins; i++){
            int cell = netCell[net][i];
            double x, y;
            bool mov;
            if(cell < numCells){
                x = varX[cell];
                y = varY[cell];
                mov = true;
            }else{
                x = cellX[cell] + netPinX[net][i];
                y = cellY[cell] + netPinY[net][i];
                mov = false;
            }
            starX += x;
            starY += y;
            B2BaddInC_b(net, Cx, bx, cell, star, x, 0.0, mov, true, w, &Cxsize, idxGetX, idxSetX);
            B2BaddInC_b(net, Cy, by, cell, star, y, 0.0, mov, true, w, &Cysize, idxGetY, idxSetY);
        }
        varX[star] = starX / (double)nPins;
        varY[star] = starY / (double)nPins;
    }
}

void CliqueModel(){
    for(int net=0; net<numNets; net++){
        int nPins = netCell[net].size();
        if(nPins < 2){
            continue;
        }
        double w = 2.0 * netWeight[net] / (double)nPins / (double)(nPins - 1);
        for(int i=0; i<nPins; i++){
            int cellI = netCell[net][i];
            double xi, yi, xj, yj;
            bool movi, movj;
            if(cellI < numCells){
                xi = varX[cellI];
                yi = varY[cellI];
                movi = true;
            }else{
                xi = cellX[cellI] + netPinX[net][i];
                yi = cellY[cellI] + netPinY[net][i];
                movi = false;
            }
            for(int j=i+1; j<nPins; j++){
                int cellJ = netCell[net][j];
                if(cellJ < numCells){
                    xj = varX[cellJ];
                    yj = varY[cellJ];
                    movj = true;
                }else{
                    xj = cellX[cellJ] + netPinX[net][j];
                    yj = cellY[cellJ] + netPinY[net][j];
                    movj = false;
                }
                B2BaddInC_b(net, Cx, bx, cellI, cellJ, xi, xj, movi, movj, w, &Cxsize, idxGetX, idxSetX);
                B2BaddInC_b(net, Cy, by, cellI, cellJ, yi, yj, movi, movj, w, &Cysize, idxGetY, idxSetY);
            }
        }
    }
}

void MSTModel(){
    for(int net=0; net<numNets; net++){
        int nPins = netCell[net].size();
        if(nPins<2){
            continue;
        }
        double w = 1.0 * netWeight[net] / (double)nPins;
        int star = netCell[net][0];
        double starX = 0.0;
        double starY = 0.0;
        for(int i=1; i<nPins; i++){
            int cell = netCell[net][i];
            double x, y;
            bool mov;
            if(cell < numCells){
                x = varX[cell];
                y = varY[cell];
                mov = true;
            }else{
                x = cellX[cell] + netPinX[net][i];
                y = cellY[cell] + netPinY[net][i];
                mov = false;
            }
            starX += x;
            starY += y;
            B2BaddInC_b(net, Cx, bx, cell, star, x, 0.0, mov, true, w, &Cxsize, idxGetX, idxSetX);
            B2BaddInC_b(net, Cy, by, cell, star, y, 0.0, mov, true, w, &Cysize, idxGetY, idxSetY);
        }
        varX[star] = starX / (double)nPins;
        varY[star] = starY / (double)nPins;
    }
}

void HybridModel(){
    for(int net=0; net<numNets; net++){
        int nPins = netCell[net].size();
        if(nPins < 2){
            continue;
        }else if(nPins == 2){
            double w = netWeight[net];
            int i = netCell[net][0];
            bool movi;
            double xi,yi;
            if(i < numCells){
                xi = varX[i];
                yi = varY[i];
                movi = true;
            }else{
                xi = cellX[i] + netPinX[net][0];
                yi = cellY[i] + netPinY[net][0];
                movi = false;
            }
            int j = netCell[net][1];
            bool movj;
            double xj,yj;
            if(j < numCells){
                xj = varX[j];
                yj = varY[j];
                movj = true;
            }else{
                xj = cellX[j] + netPinX[net][1];
                yj = cellY[j] + netPinY[net][1];
                movj = false;
            }
            B2BaddInC_b(net, Cx, bx, i, j, xi, xj, movi, movj, w, &Cxsize, idxGetX, idxSetX);
            B2BaddInC_b(net, Cy, by, i, j, yi, yj, movi, movj, w, &Cysize, idxGetY, idxSetY);
        }else if(nPins == 3){
            double w = netWeight[net] / 3.0;
            int i = netCell[net][0];
            bool movi;
            double xi,yi;
            if(i < numCells){
                xi = varX[i];
                yi = varY[i];
                movi = true;
            }else{
                xi = cellX[i] + netPinX[net][0];
                yi = cellY[i] + netPinY[net][0];
                movi = false;
            }
            int j = netCell[net][1];
            bool movj;
            double xj,yj;
            if(j < numCells){
                xj = varX[j];
                yj = varY[j];
                movj = true;
            }else{
                xj = cellX[j] + netPinX[net][1];
                yj = cellY[j] + netPinY[net][1];
                movj = false;
            }
            int k = netCell[net][2];
            bool movk;
            double xk,yk;
            if(k < numCells){
                xk = varX[k];
                yk = varY[k];
                movk = true;
            }else{
                xk = cellX[k] + netPinX[net][2];
                yk = cellY[k] + netPinY[net][2];
                movk = false;
            }
            B2BaddInC_b(net, Cx, bx, i, j, xi, xj, movi, movj, w, &Cxsize, idxGetX, idxSetX);
            B2BaddInC_b(net, Cy, by, i, j, yi, yj, movi, movj, w, &Cysize, idxGetY, idxSetY);
            B2BaddInC_b(net, Cx, bx, j, k, xj, xk, movj, movk, w, &Cxsize, idxGetX, idxSetX);
            B2BaddInC_b(net, Cy, by, j, k, yj, yk, movj, movk, w, &Cysize, idxGetY, idxSetY);
            B2BaddInC_b(net, Cx, bx, k, i, xk, xi, movk, movi, w, &Cxsize, idxGetX, idxSetX);
            B2BaddInC_b(net, Cy, by, k, i, yk, yi, movk, movi, w, &Cysize, idxGetY, idxSetY);

        }else{
            double w = 1.0 * netWeight[net] / (double)nPins;
            int star = numCells+net;
            double starX = 0.0;
            double starY = 0.0;
            for(int i=0; i<nPins; i++){
                int cell = netCell[net][i];
                double x, y;
                bool mov;
                if(cell < numCells){
                    x = varX[cell];
                    y = varY[cell];
                    mov = true;
                }else{
                    x = cellX[cell] + netPinX[net][i];
                    y = cellY[cell] + netPinY[net][i];
                    mov = false;
                }
                starX += x;
                starY += y;
                B2BaddInC_b(net, Cx, bx, cell, star, x, 0.0, mov, true, w, &Cxsize, idxGetX, idxSetX);
                B2BaddInC_b(net, Cy, by, cell, star, y, 0.0, mov, true, w, &Cysize, idxGetY, idxSetY);
            }
            varX[star] = starX / (double)nPins;
            varY[star] = starY / (double)nPins;
        }
    }
}

void UpdateBoxConstraintRect(){
    findCellRegionDist();
    //legalizeRegion();
    for(int i=0; i<numCells; i++){
        int f=cellFence[i];
        int r=cellFenceRect[i];
        lox[i]=fenceRectLX[f][r];
        hix[i]=fenceRectHX[f][r];
        loy[i]=fenceRectLY[f][r];
        hiy[i]=fenceRectHY[f][r];

        cellX[i]=max(lox[i], min(hix[i], cellX[i]));
        cellY[i]=max(loy[i], min(hiy[i], cellY[i]));
    }
}
void UpdateBoxConstraintBBox(){
    vector<double> lx(numFences, coreHX);
    vector<double> ly(numFences, coreHY);
    vector<double> hx(numFences, coreLX);
    vector<double> hy(numFences, coreLY);
    for(int f=0; f<numFences; f++){
        int nRects = fenceRectLX[f].size();
        for(int r=0; r<nRects; r++){
            lx[f] = min(lx[f], fenceRectLX[f][r]);
            ly[f] = min(ly[f], fenceRectLY[f][r]);
            hx[f] = max(hx[f], fenceRectHX[f][r]);
            hy[f] = max(hy[f], fenceRectHY[f][r]);
        }
    }
    for(int i=0; i<numCells; i++){
        lox[i]=lx[cellFence[i]];
        loy[i]=ly[cellFence[i]];
        hix[i]=hx[cellFence[i]];
        hiy[i]=hy[cellFence[i]];
        cellX[i]=max(lox[i], min(hix[i], cellX[i]));
        cellY[i]=max(loy[i], min(hiy[i], cellY[i]));
    }
}
void UpdateBoxConstraintRelaxedRect(double relax, double maxDisp){
    //relax: 0.0 = Sub-Fence 1.0 = Fence-BBox
    findCellRegionDist();

    vector<double> flx(numFences, coreHX);
    vector<double> fly(numFences, coreHY);
    vector<double> fhx(numFences, coreLX);
    vector<double> fhy(numFences, coreLY);
    for(int f=0; f<numFences; f++){
        int nRects = fenceRectLX[f].size();
        for(int r=0; r<nRects; r++){
            flx[f] = min(flx[f], fenceRectLX[f][r]);
            fly[f] = min(fly[f], fenceRectLY[f][r]);
            fhx[f] = max(fhx[f], fenceRectHX[f][r]);
            fhy[f] = max(fhy[f], fenceRectHY[f][r]);
        }
    }

    for(int i=0; i<numCells; i++){
        int f=cellFence[i];
        int r=cellFenceRect[i];
        double rlx = fenceRectLX[f][r];
        double rly = fenceRectLY[f][r];
        double rhx = fenceRectHX[f][r];
        double rhy = fenceRectHY[f][r];

        lox[i] = rlx + (flx[f] - rlx) * relax;
        hix[i] = rhx + (fhx[f] - rhx) * relax;
        loy[i] = rly + (fly[f] - rly) * relax;
        hiy[i] = rhy + (fhy[f] - rhy) * relax;

        if(maxDisp>=0){
            double dlx=cellX[i]-maxDisp;
            double dhx=cellX[i]+maxDisp;
            double dly=cellY[i]-maxDisp;
            double dhy=cellY[i]+maxDisp;
            lox[i]=max(dlx, min(dhx, lox[i]));
            hix[i]=max(dlx, min(dhx, hix[i]));
            loy[i]=max(dly, min(dhy, loy[i]));
            hiy[i]=max(dly, min(dhy, hiy[i]));
        }

        cellX[i]=max(lox[i], min(hix[i], cellX[i]));
        cellY[i]=max(loy[i], min(hiy[i], cellY[i]));
    }
}

void jacobiM_inverse(){
    for(int i=0; i<numMovables; i++){
        int idxx=idxGetX(i,i);
        if(idxx<0){
            Mx_inverse[i]=0;
        }else{
            Mx_inverse[i]=Cx[idxx].num>MinCellDist?1.0/Cx[idxx].num : 1.0/MinCellDist;
        }
        int idxy=idxGetY(i,i);
        if(idxy<0){
            My_inverse[i]=0;
        }else{
            My_inverse[i]=Cy[idxy].num>MinCellDist?1.0/Cy[idxy].num : 1.0/MinCellDist;
        }
    }
}

void pseudonetAddInC_b(double alpha){
    double weight;
    for(int i=0; i<numCells; i++){
        //weight = alpha / max(MinCellDist, cellDispX[i]);
        weight=alpha/max(MinCellDist, abs(lastVarX[i]-varX[i]));
        addToC(Cx, i, i, weight, &Cxsize, idxGetX, idxSetX);
        bx[i]+=(weight*varX[i]);

        //weight = alpha / max(MinCellDist, cellDispY[i]);
        weight=alpha/max(MinCellDist, abs(lastVarY[i]-varY[i]));
        addToC(Cy, i, i, weight, &Cysize, idxGetY, idxSetY);
        by[i]+=(weight*varY[i]);
    }
}

void lowerBound(
        double epsilon,
        double pseudoAlpha,
        int repeat,
        LBMode mode, NetModel model,
        double relax,
        double maxDisp){

    int CGi_max=500; //--The maximum iteration times of CG
    initCG(model);

    copy(cellX.begin(), cellX.begin()+numCells, varX.begin());
    copy(cellY.begin(), cellY.begin()+numCells, varY.begin());

    for(int r=0; r<repeat; r++){
        bool enableBox=false;
        resetCG();

        if(numFences == 1){
            enableBox = false;
        }else if(mode == LBModeFenceBBox){
            UpdateBoxConstraintRelaxedRect(1.0, maxDisp);
            enableBox=true;
        }else if(mode == LBModeFenceRect){
            UpdateBoxConstraintRelaxedRect(0.0, maxDisp);
            enableBox=true;
        }else if(mode == LBModeFenceRelax){
            UpdateBoxConstraintRelaxedRect(relax, maxDisp);
            enableBox = true;
        }else{
            //don't find box constraint
            enableBox=false;
        }

        if(model == NetModelClique){
            CliqueModel();
        }else if(model == NetModelStar){
            StarModel();
        }else if(model == NetModelB2B){
            B2BModel();
        }else if(model == NetModelHybrid){
            HybridModel();
        }else if(model == NetModelMST){
            MSTModel();
        }else{
            printlog(LOG_ERROR, "net model not handled");
        }


        if(pseudoAlpha>0.0){
            pseudonetAddInC_b(pseudoAlpha);
        }

        //sort(Cx.begin(), Cx.end(), SparseMatrix::compareRowCol);
        //sort(Cy.begin(), Cy.end(), SparseMatrix::compareRowCol);
        // Update the diagonal inverse matrix M_inverse after updating matrix C
        jacobiM_inverse();
        //jacobiCG(Cx, bx, varX, lox, hix, Mx_inverse, CGi_max, epsilon, numMovables, Cxsize, enableBox);
        //jacobiCG(Cy, by, varY, loy, hiy, My_inverse, CGi_max, epsilon, numMovables, Cysize, enableBox);
        jacobiCG(   Cx, bx, varX, lox, hix, Mx_inverse, CGi_max, epsilon, numMovables, Cxsize,
                    Cy, by, varY, loy, hiy, My_inverse, CGi_max, epsilon, numMovables, Cysize,
                    enableBox);
        //copy(varX.begin(), varX.begin()+numCells, cellX.begin());
        //copy(varY.begin(), varY.begin()+numCells, cellY.begin());
        //printlog(LOG_INFO, "after cg: %ld", (long)hpwl());
    }
    
    copy(varX.begin(), varX.begin()+numCells, cellX.begin());
    copy(varY.begin(), varY.begin()+numCells, cellY.begin());
    copy(varX.begin(), varX.begin()+numCells, lastVarX.begin());
    copy(varY.begin(), varY.begin()+numCells, lastVarY.begin());
}

