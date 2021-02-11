#include "gp_global.h"
#include <pthread.h>
#include "gp_cg.h"
#include "gp_setting.h"

using namespace gp;

void *jacobiCGThread(void *params);

void vectorXEqualYPlusATimesD(vector<double> &x, vector<double> &y, double a, vector<double> &d, int n){
    for(int i=0; i<n; i++){
        x[i]=y[i]+d[i]*a;
    }
}
void vectorXEqualYMinusATimesD(vector<double> &x, vector<double> &y, double a, vector<double> &d, int n){
    for(int i=0; i<n; i++){
        x[i]=y[i]-d[i]*a;
    }
}
void vectorPlus(vector<double> &x1, vector<double> &x2, vector<double> &x, int n){    
    for(int i=0; i<n; ++i){
        x[i]=x1[i]+x2[i];
    }
}
void vectorSubtract(vector<double> &x1, vector<double> &x2, vector<double> &x, int n){
    for(int i=0; i<n; ++i){
        x[i]=x1[i]-x2[i];
    }
}
void vectorMultiply(vector<double> &x1, vector<double> &x2, double &result, int n){
    result=0;
    for(int i=0; i<n; ++i){
        result += x1[i]*x2[i];
    }
}
void vectorScalar(vector<double> &x1, double num, vector<double> &x, int n){
    for(int i=0; i<n; ++i){
        x[i]=x1[i]*num;
    }
}
void diagMVMultiply(vector<double> &x1, vector<double> &x2, vector<double> &x, int n){
    for(int i=0; i<n; ++i){
        x[i]=x1[i]*x2[i];
    }
}

void sparseMVMultiply(vector<SparseMatrix> &A, vector<double> &x1, vector<double> &x, int n, int Asize){
    for(int i=0; i<n; ++i){
        x[i] = 0.0;
    }
    int Arow, Acol;
    for(int i=0; i<Asize; i++){
        Arow = A[i].row;
        Acol = A[i].col;
        x[Arow] += A[i].num * x1[Acol];
        if(Arow != Acol){
            x[Acol] += A[i].num * x1[Arow];
        }
    }
}

double normVector(vector<double> &x1, int n){
   double res=0;
   for(int i=0;i<n;i++){
    res = x1[i]*x1[i];
   } 
   res = sqrt(res);

   return res;
}

//copy x1 value to x2
void copyVector(vector<double> &x1, vector<double> &x2, int n){
    for(int i=0;i<n;i++){
    x2[i] = x1[i];
    }
}

void *jacobiCGThread(void *params){
    JacobiCG_params *p = (JacobiCG_params*)params;
    jacobiCG(*(p->A), *(p->b), *(p->x), *(p->lo), *(p->hi), *(p->M_inverse), p->i_max, p->epsilon, p->n, p->Asize, p->box);
    return NULL;
}

void adjustD(vector<double> &od, vector<double> &d, vector<double> &x, vector<double> &lx, vector<double> &hx, double a, int n){
    for(int i=0; i<n; i++){
        double nx=x[i]+d[i]*a;
        if(nx<lx[i]){
            od[i]=(lx[i]-x[i])/a;
        }else if(nx>hx[i]){
            od[i]=(hx[i]-x[i])/a;
        }else{
            od[i]=d[i];
        }
    }
}
double vectorLength(vector<double> &d, int n){
    double len=0.0;
    for(int i=0; i<n; i++){
        len+=d[i]*d[i];
    }
    return sqrt(len);
}

/*----------------------------------------
    Jacobi preconditioned CG solver
-----------------------------------------*/

void jacobiCG(
        vector<SparseMatrix> &A,
        vector<double> &b,
        vector<double> &x,
        vector<double> &lo,
        vector<double> &hi,
        vector<double> &M_inverse,
        int i_max, double epsilon, int n, int Asize, bool box){
    double alpha, beta, delta_new, delta_old, delta0, temp_num, error_tolerant;
    vector<double> r(n);
    vector<double> d(n);
    vector<double> q(n);
    vector<double> s(n);
    vector<double> temp(n);

    vector<double> od(n);

    sparseMVMultiply(A, x, temp, n, Asize);                 // r = b-Ax
    vectorSubtract(b, temp, r, n);
    diagMVMultiply(M_inverse, r, d, n);                     // d = M(-1)r
    vectorMultiply(r, d, delta_new, n);                    // delta_new = r(T)d
    delta0=delta_new;                                       // delta0 = delta_new
    
    error_tolerant=epsilon*delta0;
    
    for(int i=0; i<i_max && delta_new > error_tolerant; i++){
        sparseMVMultiply(A, d, q, n, Asize);                //q = Ad
        vectorMultiply(d, q, temp_num, n);
        alpha=delta_new/temp_num;                           //alpha = delta_new/(d(T)q)
        //adjust d here
        if(box){
            adjustD(od, d, x, lo, hi, alpha, n);
            vectorXEqualYPlusATimesD(x, x, alpha, od, n);        //x = x+alpha*d
        }else{
            vectorXEqualYPlusATimesD(x, x, alpha, d, n);        //x = x+alpha*d
        }
        if(i%50==0){
            sparseMVMultiply(A, x, temp, n, Asize);
            vectorSubtract(b, temp, r, n);
        }else{
            vectorXEqualYMinusATimesD(r, r, alpha, q, n);
        }
        diagMVMultiply(M_inverse, r, s, n);                 //s = M(-1)r
        delta_old=delta_new;                                //delta_old = delta_new
        vectorMultiply(r, s, delta_new, n);                //delta_new = r(T)s
        beta=delta_new/delta_old;                           //beta = delta_new/delta_old
        vectorXEqualYPlusATimesD(d, s, beta, d, n);         //d = s+beta*d
    }
}

void jacobiCG(
        vector<SparseMatrix> &Ax, vector<double> &bx, vector<double> &xx,
        vector<double> &lox, vector<double> &hix, vector<double> &M_inversex,
        int i_maxx, double epsilonx, int nx, int Asizex,
        vector<SparseMatrix> &Ay, vector<double> &by, vector<double> &xy,
        vector<double> &loy, vector<double> &hiy, vector<double> &M_inversey,
        int i_maxy, double epsilony, int ny, int Asizey,
        bool box){
    if(GPModule::NumThreads > 1){
    //if(gpSetting.nThreads > 1){
        pthread_t threads[2];
        JacobiCG_params params[2];
        int rets[2];

        params[0].A         = &Ax;
        params[0].b         = &bx;
        params[0].x         = &xx;
        params[0].lo        = &lox;
        params[0].hi        = &hix;
        params[0].M_inverse = &M_inversex;
        params[0].i_max     = i_maxx;
        params[0].epsilon   = epsilonx;
        params[0].n         = nx;
        params[0].Asize     = Asizex;
        params[0].box       = box;

        params[1].A         = &Ay;
        params[1].b         = &by;
        params[1].x         = &xy;
        params[1].lo        = &loy;
        params[1].hi        = &hiy;
        params[1].M_inverse = &M_inversey;
        params[1].i_max     = i_maxy;
        params[1].epsilon   = epsilony;
        params[1].n         = ny;
        params[1].Asize     = Asizey;
        params[1].box       = box;

        rets[0]=pthread_create(threads+0, NULL, jacobiCGThread, (void*)(params+0));
        rets[1]=pthread_create(threads+1, NULL, jacobiCGThread, (void*)(params+1));
        
        rets[0]=pthread_join(threads[0], NULL);
        rets[1]=pthread_join(threads[1], NULL);

        if(rets[0]!=0 || rets[1]!=0){
            printlog(LOG_ERROR, "solver returned: %d,%d", rets[0], rets[1]);
        }
    }else{
        jacobiCG(Ax, bx, xx, lox, hix, M_inversex, i_maxx, epsilonx, nx, Asizex, box);
        jacobiCG(Ay, by, xy, loy, hiy, M_inversey, i_maxy, epsilony, ny, Asizey, box);
    }
}

