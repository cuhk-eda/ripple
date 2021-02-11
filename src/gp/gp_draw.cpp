#include "gp_draw.h"

#include "../db/db.h"
#include "../vi/vi.h"
#include "gp_data.h"
#include "gp_global.h"

void drawcell(const char* prefix, int iter)
{
    gp_copy_out();
    vi::Visualizer vis;
    vis.setWindow(3000, 0);
    vis.setViewport(database.dieLX, database.dieLY, database.dieHX, database.dieHY);
    database.draw(&vis);

    char filenamestr[256];
    if (iter < 0) {
        sprintf(filenamestr, "%s.png", prefix);
    } else {
        sprintf(filenamestr, "%s%d.png", prefix, iter);
    }
    vis.save(string(filenamestr));
}
