#ifndef _DB_SITE_H_
#define _DB_SITE_H_

namespace db {
class Site {
private:
    string _name = "";
    string _siteClassName = "";
    int _width = 0;
    int _height = 0;

public:
    Site(const string& name, const string& siteClassName, int width, int height)
        : _name(name), _siteClassName(siteClassName), _width(width), _height(height) {}

    const string& name() const { return _name; }
    const string& siteClassName() const { return _siteClassName; }
    int width() const { return _width; }
    int height() const { return _height; }

    void name(const string& value) { _name = value; }
    void siteClassName(const string& value) { _siteClassName = value; }
    void width(const int value) { _width = value; }
    void height(const int value) { _height = value; }
};
}  // namespace db
#endif