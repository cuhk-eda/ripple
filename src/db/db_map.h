#ifndef _DB_MAP_H_
#define _DB_MAP_H_

namespace db {
class SiteMap : virtual public Drawable {
private:
    unsigned nx = 0;
    unsigned ny = 0;
    unsigned char** sites = nullptr;
    unsigned char** regions = nullptr;

public:
    static const char SiteBlocked = 1;          //  nothing can be placed in the site
    static const char SiteM2Blocked = 2;        //  any part of it is blocked by M2 metal
    static const char SiteM3Blocked = 4;        //  any part of it is blocked by M3 metal
    static const char SiteM2BlockedIOPin = 8;   //  any part of it is blocked by M2 metal
    int siteL, siteR;
    int siteB, siteT;
    int siteStepX, siteStepY;
    int siteNX, siteNY;

    unsigned long long nSites = 0;
    unsigned long long nPlaceable = 0;
    vector<long long> nRegionSites;

    void initSiteMap(unsigned nx, unsigned ny);

    void setSiteMap(const unsigned x, const unsigned y, const unsigned char property) { setBit(sites[x][y], property); }
    inline void unsetSiteMap(const unsigned x, const unsigned y, const unsigned char property) {
        unsetBit(sites[x][y], property);
    }
    inline bool getSiteMap(const unsigned x, const unsigned y, const unsigned char property) const {
        return (getBit(sites[x][y], property) == property);
    }
    unsigned char getSiteMap(int x, int y) const { return sites[x][y]; }

    void getSiteBound(int x, int y, int& lx, int& ly, int& hx, int& hy) const;

    void blockRegion(const unsigned x, const unsigned y);
    inline void setRegion(const unsigned x, const unsigned y, unsigned char region) { regions[x][y] = region; }
    inline unsigned char getRegion(const unsigned x, const unsigned y) const { return regions[x][y]; }

    void setSites(const int lx,
                  const int ly,
                  const int hx,
                  const int hy,
                  const unsigned char property,
                  const bool isContained = false);

    void unsetSites(int lx, int ly, int hx, int hy, unsigned char property);
    void blockRegion(int lx, int ly, int hx, int hy);
    void setRegion(int lx, int ly, int hx, int hy, unsigned char region);

    int globalX() const;
    int globalY() const;
    int localX() const;
    int localY() const;
    int boundL() const;
    int boundR() const;
    int boundB() const;
    int boundT() const;

    Drawable* parent() const;
    void draw(Visualizer* v) const;
};
}  // namespace db

#endif
