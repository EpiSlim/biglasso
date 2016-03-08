#include <math.h>
#include <string.h>
#include "Rinternals.h"
#include "R_ext/Rdynload.h"
#include <R.h>
#include <R_ext/Applic.h>

SEXP cdfit_gaussian(SEXP X_, SEXP y_, SEXP row_idx_, SEXP center_, SEXP scale_, 
                    SEXP lambda, SEXP eps_, SEXP max_iter_, 
                    SEXP multiplier, SEXP alpha_, SEXP dfmax_, SEXP user_); 
SEXP cdfit_binomial(SEXP X_, SEXP y_, SEXP row_idx_, SEXP center_, SEXP scale_,
                    SEXP lambda, SEXP eps_, SEXP max_iter_, 
                    SEXP multiplier, SEXP alpha_, SEXP dfmax_, SEXP user_, 
                    SEXP warn_);
SEXP standardize(SEXP X_);
SEXP maxprod(SEXP X_, SEXP y_, SEXP v_, SEXP m_);
SEXP maxprod_bm(SEXP X_, SEXP y_, SEXP row_idx_, SEXP center_, SEXP scale, SEXP v_, SEXP m_);

// Cross product of y with jth column of X
double crossprod(double *X, double *y, int n, int j) {
  int nn = n*j;
  double val=0;
  for (int i=0;i<n;i++) val += X[nn+i]*y[i];
  return(val);
}

double sum(double *x, int n) {
  double val=0;
  for (int i=0;i<n;i++) val += x[i];
  return(val);
}

int checkConvergence(double *beta, double *beta_old, double eps, int l, int J) {
  int converged = 1;
  for (int j=0; j<J; j++) {
    if (fabs((beta[l*J+j]-beta_old[j])/beta_old[j]) > eps) {
      converged = 0;
      break;
    }
  }
  return(converged);
}

double lasso(double z, double l1, double l2, double v) {
  double s=0;
  if (z > 0) s = 1;
  else if (z < 0) s = -1;
  if (fabs(z) <= l1) return(0);
  else return(s*(fabs(z)-l1)/(v*(1+l2)));
}

static R_CallMethodDef callMethods[] = {
  {"cdfit_gaussian", (DL_FUNC) &cdfit_gaussian, 12},
  {"cdfit_binomial", (DL_FUNC) &cdfit_binomial, 13},
  {"maxprod_bm", (DL_FUNC) &maxprod_bm, 7},
  {NULL, NULL, 0}
};

void R_init_ncvreg(DllInfo *info) {
  R_registerRoutines(info, NULL, callMethods, NULL, NULL);
}

// NOT USED
// Weighted cross product of y with jth column of x
// double wcrossprod(double *X, double *y, double *w, int n, int j) {
//  int nn = n*j;
//  double val=0;
//  for (int i=0;i<n;i++) val += X[nn+i]*y[i]*w[i];
//  return(val);
// }

// Weighted sum of squares of jth column of X
// double wsqsum(double *X, double *w, int n, int j) {
//   int nn = n*j;
//   double val=0;
//   for (int i=0;i<n;i++) val += w[i] * pow(X[nn+i], 2);
//   return(val);
// }

// Sum of squares of jth column of X
// double sqsum(double *X, int n, int j) {
//   int nn = n*j;
//   double val=0;
//   for (int i=0;i<n;i++) val += pow(X[nn+i], 2);
//   return(val);
// }

  