#ifndef _CG_H_
#define _CG_H_

class SparseMatrix{
    public:
        int row;
        int col;
        double num;
        SparseMatrix() : row(-1), col(-1), num(0.0){
        }
        SparseMatrix(int r, int c, double v) : row(r), col(c), num(v){
        }
        SparseMatrix(int r, int c) : row(r), col(c), num(0.0){
        }
        static bool compareRowCol(const SparseMatrix &a, const SparseMatrix &b){
            if(a.row == b.row){
                return a.col < b.col;
            }
            return a.row < b.row;
        }
};
class JacobiCG_params{
    public:
        vector<SparseMatrix> *A;
        vector<double> *b;
        vector<double> *x;
        vector<double> *lo;
        vector<double> *hi;
        vector<double> *M_inverse;
        int i_max;
        double epsilon;
        int n;
        int Asize;
        bool box;
};
/*
   CG(Conjuate Gradient) solver using Jacobi preconditioned Ax=b
input:
A           : the sparse symmetric matrix
b, x        : the vector of Ax=b
M_inverse   : the inverse matrix of Jacobi preconditioned matrix M
i_max       : the maximum number of iterations in CG
epsilon     : error_tolerance<1
n           : the size of vector b, x (since the M is diagonal matrix, M_inverse is stored as vector)
Asize       : the non-zero elements in matrix A

*/
void jacobiCG(
        vector<SparseMatrix> &Ax, vector<double> &bx, vector<double> &xx,
        vector<double> &lox, vector<double> &hix, vector<double> &M_inversex,
        int i_maxx, double epsilonx, int nx, int Asizex,
        vector<SparseMatrix> &Ay, vector<double> &by, vector<double> &xy,
        vector<double> &loy, vector<double> &hiy, vector<double> &M_inversey,
        int i_maxy, double epsilony, int ny, int Asizey,
        bool box
);

void jacobiCG(
        vector<SparseMatrix> &A,
        vector<double> &b,
        vector<double> &x,
        vector<double> &lo,
        vector<double> &hi,
        vector<double> &M_inverse,
        int i_max, double epsilon, int n, int Asize, bool box);

#endif /*_CG_H_*/
