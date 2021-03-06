#include "utilities.h"
//#include "gperftools/profiler.h"


// apply EDPP 

void edpp_screen_batch(int *discard_beta, int n, int p, double rhs2, double *Xtr, double *lhs2,
		       double c, double c1, double *m, double alpha, vector<int> &col_idx);/* {
  int j;
  for(j = 0; j < p; j ++) {
    if(fabs(c1 * Xtr[j] + c / 2 * lhs2[j]) < n - c / 2 * rhs2) {
      discard_beta[j] = 1;
    } else {
      discard_beta[j] = 0;
    }
  }
}
*/

void sedpp_recal(XPtr<BigMatrix> xpMat, double *r, double sumResid, double *lhs2, double *Xty,
                 double *Xtr, double *yhat, double ytyhat, double yhat_norm2,
                 int *row_idx, vector<int>& col_idx, NumericVector& center,
		 NumericVector& scale, int n, int p);

void update_zj(vector<double> &z,
               int *reject, int *reject_old,
               XPtr<BigMatrix> xpMat, int *row_idx,vector<int> &col_idx,
               NumericVector &center, NumericVector &scale, 
               double sumResid, double *r, double *m, int n, int p);

int check_strong_set(int *ever_active, int *strong_set, vector<double> &z, XPtr<BigMatrix> xpMat, 
                     int *row_idx, vector<int> &col_idx,
                     NumericVector &center, NumericVector &scale, double *a,
                     double lambda, double sumResid, double alpha, 
                     double *r, double *m, int n, int p);
  
// check edpp set
/*
int check_edpp_set(int *ever_active, int *discard_beta, vector<double> &z, 
                   XPtr<BigMatrix> xpMat, int *row_idx, vector<int> &col_idx,
                   NumericVector &center, NumericVector &scale, double *a,
                   double lambda, double sumResid, double alpha, 
                   double *r, double *m, int n, int p); {
  MatrixAccessor<double> xAcc(*xpMat);
  double *xCol, sum, l1, l2;
  int j, jj, violations = 0;
  
#pragma omp parallel for private(j, sum, l1, l2) reduction(+:violations) schedule(static) 
  for (j = 0; j < p; j++) {
    if (ever_active[j] == 0 && discard_beta[j] == 0) {
      jj = col_idx[j];
      xCol = xAcc[jj];
      sum = 0.0;
      for (int i=0; i < n; i++) {
        sum = sum + xCol[row_idx[i]] * r[i];
      }
      z[j] = (sum - center[jj] * sumResid) / (scale[jj] * n);
      l1 = lambda * m[jj] * alpha;
      l2 = lambda * m[jj] * (1 - alpha);
      if (fabs(z[j] - a[j] * l2) > l1) {
        ever_active[j] = 1;
        violations++;
      }
    }
  }
  return violations;
}*/

int check_edpp_rest_set(int *ever_active, int *strong_set, int *discard_beta, vector<double> &z,
			XPtr<BigMatrix> xpMat, int *row_idx, vector<int> &col_idx,
			NumericVector &center, NumericVector &scale, double *a, double lambda,
			double sumResid, double alpha, double *r, double *m, int n, int p) {
  
  MatrixAccessor<double> xAcc(*xpMat);
  double *xCol, sum, l1, l2;
  int j, jj, violations = 0;
  #pragma omp parallel for private(j, sum, l1, l2) reduction(+:violations) schedule(static) 
  for (j = 0; j < p; j++) {
    if (strong_set[j] == 0 && discard_beta[j] == 0) {
      jj = col_idx[j];
      xCol = xAcc[jj];
      sum = 0.0;
      for (int i=0; i < n; i++) {
        sum = sum + xCol[row_idx[i]] * r[i];
      }
      z[j] = (sum - center[jj] * sumResid) / (scale[jj] * n);
      
      l1 = lambda * m[jj] * alpha;
      l2 = lambda * m[jj] * (1 - alpha);
      if (fabs(z[j] - a[j] * l2) > l1) {
        ever_active[j] = strong_set[j] = 1;
        violations++;
      }
    }
  }
  return violations;
}

// Coordinate descent for gaussian models
RcppExport SEXP cdfit_gaussian_edpp_batch_hsr(SEXP X_, SEXP y_, SEXP row_idx_, SEXP lambda_, SEXP nlambda_,
					      SEXP lam_scale_, SEXP lambda_min_, SEXP alpha_, SEXP user_,
					      SEXP eps_, SEXP max_iter_, SEXP multiplier_, SEXP dfmax_,
					      SEXP ncore_, SEXP recal_thresh_, SEXP verbose_) {
  //ProfilerStart("SEDPP-Batch-SSR.out");
  XPtr<BigMatrix> xMat(X_);
  double *y = REAL(y_);
  int *row_idx = INTEGER(row_idx_);
  double lambda_min = REAL(lambda_min_)[0];
  double alpha = REAL(alpha_)[0];
  int n = Rf_length(row_idx_); // number of observations used for fitting model
  int p = xMat->ncol();
  int lam_scale = INTEGER(lam_scale_)[0];
  int L = INTEGER(nlambda_)[0];
  int user = INTEGER(user_)[0];
  int verbose = INTEGER(verbose_)[0];
  double eps = REAL(eps_)[0];
  int max_iter = INTEGER(max_iter_)[0];
  double *m = REAL(multiplier_);
  int dfmax = INTEGER(dfmax_)[0];
  double recal_thresh = REAL(recal_thresh_)[0];
  
  NumericVector lambda(L);
  NumericVector center(p);
  NumericVector scale(p);
  int p_keep = 0;
  int *p_keep_ptr = &p_keep;
  
  vector<int> col_idx;
  vector<double> z;
  double lambda_max = 0.0;
  double *lambda_max_ptr = &lambda_max;
  int xmax_idx = 0;
  int *xmax_ptr = &xmax_idx;
  
  // set up omp
  int useCores = INTEGER(ncore_)[0];
#ifdef BIGLASSO_OMP_H_
  int haveCores = omp_get_num_procs();
  if(useCores < 1) {
    useCores = haveCores;
  }
  omp_set_dynamic(0);
  omp_set_num_threads(useCores);
#endif

  if (verbose) {
    char buff1[100];
    time_t now1 = time (0);
    strftime (buff1, 100, "%Y-%m-%d %H:%M:%S.000", localtime (&now1));
    Rprintf("\nPreprocessing start: %s\n", buff1);
  }
  
  standardize_and_get_residual(center, scale, p_keep_ptr, col_idx, z, 
                               lambda_max_ptr, xmax_ptr, xMat, 
                               y, row_idx, lambda_min, alpha, n, p);
  p = p_keep; // set p = p_keep, only loop over columns whose scale > 1e-6

  if (verbose) {
    char buff1[100];
    time_t now1 = time (0);
    strftime (buff1, 100, "%Y-%m-%d %H:%M:%S.000", localtime (&now1));
    Rprintf("Preprocessing end: %s\n", buff1);
    Rprintf("\n-----------------------------------------------\n");
  }

  
  // Objects to be returned to R
  arma::sp_mat beta = arma::sp_mat(p, L); //Beta
  double *a = Calloc(p, double); //Beta from previous iteration
  NumericVector loss(L);
  IntegerVector iter(L);
  IntegerVector n_reject(L);
  IntegerVector n_safe_reject(L);
  
  double l1, l2, shift;
  double max_update, update, thresh; // for convergence check
  int i, j, jj, l, violations, lstart; //temp index
  int *ever_active = Calloc(p, int); // ever-active set
  int *strong_set = Calloc(p, int); // strong set
  int *discard_beta = Calloc(p, int); // index set of discarded features;
  int *discard_old = Calloc(p, int);
  double *r = Calloc(n, double);
  for (i = 0; i < n; i++) r[i] = y[i];
  double sumResid = sum(r, n);
  loss[0] = gLoss(r, n);
  thresh = eps * loss[0] / n;
  
  // EDPP
  double c;
  double *lhs2 = Calloc(p, double); //Second term on LHS
  double rhs2; // second term on RHS
  double *Xty = Calloc(p, double);
  double *Xtr = Calloc(p, double); // Xtr at previous recalculation of EDPP
  for(j = 0; j < p; j++) {
    Xty[j] = z[j] * n;
    Xtr[j] = Xty[j];
  }
  double *yhat = Calloc(n, double); // yhat at previous recalculation of EDPP
  double yhat_norm2;
  double ytyhat;
  double y_norm2 = 0; // ||y||^2
  for(i = 0; i < n; i++) y_norm2 += y[i] * y[i];
  bool SEDPP = false; // Whether using SEDPP or BEDPP
  double cutoff = 0; // cutoff for strong rule
  int gain = 0; // gain from recalculating SEDPP
  
  // lambda, equally spaced on log scale
  if (user == 0) {
    if (lam_scale) {
      // set up lambda, equally spaced on log scale
      double log_lambda_max = log(lambda_max);
      double log_lambda_min = log(lambda_min*lambda_max);
      
      double delta = (log_lambda_max - log_lambda_min) / (L-1);
      for (l = 0; l < L; l++) {
        lambda[l] = exp(log_lambda_max - l * delta);
      }
    } else { // equally spaced on linear scale
      double delta = (lambda_max - lambda_min*lambda_max) / (L-1);
      for (l = 0; l < L; l++) {
        lambda[l] = lambda_max - l * delta;
      }
    }
    lstart = 1;
    n_safe_reject[0] = p;
    n_reject[0] = p;
  } else {
    lstart = 0;
    lambda = Rcpp::as<NumericVector>(lambda_);
  } 
  
  int l_prev = lstart; // lambda index at previous recalculation of EDPP
  // compute v1 for lambda_max
  double xty = sign(crossprod_bm(xMat, y, row_idx, center[xmax_idx], scale[xmax_idx], n, xmax_idx));
  
  
  
  // Path
  for (l = lstart; l < L; l++) {
    if(verbose) {
      // output time
      char buff[100];
      time_t now = time (0);
      strftime (buff, 100, "%Y-%m-%d %H:%M:%S.000", localtime (&now));
      Rprintf("Lambda %d. Now time: %s\n", l, buff);
    }
    c = (lambda[l_prev] - lambda[l]) / lambda[l_prev] / lambda[l];
    if(l != lstart) {
      int nv = 0;
      for (int j=0; j<p; j++) {
        if (a[j] != 0) nv++;
      }
      if (nv > dfmax) {
        for (int ll=l; ll<L; ll++) iter[ll] = NA_INTEGER;
	Free(ever_active); Free(r); Free(a); Free(discard_beta); Free(lhs2); Free(Xty); Free(Xtr); Free(yhat); Free(discard_old); Free(strong_set);
        return List::create(beta, center, scale, lambda, loss, iter,  n_reject, n_safe_reject, Rcpp::wrap(col_idx));
      }
      if(gain - n_safe_reject[l - 1] * (l - l_prev) > recal_thresh * p && l != L - 1) { // Recalculate SEDPP if not discarding enough
	if(verbose) {
          // output time
          char buff[100];
          time_t now = time (0);
          strftime (buff, 100, "%Y-%m-%d %H:%M:%S.000", localtime (&now));
          Rprintf("Start recalculating SEDPP rule at lambda %d. Now time: %s\n", l, buff);
        }
        SEDPP = true;
	l_prev = l-1;
	c = (lambda[l_prev] - lambda[l]) / lambda[l_prev] / lambda[l];
        yhat_norm2 = 0;
        ytyhat = 0;
        for(i = 0; i < n; i ++){
          yhat[i] = y[i] - r[i];
          yhat_norm2 += yhat[i] * yhat[i];
          ytyhat += y[i] * yhat[i];
        }
	sedpp_recal(xMat, r, sumResid, lhs2, Xty, Xtr, yhat, ytyhat, yhat_norm2, row_idx, col_idx,
		    center, scale, n, p);
        rhs2 = sqrt(n * (y_norm2 - ytyhat * ytyhat / yhat_norm2));
	for(j = 0; j < p; j++) z[j] = Xtr[j] / n;
	if(verbose) {
          // output time
          char buff[100];
          time_t now = time (0);
          strftime (buff, 100, "%Y-%m-%d %H:%M:%S.000", localtime (&now));
          Rprintf("Done recalculating SEDPP rule at lambda %d. Now time: %s\n", l, buff);
        }

        // Reapply SEDPP
        edpp_screen_batch(discard_beta, n, p, rhs2, Xtr, lhs2, c,
                          1 / lambda[l_prev], m, alpha, col_idx);
        n_safe_reject[l] = sum(discard_beta, p);
	gain = n_safe_reject[l];
      } else {
	// Apply EDPP to discard features
	if(SEDPP) { // Apply SEDPP check
	  edpp_screen_batch(discard_beta, n, p, rhs2, Xtr, lhs2, c,
			    1 / lambda[l_prev], m, alpha, col_idx);
	} else { // Apply BEDPP check
	  edpp_screen_batch(discard_beta, n, p, rhs2, Xtr, lhs2, c,
			    (1 / lambda[l_prev] + 1 / lambda[l]) / 2, m, alpha, col_idx);
	}
	n_safe_reject[l] = sum(discard_beta, p);
	gain += n_safe_reject[l];
      }
      
    } else { //First check with lambda max
      if(verbose) {
        // output time
        char buff[100];
        time_t now = time (0);
        strftime (buff, 100, "%Y-%m-%d %H:%M:%S.000", localtime (&now));
        Rprintf("Start calculating BEDPP rule. Now time: %s\n", buff);
      }
      double xjtx;
      for(j = 0; j < p; j ++) {
        jj = col_idx[j];
        xjtx = crossprod_bm_Xj_Xk(xMat, row_idx, center, scale, n, jj, xmax_idx);
        lhs2[j] = -xty * lambda[l] * xjtx;
      }
      rhs2 = sqrt(n * y_norm2 - pow(n * lambda[l], 2));
      if(verbose) {
        // output time
        char buff[100];
        time_t now = time (0);
        strftime (buff, 100, "%Y-%m-%d %H:%M:%S.000", localtime (&now));
        Rprintf("Done calculating BEDPP rule. Now time: %s\n", buff);
      }
      edpp_screen_batch(discard_beta, n, p, rhs2, Xtr, lhs2, c,
                        1 / lambda[l_prev], m, alpha, col_idx);
      n_safe_reject[l] = sum(discard_beta, p);
      gain = n_safe_reject[l];
    }
    
    // strong set
    //update_zj(z, discard_beta, discard_old, xMat, row_idx, col_idx, center, scale, 
    //sumResid, r, m, n, p);
    if(l != lstart) cutoff = 2 * lambda[l] - lambda[l-1];
    for(j = 0; j < p; j++) {
      if(discard_beta[j]) continue;
      if(fabs(z[j]) > cutoff * alpha * m[col_idx[j]]) {
	strong_set[j] = 1;
      } else {
	strong_set[j] = 0;
      }
    }
    n_reject[l] = p - sum(strong_set, p);
    for(j = 0; j < p; j++) discard_old[j] = discard_beta[j];
    
    while(iter[l] < max_iter) {
      while (iter[l] < max_iter) {
	while (iter[l] < max_iter) {
	  iter[l]++;
        
	  max_update = 0.0;
	  for (j = 0; j < p; j++) {
	    if (ever_active[j]) {
	      jj = col_idx[j];
	      z[j] = crossprod_resid(xMat, r, sumResid, row_idx, center[jj], scale[jj], n, jj) / n + a[j];
	      l1 = lambda[l] * m[jj] * alpha;
	      l2 = lambda[l] * m[jj] * (1-alpha);
	      beta(j, l) = lasso(z[j], l1, l2, 1);
	      
	      shift = beta(j, l) - a[j];
	      if (shift != 0) {
		// compute objective update for checking convergence
		//update =  z[j] * shift - 0.5 * (1 + l2) * (pow(beta(j, l+1), 2) - pow(a[j], 2)) - l1 * (fabs(beta(j, l+1)) -  fabs(a[j]));
		update = pow(beta(j, l) - a[j], 2);
		if (update > max_update) {
		  max_update = update;
		}
		update_resid(xMat, r, shift, row_idx, center[jj], scale[jj], n, jj);
		sumResid = sum(r, n); //update sum of residual
		a[j] = beta(j, l); //update a
	      }
	      // update ever active sets
	      if (beta(j, l) != 0) {
		ever_active[j] = 1;
	      } 
	    }
	  }
	  // Check for convergence
	  if (max_update < thresh) break;
	}
	violations = check_strong_set(ever_active, strong_set, z, xMat, row_idx, col_idx, center, scale, a, lambda[l], sumResid, alpha, r, m, n, p); 
        if (violations==0) break;
      }	
	// Scan for violations in edpp set
      violations = check_edpp_rest_set(ever_active, strong_set, discard_beta, z, xMat, row_idx, col_idx, center, scale, a, lambda[l], sumResid, alpha, r, m, n, p); 
      if (violations == 0) {
	loss[l] = gLoss(r, n);
	break;
      }
      
    }
  }
  
  Free(ever_active); Free(r); Free(a); Free(discard_beta); Free(lhs2); Free(Xty); Free(Xtr); Free(yhat); Free(discard_old); Free(strong_set);
  //ProfilerStop();
  return List::create(beta, center, scale, lambda, loss, iter, n_reject, n_safe_reject, Rcpp::wrap(col_idx));
}
