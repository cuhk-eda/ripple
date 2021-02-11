#ifndef _NEWINFLATION_H_
#define _NEWINFLATION_H_

/* For sorting tiles by routing congestion */
typedef struct densityTile{
    int x, y;
    double density;
}densityTile;


void createInflation();
void deleteCongestionTiles();
bool cellInflation(int isHorizontal, double restoreRatio);

//extern double inflationSlope; // For inflation, the slope*(d/s)-(slope-1), slope=1 by default

#endif

