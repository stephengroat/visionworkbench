// __BEGIN_LICENSE__
// Copyright (C) 2006-2010 United States Government as represented by
// the Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
// __END_LICENSE__


/// \file LinearAlgebra.h
///
/// Provides standard linear algebra functions, such as eigendecomposition,
/// least-squares, and so on, utilizing LAPACK where possible.
/// Currently provides:
///  - Eigendecomposition via eigen()
///  - Singular value decomposition via svd() and complete_svd()
///  - Pseudoinverse via pseudoinverse()
#ifndef __VW_MATH_LINEAR_ALGEBRA_H__
#define __VW_MATH_LINEAR_ALGEBRA_H__

#include <vector>
#include <limits>

#include <vw/config.h>
#include <vw/Core/Exception.h>
#include <vw/Math/Matrix.h>
#include <vw/Math/LapackExports.h>

#include <boost/numeric/conversion/cast.hpp>

namespace vw {
/// Numerical linear algebra and computational geometry.
namespace math {

  namespace detail {
    template <typename T>
    f77_int FINT(const T& x) {
      return boost::numeric_cast<f77_int>(x);
    }
  }

  /// Compute the eigenvalues of a Matrix A
  ///
  /// E must be a complex vector and will be resized if necessary to
  /// the appropriate output dimensions based on the dimensions of A.
  template <class MatrixT, class EigenvaluesT>
  inline void eigen( MatrixT const& A, EigenvaluesT &e ) {
    VW_ASSERT( A.cols()==A.rows(), ArgumentErr() << "Eigendecomposition can only be performed on square matrices." );
    typedef typename MatrixT::value_type real_type;
    const f77_int lda = detail::FINT(A.rows());
    Matrix<real_type> Abuf = transpose(A);
    Vector<real_type> wr_buf( A.cols() );
    Vector<real_type> wi_buf( A.cols() );
    real_type work_size;
    f77_int n = detail::FINT(A.cols());
    f77_int lwork = -1, info;

    geev('N','N',n,&(Abuf(0,0)), lda, &(wr_buf(0)), &(wi_buf(0)), NULL, 1, NULL, 1, &work_size, lwork, &info);

    lwork = f77_int(work_size);
    std::vector<real_type> work( lwork );

    geev('N','N',n,&(Abuf(0,0)), lda, &(wr_buf(0)), &(wi_buf(0)), NULL, 1, NULL, 1, &work[0], lwork, &info);

    if (info > 0)
      vw_throw( ArgumentErr() << "eigen(): LAPACK driver geev only converged for the first " << info << " eigenvectors." );

    e.set_size( A.cols() );
    for ( unsigned i = 0; i < wr_buf.size(); ++i )
      e(i) = std::complex<real_type>(wr_buf(i), wi_buf(i));
  }

  /// Compute the entire eigendecomposition of the matrix A.
  ///
  /// E and V will contain the resulting eigendecomposition.  E will
  /// contain the eigenvalues and must be a complex vector, while V
  /// will contain eigenvectors and must be a complex matrix. E and V
  /// will be resized based on the dimensions of A.
  template <class AMatrixT, class EigenvaluesT, class VMatrixT>
  inline void eigen( AMatrixT &A, EigenvaluesT &e, VMatrixT &V ) {
    VW_ASSERT( A.cols()==A.rows(), ArgumentErr() << "Eigendecomposition can only be performed on square matrices." );
    typedef typename AMatrixT::value_type real_type;
    const f77_int lda = detail::FINT(A.rows());
    const f77_int ldvr = detail::FINT(A.cols());
    Matrix<real_type> Abuf = transpose(A);
    Matrix<real_type> Vbuf( A.rows(), A.cols() );
    Vector<real_type> wr_buf( A.cols() );
    Vector<real_type> wi_buf( A.cols() );
    real_type work_size;
    f77_int n = detail::FINT(A.cols());
    f77_int lwork = -1, info;

    geev('N','V',n,&(Abuf(0,0)), lda, &(wr_buf(0)), &(wi_buf(0)), NULL, 1, &(Vbuf(0,0)), ldvr, &work_size, lwork, &info);

    lwork = f77_int(work_size);
    std::vector<real_type> work( lwork );

    geev('N','V',n,&(Abuf(0,0)), lda, &(wr_buf(0)), &(wi_buf(0)), NULL, 1, &(Vbuf(0,0)), ldvr, &work[0], lwork, &info);
    if (info > 0)
      vw_throw( ArgumentErr() << "eigen(): LAPACK driver geev only converged for the first " << info << " eigenvectors." );

    e.set_size( A.cols() );
    V.set_size( Vbuf.cols(), Vbuf.rows() );
    for ( unsigned i = 0; i < wr_buf.size(); ++i ) {
      e(i) = std::complex<real_type>(wr_buf(i), wi_buf(i));
      // If the eigenvalue is complex, we must tease the real and
      // complex parts out of the Vbuf matrix.
      for ( unsigned r = 0; r < V.rows(); ++r )
        if (wi_buf(i) == 0)
          V(r,i) = Vbuf(i,r);
        else if (wi_buf(i) > 0)
          V(r,i) = std::complex<real_type>(Vbuf(i,r), Vbuf(i+1,r));
        else
          V(r,i) = std::complex<real_type>(Vbuf(i-1,r), -Vbuf(i,r));
    }
  }

  /// Compute the singular values of a Matrix A
  ///
  /// S will be resized if necessary to the appropriate output
  /// dimensions based on the dimensions of A.
  template <class AMatrixT, class SingularValuesT>
  inline void svd( AMatrixT const& A, SingularValuesT &s ) {
    typedef typename PromoteType<typename AMatrixT::value_type, typename SingularValuesT::value_type>::type real_type;
    const f77_int m = detail::FINT(A.rows()), n = detail::FINT(A.cols());
    const f77_int minmn = std::min(m,n);
    const f77_int lda = detail::FINT(A.rows());
    Matrix<real_type> Abuf = transpose(A);
    Vector<real_type> sbuf( minmn );
    real_type work_size;
    f77_int lwork = -1, info;
    std::vector<f77_int> iwork( 8*minmn );

    gesdd('N', m, n, &(Abuf(0,0)), lda, &(sbuf(0)), NULL, 1, NULL, 1, &work_size, lwork, &iwork[0], &info);

    lwork = f77_int(work_size);
    std::vector<real_type> work( lwork );
    gesdd('N', m, n, &(Abuf(0,0)), lda, &(sbuf(0)), NULL, 1, NULL, 1, &work[0], lwork, &iwork[0], &info);
    if (info > 0)
      vw_throw( ArgumentErr() << "svd(): LAPACK driver gesdd did not converge.  Update process failed." );
    s = sbuf;
  }

  /// Compute the singular value decomposition of the matrix A.
  ///
  /// U, S, and VT will be resized if necessary to the appropriate output
  /// dimensions based on the dimensions of A.
  template <class AMatrixT, class UMatrixT, class SingularValuesT, class VTMatrixT>
  inline void svd( AMatrixT const& A, UMatrixT &U, SingularValuesT &s, VTMatrixT &VT ) {
    typedef typename PromoteType<typename AMatrixT::value_type, typename SingularValuesT::value_type>::type temp_type1;
    typedef typename PromoteType<temp_type1, typename UMatrixT::value_type>::type temp_type2;
    typedef typename PromoteType<temp_type2, typename VTMatrixT::value_type>::type real_type;
    const f77_int m = detail::FINT(A.rows()), n = detail::FINT(A.cols());
    const f77_int minmn = std::min(m,n);
    const f77_int lda = detail::FINT(A.rows());
    Matrix<real_type> Abuf = transpose(A);
    Matrix<real_type> Ubuf( minmn, A.rows() );
    Matrix<real_type> VTbuf( A.cols(), minmn );
    Vector<real_type> sbuf( minmn );
    real_type work_size;
    f77_int lwork = -1, info;
    std::vector<f77_int> iwork( 8*minmn );
    f77_int ldu = m, ldvt = minmn;
    gesdd('S', m, n, &(Abuf(0,0)), lda, &(sbuf(0)), &(Ubuf(0,0)), ldu, &(VTbuf(0,0)), ldvt, &work_size, lwork, &iwork[0], &info);
    lwork = f77_int(work_size);
    std::vector<real_type> work( lwork );
    gesdd('S', m, n, &(Abuf(0,0)), lda, &(sbuf(0)), &(Ubuf(0,0)), ldu, &(VTbuf(0,0)), ldvt, &work[0], lwork, &iwork[0], &info);
    if (info > 0)
      vw_throw( ArgumentErr() << "svd(): LAPACK driver gesdd did not converge.  Update process failed." );
    U = transpose(Ubuf);
    VT = transpose(VTbuf);
    s = sbuf;
  }

  /// Compute the singular value decomposition of the matrix A,
  /// including complete orthogonal bases of the domain and range even
  /// when A is rectangular.
  ///
  /// U, S, and VT will be resized if necessary to the appropriate output
  /// dimensions based on the dimensions of A.
  template <class AMatrixT, class UMatrixT, class SingularValuesT, class VTMatrixT>
  inline void complete_svd( AMatrixT & A, UMatrixT &U, SingularValuesT &s, VTMatrixT &VT ) {
    typedef typename PromoteType<typename AMatrixT::value_type, typename SingularValuesT::value_type>::type temp_type1;
    typedef typename PromoteType<temp_type1, typename UMatrixT::value_type>::type temp_type2;
    typedef typename PromoteType<temp_type2, typename VTMatrixT::value_type>::type real_type;
    const f77_int m = detail::FINT(A.rows()), n = detail::FINT(A.cols());
    const f77_int minmn = std::min(m,n);
    const f77_int lda = detail::FINT(A.rows());
    Matrix<real_type> Abuf = transpose(A);
    Matrix<real_type> Ubuf( A.rows(), A.rows() );
    Matrix<real_type> VTbuf( A.cols(), A.cols() );
    Vector<real_type> sbuf( minmn );
    real_type work_size;
    f77_int lwork = -1, info;
    std::vector<f77_int> iwork( 8*minmn );
    f77_int ldu = m, ldvt = n;
    gesdd('A', m, n, &(Abuf(0,0)), lda, &(sbuf(0)), &(Ubuf(0,0)), ldu, &(VTbuf(0,0)), ldvt, &work_size, lwork, &iwork[0], &info);
    lwork = f77_int(work_size);
    std::vector<real_type> work( lwork );
    gesdd('A', m, n, &(Abuf(0,0)), lda, &(sbuf(0)), &(Ubuf(0,0)), ldu, &(VTbuf(0,0)), ldvt, &work[0], lwork, &iwork[0], &info);
    if (info > 0)
      vw_throw( ArgumentErr() << "svd(): LAPACK driver gesdd did not converge.  Update process failed." );
    U = transpose(Ubuf);
    VT = transpose(VTbuf);
    s = sbuf;
  }

  /// Compute the QR decomposition of the matrix A,
  /// A = Q*R with Q an unitary matrix and R an upper triangular matrix
  ///
  /// Q and R will be resized if necessary to the appropriate output
  /// dimensions based on the dimensions of A.
  template <class AMatrixT, class QMatrixT, class RMatrixT>
  inline void qrd( AMatrixT const& A, QMatrixT &Q, RMatrixT &R ) {
    typedef typename PromoteType<typename AMatrixT::value_type, typename QMatrixT::value_type>::type temp_type1;
    typedef typename PromoteType<temp_type1, typename RMatrixT::value_type>::type real_type;
    const f77_int m = detail::FINT(A.rows()), n = detail::FINT(A.cols());
    const f77_int minmn = std::min(m,n);
    const f77_int lda = detail::FINT(A.rows());

    Q.set_size(A.rows(), A.rows());
    R.set_size(A.rows(), A.cols());

    Matrix<real_type> Abuf = transpose(A);
    Vector<real_type> Tau( minmn );

    real_type work_size;
    f77_int lwork = -1, info;
    geqrf(m, n, &(Abuf(0, 0)), lda, &(Tau(0)), &work_size, lwork, &info);
    lwork = (f77_int)(work_size);
    std::vector<real_type> work(lwork);
    geqrf(m, n, &(Abuf(0, 0)), lda, &(Tau(0)), &work[0], lwork, &info);

    R.set_size(transpose(Abuf).rows(), transpose(Abuf).cols());
    for (size_t i = 0; i < R.rows(); i++)
      for (size_t j = 0; j < R.cols(); j++)
        R(i, j) = j >= i ? Abuf(j, i) : 0;

    lwork = -1;
    orgqr(m, n, minmn, &Abuf(0, 0), lda, &(Tau(0)), &work_size, lwork, &info);
    lwork = (f77_int)(work_size);
    work.resize(lwork);
    orgqr(m, n, minmn, &Abuf(0, 0), lda, &(Tau(0)), &work[0], lwork, &info);

    Q = transpose(Abuf);
  }

  /// Compute the RQ decomposition of the matrix A,
  /// A = R*Q with R an upper triangular matrix and Q an unitary matrix
  ///
  /// R and Q will be resized if necessary to the appropriate output
  /// dimensions based on the dimensions of A.
  template <class AMatrixT, class QMatrixT, class RMatrixT>
  inline void rqd( AMatrixT const& A, RMatrixT &R, QMatrixT &Q ) {
    typedef typename PromoteType<typename AMatrixT::value_type, typename QMatrixT::value_type>::type temp_type1;
    typedef typename PromoteType<temp_type1, typename RMatrixT::value_type>::type real_type;
    const f77_int m = detail::FINT(A.rows()), n = detail::FINT(A.cols());
    const f77_int minmn = std::min(m,n);
    const f77_int lda = detail::FINT(A.rows());

    Q.set_size(minmn, n);
    R.set_size(m, minmn);

    Matrix<real_type> Abuf(transpose(A));
    Vector<real_type> Tau( minmn );

    real_type work_size;
    f77_int lwork = -1, info;
    gerqf(m, n, &(Abuf(0, 0)), lda, &(Tau(0)), &work_size, lwork, &info);
    lwork = (f77_int)(work_size);
    std::vector<real_type> work(lwork);
    gerqf(m, n, &(Abuf(0, 0)), lda, &(Tau(0)), &work[0], lwork, &info);

    R.set_size(transpose(Abuf).rows(), transpose(Abuf).cols());
    for (size_t i = 0; i < R.rows(); i++)
      for (size_t j = 0; j < R.cols(); j++)
        R(i, j) = j >= i ? Abuf(j, i) : 0;

    lwork = -1;
    orgrq(m, n, minmn, &Abuf(0, 0), lda, &(Tau(0)), &work_size, lwork, &info);
    lwork = (f77_int)(work_size);
    work.resize(lwork);
    orgrq(m, n, minmn, &Abuf(0, 0), lda, &(Tau(0)), &work[0], lwork, &info);

    Q = transpose(Abuf);
  }


  /// Computes the pseudoinverse A* of a real matrix A.
  template <class AMatrixT>
  Matrix<typename AMatrixT::value_type> pseudoinverse( AMatrixT & A, double cond = 0 ) {
    Matrix<typename AMatrixT::value_type> u, vt;
    Vector<typename AMatrixT::value_type> s;
    svd( A, u, s, vt );
    Matrix<typename AMatrixT::value_type> si(s.size(),s.size());
    for( unsigned i=0; i<s.size(); ++i ) {
      for( unsigned j=0; j<s.size(); ++j ) {
        if( i==j ) si(i,j) = ( fabs(s(i)) <= cond*s(0) ) ? 0 : 1/s(i);
        else si(i,j) = 0;
      }
    }
    return transpose(vt)*si*transpose(u);
  }

  // gelsd seems to lock up on some problems (see TestLinearAlgebra.h)
  // gelss seems to be more reliable
#define USE_GELSS 1

  /// Computes the minimum-norm solution to a real linear least squares problem.
  template <class AMatrixT, class BVectorT>
  Vector<typename PromoteType<typename AMatrixT::value_type, typename BVectorT::value_type>::type>
  least_squares( AMatrixT & A, BVectorT & B, double cond = -1 ) {
    typedef typename PromoteType<typename AMatrixT::value_type, typename BVectorT::value_type>::type real_type;
    Matrix<real_type> Abuf = transpose(A);
    const f77_int m = detail::FINT(A.rows()), n = detail::FINT(A.cols());
    const f77_int minmn = std::min(m,n), maxmn = std::max(m,n);
    Vector<real_type> Bbuf(maxmn);
    subvector(Bbuf,0,m) = B;
    const f77_int nrhs = 1, lda = detail::FINT(A.rows()), ldb = maxmn;
    std::vector<real_type> s( minmn );
    real_type const rcond = boost::numeric_cast<real_type>(cond);
    f77_int rank;
    f77_int lwork = -1, info;
    real_type work_size;
#if USE_GELSS
    gelss( m, n, nrhs, &(Abuf(0,0)), lda, &(Bbuf(0)), ldb, &s[0], rcond, &rank, &work_size, lwork, &info );
    lwork = f77_int(work_size);
    std::vector<real_type> work( lwork );
    gelss( m, n, nrhs, &(Abuf(0,0)), lda, &(Bbuf(0)), ldb, &s[0], rcond, &rank, &work[0], lwork, &info );
#else
    std::vector<f77_int> iwork( (3*int(log(minmn+1.)/log(2.))+11)*minmn ); // log2(x) = log(x)/log(2)
    gelsd( m, n, nrhs, &(Abuf(0,0)), lda, &(Bbuf(0)), ldb, &s[0], rcond, &rank, &work_size, lwork, &iwork[0], &info );
    lwork = int(work_size);
    std::vector<real_type> work( lwork );
    gelsd( m, n, nrhs, &(Abuf(0,0)), lda, &(Bbuf(0)), ldb, &s[0], rcond, &rank, &work[0], lwork, &iwork[0], &info );
#endif

    Bbuf.set_size(n,true);
    return Bbuf;
  }


  // ---------------------------------------------------------------------------
  // x = solve(A,b) - Computes the solution to a real system of linear
  // equations:
  //
  //     A*x=b
  //
  // Based on the LAPACK GESV routines, this solution is computed
  // using LU decomposition and back/forward substitution.
  // ---------------------------------------------------------------------------

  /// Computes the minimum-norm solution to a real linear least squares problem.
  template <class AMatrixT, class BVectorT>
  Vector<typename PromoteType<typename AMatrixT::value_type, typename BVectorT::value_type>::type>
  solve( AMatrixT & A, BVectorT & B ) {
    typedef typename PromoteType<typename AMatrixT::value_type, typename BVectorT::value_type>::type real_type;

    const f77_int n = detail::FINT(A.cols());
    const f77_int lda = detail::FINT(A.rows());
    const f77_int nrhs = 1;
    Vector<f77_int> ipiv( A.rows() );
    const f77_int ldb = detail::FINT(B.size());
    f77_int info;

    Matrix<real_type> Abuf = transpose(A);
    Vector<real_type> result = B;
    gesv(n,nrhs,&(Abuf(0,0)), lda, &(ipiv(0)), &(result(0)), ldb, &info);

    if (info > 0)
      vw_throw( ArgumentErr() << "solve(): LAPACK driver gesv could not solve equation.  Factor " << info << " of A is zero, so A is singular." );

    return result;
  }


  // ---------------------------------------------------------------------------
  // x = solve_symmetric(A,b) where A is a symmetric, positive definite matrix.
  // Computes the solution to a real system of linear equations:
  //
  //     A*x=b
  //
  // Based on the LAPACK POSV routines, this solution is computed
  // using cholesky decomposition and back/forward substitution.  This
  // routine is twice as efficient as the normal version of solve()
  // when symmetric matrices are available.
  // ---------------------------------------------------------------------------

  /// Solve the equation Ax=b where A is a symmetric positive definite
  /// matrix.  This version of this method will modify A and b.  Upon
  /// return, A contains its cholesky factorization, and b will
  /// contain the result of the computation, x.  Because this method
  /// does not copy A or b, it can perform considerably faster than
  /// the non-nocopy method below.
  template <class AMatrixT, class BVectorT>
  void solve_symmetric_nocopy( AMatrixT & A, BVectorT & B ) {
    const f77_int n = detail::FINT(A.cols());
    const f77_int nrhs = 1;
    const f77_int lda = detail::FINT(A.rows());
    const f77_int ldb = detail::FINT(B.size());
    f77_int info;
    posv('L',n,nrhs,&(A(0,0)), lda, &(B(0)), ldb, &info);

    if (info > 0)
      vw_throw( ArgumentErr() << "solve_symmetric(): LAPACK driver posv could not solve equation because A is not symmetric positive definite." );
  }

  /// Solve the equations X'*A'=B' where A is a symmetric positive definite
  /// matrix, B is a matrix.
  ///  This version of this method will modify A and B.  Upon
  /// return, A contains its cholesky factorization, and B will
  /// contain the result of the computation, X.  Because this method
  /// does not copy A or b, it can perform considerably faster than
  /// the non-nocopy method below.
  /// IMPORTANT: B argument MUST be transpose, since LAPACK matrices
  /// are assumed to be stacked by columns (not rows as in Vision Workbench).

  template <class AMatrixT, class BMatrixT>
  void multi_solve_symmetric_nocopy( AMatrixT & A, BMatrixT & B ) {
    const f77_int n = detail::FINT(A.cols());
    const f77_int nrhs = detail::FINT(B.rows());
    const f77_int lda = detail::FINT(A.cols());
    const f77_int ldb = detail::FINT(B.cols());
    f77_int info;
    posv('L',n,nrhs,&(A(0,0)), lda, &(B(0,0)), ldb, &info);
    if (info > 0)
      vw_throw( ArgumentErr() << "solve_symmetric(): LAPACK driver posv could not solve equation because A is not symmetric positive definite." );
  }

  /// Solve the equation Ax=b where A is a symmetric positive definite
  /// matrix.  This version of this method will not modify A and b.
  /// The result (x) is returned as the return value.
  template <class AMatrixT, class BVectorT>
  Vector<typename PromoteType<typename AMatrixT::value_type, typename BVectorT::value_type>::type>
  solve_symmetric( AMatrixT & A, BVectorT & B ) {
    typedef typename PromoteType<typename AMatrixT::value_type, typename BVectorT::value_type>::type real_type;
    Matrix<real_type> Abuf = A;
    Vector<real_type> result = B;
    solve_symmetric_nocopy(Abuf,result);
    return result;
  }

  /// Solve the equations AX=B where A is a symmetric positive definite
  /// matrix.  This version of this method will not modify A and B.
  /// The result (X is returned as the return value.
  template <class AMatrixT, class BMatrixT>
  Matrix<typename PromoteType<typename AMatrixT::value_type, typename BMatrixT::value_type>::type>
  multi_solve_symmetric( AMatrixT & A, BMatrixT & B ) {
    typedef typename PromoteType<typename AMatrixT::value_type, typename BMatrixT::value_type>::type real_type;
    Matrix<real_type> Abuf = A; // A symmetric ==> transpose unnecessary
    Matrix<real_type> result = transpose(B);
    multi_solve_symmetric_nocopy(Abuf,result);
    return transpose(result);
  }

  // Solve for the rank of a matrix .. using previous SVD results
  template <class MatrixT, class MatrixT2, class VectorT>
  inline int rank( MatrixBase<MatrixT> const& A,
                   MatrixBase<MatrixT2> const& /*U*/,
                   VectorBase<VectorT> const& S,
                   MatrixBase<MatrixT2> const& /*V*/,
                   double const& thresh = -1 ) {
    typedef typename MatrixT::value_type value_type;
    double th = ( thresh >= 0. ? thresh : 0.5*sqrt(A.impl().cols()+A.impl().rows()+1.)*S.impl()[0]*std::numeric_limits<value_type>::epsilon() );
    int nr = 0;
    for ( unsigned j = 0; j < S.impl().size(); j++ ) {
      if ( S.impl()[j] > th )
        nr++;
    }
    return nr;
  }

  // Solve for the rank of a matrix. Implementation from pg 68, of Numerical Receipes 3rd Edition
  template <class MatrixT>
  inline int rank( MatrixBase<MatrixT> const& A,
                   double const& thresh = -1 ) {
    typedef typename MatrixT::value_type value_type;
    Matrix<value_type> U, V;
    Vector<value_type> S;
    complete_svd( A.impl(), U, S, V );
    V = transpose(V);
    return rank(A,U,S,V,thresh);
  }

  // Solve for nullity .. using previous SVD results
  template <class MatrixT, class MatrixT2, class VectorT>
  inline size_t nullity( MatrixBase<MatrixT> const& A,
                         MatrixBase<MatrixT2> const& /*U*/,
                         VectorBase<VectorT> const& S,
                         MatrixBase<MatrixT2> const& /*V*/,
                         double const& thresh = -1 ) {
    typedef typename MatrixT::value_type value_type;
    double th = ( thresh >= 0. ? thresh : 0.5*sqrt(A.impl().cols()+A.impl().rows()+1.)*S.impl()[0]*std::numeric_limits<value_type>::epsilon() );
    size_t nn = boost::numeric_cast<size_t>(A.impl().cols()-S.impl().size());
    for ( size_t j = 0; j < S.impl().size(); j++ ) {
      if ( S.impl()[j] <= th )
        nn++;
    }
    return nn;
  }

  // Nullity of a matrix (again from Numerical Receipes)
  template <class MatrixT>
  inline size_t nullity( MatrixBase<MatrixT> const& A, double thresh = -1 ) {
    typedef typename MatrixT::value_type value_type;
    Matrix<value_type> U, V;
    Vector<value_type> S;
    complete_svd( A.impl(), U, S, V );
    V = transpose(V);
    return nullity(A,U,S,V,thresh);
  }

  // Solve for the nullspace of a Matrix A. If Ax = [0], the nullspace
  // is an x that is not zero.
  template <class MatrixT >
  inline Matrix<typename MatrixT::value_type> nullspace( MatrixBase<MatrixT> const& A, double thresh = -1 ) {
    typedef typename MatrixT::value_type value_type;
    Matrix<value_type> U, V;
    Vector<value_type> S;
    complete_svd( A.impl(), U, S, V );
    V = transpose(V);

    size_t nty = nullity(A.impl(),U,S,V,thresh);
    if ( nty == 0 )
      return Matrix<value_type>(0,0);
    Matrix<value_type> nullsp(A.impl().cols(), nty );
    double th = ( thresh >= 0. ? thresh : 0.5*sqrt(A.impl().cols()+A.impl().rows()+1.)*S[0]*std::numeric_limits<value_type>::epsilon() );
    size_t nn = 0;
    for ( size_t j = 0; j < A.impl().cols(); j++ ) {
      if ( j < S.size() )
        if ( S[j] > th )
          continue;
      for ( size_t jj = 0; jj < A.impl().cols(); jj++ )
        nullsp(jj,nn) = V[jj][j];
      nn++;
    }
    return nullsp;
  }

} // namespace math
} // namespace vw

#endif // __VW_MATH_LINEAR_ALGEBRA_H__
