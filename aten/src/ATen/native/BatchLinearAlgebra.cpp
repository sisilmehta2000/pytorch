#include <ATen/ATen.h>
#include <ATen/CPUApplyUtils.h>
#include <ATen/Dispatch.h>
#include <ATen/NativeFunctions.h>
#include <ATen/ExpandUtils.h>

#include <ATen/native/LinearAlgebraUtils.h>
#include <ATen/native/Resize.h>
#include <ATen/native/cpu/zmath.h>
#include <ATen/Parallel.h>

#include <TH/TH.h>  // for USE_LAPACK

#include <vector>

// First the required LAPACK implementations are registered here.
// A comment above the registered LAPACK routine suggest which batched
// linear algebra function uses that routine
#ifdef USE_LAPACK

// gesv
extern "C" void zgesv_(int *n, int *nrhs, std::complex<double> *a, int *lda, int *ipiv, std::complex<double> *b, int *ldb, int *info);
extern "C" void cgesv_(int *n, int *nrhs, std::complex<float> *a, int *lda, int *ipiv, std::complex<float> *b, int *ldb, int *info);
extern "C" void dgesv_(int *n, int *nrhs, double *a, int *lda, int *ipiv, double *b, int *ldb, int *info);
extern "C" void sgesv_(int *n, int *nrhs, float *a, int *lda, int *ipiv, float *b, int *ldb, int *info);

// getrf
extern "C" void zgetrf_(int *m, int *n, std::complex<double> *a, int *lda, int *ipiv, int *info);
extern "C" void cgetrf_(int *m, int *n, std::complex<float> *a, int *lda, int *ipiv, int *info);
extern "C" void dgetrf_(int *m, int *n, double *a, int *lda, int *ipiv, int *info);
extern "C" void sgetrf_(int *m, int *n, float *a, int *lda, int *ipiv, int *info);

// getri
extern "C" void zgetri_(int *n, std::complex<double> *a, int *lda, int *ipiv, std::complex<double> *work, int *lwork, int *info);
extern "C" void cgetri_(int *n, std::complex<float> *a, int *lda, int *ipiv, std::complex<float> *work, int *lwork, int *info);
extern "C" void dgetri_(int *n, double *a, int *lda, int *ipiv, double *work, int *lwork, int *info);
extern "C" void sgetri_(int *n, float *a, int *lda, int *ipiv, float *work, int *lwork, int *info);

// potrs
extern "C" void zpotrs_(char *uplo, int *n, int *nrhs, std::complex<double> *a, int *lda, std::complex<double> *b, int *ldb, int *info);
extern "C" void cpotrs_(char *uplo, int *n, int *nrhs, std::complex<float> *a, int *lda, std::complex<float> *b, int *ldb, int *info);
extern "C" void dpotrs_(char *uplo, int *n, int *nrhs, double *a, int *lda, double *b, int *ldb, int *info);
extern "C" void spotrs_(char *uplo, int *n, int *nrhs, float *a, int *lda, float *b, int *ldb, int *info);

// potrf
extern "C" void zpotrf_(char *uplo, int *n, std::complex<double> *a, int *lda, int *info);
extern "C" void cpotrf_(char *uplo, int *n, std::complex<float> *a, int *lda, int *info);
extern "C" void dpotrf_(char *uplo, int *n, double *a, int *lda, int *info);
extern "C" void spotrf_(char *uplo, int *n, float *a, int *lda, int *info);

// trtrs
extern "C" void ztrtrs_(char *uplo, char *trans, char *diag, int *n, int *nrhs, std::complex<double> *a, int *lda, std::complex<double> *b, int *ldb, int *info);
extern "C" void ctrtrs_(char *uplo, char *trans, char *diag, int *n, int *nrhs, std::complex<float> *a, int *lda, std::complex<float> *b, int *ldb, int *info);
extern "C" void dtrtrs_(char *uplo, char *trans, char *diag, int *n, int *nrhs, double *a, int *lda, double *b, int *ldb, int *info);
extern "C" void strtrs_(char *uplo, char *trans, char *diag, int *n, int *nrhs, float *a, int *lda, float *b, int *ldb, int *info);

// geqrf
extern "C" void zgeqrf_(int *m, int *n, std::complex<double> *a, int *lda, std::complex<double> *tau, std::complex<double> *work, int *lwork, int *info);
extern "C" void cgeqrf_(int *m, int *n, std::complex<float> *a, int *lda, std::complex<float> *tau, std::complex<float> *work, int *lwork, int *info);
extern "C" void dgeqrf_(int *m, int *n, double *a, int *lda, double *tau, double *work, int *lwork, int *info);
extern "C" void sgeqrf_(int *m, int *n, float *a, int *lda, float *tau, float *work, int *lwork, int *info);

// orgqr
extern "C" void zungqr_(int *m, int *n, int *k, std::complex<double> *a, int *lda, std::complex<double> *tau, std::complex<double> *work, int *lwork, int *info);
extern "C" void cungqr_(int *m, int *n, int *k, std::complex<float> *a, int *lda, std::complex<float> *tau, std::complex<float> *work, int *lwork, int *info);
extern "C" void dorgqr_(int *m, int *n, int *k, double *a, int *lda, double *tau, double *work, int *lwork, int *info);
extern "C" void sorgqr_(int *m, int *n, int *k, float *a, int *lda, float *tau, float *work, int *lwork, int *info);

// syev
extern "C" void zheev_(char *jobz, char *uplo, int *n, std::complex<double> *a, int *lda, double *w, std::complex<double> *work, int *lwork, double *rwork, int *info);
extern "C" void cheev_(char *jobz, char *uplo, int *n, std::complex<float> *a, int *lda, float *w, std::complex<float> *work, int *lwork, float *rwork, int *info);
extern "C" void dsyev_(char *jobz, char *uplo, int *n, double *a, int *lda, double *w, double *work, int *lwork, int *info);
extern "C" void ssyev_(char *jobz, char *uplo, int *n, float *a, int *lda, float *w, float *work, int *lwork, int *info);

// syevd
extern "C" void zheevd_(char *jobz, char *uplo, int *n, std::complex<double> *a, int *lda, double *w, std::complex<double> *work, int *lwork, double *rwork, int *lrwork, int *iwork, int *liwork, int *info);
extern "C" void cheevd_(char *jobz, char *uplo, int *n, std::complex<float> *a, int *lda, float *w, std::complex<float> *work, int *lwork, float *rwork, int *lrwork, int *iwork, int *liwork, int *info);
extern "C" void dsyevd_(char *jobz, char *uplo, int *n, double *a, int *lda, double *w, double *work, int *lwork, int *iwork, int *liwork, int *info);
extern "C" void ssyevd_(char *jobz, char *uplo, int *n, float *a, int *lda, float *w, float *work, int *lwork, int *iwork, int *liwork, int *info);

// gesdd
extern "C" void zgesdd_(char *jobz, int *m, int *n, std::complex<double> *a, int *lda,
                        double *s, std::complex<double> *u, int *ldu, std::complex<double> *vt, int *ldvt, std::complex<double> *work, int *lwork, double *rwork, int *iwork, int *info);
extern "C" void cgesdd_(char *jobz, int *m, int *n, std::complex<float> *a, int *lda,
                        float *s, std::complex<float> *u, int *ldu, std::complex<float> *vt, int *ldvt, std::complex<float> *work, int *lwork, float *rwork, int *iwork, int *info);
extern "C" void dgesdd_(char *jobz, int *m, int *n, double *a, int *lda,
                        double *s, double *u, int *ldu, double *vt, int *ldvt, double *work, int *lwork, int *iwork, int *info);
extern "C" void sgesdd_(char *jobz, int *m, int *n, float *a, int *lda,
                        float *s, float *u, int *ldu, float *vt, int *ldvt, float *work, int *lwork, int *iwork, int *info);

// getrs
extern "C" void zgetrs_(char *trans, int *n, int *nrhs, std::complex<double> *a, int *lda, int *ipiv, std::complex<double> *b, int *ldb, int *info);
extern "C" void cgetrs_(char *trans, int *n, int *nrhs, std::complex<float> *a, int *lda, int *ipiv, std::complex<float> *b, int *ldb, int *info);
extern "C" void dgetrs_(char *trans, int *n, int *nrhs, double *a, int *lda, int *ipiv, double *b, int *ldb, int *info);
extern "C" void sgetrs_(char *trans, int *n, int *nrhs, float *a, int *lda, int *ipiv, float *b, int *ldb, int *info);

// gels
extern "C" void zgels_(char *trans, int *m, int *n, int *nrhs,
    std::complex<double> *a, int *lda, std::complex<double> *b, int *ldb,
    std::complex<double> *work, int *lwork, int *info);
extern "C" void cgels_(char *trans, int *m, int *n, int *nrhs,
    std::complex<float> *a, int *lda, std::complex<float> *b, int *ldb,
    std::complex<float> *work, int *lwork, int *info);
extern "C" void dgels_(char *trans, int *m, int *n, int *nrhs,
    double *a, int *lda, double *b, int *ldb,
    double *work, int *lwork, int *info);
extern "C" void sgels_(char *trans, int *m, int *n, int *nrhs,
    float *a, int *lda, float *b, int *ldb,
    float *work, int *lwork, int *info);

// gelsd
extern "C" void zgelsd_(int *m, int *n, int *nrhs,
    std::complex<double> *a, int *lda, std::complex<double> *b, int *ldb,
    double *s, double *rcond, int *rank,
    std::complex<double> *work, int *lwork, double *rwork, int *iwork, int *info);
extern "C" void cgelsd_(int *m, int *n, int *nrhs,
    std::complex<float> *a, int *lda, std::complex<float> *b, int *ldb,
    float *s, float *rcond, int *rank,
    std::complex<float> *work, int *lwork, float *rwork, int *iwork, int *info);
extern "C" void dgelsd_(int *m, int *n, int *nrhs,
    double *a, int *lda, double *b, int *ldb,
    double *s, double *rcond, int *rank,
    double *work, int *lwork, int *iwork, int *info);
extern "C" void sgelsd_(int *m, int *n, int *nrhs,
    float *a, int *lda, float *b, int *ldb,
    float *s, float *rcond, int *rank,
    float *work, int *lwork, int *iwork, int *info);

// gelsy
extern "C" void zgelsy_(int *m, int *n, int *nrhs,
    std::complex<double> *a, int *lda, std::complex<double> *b, int *ldb,
    int *jpvt, double *rcond, int *rank,
    std::complex<double> *work, int *lwork,
    double *rwork, int *info);
extern "C" void cgelsy_(int *m, int *n, int *nrhs,
    std::complex<float> * a, int *lda, std::complex<float> *b, int *ldb,
    int *jpvt, float *rcond, int *rank,
    std::complex<float> *work, int *lwork,
    float *rwork, int *info);
extern "C" void dgelsy_(int *m, int *n, int *nrhs,
    double *a, int *lda, double *b, int *ldb,
    int *jpvt, double *rcond, int *rank,
    double *work, int *lwork, int *info);
extern "C" void sgelsy_(int *m, int *n, int *nrhs,
    float *a, int *lda, float *b, int *ldb,
    int *jpvt, float *rcond, int *rank,
    float *work, int *lwork, int *info);

// gelss
extern "C" void zgelss_(int *m, int *n, int *nrhs,
    std::complex<double> *a, int *lda, std::complex<double> *b, int *ldb,
    double *s, double *rcond, int *rank,
    std::complex<double> *work, int *lwork,
    double *rwork, int *info);
extern "C" void cgelss_(int *m, int *n, int *nrhs,
    std::complex<float> *a, int *lda, std::complex<float> *b, int *ldb,
    float *s, float *rcond, int *rank,
    std::complex<float> *work, int *lwork,
    float *rwork, int *info);
extern "C" void dgelss_(int *m, int *n, int *nrhs,
    double *a, int *lda, double *b, int *ldb,
    double *s, double *rcond, int *rank,
    double *work, int *lwork, int *info);
extern "C" void sgelss_(int *m, int *n, int *nrhs,
    float *a, int *lda, float *b, int *ldb,
    float *s, float *rcond, int *rank,
    float *work, int *lwork, int *info);
#endif

namespace at {
namespace native {

#ifdef USE_LAPACK
// Define the per-batch functions to be used in the main implementation of the batched
// linear algebra operations
template<class scalar_t>
void lapackSolve(int n, int nrhs, scalar_t *a, int lda, int *ipiv, scalar_t *b, int ldb, int *info);

template<class scalar_t>
void lapackLu(int m, int n, scalar_t *a, int lda, int *ipiv, int *info);

template<class scalar_t>
void lapackGetri(int n, scalar_t *a, int lda, int *ipiv, scalar_t *work, int lwork, int *info);

template<class scalar_t>
void lapackCholeskySolve(char uplo, int n, int nrhs, scalar_t *a, int lda, scalar_t *b, int ldb, int *info);

template<class scalar_t>
void lapackCholesky(char uplo, int n, scalar_t *a, int lda, int *info);

template<class scalar_t>
void lapackTriangularSolve(char uplo, char trans, char diag, int n, int nrhs, scalar_t *a, int lda, scalar_t *b, int ldb, int *info);

template<class scalar_t>
void lapackGeqrf(int m, int n, scalar_t *a, int lda, scalar_t *tau, scalar_t *work, int lwork, int *info);

template<class scalar_t>
void lapackOrgqr(int m, int n, int k, scalar_t *a, int lda, scalar_t *tau, scalar_t *work, int lwork, int *info);

template<class scalar_t, class value_t=scalar_t>
void lapackSymeig(char jobz, char uplo, int n, scalar_t *a, int lda, value_t *w, scalar_t *work, int lwork, value_t *rwork, int *info);

template<class scalar_t, class value_t=scalar_t>
void lapackSyevd(char jobz, char uplo, int n, scalar_t *a, int lda, value_t *w, scalar_t *work, int lwork, value_t *rwork, int lrwork, int *iwork, int liwork, int *info);

template<class scalar_t, class value_t=scalar_t>
void lapackSvd(char jobz, int m, int n, scalar_t *a, int lda,
               value_t *s, scalar_t *u, int ldu, scalar_t *vt, int ldvt, scalar_t *work, int lwork, value_t *rwork, int *iwork, int *info);

template<class scalar_t>
void lapackLuSolve(char trans, int n, int nrhs, scalar_t *a, int lda, int *ipiv, scalar_t *b, int ldb, int *info);

template<class scalar_t>
void lapackGels(char trans, int m, int n, int nrhs,
    scalar_t *a, int lda, scalar_t *b, int ldb,
    scalar_t *work, int lwork, int *info);

template<class scalar_t, class value_t = scalar_t>
void lapackGelsd(int m, int n, int nrhs,
    scalar_t *a, int lda, scalar_t *b, int ldb,
    value_t *s, value_t rcond, int *rank,
    scalar_t* work, int lwork,
    value_t *rwork, int* iwork, int *info);

template<class scalar_t, class value_t = scalar_t>
void lapackGelsy(int m, int n, int nrhs,
    scalar_t *a, int lda, scalar_t *b, int ldb,
    int *jpvt, value_t rcond, int *rank,
    scalar_t *work, int lwork, value_t* rwork, int *info);

template<class scalar_t, class value_t = scalar_t>
void lapackGelss(int m, int n, int nrhs,
    scalar_t *a, int lda, scalar_t *b, int ldb,
    value_t *s, value_t rcond, int *rank,
    scalar_t *work, int lwork,
    value_t *rwork, int *info);

enum class LapackLstsqDriver : int64_t { Gels, Gelsd, Gelsy, Gelss};

template<LapackLstsqDriver, class scalar_t, class value_t = scalar_t>
struct lapackLstsq_impl;

template<class scalar_t, class value_t>
struct lapackLstsq_impl<LapackLstsqDriver::Gels, scalar_t, value_t> {
  static void call(
      char trans, int m, int n, int nrhs,
      scalar_t *a, int lda, scalar_t *b, int ldb,
      scalar_t *work, int lwork, int *info, // Gels flavor
      int *jpvt, value_t rcond, int *rank, value_t* rwork, // Gelsy flavor
      value_t *s, // Gelss flavor
      int *iwork // Gelsd flavor
      ) {
    lapackGels<scalar_t>(
        trans, m, n, nrhs,
        a, lda, b, ldb,
        work, lwork, info);
  }
};

template<class scalar_t, class value_t>
struct lapackLstsq_impl<LapackLstsqDriver::Gelsy, scalar_t, value_t> {
  static void call(
      char trans, int m, int n, int nrhs,
      scalar_t *a, int lda, scalar_t *b, int ldb,
      scalar_t *work, int lwork, int *info, // Gels flavor
      int *jpvt, value_t rcond, int *rank, value_t* rwork, // Gelsy flavor
      value_t *s, // Gelss flavor
      int *iwork // Gelsd flavor
      ) {
    lapackGelsy<scalar_t, value_t>(
        m, n, nrhs,
        a, lda, b, ldb,
        jpvt, rcond, rank,
        work, lwork, rwork, info);
  }
};

template<class scalar_t, class value_t>
struct lapackLstsq_impl<LapackLstsqDriver::Gelsd, scalar_t, value_t> {
  static void call(
      char trans, int m, int n, int nrhs,
      scalar_t *a, int lda, scalar_t *b, int ldb,
      scalar_t *work, int lwork, int *info, // Gels flavor
      int *jpvt, value_t rcond, int *rank, value_t* rwork, // Gelsy flavor
      value_t *s, // Gelss flavor
      int *iwork // Gelsd flavor
      ) {
    lapackGelsd<scalar_t, value_t>(
        m, n, nrhs,
        a, lda, b, ldb,
        s, rcond, rank,
        work, lwork,
        rwork, iwork, info);
  }
};

template<class scalar_t, class value_t>
struct lapackLstsq_impl<LapackLstsqDriver::Gelss, scalar_t, value_t> {
  static void call(
      char trans, int m, int n, int nrhs,
      scalar_t *a, int lda, scalar_t *b, int ldb,
      scalar_t *work, int lwork, int *info, // Gels flavor
      int *jpvt, value_t rcond, int *rank, value_t* rwork, // Gelsy flavor
      value_t *s, // Gelss flavor
      int *iwork // Gelsd flavor
      ) {
    lapackGelss<scalar_t, value_t>(
        m, n, nrhs,
        a, lda, b, ldb,
        s, rcond, rank,
        work, lwork,
        rwork, info);
  }
};

template<LapackLstsqDriver driver, class scalar_t, class value_t = scalar_t>
void lapackLstsq(
    char trans, int m, int n, int nrhs,
    scalar_t *a, int lda, scalar_t *b, int ldb,
    scalar_t *work, int lwork, int *info, // Gels flavor
    int *jpvt, value_t rcond, int *rank, value_t* rwork, // Gelsy flavor
    value_t *s, // Gelss flavor
    int *iwork // Gelsd flavor
    ) {
  lapackLstsq_impl<driver, scalar_t, value_t>::call(
      trans, m, n, nrhs,
      a, lda, b, ldb,
      work, lwork, info,
      jpvt, rcond, rank, rwork,
      s,
      iwork);
}

template<> void lapackSolve<c10::complex<double>>(int n, int nrhs, c10::complex<double> *a, int lda, int *ipiv, c10::complex<double> *b, int ldb, int *info) {
  zgesv_(&n, &nrhs, reinterpret_cast<std::complex<double>*>(a), &lda, ipiv, reinterpret_cast<std::complex<double>*>(b), &ldb, info);
}

template<> void lapackSolve<c10::complex<float>>(int n, int nrhs, c10::complex<float> *a, int lda, int *ipiv, c10::complex<float> *b, int ldb, int *info) {
  cgesv_(&n, &nrhs, reinterpret_cast<std::complex<float>*>(a), &lda, ipiv, reinterpret_cast<std::complex<float>*>(b), &ldb, info);
}

template<> void lapackSolve<double>(int n, int nrhs, double *a, int lda, int *ipiv, double *b, int ldb, int *info) {
  dgesv_(&n, &nrhs, a, &lda, ipiv, b, &ldb, info);
}

template<> void lapackSolve<float>(int n, int nrhs, float *a, int lda, int *ipiv, float *b, int ldb, int *info) {
  sgesv_(&n, &nrhs, a, &lda, ipiv, b, &ldb, info);
}

template<> void lapackGetri<c10::complex<double>>(int n, c10::complex<double> *a, int lda, int *ipiv, c10::complex<double> *work, int lwork, int *info) {
  zgetri_(&n, reinterpret_cast<std::complex<double>*>(a), &lda, ipiv, reinterpret_cast<std::complex<double>*>(work), &lwork, info);
}

template<> void lapackGetri<c10::complex<float>>(int n, c10::complex<float> *a, int lda, int *ipiv, c10::complex<float> *work, int lwork, int *info) {
  cgetri_(&n, reinterpret_cast<std::complex<float>*>(a), &lda, ipiv, reinterpret_cast<std::complex<float>*>(work), &lwork, info);
}

template<> void lapackGetri<double>(int n, double *a, int lda, int *ipiv, double *work, int lwork, int *info) {
  dgetri_(&n, a, &lda, ipiv, work, &lwork, info);
}

template<> void lapackGetri<float>(int n, float *a, int lda, int *ipiv, float *work, int lwork, int *info) {
  sgetri_(&n, a, &lda, ipiv, work, &lwork, info);
}

template<> void lapackLu<c10::complex<double>>(int m, int n, c10::complex<double> *a, int lda, int *ipiv, int *info) {
  zgetrf_(&m, &n, reinterpret_cast<std::complex<double>*>(a), &lda, ipiv, info);
}

template<> void lapackLu<c10::complex<float>>(int m, int n, c10::complex<float> *a, int lda, int *ipiv, int *info) {
  cgetrf_(&m, &n, reinterpret_cast<std::complex<float>*>(a), &lda, ipiv, info);
}

template<> void lapackLu<double>(int m, int n, double *a, int lda, int *ipiv, int *info) {
  dgetrf_(&m, &n, a, &lda, ipiv, info);
}

template<> void lapackLu<float>(int m, int n, float *a, int lda, int *ipiv, int *info) {
  sgetrf_(&m, &n, a, &lda, ipiv, info);
}

template<> void lapackCholeskySolve<c10::complex<double>>(char uplo, int n, int nrhs, c10::complex<double> *a, int lda, c10::complex<double> *b, int ldb, int *info) {
  zpotrs_(&uplo, &n, &nrhs, reinterpret_cast<std::complex<double>*>(a), &lda, reinterpret_cast<std::complex<double>*>(b), &ldb, info);
}

template<> void lapackCholeskySolve<c10::complex<float>>(char uplo, int n, int nrhs, c10::complex<float> *a, int lda, c10::complex<float> *b, int ldb, int *info) {
  cpotrs_(&uplo, &n, &nrhs, reinterpret_cast<std::complex<float>*>(a), &lda, reinterpret_cast<std::complex<float>*>(b), &ldb, info);
}

template<> void lapackCholeskySolve<double>(char uplo, int n, int nrhs, double *a, int lda, double *b, int ldb, int *info) {
  dpotrs_(&uplo, &n, &nrhs, a, &lda, b, &ldb, info);
}

template<> void lapackCholeskySolve<float>(char uplo, int n, int nrhs, float *a, int lda, float *b, int ldb, int *info) {
  spotrs_(&uplo, &n, &nrhs, a, &lda, b, &ldb, info);
}

template<> void lapackCholesky<c10::complex<double>>(char uplo, int n, c10::complex<double> *a, int lda, int *info) {
  zpotrf_(&uplo, &n, reinterpret_cast<std::complex<double>*>(a), &lda, info);
}

template<> void lapackCholesky<c10::complex<float>>(char uplo, int n, c10::complex<float> *a, int lda, int *info) {
  cpotrf_(&uplo, &n, reinterpret_cast<std::complex<float>*>(a), &lda, info);
}

template<> void lapackCholesky<double>(char uplo, int n, double *a, int lda, int *info) {
  dpotrf_(&uplo, &n, a, &lda, info);
}

template<> void lapackCholesky<float>(char uplo, int n, float *a, int lda, int *info) {
  spotrf_(&uplo, &n, a, &lda, info);
}

template<> void lapackTriangularSolve<c10::complex<double>>(char uplo, char trans, char diag, int n, int nrhs, c10::complex<double> *a, int lda, c10::complex<double> *b, int ldb, int *info) {
  ztrtrs_(&uplo, &trans, &diag, &n, &nrhs, reinterpret_cast<std::complex<double>*>(a), &lda, reinterpret_cast<std::complex<double>*>(b), &ldb, info);
}

template<> void lapackTriangularSolve<c10::complex<float>>(char uplo, char trans, char diag, int n, int nrhs, c10::complex<float> *a, int lda, c10::complex<float> *b, int ldb, int *info) {
  ctrtrs_(&uplo, &trans, &diag, &n, &nrhs, reinterpret_cast<std::complex<float>*>(a), &lda, reinterpret_cast<std::complex<float>*>(b), &ldb, info);
}

template<> void lapackTriangularSolve<double>(char uplo, char trans, char diag, int n, int nrhs, double *a, int lda, double *b, int ldb, int *info) {
  dtrtrs_(&uplo, &trans, &diag, &n, &nrhs, a, &lda, b, &ldb, info);
}

template<> void lapackTriangularSolve<float>(char uplo, char trans, char diag, int n, int nrhs, float *a, int lda, float *b, int ldb, int *info) {
  strtrs_(&uplo, &trans, &diag, &n, &nrhs, a, &lda, b, &ldb, info);
}

template<> void lapackGeqrf<c10::complex<double>>(int m, int n, c10::complex<double> *a, int lda, c10::complex<double> *tau, c10::complex<double> *work, int lwork, int *info) {
  zgeqrf_(&m, &n, reinterpret_cast<std::complex<double>*>(a), &lda, reinterpret_cast<std::complex<double>*>(tau), reinterpret_cast<std::complex<double>*>(work), &lwork, info);
}

template<> void lapackGeqrf<c10::complex<float>>(int m, int n, c10::complex<float> *a, int lda, c10::complex<float> *tau, c10::complex<float> *work, int lwork, int *info) {
  cgeqrf_(&m, &n, reinterpret_cast<std::complex<float>*>(a), &lda, reinterpret_cast<std::complex<float>*>(tau), reinterpret_cast<std::complex<float>*>(work), &lwork, info);
}

template<> void lapackGeqrf<double>(int m, int n, double *a, int lda, double *tau, double *work, int lwork, int *info) {
  dgeqrf_(&m, &n, a, &lda, tau, work, &lwork, info);
}

template<> void lapackGeqrf<float>(int m, int n, float *a, int lda, float *tau, float *work, int lwork, int *info) {
  sgeqrf_(&m, &n, a, &lda, tau, work, &lwork, info);
}

template<> void lapackOrgqr<c10::complex<double>>(int m, int n, int k, c10::complex<double> *a, int lda, c10::complex<double> *tau, c10::complex<double> *work, int lwork, int *info) {
  zungqr_(&m, &n, &k, reinterpret_cast<std::complex<double>*>(a), &lda, reinterpret_cast<std::complex<double>*>(tau), reinterpret_cast<std::complex<double>*>(work), &lwork, info);
}

template<> void lapackOrgqr<c10::complex<float>>(int m, int n, int k, c10::complex<float> *a, int lda, c10::complex<float> *tau, c10::complex<float> *work, int lwork, int *info) {
  cungqr_(&m, &n, &k, reinterpret_cast<std::complex<float>*>(a), &lda, reinterpret_cast<std::complex<float>*>(tau), reinterpret_cast<std::complex<float>*>(work), &lwork, info);
}

template<> void lapackOrgqr<double>(int m, int n, int k, double *a, int lda, double *tau, double *work, int lwork, int *info) {
  dorgqr_(&m, &n, &k, a, &lda, tau, work, &lwork, info);
}

template<> void lapackOrgqr<float>(int m, int n, int k, float *a, int lda, float *tau, float *work, int lwork, int *info) {
  sorgqr_(&m, &n, &k, a, &lda, tau, work, &lwork, info);
}

template<> void lapackSymeig<c10::complex<double>, double>(char jobz, char uplo, int n, c10::complex<double> *a, int lda, double *w, c10::complex<double> *work, int lwork, double *rwork, int *info) {
  zheev_(&jobz, &uplo, &n, reinterpret_cast<std::complex<double>*>(a), &lda, w, reinterpret_cast<std::complex<double>*>(work), &lwork, rwork, info);
}

template<> void lapackSymeig<c10::complex<float>, float>(char jobz, char uplo, int n, c10::complex<float> *a, int lda, float *w, c10::complex<float> *work, int lwork, float *rwork, int *info) {
  cheev_(&jobz, &uplo, &n, reinterpret_cast<std::complex<float>*>(a), &lda, w, reinterpret_cast<std::complex<float>*>(work), &lwork, rwork, info);
}

template<> void lapackSymeig<double>(char jobz, char uplo, int n, double *a, int lda, double *w, double *work, int lwork, double* rwork, int *info) {
  (void)rwork;  // unused
  dsyev_(&jobz, &uplo, &n, a, &lda, w, work, &lwork, info);
}

template<> void lapackSymeig<float>(char jobz, char uplo, int n, float *a, int lda, float *w, float *work, int lwork, float* rwork, int *info) {
  (void)rwork;  // unused
  ssyev_(&jobz, &uplo, &n, a, &lda, w, work, &lwork, info);
}

template<> void lapackSyevd<c10::complex<double>, double>(char jobz, char uplo, int n, c10::complex<double> *a, int lda, double *w, c10::complex<double> *work, int lwork, double *rwork, int lrwork, int *iwork, int liwork, int *info) {
  zheevd_(&jobz, &uplo, &n, reinterpret_cast<std::complex<double>*>(a), &lda, w, reinterpret_cast<std::complex<double>*>(work), &lwork, rwork, &lrwork, iwork, &liwork, info);
}

template<> void lapackSyevd<c10::complex<float>, float>(char jobz, char uplo, int n, c10::complex<float> *a, int lda, float *w, c10::complex<float> *work, int lwork, float *rwork, int lrwork, int *iwork, int liwork, int *info) {
  cheevd_(&jobz, &uplo, &n, reinterpret_cast<std::complex<float>*>(a), &lda, w, reinterpret_cast<std::complex<float>*>(work), &lwork, rwork, &lrwork, iwork, &liwork, info);
}

template<> void lapackSyevd<double>(char jobz, char uplo, int n, double *a, int lda, double *w, double *work, int lwork, double *rwork, int lrwork, int *iwork, int liwork, int *info) {
  (void)rwork;  // unused
  (void)lrwork;  // unused
  dsyevd_(&jobz, &uplo, &n, a, &lda, w, work, &lwork, iwork, &liwork, info);
}

template<> void lapackSyevd<float>(char jobz, char uplo, int n, float *a, int lda, float *w, float *work, int lwork, float *rwork, int lrwork, int *iwork, int liwork, int *info) {
  (void)rwork;  // unused
  (void)lrwork;  // unused
  ssyevd_(&jobz, &uplo, &n, a, &lda, w, work, &lwork, iwork, &liwork, info);
}

template<> void lapackSvd<c10::complex<double>, double>(char jobz, int m, int n, c10::complex<double> *a, int lda,
                                  double *s, c10::complex<double> *u, int ldu, c10::complex<double> *vt, int ldvt, c10::complex<double> *work, int lwork, double *rwork, int *iwork, int *info) {
  zgesdd_(&jobz, &m, &n, reinterpret_cast<std::complex<double>*>(a), &lda, s, reinterpret_cast<std::complex<double>*>(u), &ldu,
          reinterpret_cast<std::complex<double>*>(vt), &ldvt, reinterpret_cast<std::complex<double>*>(work), &lwork, rwork, iwork, info);
}

template<> void lapackSvd<c10::complex<float>, float>(char jobz, int m, int n, c10::complex<float> *a, int lda,
                                 float *s, c10::complex<float> *u, int ldu, c10::complex<float> *vt, int ldvt, c10::complex<float> *work, int lwork, float *rwork, int *iwork, int *info) {
  cgesdd_(&jobz, &m, &n, reinterpret_cast<std::complex<float>*>(a), &lda, s, reinterpret_cast<std::complex<float>*>(u), &ldu,
          reinterpret_cast<std::complex<float>*>(vt), &ldvt, reinterpret_cast<std::complex<float>*>(work), &lwork, rwork, iwork, info);
}

template<> void lapackSvd<double>(char jobz, int m, int n, double *a, int lda,
                                  double *s, double *u, int ldu, double *vt, int ldvt, double *work, int lwork, double *rwork, int *iwork, int *info) {
  dgesdd_(&jobz, &m, &n, a, &lda, s, u, &ldu, vt, &ldvt, work, &lwork, iwork, info);
}

template<> void lapackSvd<float>(char jobz, int m, int n, float *a, int lda,
                                 float *s, float *u, int ldu, float *vt, int ldvt, float *work, int lwork, float *rwork, int *iwork, int *info) {
  sgesdd_(&jobz, &m, &n, a, &lda, s, u, &ldu, vt, &ldvt, work, &lwork, iwork, info);
}

template<> void lapackLuSolve<c10::complex<double>>(char trans, int n, int nrhs, c10::complex<double> *a, int lda, int *ipiv, c10::complex<double> *b, int ldb, int *info) {
  zgetrs_(&trans, &n, &nrhs, reinterpret_cast<std::complex<double>*>(a), &lda, ipiv, reinterpret_cast<std::complex<double>*>(b), &ldb, info);
}

template<> void lapackLuSolve<c10::complex<float>>(char trans, int n, int nrhs, c10::complex<float> *a, int lda, int *ipiv, c10::complex<float> *b, int ldb, int *info) {
  cgetrs_(&trans, &n, &nrhs, reinterpret_cast<std::complex<float>*>(a), &lda, ipiv, reinterpret_cast<std::complex<float>*>(b), &ldb, info);
}

template<> void lapackLuSolve<double>(char trans, int n, int nrhs, double *a, int lda, int *ipiv, double *b, int ldb, int *info) {
  dgetrs_(&trans, &n, &nrhs, a, &lda, ipiv, b, &ldb, info);
}

template<> void lapackLuSolve<float>(char trans, int n, int nrhs, float *a, int lda, int *ipiv, float *b, int ldb, int *info) {
  sgetrs_(&trans, &n, &nrhs, a, &lda, ipiv, b, &ldb, info);
}

template<> void lapackGels<c10::complex<double>>(
    char trans, int m, int n, int nrhs,
    c10::complex<double> *a, int lda, c10::complex<double> *b, int ldb,
    c10::complex<double> *work, int lwork, int *info) {
  zgels_(&trans, &m, &n, &nrhs,
      reinterpret_cast<std::complex<double>*>(a), &lda,
      reinterpret_cast<std::complex<double>*>(b), &ldb,
      reinterpret_cast<std::complex<double>*>(work), &lwork, info);
}

template<> void lapackGels<c10::complex<float>>(
    char trans, int m, int n, int nrhs,
    c10::complex<float> *a, int lda, c10::complex<float> *b, int ldb,
    c10::complex<float> *work, int lwork, int *info) {
  cgels_(&trans, &m, &n, &nrhs,
      reinterpret_cast<std::complex<float>*>(a), &lda,
      reinterpret_cast<std::complex<float>*>(b), &ldb,
      reinterpret_cast<std::complex<float>*>(work), &lwork, info);
}

template<> void lapackGels<double>(
    char trans, int m, int n, int nrhs,
    double *a, int lda, double *b, int ldb,
    double *work, int lwork, int *info) {
  dgels_(&trans, &m, &n, &nrhs,
      a, &lda, b, &ldb, work, &lwork, info);
}

template<> void lapackGels<float>(
    char trans, int m, int n, int nrhs,
    float *a, int lda, float *b, int ldb,
    float *work, int lwork, int *info) {
  sgels_(&trans, &m, &n, &nrhs,
      a, &lda, b, &ldb, work, &lwork, info);
}

template<> void lapackGelsd<c10::complex<double>, double>(
    int m, int n, int nrhs,
    c10::complex<double> *a, int lda, c10::complex<double> *b, int ldb,
    double *s, double rcond, int *rank,
    c10::complex<double> *work, int lwork,
    double *rwork, int *iwork, int *info) {
  zgelsd_(&m, &n, &nrhs,
      reinterpret_cast<std::complex<double>*>(a), &lda,
      reinterpret_cast<std::complex<double>*>(b), &ldb,
      s, &rcond, rank,
      reinterpret_cast<std::complex<double>*>(work), &lwork,
      rwork, iwork, info);
}

template<> void lapackGelsd<c10::complex<float>, float>(
    int m, int n, int nrhs,
    c10::complex<float> *a, int lda, c10::complex<float> *b, int ldb,
    float *s, float rcond, int *rank,
    c10::complex<float> *work, int lwork,
    float *rwork, int *iwork, int *info) {
  cgelsd_(&m, &n, &nrhs,
      reinterpret_cast<std::complex<float>*>(a), &lda,
      reinterpret_cast<std::complex<float>*>(b), &ldb,
      s, &rcond, rank,
      reinterpret_cast<std::complex<float>*>(work), &lwork,
      rwork, iwork, info);
}

template<> void lapackGelsd<double>(
    int m, int n, int nrhs,
    double *a, int lda, double *b, int ldb,
    double *s, double rcond, int *rank,
    double *work, int lwork,
    double *rwork, int *iwork, int *info) {
  dgelsd_(&m, &n, &nrhs,
      a, &lda, b, &ldb,
      s, &rcond, rank,
      work, &lwork, iwork, info);
}

template<> void lapackGelsd<float>(
    int m, int n, int nrhs,
    float *a, int lda, float *b, int ldb,
    float *s, float rcond, int *rank,
    float *work, int lwork,
    float *rwork, int *iwork, int *info) {
  sgelsd_(&m, &n, &nrhs,
      a, &lda, b, &ldb,
      s, &rcond, rank,
      work, &lwork, iwork, info);
}

template<> void lapackGelsy<c10::complex<double>, double>(
    int m, int n, int nrhs,
    c10::complex<double> *a, int lda, c10::complex<double> *b, int ldb,
    int *jpvt, double rcond, int *rank,
    c10::complex<double> *work, int lwork, double *rwork, int *info) {
  zgelsy_(&m, &n, &nrhs,
      reinterpret_cast<std::complex<double>*>(a), &lda,
      reinterpret_cast<std::complex<double>*>(b), &ldb,
      jpvt, &rcond, rank,
      reinterpret_cast<std::complex<double>*>(work), &lwork,
      rwork, info);
}

template<> void lapackGelsy<c10::complex<float>, float>(
    int m, int n, int nrhs,
    c10::complex<float> *a, int lda, c10::complex<float> *b, int ldb,
    int *jpvt, float rcond, int *rank,
    c10::complex<float> *work, int lwork, float *rwork, int *info) {
  cgelsy_(&m, &n, &nrhs,
      reinterpret_cast<std::complex<float>*>(a), &lda,
      reinterpret_cast<std::complex<float>*>(b), &ldb,
      jpvt, &rcond, rank,
      reinterpret_cast<std::complex<float>*>(work), &lwork,
      rwork, info);
}

template<> void lapackGelsy<double>(
    int m, int n, int nrhs,
    double *a, int lda, double *b, int ldb,
    int *jpvt, double rcond, int *rank,
    double *work, int lwork, double *rwork, int *info) {
  dgelsy_(&m, &n, &nrhs,
      a, &lda, b, &ldb,
      jpvt, &rcond, rank,
      work, &lwork, info);
}

template<> void lapackGelsy<float>(
    int m, int n, int nrhs,
    float *a, int lda, float *b, int ldb,
    int *jpvt, float rcond, int *rank,
    float *work, int lwork, float *rwork, int *info) {
  sgelsy_(&m, &n, &nrhs,
      a, &lda, b, &ldb,
      jpvt, &rcond, rank,
      work, &lwork, info);
}

template<> void lapackGelss<c10::complex<double>, double>(
    int m, int n, int nrhs,
    c10::complex<double> *a, int lda, c10::complex<double> *b, int ldb,
    double *s, double rcond, int *rank,
    c10::complex<double> *work, int lwork,
    double *rwork, int *info
    ) {
  zgelss_(&m, &n, &nrhs,
      reinterpret_cast<std::complex<double>*>(a), &lda,
      reinterpret_cast<std::complex<double>*>(b), &ldb,
      s, &rcond, rank,
      reinterpret_cast<std::complex<double>*>(work), &lwork,
      rwork, info);
}

template<> void lapackGelss<c10::complex<float>, float>(
    int m, int n, int nrhs,
    c10::complex<float> *a, int lda, c10::complex<float> *b, int ldb,
    float *s, float rcond, int *rank,
    c10::complex<float> *work, int lwork,
    float *rwork, int *info
    ) {
  cgelss_(&m, &n, &nrhs,
      reinterpret_cast<std::complex<float>*>(a), &lda,
      reinterpret_cast<std::complex<float>*>(b), &ldb,
      s, &rcond, rank,
      reinterpret_cast<std::complex<float>*>(work), &lwork,
      rwork, info);
}

template<> void lapackGelss<double>(
    int m, int n, int nrhs,
    double *a, int lda, double *b, int ldb,
    double *s, double rcond, int *rank,
    double *work, int lwork,
    double *rwork, int *info) {
  dgelss_(&m, &n, &nrhs,
      a, &lda, b, &ldb,
      s, &rcond, rank,
      work, &lwork, info);
}

template<> void lapackGelss<float>(
    int m, int n, int nrhs,
    float *a, int lda, float *b, int ldb,
    float *s, float rcond, int *rank,
    float *work, int lwork,
    float *rwork, int *info) {
  sgelss_(&m, &n, &nrhs,
      a, &lda, b, &ldb,
      s, &rcond, rank,
      work, &lwork, info);
}
#endif

// Below of the definitions of the functions operating on a batch that are going to be dispatched
// in the main helper functions for the linear algebra operations

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ solve ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<typename scalar_t>
static void apply_solve(Tensor& b, Tensor& A, std::vector<int64_t>& infos) {
#ifndef USE_LAPACK
  AT_ERROR("solve: LAPACK library not found in compilation");
#else
  auto A_data = A.data_ptr<scalar_t>();
  auto b_data = b.data_ptr<scalar_t>();
  auto A_mat_stride = matrixStride(A);
  auto b_mat_stride = matrixStride(b);
  auto batch_size = batchCount(A);
  auto n = A.size(-2);
  auto nrhs = b.size(-1);
  auto lda = std::max(int64_t{1}, n);

  auto ipiv = at::empty({n}, b.options().dtype(kInt));
  auto ipiv_data = ipiv.data_ptr<int>();

  int info;
  for (int64_t i = 0; i < batch_size; i++) {
    scalar_t* A_working_ptr = &A_data[i * A_mat_stride];
    scalar_t* b_working_ptr = &b_data[i * b_mat_stride];
    lapackSolve<scalar_t>(n, nrhs, A_working_ptr, lda, ipiv_data, b_working_ptr, lda, &info);
    infos[i] = info;
    if (info != 0) {
      return;
    }
  }
#endif
}

std::tuple<Tensor, Tensor> _solve_helper_cpu(const Tensor& self, const Tensor& A) {
  auto self_working_copy = cloneBatchedColumnMajor(self);
  auto A_working_copy = cloneBatchedColumnMajor(A);
  std::vector<int64_t> infos(batchCount(self), 0);
  AT_DISPATCH_FLOATING_AND_COMPLEX_TYPES(self.scalar_type(), "solve_cpu", [&]{
    apply_solve<scalar_t>(self_working_copy, A_working_copy, infos);
  });
  if (self.dim() > 2) {
    batchCheckErrors(infos, "solve_cpu");
  } else {
    singleCheckErrors(infos[0], "solve_cpu");
  }
  return std::tuple<Tensor, Tensor>(self_working_copy, A_working_copy);
}

// Supports arbitrary batch dimensions for self and A
std::tuple<Tensor,Tensor> solve(const Tensor& self, const Tensor& A) {
  TORCH_CHECK(self.dim() >= 2,
           "B should have at least 2 dimensions, but has ", self.dim(), " dimensions instead");
  TORCH_CHECK(A.dim() >= 2,
           "A should have at least 2 dimensions, but has ", A.dim(), " dimensions instead");
  Tensor self_broadcasted, A_broadcasted;
  std::tie(self_broadcasted, A_broadcasted) = _linalg_broadcast_batch_dims(self, A, "solve");
  return at::_solve_helper(self_broadcasted, A_broadcasted);
}

std::tuple<Tensor&,Tensor&> solve_out(Tensor& solution, Tensor& lu, const Tensor& self, const Tensor& A) {
  Tensor solution_tmp, lu_tmp;
  std::tie(solution_tmp, lu_tmp) = at::_solve_helper(self, A);
  solution.resize_as_(solution_tmp).copy_(solution_tmp);
  lu.resize_as_(lu_tmp).copy_(lu_tmp);
  return std::tuple<Tensor&, Tensor&>(solution, lu);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ inverse ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template <typename scalar_t>
static void apply_inverse(Tensor& self, std::vector<int64_t>& infos) {
#ifndef USE_LAPACK
  AT_ERROR("inverse: LAPACK library not found in compilation");
#else
  using value_t = typename c10::scalar_value_type<scalar_t>::type;
  auto self_data = self.data_ptr<scalar_t>();
  auto self_matrix_stride = matrixStride(self);
  auto batch_size = batchCount(self);
  auto n = self.size(-2);

  auto ipiv = at::empty({n}, self.options().dtype(kInt));
  auto ipiv_data = ipiv.data_ptr<int>();

  int info;
  // Run once, first to get the optimum work size
  // Since we deal with batches of matrices with the same dimensions, doing this outside
  // the loop saves (batch_size - 1) workspace queries which would provide the same result
  // and (batch_size - 1) calls to allocate and deallocate workspace using at::empty()
  int lwork = -1;
  scalar_t wkopt;
  lapackGetri<scalar_t>(n, self_data, n, ipiv_data, &wkopt, lwork, &info);
  lwork = static_cast<int>(real_impl<scalar_t, value_t>(wkopt));
  Tensor work = at::empty({lwork}, self.options());
  auto work_data = work.data_ptr<scalar_t>();

  for (int64_t i = 0; i < batch_size; i++) {
    scalar_t* self_working_ptr = &self_data[i * self_matrix_stride];
    lapackLu<scalar_t>(n, n, self_working_ptr, n, ipiv_data, &info);
    infos[i] = info;
    if (info != 0) {
      return;
    }

    // now compute the actual inverse
    lapackGetri<scalar_t>(n, self_working_ptr, n, ipiv_data, work_data, lwork, &info);
    infos[i] = info;
    if (info != 0) {
      return;
    }
  }
#endif
}

Tensor _inverse_helper_cpu(const Tensor& self) {
  std::vector<int64_t> infos(batchCount(self), 0);
  auto self_working_copy = cloneBatchedColumnMajor(self);
  AT_DISPATCH_FLOATING_AND_COMPLEX_TYPES(self.scalar_type(), "inverse_cpu", [&]{
    apply_inverse<scalar_t>(self_working_copy, infos);
  });
  if (self.dim() > 2) {
    batchCheckErrors(infos, "inverse_cpu");
  } else {
    singleCheckErrors(infos[0], "inverse_cpu");
  }
  return self_working_copy;
}

Tensor inverse(const Tensor &self) {
  if (self.numel() == 0) {
    return at::empty_like(self, LEGACY_CONTIGUOUS_MEMORY_FORMAT);
  }
  squareCheckInputs(self);
  return at::_inverse_helper(self);
}

Tensor& inverse_out(Tensor &result, const Tensor &self) {
  if (self.size(-1) == 0) {
    return result.resize_as_(self);
  }
  result.copy_(native::inverse(self));
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ cholesky_solve ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<typename scalar_t>
static void apply_cholesky_solve(Tensor& b, Tensor& A, bool upper, std::vector<int64_t>& infos) {
#ifndef USE_LAPACK
  AT_ERROR("cholesky_solve: LAPACK library not found in compilation");
#else
  char uplo = upper ? 'U' : 'L';

  auto A_data = A.data_ptr<scalar_t>();
  auto b_data = b.data_ptr<scalar_t>();
  auto A_mat_stride = matrixStride(A);
  auto b_mat_stride = matrixStride(b);
  auto batch_size = batchCount(A);
  auto n = A.size(-2);
  auto nrhs = b.size(-1);

  int info;
  for (int64_t i = 0; i < batch_size; i++) {
    scalar_t* A_working_ptr = &A_data[i * A_mat_stride];
    scalar_t* b_working_ptr = &b_data[i * b_mat_stride];
    lapackCholeskySolve<scalar_t>(uplo, n, nrhs, A_working_ptr, n, b_working_ptr, n, &info);
    infos[i] = info;
    if (info != 0) {
      return;
    }
  }
#endif
}

Tensor _cholesky_solve_helper_cpu(const Tensor& self, const Tensor& A, bool upper) {
  auto self_working_copy = cloneBatchedColumnMajor(self);
  auto A_working_copy = cloneBatchedColumnMajor(A);
  std::vector<int64_t> infos(batchCount(self), 0);
  AT_DISPATCH_FLOATING_AND_COMPLEX_TYPES(self.scalar_type(), "cholesky_solve_cpu", [&]{
    apply_cholesky_solve<scalar_t>(self_working_copy, A_working_copy, upper, infos);
  });
  if (self.dim() > 2) {
    batchCheckErrors(infos, "cholesky_solve_cpu");
  } else {
    singleCheckErrors(infos[0], "cholesky_solve_cpu");
  }
  return self_working_copy;
}

// Supports arbitrary batch dimensions for self and A
Tensor cholesky_solve(const Tensor& self, const Tensor& A, bool upper) {
  TORCH_CHECK(self.dim() >= 2,
           "b should have at least 2 dimensions, but has ", self.dim(), " dimensions instead");
  TORCH_CHECK(A.dim() >= 2,
           "u should have at least 2 dimensions, but has ", A.dim(), " dimensions instead");
  Tensor self_broadcasted, A_broadcasted;
  std::tie(self_broadcasted, A_broadcasted) = _linalg_broadcast_batch_dims(self, A, "cholesky_solve");
  return at::_cholesky_solve_helper(self_broadcasted, A_broadcasted, upper);
}

Tensor& cholesky_solve_out(Tensor& result, const Tensor& self, const Tensor& A, bool upper) {
  Tensor result_tmp;
  result_tmp = at::cholesky_solve(self, A, upper);
  result.resize_as_(result_tmp).copy_(result_tmp);
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ cholesky ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<typename scalar_t>
static void apply_cholesky(Tensor& self, bool upper, std::vector<int64_t>& infos) {
#ifndef USE_LAPACK
  AT_ERROR("cholesky: LAPACK library not found in compilation");
#else
  char uplo = upper ? 'U' : 'L';

  auto self_data = self.data_ptr<scalar_t>();
  auto self_matrix_stride = matrixStride(self);
  auto batch_size = batchCount(self);
  auto n = self.size(-2);
  auto lda = std::max<int64_t>(1, n);

  int info;
  for (int64_t i = 0; i < batch_size; i++) {
    scalar_t* self_working_ptr = &self_data[i * self_matrix_stride];
    lapackCholesky<scalar_t>(uplo, n, self_working_ptr, lda, &info);
    infos[i] = info;
    if (info != 0) {
      return;
    }
  }
#endif
}

Tensor _cholesky_helper_cpu(const Tensor& self, bool upper) {
  std::vector<int64_t> infos(batchCount(self), 0);
  auto self_working_copy = cloneBatchedColumnMajor(self);
  AT_DISPATCH_FLOATING_AND_COMPLEX_TYPES(self.scalar_type(), "cholesky_cpu", [&]{
    apply_cholesky<scalar_t>(self_working_copy, upper, infos);
  });
  if (self.dim() > 2) {
    batchCheckErrors(infos, "cholesky_cpu");
  } else {
    singleCheckErrors(infos[0], "cholesky_cpu");
  }
  return self_working_copy;
}

Tensor cholesky(const Tensor &self, bool upper) {
  if (self.size(-1) == 0) {
    return at::empty_like(self, LEGACY_CONTIGUOUS_MEMORY_FORMAT);
  }
  squareCheckInputs(self);

  auto raw_cholesky_output = at::_cholesky_helper(self, upper);
  if (upper) {
    return raw_cholesky_output.triu_();
  } else {
    return raw_cholesky_output.tril_();
  }
}

Tensor& cholesky_out(Tensor &result, const Tensor &self, bool upper) {
  if (self.size(-1) == 0) {
    return result.resize_as_(self);
  }
  result.copy_(native::cholesky(self, upper));
  return result;
}

Tensor linalg_cholesky(const Tensor &self) {
  squareCheckInputs(self);
  return at::_cholesky_helper(self, /*upper=*/false).tril_();
}

Tensor& linalg_cholesky_out(Tensor &result, const Tensor &self) {
  TORCH_CHECK(result.scalar_type() == self.scalar_type(),
    "result dtype ", result.scalar_type(), " does not match self dtype ", self.scalar_type());
  Tensor result_tmp = at::linalg_cholesky(self);
  at::native::resize_output(result, result_tmp.sizes());
  result.copy_(result_tmp);
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ lu ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<typename scalar_t>
static void apply_lu(Tensor& self, Tensor& pivots, Tensor& infos) {
#ifndef USE_LAPACK
  AT_ERROR("lu: LAPACK library not found in compilation");
#else
  auto self_data = self.data_ptr<scalar_t>();
  auto pivots_data = pivots.data_ptr<int>();
  auto infos_data = infos.data_ptr<int>();
  auto self_matrix_stride = matrixStride(self);
  auto pivots_matrix_stride = pivots.size(-1);
  auto batch_size = batchCount(self);
  auto m = self.size(-2);
  auto n = self.size(-1);

  for (int64_t i = 0; i < batch_size; i++) {
    scalar_t* self_working_ptr = &self_data[i * self_matrix_stride];
    int* pivots_working_ptr = &pivots_data[i * pivots_matrix_stride];
    int* infos_working_ptr = &infos_data[i];
    lapackLu<scalar_t>(m, n, self_working_ptr, m, pivots_working_ptr, infos_working_ptr);
  }
#endif
}

std::tuple<Tensor, Tensor, Tensor> _lu_with_info_cpu(const Tensor& self, bool pivot, bool check_errors) {
  TORCH_CHECK(pivot, "lu without pivoting is not implemented on the CPU");
  TORCH_CHECK(self.dim() >= 2,
           "expected tensor with 2 or more dimensions, got size: ", self.sizes(),
           " instead");
  auto m = self.size(-2);
  auto n = self.size(-1);
  auto req_size = self.sizes().vec();
  req_size.pop_back();
  req_size.back() = std::min(m, n);
  auto pivots_tensor = at::empty(req_size, self.options().dtype(kInt));
  req_size.pop_back();
  auto infos_tensor = at::zeros(req_size, self.options().dtype(kInt));

  Tensor self_working_copy;
  if (self.numel() == 0) {
    self_working_copy = at::empty_like(self, LEGACY_CONTIGUOUS_MEMORY_FORMAT);
  } else {
    self_working_copy = cloneBatchedColumnMajor(self);
    AT_DISPATCH_FLOATING_AND_COMPLEX_TYPES(self.scalar_type(), "lu_cpu", [&]{
      apply_lu<scalar_t>(self_working_copy, pivots_tensor, infos_tensor);
    });
  }
  if (check_errors) {
    if (self.dim() > 2) {
      batchCheckErrors(infos_tensor, "lu", /*allow_singular=*/true);
    } else {
      singleCheckErrors(infos_tensor.item<int64_t>(), "lu", /*allow_singular=*/true);
    }
  }
  return std::make_tuple(self_working_copy, pivots_tensor, infos_tensor);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ triangular_solve ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<typename scalar_t>
static void apply_triangular_solve(Tensor& b, Tensor& A, Tensor& infos, bool upper, bool transpose, bool unitriangular) {
#ifndef USE_LAPACK
  AT_ERROR("triangular_solve: LAPACK library not found in compilation");
#else
  char uplo = upper ? 'U' : 'L';
  char trans = transpose ? 'T' : 'N';
  char diag = unitriangular ? 'U' : 'N';

  auto A_data = A.data_ptr<scalar_t>();
  auto b_data = b.data_ptr<scalar_t>();
  auto A_mat_stride = matrixStride(A);
  auto b_mat_stride = matrixStride(b);
  auto batch_size = batchCount(A);
  auto n = A.size(-2);
  auto nrhs = b.size(-1);
  auto infos_data = infos.data_ptr<int>();

  for (int64_t i = 0; i < batch_size; i++) {
    scalar_t* A_working_ptr = &A_data[i * A_mat_stride];
    scalar_t* b_working_ptr = &b_data[i * b_mat_stride];
    int* infos_working_ptr = &infos_data[i];
    lapackTriangularSolve<scalar_t>(uplo, trans, diag, n, nrhs, A_working_ptr, n, b_working_ptr, n, infos_working_ptr);
  }
#endif
}

std::tuple<Tensor, Tensor> _triangular_solve_helper_cpu(const Tensor& self, const Tensor& A,
                                                        bool upper, bool transpose, bool unitriangular) {
  auto self_working_copy = cloneBatchedColumnMajor(self);
  auto A_working_copy = cloneBatchedColumnMajor(A);
  auto infos_tensor = at::empty(batchCount(self), self.options().dtype(kInt));
  AT_DISPATCH_FLOATING_AND_COMPLEX_TYPES(self.scalar_type(), "triangular_solve_cpu", [&]{
    apply_triangular_solve<scalar_t>(self_working_copy, A_working_copy, infos_tensor, upper, transpose, unitriangular);
  });
  if (self.dim() > 2) {
    batchCheckErrors(infos_tensor, "triangular_solve_cpu");
  } else {
    singleCheckErrors(infos_tensor.item<int64_t>(), "triangular_solve_cpu");
  }
  return std::tuple<Tensor, Tensor>(self_working_copy, A_working_copy);
}

// Supports arbitrary batch dimensions for self and A
std::tuple<Tensor, Tensor> triangular_solve(const Tensor& self, const Tensor& A,
                                            bool upper, bool transpose, bool unitriangular) {
  TORCH_CHECK(self.dim() >= 2,
           "b should have at least 2 dimensions, but has ", self.dim(), " dimensions instead");
  TORCH_CHECK(A.dim() >= 2,
           "u should have at least 2 dimensions, but has ", A.dim(), " dimensions instead");
  Tensor self_broadcasted, A_broadcasted;
  std::tie(self_broadcasted, A_broadcasted) = _linalg_broadcast_batch_dims(self, A, "triangular_solve");
  return at::_triangular_solve_helper(self_broadcasted, A_broadcasted, upper, transpose, unitriangular);
}

std::tuple<Tensor&, Tensor&> triangular_solve_out(Tensor& result, Tensor& clone_A, const Tensor& self, const Tensor& A,
                                                  bool upper, bool transpose, bool unitriangular) {
  Tensor result_tmp, clone_A_tmp;
  std::tie(result_tmp, clone_A_tmp) = at::_triangular_solve_helper(self, A, upper, transpose, unitriangular);
  result.resize_as_(result_tmp).copy_(result_tmp);
  clone_A.resize_as_(clone_A_tmp).copy_(clone_A_tmp);
  return std::tuple<Tensor&, Tensor&>(result, clone_A);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ qr ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<typename scalar_t>
static void apply_geqrf(Tensor& self, Tensor& tau, int64_t m, int64_t n,
                        std::vector<int64_t>& infos) {
#ifndef USE_LAPACK
  AT_ERROR("qr: LAPACK library not found in compilation");
#else
  using value_t = typename c10::scalar_value_type<scalar_t>::type;
  auto self_data = self.data_ptr<scalar_t>();
  auto tau_data = tau.data_ptr<scalar_t>();
  auto self_matrix_stride = matrixStride(self);
  auto tau_stride = tau.size(-1);
  auto batch_size = batchCount(self);

  int info;
  // Run once, first to get the optimum work size.
  // Since we deal with batches of matrices with the same dimensions, doing this outside
  // the loop saves (batch_size - 1) workspace queries which would provide the same result
  // and (batch_size - 1) calls to allocate and deallocate workspace using at::empty()
  int lwork = -1;
  scalar_t wkopt;
  lapackGeqrf<scalar_t>(m, n, self_data, m, tau_data, &wkopt, lwork, &info);
  lwork = static_cast<int>(real_impl<scalar_t, value_t>(wkopt));
  Tensor work = at::empty({lwork}, self.options());

  for (int64_t i = 0; i < batch_size; i++) {
    scalar_t* self_working_ptr = &self_data[i * self_matrix_stride];
    scalar_t* tau_working_ptr = &tau_data[i * tau_stride];

    // now compute the actual R and TAU
    lapackGeqrf<scalar_t>(m, n, self_working_ptr, m, tau_working_ptr, work.data_ptr<scalar_t>(), lwork, &info);
    infos[i] = info;
    if (info != 0) {
      return;
    }
  }
#endif
}

template<typename scalar_t>
static void apply_orgqr(Tensor& self, const Tensor& tau, int64_t m, int64_t n_columns,
                        int64_t k, std::vector<int64_t>& infos) {
#ifndef USE_LAPACK
  AT_ERROR("qr: LAPACK library not found in compilation");
#else
  using value_t = typename c10::scalar_value_type<scalar_t>::type;
  auto self_data = self.data_ptr<scalar_t>();
  auto tau_data = tau.data_ptr<scalar_t>();
  auto self_matrix_stride = matrixStride(self);
  auto tau_stride = tau.size(-1);
  auto batch_size = batchCount(self);

  int info;
  // Run once, first to get the optimum work size.
  // Since we deal with batches of matrices with the same dimensions, doing this outside
  // the loop saves (batch_size - 1) workspace queries which would provide the same result
  // and (batch_size - 1) calls to allocate and deallocate workspace using at::empty()
  int lwork = -1;
  scalar_t wkopt;
  lapackOrgqr<scalar_t>(m, n_columns, k, self_data, m, tau_data, &wkopt, lwork, &info);
  lwork = static_cast<int>(real_impl<scalar_t, value_t>(wkopt));
  Tensor work = at::empty({lwork}, self.options());

  for (int64_t i = 0; i < batch_size; i++) {
    scalar_t* self_working_ptr = &self_data[i * self_matrix_stride];
    scalar_t* tau_working_ptr = &tau_data[i * tau_stride];

    // now compute the actual Q
    lapackOrgqr<scalar_t>(m, n_columns, k, self_working_ptr, m, tau_working_ptr, work.data_ptr<scalar_t>(), lwork, &info);
    infos[i] = info;
    if (info != 0) {
      return;
    }
  }
#endif
}

std::tuple<Tensor, Tensor> _qr_helper_cpu(const Tensor& self, bool some) {
  std::vector<int64_t> infos(batchCount(self), 0);
  int64_t m = self.size(-2), n = self.size(-1);

  // Setup inputs for apply_geqrf
  auto self_sizes = self.sizes().vec();
  self_sizes.pop_back();
  self_sizes[self.dim() - 2] = std::min(m, n);
  auto tau_working_copy = at::empty(self_sizes, self.options());
  Tensor q_working_copy;

  // Setup input geometry for apply_orgqr
  std::vector<int64_t> q_sizes, q_strides;
  int64_t n_columns_q;
  Tensor R;
  std::tie(q_sizes, q_strides, n_columns_q) = _compute_geometry_for_Q(self, some);

  // If there are no elements, then we simply return a pair of tensors of required dimensions
  if (self.numel() == 0) {
    // Fix the number of columns of q appropriately
    q_sizes[self.dim() - 1] = n_columns_q;
    q_working_copy = at::eye(q_sizes[self.dim() - 2], q_sizes[self.dim() - 1], self.options());
    q_working_copy = q_working_copy.expand_as(q_working_copy);

    // We repurpose the same q_sizes for R
    // Fix the number of rows and columns of q_working_copy appropriately
    q_sizes[self.dim() - 1] = n;
    q_sizes[self.dim() - 2] = n_columns_q;
    R = at::empty(q_sizes, self.options());
    return std::make_tuple(q_working_copy, R);
  }

  // First perform GEQRF for R and TAU (the elementary reflectors)
  // We will need to generate R from the upper triangular matrix from the
  // matrix input to GEQRF.
  q_working_copy = at::empty_strided(q_sizes, q_strides, self.options());
  q_working_copy.narrow(-1, 0, n).copy_(self);

  AT_DISPATCH_FLOATING_AND_COMPLEX_TYPES(self.scalar_type(), "qr_cpu", [&]{
    apply_geqrf<scalar_t>(q_working_copy, tau_working_copy, m, n, infos);
  });
  if (self.dim() > 2) {
    batchCheckErrors(infos, "qr_cpu");
  } else {
    singleCheckErrors(infos[0], "qr_cpu");
  }

  R = q_working_copy.slice(-2, 0, n_columns_q).slice(-1, 0, n).triu();

  // Next perform ORGQR for Q using the results (both raw R and TAU) from GEQRF
  AT_DISPATCH_FLOATING_AND_COMPLEX_TYPES(self.scalar_type(), "qr_cpu", [&]{
    apply_orgqr<scalar_t>(q_working_copy, tau_working_copy, m, n_columns_q, std::min(m, n), infos);
  });
  if (self.dim() > 2) {
    batchCheckErrors(infos, "qr_cpu");
  } else {
    singleCheckErrors(infos[0], "qr_cpu");
  }
  return std::make_tuple(q_working_copy.narrow(-1, 0, n_columns_q), R);
}

std::tuple<Tensor,Tensor> qr(const Tensor& self, bool some) {
  TORCH_CHECK(self.dim() >= 2,
              "self should have at least 2 dimensions, but has ", self.dim(), " dimensions instead");
  return at::_qr_helper(self, some);
}

std::tuple<Tensor&,Tensor&> qr_out(Tensor& Q, Tensor& R, const Tensor& self, bool some) {
  TORCH_CHECK(self.dim() >= 2,
              "self should have at least 2 dimensions, but has ", self.dim(), " dimensions instead");
  Tensor Q_tmp, R_tmp;
  std::tie(Q_tmp, R_tmp) = at::_qr_helper(self, some);
  Q.resize_as_(Q_tmp).copy_(Q_tmp);
  R.resize_as_(R_tmp).copy_(R_tmp);
  return std::tuple<Tensor&, Tensor&>(Q, R);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ syevd ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// This function computes eigenvalues 'w' and eigenvectors 'v' of the input that is stored initially in 'v'
// The computation is done in-place: 'v' stores the input and will be overriden, 'w' should be an allocated empty array
// compute_v controls whether eigenvectors should be computed
// uplo_str controls the portion of input matrix to consider in computations, allowed values are "u", "U", "l", "L"
// infos is used to store information for possible checks for error
// This function doesn't do any error checks and it's assumed that every argument is valid
template <typename scalar_t>
static void apply_syevd(Tensor& w, Tensor& v, bool compute_v, const std::string& uplo_str, std::vector<int64_t>& infos) {
#ifndef USE_LAPACK
  AT_ERROR("syevd: LAPACK library not found in compilation");
#else
  using value_t = typename c10::scalar_value_type<scalar_t>::type;

  auto v_data = v.data_ptr<scalar_t>();
  auto w_data = w.data_ptr<value_t>();
  auto v_matrix_stride = matrixStride(v);
  auto w_stride = w.size(-1);
  auto batch_size = batchCount(v);
  auto n = v.size(-1);
  auto lda = std::max(int64_t{1}, n);

  // NumPy allows lowercase input for UPLO argument
  // It is assumed that uplo_str is either "U" or "L"
  char uplo = std::toupper(uplo_str[0]);
  char jobz = compute_v ? 'V' : 'N';

  // Using 'int' instead of int32_t or int64_t is consistent with the current LAPACK interface
  // It really should be changed in the future to something like lapack_int that depends on the specific LAPACK library that is linked
  // or switch to supporting only 64-bit indexing by default.
  int info;
  int lwork = -1;
  int lrwork = -1;
  int liwork = -1;
  scalar_t work_query;
  value_t rwork_query;
  int iwork_query;

  // Run lapackSyevd once, first to get the optimum work size.
  // Since we deal with batches of matrices with the same dimensions, doing this outside
  // the main loop saves (batch_size - 1) workspace queries which would provide the same result
  // and (batch_size - 1) calls to allocate and deallocate workspace using at::empty()
  lapackSyevd<scalar_t, value_t>(jobz, uplo, n, v_data, lda, w_data, &work_query, lwork, &rwork_query, lrwork, &iwork_query, liwork, &info);

  lwork = std::max<int>(1, real_impl<scalar_t, value_t>(work_query));
  Tensor work = at::empty({lwork}, v.options());
  liwork = std::max<int>(1, iwork_query);
  Tensor iwork = at::empty({liwork}, at::kInt);

  Tensor rwork;
  value_t* rwork_data = nullptr;
  if (isComplexType(at::typeMetaToScalarType(v.dtype()))) {
    lrwork = std::max<int>(1, rwork_query);
    rwork = at::empty({lrwork}, w.options());
    rwork_data = rwork.data_ptr<value_t>();
  }

  // Now call lapackSyevd for each matrix in the batched input
  for (auto i = decltype(batch_size){0}; i < batch_size; i++) {
    scalar_t* v_working_ptr = &v_data[i * v_matrix_stride];
    value_t* w_working_ptr = &w_data[i * w_stride];
    lapackSyevd<scalar_t, value_t>(jobz, uplo, n, v_working_ptr, lda, w_working_ptr, work.data_ptr<scalar_t>(), lwork, rwork_data, lrwork, iwork.data_ptr<int>(), liwork, &info);
    infos[i] = info;
    // The current behaviour for Linear Algebra functions to raise an error if something goes wrong or input doesn't satisfy some requirement
    // therefore return early since further computations will be wasted anyway
    if (info != 0) {
      return;
    }
  }
#endif
}

// This function computes eigenvalues 'w' and eigenvectors 'v' of the tensor 'self'
// compute_eigenvectors controls whether eigenvectors should be computed
// uplo controls the portion of input matrix to consider in computations, allowed values are "u", "U", "l", "L"
// This function prepares correct input for 'apply_syevd' and checks for possible errors using 'infos'
std::tuple<Tensor, Tensor> _syevd_helper_cpu(const Tensor& self, bool compute_eigenvectors, std::string uplo) {
  std::vector<int64_t> infos(batchCount(self), 0);

  auto self_sizes = self.sizes().vec();
  self_sizes.pop_back();
  ScalarType dtype = toValueType(typeMetaToScalarType(self.dtype()));
  auto eigvals = at::empty(self_sizes, self.options().dtype(dtype));

  auto eigvecs = cloneBatchedColumnMajor(self);
  AT_DISPATCH_FLOATING_AND_COMPLEX_TYPES(self.scalar_type(), "syevd_cpu", [&]{
    apply_syevd<scalar_t>(eigvals, eigvecs, compute_eigenvectors, uplo, infos);
  });

  if (self.dim() > 2) {
    batchCheckErrors(infos, "syevd_cpu");
  } else {
    singleCheckErrors(infos[0], "syevd_cpu");
  }
  if (compute_eigenvectors) {
    return std::tuple<Tensor, Tensor>(eigvals, eigvecs);
  } else {
    return std::tuple<Tensor, Tensor>(eigvals, at::empty({0}, self.options()));
  }
}

std::tuple<Tensor, Tensor> linalg_eigh(const Tensor& self, std::string uplo) {
  squareCheckInputs(self);
  checkUplo(uplo);
  return at::_syevd_helper(self, /*compute_eigenvectors=*/true, uplo);
}

// TODO: it's possible to make the _out variant to be a primal function and implement linalg_eigh on top of _out
// TODO: implement _out variant avoiding copy and using already allocated storage directly
std::tuple<Tensor&, Tensor&> linalg_eigh_out(Tensor& eigvals, Tensor& eigvecs, const Tensor& self, std::string uplo) {
  TORCH_CHECK(eigvecs.scalar_type() == self.scalar_type(),
    "eigvecs dtype ", eigvecs.scalar_type(), " does not match self dtype ", self.scalar_type());
  ScalarType real_dtype = toValueType(typeMetaToScalarType(self.dtype()));
  TORCH_CHECK(eigvals.scalar_type() == real_dtype,
    "eigvals dtype ", eigvals.scalar_type(), " does not match self dtype ", real_dtype);

  Tensor eigvals_tmp, eigvecs_tmp;
  std::tie(eigvals_tmp, eigvecs_tmp) = at::linalg_eigh(self, uplo);

  at::native::resize_output(eigvals, eigvals_tmp.sizes());
  eigvals.copy_(eigvals_tmp);
  at::native::resize_output(eigvecs, eigvecs_tmp.sizes());
  eigvecs.copy_(eigvecs_tmp);

  return std::tuple<Tensor&, Tensor&>(eigvals, eigvecs);
}

Tensor linalg_eigvalsh(const Tensor& self, std::string uplo) {
  squareCheckInputs(self);
  checkUplo(uplo);
  Tensor eigvals, eigvecs;
  std::tie(eigvals, eigvecs) = at::_syevd_helper(self, /*compute_eigenvectors=*/false, uplo);
  return eigvals;
}

// TODO: it's possible to make the _out variant to be a primal function and implement linalg_eigvalsh on top of _out
// TODO: implement _out variant avoiding copy and using already allocated storage directly
Tensor& linalg_eigvalsh_out(Tensor& result, const Tensor& self, std::string uplo) {
  ScalarType real_dtype = toValueType(typeMetaToScalarType(self.dtype()));
  TORCH_CHECK(result.scalar_type() == real_dtype,
    "result dtype ", result.scalar_type(), " does not match self dtype ", real_dtype);

  Tensor result_tmp = at::linalg_eigvalsh(self, uplo);

  at::native::resize_output(result, result_tmp.sizes());
  result.copy_(result_tmp);

  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ symeig ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template <typename scalar_t>
static void apply_symeig(Tensor& self, Tensor& eigvals, bool eigenvectors, bool upper, std::vector<int64_t>& infos) {
#ifndef USE_LAPACK
  AT_ERROR("symeig: LAPACK library not found in compilation");
#else
  using value_t = typename c10::scalar_value_type<scalar_t>::type;
  auto self_data = self.data_ptr<scalar_t>();
  auto eigvals_data = eigvals.data_ptr<value_t>();
  auto self_matrix_stride = matrixStride(self);
  auto eigvals_stride = eigvals.size(-1);
  auto batch_size = batchCount(self);
  auto n = self.size(-1);

  char uplo = upper ? 'U' : 'L';
  char jobz = eigenvectors ? 'V' : 'N';

  int info;
  // Run once, first to get the optimum work size.
  // Since we deal with batches of matrices with the same dimensions, doing this outside
  // the loop saves (batch_size - 1) workspace queries which would provide the same result
  // and (batch_size - 1) calls to allocate and deallocate workspace using at::empty()
  int lwork = -1;
  scalar_t wkopt;

  Tensor rwork;
  value_t* rwork_data = nullptr;
  if (isComplexType(at::typeMetaToScalarType(self.dtype()))) {
    int64_t lrwork = std::max(int64_t(1), 3 * n - 2);
    ScalarType dtype = toValueType(typeMetaToScalarType(self.dtype()));
    rwork = at::empty({lrwork}, self.options().dtype(dtype));
    rwork_data = rwork.data_ptr<value_t>();
  }

  lapackSymeig<scalar_t, value_t>(jobz, uplo, n, self_data, n, eigvals_data, &wkopt, lwork, rwork_data, &info);
  lwork = static_cast<int>(real_impl<scalar_t, value_t>(wkopt));
  Tensor work = at::empty({lwork}, self.options());

  for (int64_t i = 0; i < batch_size; i++) {
    scalar_t* self_working_ptr = &self_data[i * self_matrix_stride];
    value_t* eigvals_working_ptr = &eigvals_data[i * eigvals_stride];

    // now compute the eigenvalues and the eigenvectors (optionally)
    lapackSymeig<scalar_t, value_t>(jobz, uplo, n, self_working_ptr, n, eigvals_working_ptr, work.data_ptr<scalar_t>(), lwork, rwork_data, &info);
    infos[i] = info;
    if (info != 0) {
      return;
    }
  }
#endif
}

std::tuple<Tensor, Tensor> _symeig_helper_cpu(const Tensor& self, bool eigenvectors, bool upper) {
  std::vector<int64_t> infos(batchCount(self), 0);

  auto self_sizes = self.sizes().vec();
  self_sizes.pop_back();
  ScalarType dtype = toValueType(typeMetaToScalarType(self.dtype()));
  auto eigvals = at::empty(self_sizes, self.options().dtype(dtype));

  if (self.numel() == 0) {
    return std::tuple<Tensor, Tensor>(eigvals, at::empty_like(self, LEGACY_CONTIGUOUS_MEMORY_FORMAT));
  }

  auto self_working_copy = cloneBatchedColumnMajor(self);
  AT_DISPATCH_FLOATING_AND_COMPLEX_TYPES(self.scalar_type(), "symeig_cpu", [&]{
    apply_symeig<scalar_t>(self_working_copy, eigvals, eigenvectors, upper, infos);
  });

  if (self.dim() > 2) {
    batchCheckErrors(infos, "symeig_cpu");
  } else {
    singleCheckErrors(infos[0], "symeig_cpu");
  }
  if (eigenvectors) {
    return std::tuple<Tensor, Tensor>(eigvals, self_working_copy);
  } else {
    return std::tuple<Tensor, Tensor>(eigvals, at::empty({0}, self.options()));
  }
}

std::tuple<Tensor, Tensor> symeig(const Tensor& self, bool eigenvectors, bool upper) {
  squareCheckInputs(self);
  return at::_symeig_helper(self, eigenvectors, upper);
}

std::tuple<Tensor&, Tensor&> symeig_out(Tensor& vals, Tensor& vecs, const Tensor& self, bool eigenvectors, bool upper) {
  squareCheckInputs(self);
  Tensor vals_tmp, vecs_tmp;
  std::tie(vals_tmp, vecs_tmp) = at::_symeig_helper(self, eigenvectors, upper);
  vals.resize_as_(vals_tmp).copy_(vals_tmp);
  vecs.resize_as_(vecs_tmp).copy_(vecs_tmp);
  return std::tuple<Tensor&, Tensor&>(vals, vecs);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ svd ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template <typename scalar_t>
static void apply_svd(Tensor& self, Tensor& U, Tensor& S, Tensor& VT,
                      char jobz, std::vector<int64_t>& infos) {
#ifndef USE_LAPACK
  AT_ERROR("svd: LAPACK library not found in compilation");
#else
  using value_t = typename c10::scalar_value_type<scalar_t>::type;
  auto self_data = self.data_ptr<scalar_t>();
  auto U_data = U.data_ptr<scalar_t>();
  auto S_data = S.data_ptr<value_t>();
  auto VT_data = VT.data_ptr<scalar_t>();
  auto self_stride = matrixStride(self);
  auto U_stride = matrixStride(U);
  auto S_stride = S.size(-1);
  auto VT_stride = matrixStride(VT);
  auto batchsize = batchCount(self);

  int info;
  auto m = self.size(-2);
  auto n = self.size(-1);
  auto mn = std::min(m, n);
  Tensor iwork = at::empty({8 * mn}, at::kInt);
  auto iwork_data = iwork.data_ptr<int>();
  Tensor rwork;
  value_t* rwork_data = nullptr;
  if (isComplexType(at::typeMetaToScalarType(self.dtype()))) {
    auto lrwork  = computeLRWorkDim(jobz, m, n);
    // rwork is an array of floats or doubles depending on the type
    rwork = at::empty({std::max(int64_t(1), lrwork)}, at::typeMetaToScalarType(S.dtype()));
    rwork_data = rwork.data_ptr<value_t>();
  }

  // Run once, first to get the optimum work size.
  // Since we deal with batches of matrices with the same dimensions, doing this outside
  // the loop saves (batch_size - 1) workspace queries which would provide the same result
  // and (batch_size - 1) calls to allocate and deallocate workspace using at::empty()
  int lwork = -1;
  scalar_t wkopt;
  lapackSvd<scalar_t, value_t>(jobz, m, n, self_data, m, S_data, U_data, m, VT_data, n, &wkopt, lwork, rwork_data, iwork_data, &info);
  lwork = static_cast<int>(real_impl<scalar_t, value_t>(wkopt));
  Tensor work = at::empty({lwork}, self.options());
  auto work_data = work.data_ptr<scalar_t>();

  for (int64_t i = 0; i < batchsize; i++) {
    scalar_t* self_working_ptr = &self_data[i * self_stride];
    value_t* S_working_ptr = &S_data[i * S_stride];
    scalar_t* U_working_ptr = &U_data[i * U_stride];
    scalar_t* VT_working_ptr = &VT_data[i * VT_stride];

    // Compute S, U (optionally) and VT (optionally)
    lapackSvd<scalar_t, value_t>(jobz, m, n, self_working_ptr, m,
                        S_working_ptr, U_working_ptr, m, VT_working_ptr, n, work_data, lwork, rwork_data, iwork_data, &info);
    infos[i] = info;
    if (info != 0) {
      return;
    }
  }
#endif
}

std::tuple<Tensor, Tensor, Tensor> _svd_helper_cpu(const Tensor& self, bool some, bool compute_uv) {
  std::vector<int64_t> infos(batchCount(self), 0);
  int64_t m = self.size(-2), n = self.size(-1);
  int64_t k = std::min(m, n);

  char jobz = compute_uv ? (some ? 'S' : 'A') : 'N';

  Tensor U_working_copy, S_working_copy, VT_working_copy;
  std::tie(U_working_copy, S_working_copy, VT_working_copy) = _create_U_S_VT(self, some, compute_uv);

  if (self.numel() > 0) {
    auto self_working_copy = cloneBatchedColumnMajor(self);

    AT_DISPATCH_FLOATING_AND_COMPLEX_TYPES(self.scalar_type(), "svd_cpu", [&]{
      apply_svd<scalar_t>(self_working_copy, U_working_copy, S_working_copy, VT_working_copy, jobz, infos);
    });

    if (self.dim() > 2) {
      batchCheckErrors(infos, "svd_cpu");
    } else {
      singleCheckErrors(infos[0], "svd_cpu");
    }

    if (compute_uv) {
      if (some) {
        VT_working_copy = VT_working_copy.narrow(-1, 0, k);
      }
    } else {
      VT_working_copy.zero_();
      U_working_copy.zero_();
    }
  } else {
    U_working_copy.zero_();
    VT_working_copy.zero_();
  }
  return std::make_tuple(U_working_copy, S_working_copy, VT_working_copy);
}

std::tuple<Tensor, Tensor, Tensor> svd(const Tensor& self, bool some, bool compute_uv) {
  TORCH_CHECK(self.dim() >= 2,
              "self should have at least 2 dimensions, but has ", self.dim(), " dimensions instead");
  return at::_svd_helper(self, some, compute_uv);
}

std::tuple<Tensor&, Tensor&, Tensor&> svd_out(Tensor& U, Tensor& S, Tensor& VT,
                                              const Tensor& self, bool some, bool compute_uv) {
  TORCH_CHECK(self.dim() >= 2,
              "self should have at least 2 dimensions, but has ", self.dim(), " dimensions instead");
  Tensor U_tmp, S_tmp, VT_tmp;
  std::tie(U_tmp, S_tmp, VT_tmp) = at::_svd_helper(self, some, compute_uv);
  U.resize_as_(U_tmp).copy_(U_tmp);
  S.resize_as_(S_tmp).copy_(S_tmp);
  VT.resize_as_(VT_tmp).copy_(VT_tmp);
  return std::tuple<Tensor&, Tensor&, Tensor&>(U, S, VT);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ lstsq ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifdef USE_LAPACK
template<class scalar_t, class value_t, class func_t>
struct LapackLstsqHelper {
  using self_type = LapackLstsqHelper;

  // we use `driver_type` to decide how to initialize
  // relevant to specific drivers parameters
  LapackLstsqDriver driver_type;
  func_t driver;

  bool is_complex;
  at::ScalarType scalar_type;
  IntArrayRef batch_shape;
  // the strides below store the offsets to different lstsq problems in a batch
  int64_t a_stride;
  int64_t b_stride;
  int64_t s_stride;

  // variables below correspond to LAPACK inputs.
  // for more information check the LAPACK documentation on
  // `?gels`, `?gelsy`, `?gelsd`, `?gelss`
  char trans;
  int m;
  int n;
  int nrhs;
  scalar_t* a_working_ptr = nullptr;
  int lda;
  scalar_t* b_working_ptr = nullptr;
  int ldb;
  Tensor work;
  scalar_t work_opt; // used to decide the opt `work` size with lwork=-1
  scalar_t* work_ptr = &work_opt;
  int lwork = -1; // default value to decide the opt size for workspace arrays
  int info = 0;
  Tensor jpvt;
  int* jpvt_ptr = nullptr;
  value_t rcond;
  Tensor rank;
  int rank_opt;
  int64_t* rank_working_ptr = nullptr;
  Tensor rwork;
  value_t rwork_opt; // used to decide the opt `rwork` size with lwork=-1
  value_t* rwork_ptr = &rwork_opt;
  Tensor s;
  value_t* s_working_ptr = nullptr;
  Tensor iwork;
  int iwork_opt; // used to decide the opt `iwork` size with lwork=-1
  int* iwork_ptr = &iwork_opt;

  LapackLstsqHelper(LapackLstsqDriver driver_type, func_t driver)
    : driver_type{driver_type}, driver{driver}
  {}

  self_type& set_trans(char trans) { this->trans = trans; return *this; }
  self_type& set_m(int m) { this->m = m; return *this; }
  self_type& set_n(int n) { this->n = n; return *this; }
  self_type& set_nrhs(int nrhs) { this->nrhs = nrhs; return *this; }
  self_type& set_a(const Tensor& a) {
    this->a_working_ptr = a.data_ptr<scalar_t>();
    this->scalar_type = a.scalar_type();
    this->is_complex = a.is_complex();
    // `a` is persistent, should be safe to store its properties in references.
    this->batch_shape = IntArrayRef(a.sizes().data(), a.dim() - 2);
    this->a_stride = matrixStride(a);
    return *this;
  }
  self_type& set_lda(int lda) { this->lda = lda; return *this; }
  self_type& set_b(const Tensor& b) {
    this->b_working_ptr = b.data_ptr<scalar_t>();
    this->b_stride = matrixStride(b);
    return *this;
  }
  self_type& set_ldb(int ldb) { this->ldb = ldb; return *this; }
  self_type& set_work() {
    lwork = static_cast<int>(real_impl<scalar_t, value_t>(work_opt));
    work = at::empty({lwork}, scalar_type);
    work_ptr = work.data_ptr<scalar_t>();
    return *this;
  }
  self_type& set_jpvt() {
    // handle `jpvt` workspace array (relevant for `?gelsy` which uses
    // a QR factorization with column pivoting).
    if (LapackLstsqDriver::Gelsy == driver_type) {
      jpvt = at::empty({std::max<int64_t>(1, n)}, at::kInt);
      jpvt_ptr = jpvt.data_ptr<int>();
    }
    return *this;
  }
  self_type& set_rcond(double cond) { this->rcond = static_cast<value_t>(cond); return *this; }
  self_type& set_rank() {
    // only `?gels` is not rank-revealing
    if (LapackLstsqDriver::Gels != driver_type) {
      if (!batch_shape.size()) {
        rank = at::empty({1}, at::kLong);
      }
      else {
        rank = at::empty(batch_shape, at::kLong);
      }
      rank_working_ptr = rank.data_ptr<int64_t>();
    }
    return *this;
  }
  self_type& set_rwork() {
    // `rwork` only makes sense for complex flavors and
    // `?gelsy`, `?gelsd` and `?gelss` drivers
    if (!this->is_complex || LapackLstsqDriver::Gels == driver_type) {
      return *this;
    }

    int64_t rwork_len;
    switch (this->driver_type) {
      case LapackLstsqDriver::Gelsy:
        rwork_len = std::max<int64_t>(1, 2 * n);
        break;
      case LapackLstsqDriver::Gelss:
        rwork_len = std::max<int64_t>(1, 5 * std::min(m, n));
        break;
      // case LapackLstsqDriver::Gelsd:
      default:
        rwork_len = static_cast<int>(rwork_opt);
    }
    rwork = at::empty({rwork_len}, c10::toValueType(scalar_type));
    rwork_ptr = rwork.data_ptr<value_t>();
    return *this;
  }
  self_type& set_s() {
    // `?gelsd` and `?gelss` are SVD-based
    // and we can extract singular values from them.
    if (LapackLstsqDriver::Gelsd == driver_type
      || LapackLstsqDriver::Gelss == driver_type) {
      auto s_shape = batch_shape.vec();
      s_shape.push_back(std::min(m, n));
      s = at::empty(s_shape, c10::toValueType(scalar_type));
      s_working_ptr = s.data_ptr<value_t>();
      s_stride = s.size(-1);
    }
    return *this;
  }
  self_type& set_iwork() {
    // handle `iwork` workspace array (relevant only for `?gelsd`)
    if (LapackLstsqDriver::Gelsd == driver_type) {
      iwork = at::empty({iwork_opt}, at::kInt);
      iwork_ptr = iwork.data_ptr<int>();
    }
    return *this;
  }

  self_type& call_driver() {
    driver(trans, m, n, nrhs,
      a_working_ptr, lda,
      b_working_ptr, ldb,
      work_ptr, lwork,
      &info,
      jpvt_ptr,
      rcond,
      &rank_opt,
      rwork_ptr,
      s_working_ptr,
      iwork_ptr);
    // we want the output `rank` Tensor to be of type int64_t,
    // however LAPACK accepts int. That is why we use an integer
    // variable that then gets promoted and written into `rank`.
    // We use this approach over a tensor cast for better performance.
    if (rank_working_ptr) {
      *rank_working_ptr = static_cast<int64_t>(rank_opt);
    }
    return *this;
  }

  self_type& next() {
    // advance to the next problem in a batch
    a_working_ptr += a_stride;
    b_working_ptr += b_stride;
    rank_working_ptr = rank_working_ptr ? rank_working_ptr + 1 : nullptr;
    s_working_ptr = s_working_ptr ? s_working_ptr + s_stride : nullptr;
    return *this;
  }
};

// we use `enum class LapackLstsqDriver` as keys in an unordered_map.
// Clang5 and Gcc5 do not support std::hash for enum classes, hence
// we provide our own hash function.
struct LapackLstsqDriverHash {
  std::size_t operator()(const LapackLstsqDriver& driver) const {
    return static_cast<std::size_t>(driver);
  }
};
#endif

std::tuple<Tensor, Tensor, Tensor> _lstsq_helper_cpu(
    const Tensor& a, const Tensor& b, double cond, std::string driver_name) {
#ifndef USE_LAPACK
  AT_ERROR("linalg.lstsq: LAPACK library not found in compilation");
#else
  std::vector<int64_t> infos(batchCount(a), 0);

  static auto driver_string_to_enum = std::unordered_map<std::string, LapackLstsqDriver>({
    {"gels", LapackLstsqDriver::Gels},
    {"gelsy", LapackLstsqDriver::Gelsy},
    {"gelsd", LapackLstsqDriver::Gelsd},
    {"gelss", LapackLstsqDriver::Gelss}
  });
  auto driver_enum = driver_string_to_enum[driver_name];

  Tensor rank;
  Tensor singular_values;

  AT_DISPATCH_FLOATING_AND_COMPLEX_TYPES(a.scalar_type(), "linalg.lstsq_cpu", [&] {
    using value_t = typename c10::scalar_value_type<scalar_t>::type;

    auto driver = lapackLstsq<LapackLstsqDriver::Gelsd, scalar_t, value_t>;
    static auto driver_enum_to_func
      = std::unordered_map<LapackLstsqDriver, decltype(driver), LapackLstsqDriverHash>({
      {LapackLstsqDriver::Gels, lapackLstsq<LapackLstsqDriver::Gels, scalar_t, value_t>},
      {LapackLstsqDriver::Gelsy, lapackLstsq<LapackLstsqDriver::Gelsy, scalar_t, value_t>},
      {LapackLstsqDriver::Gelsd, lapackLstsq<LapackLstsqDriver::Gelsd, scalar_t, value_t>},
      {LapackLstsqDriver::Gelss, lapackLstsq<LapackLstsqDriver::Gelss, scalar_t, value_t>}
    });
    driver = driver_enum_to_func[driver_enum];

    auto m = a.size(-2);
    auto n = a.size(-1);
    auto nrhs = b.size(-1);
    auto driver_helper = LapackLstsqHelper<scalar_t, value_t, decltype(driver)>(driver_enum, driver)
      .set_trans('N')
      .set_m(m)
      .set_n(n)
      .set_nrhs(nrhs)
      .set_a(a)
      .set_lda(std::max<int64_t>(1, m))
      .set_b(b)
      .set_ldb(std::max<int64_t>(1, std::max(m, n)))
      .set_jpvt()
      .set_rcond(cond)
      .set_rank()
      .set_s()
      .call_driver() // initial call to deduce optimal sizes for workspace arrays
      .set_work()
      .set_rwork()
      .set_iwork();

    // solve each problem in a batch separately
    for (int64_t i = 0; i < batchCount(a); ++i) {
      driver_helper.call_driver().next();
      infos[i] = driver_helper.info;
      if (driver_helper.info) {
        break;
      }
    }

    rank = driver_helper.rank;
    singular_values = driver_helper.s;
  });

  // Check infos for potential errors
  if (a.dim() > 2) {
    batchCheckErrors(infos, "torch.linalg.lstsq_cpu");
  }
  else {
    singleCheckErrors(infos[0], "torch.linalg.lstsq_cpu");
  }

  return std::make_tuple(b, rank, singular_values);
#endif
}

std::tuple<Tensor, Tensor, Tensor> linalg_lstsq(
    const Tensor& self, const Tensor& b,
    c10::optional<double> cond,
    c10::optional<std::string> driver_name) {
  TORCH_CHECK(
    self.dim() >= 2,
    "input `self` Tensor should be at least 2D"
  );
  TORCH_CHECK(
    b.dim() >= 1,
    "input 'b' Tensor should be at least 1D"
  );
  auto dim_diff = self.dim() - b.dim();
  TORCH_CHECK(
    0 <= dim_diff && dim_diff <= 1,
    "self.dim() must be greater or equal to b.dim() and "
    "(self.dim() - b.dim()) <= 1"
  );
  Tensor b2d = dim_diff ? b.unsqueeze(-1) : b;
  TORCH_CHECK(
    self.size(-2) == b2d.size(-2),
    dim_diff ? "self.size(-2) should match b.size(-1)" :
      "self.size(-2) should match b.size(-2)"
  );

  auto driver_str = driver_name.has_value() ? driver_name.value() : "gelsd";
  // convert `driver_str` to lower case inplace.
  std::transform(driver_str.begin(), driver_str.end(), driver_str.begin(),
    [](unsigned char c) { return std::tolower(c); });
  static std::unordered_set<std::string> allowed_drivers = {
    "gels", "gelsy", "gelsd", "gelss"
  };
  TORCH_CHECK(
    allowed_drivers.find(driver_str) != allowed_drivers.end(),
    "torch.linalg.lstsq: parameter 'driver_name' should be one of "
    "(gels, gelsy, gelsd, gelss)"
  );

  auto self_working_copy = cloneBatchedColumnMajor(self);

  // Tensor b must be of size (..., max(m, n), nrhs)
  // plus in column-major order
  auto m = self.size(-2);
  auto n = self.size(-1);
  auto b_working_copy_sizes = b2d.sizes().vec();
  b_working_copy_sizes[b2d.dim() - 2] = std::max(m, n);
  auto b_working_copy_strides = at::detail::defaultStrides(b2d.sizes());
  b_working_copy_strides[b2d.dim() - 2] = 1;
  b_working_copy_strides[b2d.dim() - 1] = std::max(m, n);
  Tensor b_working_copy;
  b_working_copy = at::empty_strided(
    b_working_copy_sizes, b_working_copy_strides, b.options());
  b_working_copy.narrow(-2, 0, m).copy_(b2d);

  double rcond;
  if (cond.has_value()) {
    rcond = cond.value();
  }
  else {
    auto value_type = c10::toValueType(self.scalar_type());
    rcond = _get_epsilon(value_type);
  }

  Tensor x, rank, singular_values;
  std::tie(x, rank, singular_values) =
    at::_lstsq_helper(self_working_copy, b_working_copy, rcond, driver_str);
  return std::make_tuple(x, rank, singular_values);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ lu_solve ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<typename scalar_t>
static void apply_lu_solve(Tensor& b, const Tensor& lu, const Tensor& pivots, std::vector<int64_t>& infos) {
#ifndef USE_LAPACK
  AT_ERROR("lu_solve: LAPACK library not found in compilation");
#else
  auto b_data = b.data_ptr<scalar_t>();
  auto lu_data = lu.data_ptr<scalar_t>();
  auto pivots_data = pivots.data_ptr<int>();
  auto b_stride = matrixStride(b);
  auto lu_stride = matrixStride(lu);
  auto pivots_stride = pivots.size(-1);
  auto batch_size = batchCount(b);

  auto n = lu.size(-2);
  auto nrhs = b.size(-1);

  int info;
  for (int64_t i = 0; i < batch_size; i++) {
    scalar_t* b_working_ptr = &b_data[i * b_stride];
    scalar_t* lu_working_ptr = &lu_data[i * lu_stride];
    int* pivots_working_ptr = &pivots_data[i * pivots_stride];
    lapackLuSolve<scalar_t>('N', n, nrhs, lu_working_ptr, n, pivots_working_ptr,
                            b_working_ptr, n, &info);
    infos[i] = info;
    if (info != 0) {
      return;
    }
  }
#endif
}

Tensor _lu_solve_helper_cpu(const Tensor& self, const Tensor& LU_data, const Tensor& LU_pivots) {
  auto self_working_copy = cloneBatchedColumnMajor(self);
  auto LU_data_working_copy = cloneBatchedColumnMajor(LU_data);
  auto LU_pivots_working_copy = LU_pivots.is_contiguous() ? LU_pivots : LU_pivots.contiguous();
  std::vector<int64_t> infos(batchCount(self), 0);

  if (self.numel() == 0 || LU_data.numel() == 0) {
    return at::zeros_like(self, LEGACY_CONTIGUOUS_MEMORY_FORMAT);
  }
  AT_DISPATCH_FLOATING_AND_COMPLEX_TYPES(self.scalar_type(), "lu_solve_cpu", [&]{
    apply_lu_solve<scalar_t>(self_working_copy, LU_data_working_copy, LU_pivots_working_copy, infos);
  });
  if (self.dim() > 2) {
    batchCheckErrors(infos, "lu_solve_cpu");
  } else {
    singleCheckErrors(infos[0], "lu_solve_cpu");
  }
  return self_working_copy;
}

// Supports arbitrary batch dimensions for self and LU_data (implicitly LU_pivots also)
Tensor lu_solve(const Tensor& self, const Tensor& LU_data, const Tensor& LU_pivots) {
  TORCH_CHECK(self.dim() >= 2,
              "b should have at least 2 dimensions, but has ", self.dim(), " dimensions instead");
  TORCH_CHECK(LU_data.dim() >= 2,
              "LU_data should have at least 2 dimensions, but has ", LU_data.dim(), " dimensions instead");
  TORCH_CHECK(LU_pivots.size(-1) == LU_data.size(-1),
              "Number of pivots per batch should be same as the dimension of the matrix");
  TORCH_CHECK(LU_pivots.dtype() == at::kInt,
              "LU_pivots should be a Tensor of scalar type Int");
  TORCH_CHECK(LU_pivots.device() == LU_data.device(),
              "Expected LU_pivots and LU_data to be on the same device, "
              "but found LU_pivots on ", LU_pivots.device(), " and LU_data on ",
              LU_data.device(), " instead");

  // We check whether the batch dimensions of LU_pivots match the batch dimensions of LU_data
  // e.g.: LU_pivots.sizes() = 4 x 3 x 2, LU_data.sizes() = 4 x 3 x 2 x 2 is a pair of correct inputs
  // e.g.: LU_pivots.sizes() = 4 x 3 x 2, LU_data.sizes() = 12 x 2 x 2 is a pair of incorrect inputs
  IntArrayRef pivots_sizes(LU_pivots.sizes().data(), LU_pivots.dim() - 1);
  IntArrayRef lu_sizes(LU_data.sizes().data(), LU_data.dim() - 2);
  TORCH_CHECK(pivots_sizes == lu_sizes,
              "batch dimensions of LU_pivots doesn't match batch dimensions of LU_data");

  Tensor self_broadcasted, LU_data_broadcasted;
  std::tie(self_broadcasted, LU_data_broadcasted) = _linalg_broadcast_batch_dims(self, LU_data, "lu_solve");

  // Now, we need to broadcast pivots too for the batch dimensions to match
  IntArrayRef new_pivots_sizes(LU_data_broadcasted.sizes().data(), LU_data_broadcasted.dim() - 1);
  Tensor LU_pivots_broadcasted = LU_pivots.expand(new_pivots_sizes);
  return at::_lu_solve_helper(self_broadcasted, LU_data_broadcasted, LU_pivots_broadcasted);
}

Tensor& lu_solve_out(Tensor& result, const Tensor& self, const Tensor& LU_data, const Tensor& LU_pivots) {
  Tensor result_tmp = at::lu_solve(self, LU_data, LU_pivots);
  result.resize_as_(result_tmp).copy_(result_tmp);
  return result;
}

}}  // namespace at::native
