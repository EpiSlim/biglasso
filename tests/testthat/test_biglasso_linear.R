
library(testthat)
library(biglasso)
library(ncvreg)
library(glmnet)

context("Testing linear regression:")

test_that("Test against OLS:", {
  ## test against OLS
  set.seed(1234)
  n <- 100
  p <- 10
  eps <- 1e-10
  tolerance <- 1e-6
  X <- matrix(rnorm(n*p), n, p)
  b <- rnorm(p)
  y <- rnorm(n, X %*% b)
  fit.ols <- lm(y ~ X)
  beta <- fit.ols$coefficients
  
  X.bm <- as.big.matrix(X)
  fit <- biglasso(X.bm, y, screen = 'None', lambda = 0, eps = eps)
  fit.edpp <- biglasso(X.bm, y, screen = 'SEDPP', lambda = 0, eps = eps)
  fit.edpp.no.active <- biglasso(X.bm, y, screen = 'SEDPP-NAC', lambda = 0, eps = eps)
  fit.ssr <- biglasso(X.bm, y, screen = 'SSR', eps = eps, lambda = 0)
  fit.ssr.dome <- biglasso(X.bm, y, screen = 'SSR-Dome', eps = eps, lambda = 0)
  fit.ssr.edpp <- biglasso(X.bm, y, screen = 'SSR-BEDPP', eps = eps, lambda = 0)
  fit.batch <- biglasso(X.bm, y, screen = 'SEDPP-Batch-SSR', eps = eps, lambda = 0)
  
  expect_equal(as.numeric(beta), as.numeric(fit$beta), tolerance = tolerance)
  expect_equal(as.numeric(beta), as.numeric(fit.edpp$beta), tolerance = tolerance)
  expect_equal(as.numeric(beta), as.numeric(fit.edpp.no.active$beta), tolerance = tolerance)
  expect_equal(as.numeric(beta), as.numeric(fit.ssr$beta), tolerance = tolerance)
  expect_equal(as.numeric(beta), as.numeric(fit.ssr.dome$beta), tolerance = tolerance)
  expect_equal(as.numeric(beta), as.numeric(fit.ssr.edpp$beta), tolerance = tolerance)
  expect_equal(as.numeric(beta), as.numeric(fit.batch$beta), tolerance = tolerance)

})

set.seed(1234)
n <- 100
p <- 200
X <- matrix(rnorm(n*p), n, p)
b <- c(rnorm(50), rep(0, p-50))
y <- rnorm(n, X %*% b)
eps <- 1e-8
tolerance <- 1e-3
lambda.min <- 0.05

fit.ncv <- ncvreg(X, y, penalty = 'lasso', eps = sqrt(eps), lambda.min = lambda.min)
cvfit.ncv <- cv.ncvreg(X, y, penalty = 'lasso', eps = sqrt(eps), 
                       lambda.min = lambda.min, seed = 1234, nfolds = 5)

X.bm <- as.big.matrix(X)
# fit <- biglasso(X.bm, y, screen = 'None', eps = eps)
fit.edpp <- biglasso(X.bm, y, screen = 'SEDPP', eps = eps)
# fit.edpp.no.active <- biglasso(X.bm, y, screen = 'SEDPP-No-Active', eps = eps)
fit.ssr <- biglasso(X.bm, y, screen = 'SSR', eps = eps)
fit.ssr.dome <- biglasso(X.bm, y, screen = 'SSR-Dome', eps = eps)
fit.ssr.edpp <- biglasso(X.bm, y, screen = 'SSR-BEDPP', eps = eps)
fit.batch <- biglasso(X.bm, y, screen = 'SEDPP-Batch-SSR', eps = eps)

# cvfit <- cv.biglasso(X.bm, y, screen = 'None', eps = eps, 
#                      ncores = 1, nfolds = 5, seed = 1234)
cvfit.edpp <- cv.biglasso(X.bm, y, screen = 'SEDPP', eps = eps,
                          ncores = 1, nfolds = 5, seed = 1234)
# cvfit.edpp.no.active <- cv.biglasso(X.bm, y, screen = 'SEDPP-No-Active', eps = eps,
#                                     ncores = 1, nfolds = 5, seed = 1234)
cvfit.ssr <- cv.biglasso(X.bm, y, screen = 'SSR', eps = eps,
                         ncores = 1, nfolds = 5, seed = 1234)
cvfit.ssr.dome <- cv.biglasso(X.bm, y, screen = 'SSR-Dome', eps = eps,
                              ncores = 1, nfolds = 5, seed = 1234)
cvfit.ssr.edpp <- cv.biglasso(X.bm, y, screen = 'SSR-BEDPP', eps = eps,
                              ncores = 1, nfolds = 5, seed = 1234)
cvfit.batch <- cv.biglasso(X.bm, y, screen = 'SEDPP-Batch-SSR', eps = eps,
                              ncores = 1, nfolds = 5, seed = 1234)

## parallel computing
fit.edpp2 <- biglasso(X.bm, y, screen = 'SEDPP', eps = eps, ncores = 2)
# fit.edpp.no.active2 <- biglasso(X.bm, y, screen = 'SEDPP-No-Active', eps = eps, ncores = 2)
fit.ssr2 <- biglasso(X.bm, y, screen = 'SSR', eps = eps, ncores = 2)
fit.ssr.dome2 <- biglasso(X.bm, y, screen = 'SSR-Dome', eps = eps, ncores = 2)
fit.ssr.edpp2 <- biglasso(X.bm, y, screen = 'SSR-BEDPP', eps = eps, ncores = 2)
fit.batch2 <- biglasso(X.bm, y, screen = 'SEDPP-Batch-SSR', eps = eps, ncores = 2)

test_that("Test against ncvreg for entire path:", {
  # expect_equal(as.numeric(fit.ncv$beta), as.numeric(fit$beta), tolerance = tolerance)
  expect_equal(as.numeric(fit.ncv$beta), as.numeric(fit.edpp$beta), tolerance = tolerance)
  # expect_equal(as.numeric(fit.ncv$beta), as.numeric(fit.edpp.no.active$beta), tolerance = tolerance)
  expect_equal(as.numeric(fit.ncv$beta), as.numeric(fit.ssr$beta), tolerance = tolerance)
  expect_equal(as.numeric(fit.ncv$beta), as.numeric(fit.ssr.dome$beta), tolerance = tolerance)
  expect_equal(as.numeric(fit.ncv$beta), as.numeric(fit.ssr.edpp$beta), tolerance = tolerance)
  expect_equal(as.numeric(fit.ncv$beta), as.numeric(fit.batch$beta), tolerance = tolerance)
})

test_that("Test parallel computing: ",{
  fit.edpp$time <- NA
  fit.edpp2$time <- NA
  fit.ssr$time <- NA
  fit.ssr2$time <- NA
  fit.ssr.dome$time <- NA
  fit.ssr.dome2$time <- NA
  fit.ssr.edpp$time <- NA
  fit.ssr.edpp2$time <- NA
  fit.batch$time <- NA
  fit.batch2$time <- NA
  expect_identical(fit.edpp, fit.edpp2)
  # expect_identical(fit.edpp.no.active, fit.edpp.no.active2)
  expect_identical(fit.ssr, fit.ssr2)
  expect_identical(fit.ssr.dome, fit.ssr.dome2)
  expect_identical(fit.ssr.edpp, fit.ssr.edpp2)
  expect_identical(fit.batch, fit.batch2)
})

test_that("Test cross validation: ",{
  # expect_equal(as.numeric(cvfit.ncv$cve), as.numeric(cvfit$cve), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$cve), as.numeric(cvfit.edpp$cve), tolerance = tolerance)
  # expect_equal(as.numeric(cvfit.ncv$cve), as.numeric(cvfit.edpp.no.active$cve), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$cve), as.numeric(cvfit.ssr$cve), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$cve), as.numeric(cvfit.ssr.dome$cve), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$cve), as.numeric(cvfit.ssr.edpp$cve), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$cve), as.numeric(cvfit.batch$cve), tolerance = tolerance)
  
  expect_equal(as.numeric(cvfit.ncv$cvse), as.numeric(cvfit.edpp$cvse), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$cvse), as.numeric(cvfit.ssr$cvse), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$cvse), as.numeric(cvfit.ssr.dome$cvse), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$cvse), as.numeric(cvfit.ssr.edpp$cvse), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$cvse), as.numeric(cvfit.batch$cvse), tolerance = tolerance)
  
  expect_equal(as.numeric(cvfit.ncv$lambda.min), as.numeric(cvfit.edpp$lambda.min), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$lambda.min), as.numeric(cvfit.ssr$lambda.min), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$lambda.min), as.numeric(cvfit.ssr.dome$lambda.min), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$lambda.min), as.numeric(cvfit.ssr.edpp$lambda.min), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$lambda.min), as.numeric(cvfit.batch$lambda.min), tolerance = tolerance)
  
})

# ------------------------------------------------------------------------------
# test elastic net
# ------------------------------------------------------------------------------
set.seed(1234)
n <- 100
p <- 200
X <- matrix(rnorm(n*p), n, p)
b <- c(rnorm(50), rep(0, p-50))
y <- rnorm(n, X %*% b)
eps <- 1e-8
tolerance <- 1e-3
lambda.min <- 0.05
alpha <- 0.5

fit.ncv <- ncvreg(X, y, penalty = 'lasso', eps = sqrt(eps), 
                  lambda.min = lambda.min, alpha = alpha)
X.bm <- as.big.matrix(X)
fit.ssr <- biglasso(X.bm, y, penalty = 'enet', screen = 'SSR', eps = eps, alpha = alpha)
fit.ssr.edpp <- biglasso(X.bm, y, penalty = 'enet', screen = 'SSR-BEDPP', eps = eps, alpha = alpha)

cvfit.ncv <- cv.ncvreg(X, y, penalty = 'lasso', eps = sqrt(eps), alpha = alpha,
                       lambda.min = lambda.min, seed = 1234, nfolds = 5)
cvfit.ssr <- cv.biglasso(X.bm, y, screen = 'SSR', penalty = 'enet', eps = eps, alpha = alpha,
                         ncores = 1, nfolds = 5, seed = 1234)
cvfit.ssr.edpp <- cv.biglasso(X.bm, y, penalty = 'enet', screen = 'SSR-BEDPP', eps = eps, alpha = alpha,
                              ncores = 2, nfolds = 5, seed = 1234)

test_that("Elastic net: test against ncvreg for entire path:", {
  expect_equal(as.numeric(fit.ncv$beta), as.numeric(fit.ssr$beta), tolerance = tolerance)
  expect_equal(as.numeric(fit.ncv$beta), as.numeric(fit.ssr.edpp$beta), tolerance = tolerance)
})

test_that("Elastic net: test cross validation: ",{
  expect_equal(as.numeric(cvfit.ncv$cve), as.numeric(cvfit.ssr$cve), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$cve), as.numeric(cvfit.ssr.edpp$cve), tolerance = tolerance)
  
  expect_equal(as.numeric(cvfit.ncv$cvse), as.numeric(cvfit.ssr$cvse), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$cvse), as.numeric(cvfit.ssr.edpp$cvse), tolerance = tolerance)
  
  expect_equal(as.numeric(cvfit.ncv$lambda.min), as.numeric(cvfit.ssr$lambda.min), tolerance = tolerance)
  expect_equal(as.numeric(cvfit.ncv$lambda.min), as.numeric(cvfit.ssr.edpp$lambda.min), tolerance = tolerance)
  
})

