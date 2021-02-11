#ifndef _DB_PLACE_H_
#define _DB_PLACE_H_

namespace db{
    class CellPlacement{
        private:
            int _x;
            int _y;
            bool _flipX;
            bool _flipY;
        public:
            const static int Unplaced = INT_MIN;
            CellPlacement(){
                _x = _y = INT_MIN;
                _flipX = _flipY = false;
            }
            CellPlacement(int x, int y, bool fx, bool fy){
                _x = x;
                _y = y;
                _flipX = fx;
                _flipY = fy;
            }
            int x() const { return _x; }
            int y() const { return _y; }
            bool flipX() const { return _flipX; }
            bool flipY() const { return _flipY; }
    };

    class Placement{
        private:
            std::unordered_map<Cell*,CellPlacement> _placement;
        public:
            Placement(){
                _placement.clear();
            }
            void place(Cell *cell){
                _placement[cell] = CellPlacement();
            }
            void place(Cell *cell, int x, int y){
                bool fx = _placement[cell].flipX();
                bool fy = _placement[cell].flipY();
                _placement[cell] = CellPlacement(x, y, fx, fy);
            }
            void place(Cell *cell, int x, int y, bool fx, bool fy){
                _placement[cell] = CellPlacement(x, y, fx, fy);
            }
            void clear(){
                _placement.clear();
            }
            int x(Cell *cell) const {
                return _placement.at(cell).x();
                //return _placement[cell].x();
            }
            int y(Cell *cell) {
                return _placement[cell].y();
            }
            int flipX(Cell *cell) {
                return _placement[cell].flipX();
            }
            int flipY(Cell *cell) {
                return _placement[cell].flipY();
            }
            bool placed(Cell *cell) {
                return (x(cell) != INT_MIN) && (y(cell) != INT_MIN);
            }
    };
}

#endif

