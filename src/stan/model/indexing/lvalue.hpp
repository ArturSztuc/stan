#ifndef STAN_MODEL_INDEXING_LVALUE_HPP
#define STAN_MODEL_INDEXING_LVALUE_HPP

#include <stan/math/prim.hpp>
#include <stan/model/indexing/index.hpp>
#include <stan/model/indexing/index_list.hpp>
#include <stan/model/indexing/rvalue_at.hpp>
#include <stan/model/indexing/rvalue_index_size.hpp>
#include <type_traits>
#include <vector>

namespace stan {

namespace model {

/**
 * Assign the specified rvalue to the specified lvalue.  The index
 * list's type must be `nil_index_list`, but its value will be
 * ignored.  The last two arguments are also ignored.
 *
 * @tparam T lvalue variable type
 * @tparam U rvalue variable type, which must be assignable to `T`
 * @param[in,out] x lvalue
 * @param[in] y rvalue
 * @param[in] name Name of lvalue variable (default "ANON"); ignored
 * @param[in] depth Indexing depth (default 0; ignored
 */
template <
    typename T, typename U,
    require_t<std::is_assignable<std::decay_t<T>&, std::decay_t<U>>>* = nullptr>
inline void assign(T&& x, const nil_index_list& /* idxs */, U&& y,
                   const char* name = "ANON", int depth = 0) {
  x = std::forward<U>(y);
}

/**
 * Assign the specified standard vector rvalue to the specified
 * standard vector lvalue.
 *
 * @tparam T lvalue container element type
 * @tparam U rvalue container element type, which must be assignable to `T`
 * @param[in] x lvalue variable
 * @param[in] y rvalue variable
 * @param[in] name name of lvalue variable (default "ANON").
 * @param[in] depth indexing depth (default 0).
 */
template <typename T, typename U, require_all_std_vector_t<T, U>* = nullptr,
          require_not_t<
              std::is_assignable<std::decay_t<T>&, std::decay_t<U>>>* = nullptr>
inline void assign(T&& x, const nil_index_list& /* idxs */, U&& y,
                   const char* name = "ANON", int depth = 0) {
  x.resize(y.size());
  for (size_t i = 0; i < y.size(); ++i)
    assign(x[i], nil_index_list(), y[i], name, depth + 1);
}

/**
 * Assign the specified Eigen vector at the specified single index
 * to the specified value.
 *
 * Types: vec[uni] <- scalar
 *
 * @tparam Vec1 Eigen type with either dynamic rows or columns, but not both.
 * @tparam U Type of value (must be assignable to T).
 * @param[in] x Vector variable to be assigned.
 * @param[in] idxs Sequence of one single index (from 1).
 * @param[in] y Value scalar.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If the index is out of bounds.
 */
template <typename Vec1, typename U, require_eigen_vector_t<Vec1>* = nullptr,
          require_stan_scalar_t<U>* = nullptr>
inline void assign(Vec1&& x,
                   const cons_index_list<index_uni, nil_index_list>& idxs,
                   const U& y, const char* name = "ANON", int depth = 0) {
  math::check_range("vector[uni] assign range", name, x.size(), idxs.head_.n_);
  x.coeffRef(idxs.head_.n_ - 1) = y;
}

/**
 * Assign the specified Eigen vector at the specified single index
 * to the specified value.
 *
 * Types: vec[uni] <- scalar
 *
 * @tparam Vec1 Eigen type with either dynamic rows or columns, but not both.
 * @tparam U Type of value (must be assignable to T).
 * @param[in] x Vector variable to be assigned.
 * @param[in] idxs Sequence of one single index (from 1).
 * @param[in] y Value scalar.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If the index is out of bounds.
 */
template <typename Vec1, typename Vec2,
          require_all_eigen_vector_t<Vec1, Vec2>* = nullptr>
inline void assign(Vec1&& x,
                   const cons_index_list<index_uni, nil_index_list>& idxs,
                   const Vec2& y, const char* name = "ANON", int depth = 0) {
  math::check_range("vector[uni] assign range", name, x.size(), idxs.head_.n_);
  math::check_range("vector[uni] assign range", name, y.size(), idxs.head_.n_);
  x.coeffRef(idxs.head_.n_ - 1) = y.coeffRef(idxs.head_.n_ - 1);
}

/**
 * Assign the specified Eigen vector at the specified min_max
 * index to the specified value.
 *
 * Types:  vec[min_max] <- vec
 *
 * @tparam Vec1 Eigen type with either dynamic rows or columns, but not both.
 * @tparam Vec2 Eigen type with either dynamic rows or columns, but not both.
 * @param[in] x vector variable to be assigned.
 * @param[in] idxs An `index_min_max`
 * @param[in] y Value vector.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the value size isn't the same as
 * the indexed size.
 */
template <typename Vec1, typename Vec2,
          require_all_eigen_vector_t<Vec1, Vec2>* = nullptr>
inline void assign(Vec1&& x,
                   const cons_index_list<index_min_max, nil_index_list>& idxs,
                   const Vec2& y, const char* name = "ANON", int depth = 0) {
  math::check_range("vector[min_max] min assign", name, x.size(),
                    idxs.head_.min_);
  math::check_range("vector[min_max] max assign", name, x.size(),
                    idxs.head_.max_);
  if (idxs.head_.positive_idx_) {
    const auto slice_start = idxs.head_.min_ - 1;
    const auto slice_size = idxs.head_.max_ - slice_start;
    math::check_size_match("vector[min_max] assign sizes", "lhs and rhs",
                           slice_size, name, y.size());
    x.segment(slice_start, slice_size) = y;
    return;
  } else {
    const auto slice_start = idxs.head_.max_ - 1;
    const auto slice_size = idxs.head_.min_ - slice_start;
    math::check_size_match("vector[reverse_min_max] assign sizes",
                           "lhs and rhs", slice_size, name, y.size());
    x.segment(slice_start, slice_size) = y.reverse();
    return;
  }
}

/**
 * Assign the specified Eigen vector at the specified multiple
 * index to the specified value.
 *
 * Types:  vec[multi] <- vec
 *
 * @tparam Vec1 Eigen type with either dynamic rows or columns, but not both.
 * @tparam Vec2 Eigen type with either dynamic rows or columns, but not both.
 * @param[in] x Row vector variable to be assigned.
 * @param[in] idxs Sequnce of cells to assign to.
 * @param[in] y Value vector.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the value size isn't the same as
 * the indexed size.
 */
template <typename Vec1, typename Vec2,
          require_all_eigen_vector_t<Vec1, Vec2>* = nullptr>
inline void assign(Vec1&& x,
                   const cons_index_list<index_multi, nil_index_list>& idxs,
                   const Vec2& y, const char* name = "ANON", int depth = 0) {
  const auto& y_ref = stan::math::to_ref(y);
  math::check_size_match("vector[multi] assign sizes", "lhs",
                         idxs.head_.ns_.size(), name, y_ref.size());
  const auto x_size = x.size();
  for (int n = 0; n < y_ref.size(); ++n) {
    math::check_range("vector[multi] assign range", name, x_size,
                      idxs.head_.ns_[n]);
    x.coeffRef(idxs.head_.ns_[n] - 1) = y_ref.coeffRef(n);
  }
}

/**
 * Assign the specified Eigen vector with an index without a specialization
 * to the specified value.
 *
 * Types:  vec[general] <- vec
 *
 * @tparam Vec1 Eigen type with either dynamic rows or columns, but not both.
 * @tparam I Type of multiple index.
 * @tparam Vec2 Eigen type with either dynamic rows or columns, but not both.
 * @param[in] x Row vector variable to be assigned.
 * @param[in] idxs An index.
 * @param[in] y Value vector.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the value size isn't the same as
 * the indexed size.
 */
template <typename Vec1, typename Vec2, typename I,
          require_all_eigen_vector_t<Vec1, Vec2>* = nullptr>
inline void assign(Vec1&& x, const cons_index_list<I, nil_index_list>& idxs,
                   const Vec2& y, const char* name = "ANON", int depth = 0) {
  const auto& y_ref = stan::math::to_ref(y);
  math::check_size_match("vector[multi] assign sizes", "lhs",
                         rvalue_index_size(idxs.head_, x.size()), name,
                         y_ref.size());
  for (int n = 0; n < y.size(); ++n) {
    int i = rvalue_at(n, idxs.head_);
    math::check_range("vector[multi] assign range", name, x.size(), i);
    x.coeffRef(i - 1) = y_ref.coeffRef(n);
  }
}

/**
 * Assign a row vector to a row of an eigen matrix.
 *
 * Types:  mat[uni] = rowvec
 *
 * @tparam Mat Eigen type with dynamic rows and columns.
 * @tparam RowVec Eigen type with dynamic columns and a compile time rows equal
 * to 1.
 * @param[in] x Matrix variable to be assigned.
 * @param[in] idxs An index holding the row to be assigned to.
 * @param[in] y Value row vector.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the number of columns in the row
 * vector and matrix do not match.
 */
template <typename Mat, typename RowVec,
          stan::internal::require_eigen_dense_dynamic_t<Mat>* = nullptr,
          require_eigen_row_vector_t<RowVec>* = nullptr>
inline void assign(Mat&& x,
                   const cons_index_list<index_uni, nil_index_list>& idxs,
                   const RowVec& y, const char* name = "ANON", int depth = 0) {
  math::check_size_match("matrix[uni] assign sizes", "lhs", x.cols(), name,
                         y.size());
  math::check_range("matrix[uni] assign range", name, x.rows(), idxs.head_.n_);
  x.row(idxs.head_.n_ - 1) = y;
}

/**
 * Assign a segment of a column matrix to an eigen matrix.
 *
 * Types:  mat[L, uni] = colvec
 *
 * @tparam Mat Eigen type with dynamic rows and columns.
 * @tparam RowVec Eigen type with dynamic rows and a compile time colums equal
 * to 1.
 * @tparam L The row index.
 * @param[in] x Matrix to be assigned.
 * @param[in] idxs Sequence of one single index (from 1).
 * @param[in] y row vector.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the number of columns in the row
 * vector and matrix do not match.
 */
template <typename Mat, typename ColVec, typename L,
          stan::internal::require_eigen_dense_dynamic_t<Mat>* = nullptr>
inline void assign(
    Mat&& x,
    const cons_index_list<L, cons_index_list<index_uni, nil_index_list>>& idxs,
    const ColVec& y, const char* name = "ANON", int depth = 0) {
  math::check_range("matrix[L, uni] assign range", name, x.cols(),
                    idxs.tail_.head_.n_);
  assign(x.col(idxs.tail_.head_.n_ - 1), index_list(idxs.head_), y, name,
         depth + 1);
}

/**
 * Assign the specified Eigen matrix at the specified min
 * index to the specified matrix value.
 *
 * Types:  mat[min] = mat
 *
 * @tparam Mat1 Eigen type with dynamic rows and columns.
 * @tparam Mat2 Eigen type with dynamic rows and columns.
 * @param[in] x Matrix variable to be assigned.
 * @param[in] idxs An indexing from a minimum index (inclusive) to
 * the end of a container.
 * @param[in] y Value matrix.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the dimensions of the indexed
 * matrix and right-hand side matrix do not match.
 */
template <
    typename Mat1, typename Mat2,
    stan::internal::require_all_eigen_dense_dynamic_t<Mat1, Mat2>* = nullptr,
    require_eigen_t<Mat2>* = nullptr>
inline void assign(Mat1&& x,
                   const cons_index_list<index_min, nil_index_list>& idxs,
                   const Mat2& y, const char* name = "ANON", int depth = 0) {
  const int x_idx_rows = rvalue_index_size(idxs.head_, x.rows());
  const auto start_row = idxs.head_.min_ - 1;
  const auto row_size = x.rows() - start_row;
  math::check_range("matrix[min] assign range", name, x.rows(), row_size);
  math::check_size_match("matrix[min] assign row sizes", "lhs", row_size, name,
                         y.rows());
  x.block(start_row, 0, row_size, x.cols()) = y;
}

/**
 * Assign the specified Eigen matrix at the specified max
 * index to the specified matrix value.
 *
 * Types:  mat[max] = mat
 *
 * @tparam Mat1 Eigen type with dynamic rows and columns.
 * @tparam Mat2 Eigen type with dynamic rows and columns.
 * @param[in] x Matrix variable to be assigned.
 * @param[in] idxs An indexing from the start of the container up to
 * the specified maximum index (inclusive).
 * @param[in] y Value matrix.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the dimensions of the indexed
 * matrix and right-hand side matrix do not match.
 */
template <
    typename Mat1, typename Mat2,
    stan::internal::require_all_eigen_dense_dynamic_t<Mat1, Mat2>* = nullptr>
inline void assign(Mat1&& x,
                   const cons_index_list<index_max, nil_index_list>& idxs,
                   const Mat2& y, const char* name = "ANON", int depth = 0) {
  math::check_range("matrix[max] assign range", name, x.cols(),
                    idxs.head_.max_);
  math::check_size_match("matrix[max] assign row sizes", "lhs", idxs.head_.max_,
                         name, y.rows());
  x.block(0, 0, idxs.head_.max_, x.cols()) = y;
}

/**
 * Assign the specified Eigen matrix at the min_max indices for the assignee.
 *
 * Types:  mat[min_max, min_max] = mat
 *
 * @tparam Mat1 Eigen type with dynamic rows and columns.
 * @tparam Mat2 Eigen type with dynamic rows and columns.
 * @param[in] x Matrix variable to be assigned.
 * @param[in] idxs An index list containing two min_max indices
 * @param[in] x Matrix variable to assign from.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the dimensions of the indexed
 * matrix and right-hand side matrix do not match.
 */
template <typename Mat1, typename Mat2,
          stan::internal::require_eigen_dense_dynamic_t<Mat1>* = nullptr,
          require_eigen_t<Mat2>* = nullptr>
inline void assign(
    Mat1&& x,
    const cons_index_list<index_min_max,
                          cons_index_list<index_min_max, nil_index_list>>& idxs,
    const Mat2& y, const char* name = "ANON", int depth = 0) {
  if (idxs.head_.positive_idx_) {
    if (idxs.tail_.head_.positive_idx_) {
      auto row_size = idxs.head_.max_ - (idxs.head_.min_ - 1);
      auto col_size = idxs.tail_.head_.max_ - (idxs.tail_.head_.min_ - 1);
      math::check_range("matrix[min_max, min_max] assign col range", name,
                        x.cols(), idxs.head_.min_);
      math::check_range("matrix[min_max, min_max] assign row range", name,
                        x.rows(), idxs.tail_.head_.min_);
      math::check_size_match("matrix[min_max, min_max] assign row sizes", "lhs",
                             row_size, name, y.rows());
      math::check_size_match("matrix[min_max, min_max] assign col sizes", "lhs",
                             col_size, name, y.cols());
      x.block(idxs.head_.min_ - 1, idxs.tail_.head_.min_ - 1, row_size,
              col_size)
          = y;
      return;
    } else {
      auto row_size = idxs.head_.max_ - (idxs.head_.min_ - 1);
      auto col_size = idxs.tail_.head_.min_ - (idxs.tail_.head_.max_ - 1);
      math::check_range("matrix[min_max, reverse_min_max] assign col range",
                        name, x.cols(), idxs.head_.min_);
      math::check_range("matrix[min_max, reverse_min_max] assign row range",
                        name, x.rows(), idxs.tail_.head_.max_);
      math::check_size_match(
          "matrix[min_max, reverse_min_max] assign row sizes", "lhs", row_size,
          name, y.rows());
      math::check_size_match(
          "matrix[min_max, reverse_min_max] assign col sizes", "lhs", col_size,
          name, y.cols());
      x.block(idxs.head_.min_ - 1, idxs.tail_.head_.max_ - 1, row_size,
              col_size)
          = y.rowwise().reverse();
      return;
    }
  } else {
    if (idxs.tail_.head_.positive_idx_) {
      auto row_size = idxs.head_.min_ - (idxs.head_.max_ - 1);
      auto col_size = idxs.tail_.head_.max_ - (idxs.tail_.head_.min_ - 1);
      math::check_range("matrix[reverse_min_max, min_max] assign col range",
                        name, x.cols(), idxs.head_.max_);
      math::check_range("matrix[reverse_min_max, min_max] assign row range",
                        name, x.rows(), idxs.tail_.head_.min_);
      math::check_size_match(
          "matrix[reverse_min_max, min_max] assign row sizes", "lhs", row_size,
          name, y.rows());
      math::check_size_match(
          "matrix[reverse_min_max, min_max] assign col sizes", "lhs", col_size,
          name, y.cols());
      x.block(idxs.head_.max_ - 1, idxs.tail_.head_.min_ - 1, row_size,
              col_size)
          = y.colwise().reverse();
      return;
    } else {
      auto row_size = idxs.head_.min_ - (idxs.head_.max_ - 1);
      auto col_size = idxs.tail_.head_.min_ - (idxs.tail_.head_.max_ - 1);
      math::check_range(
          "matrix[reverse_min_max, reverse_min_max] assign col range", name,
          x.cols(), idxs.head_.max_);
      math::check_range(
          "matrix[reverse_min_max, reverse_min_max] assign row range", name,
          x.rows(), idxs.tail_.head_.max_);
      math::check_size_match(
          "matrix[reverse_min_max, reverse_min_max] assign row sizes", "lhs",
          row_size, name, y.rows());
      math::check_size_match(
          "matrix[reverse_min_max, reverse_min_max] assign col sizes", "lhs",
          col_size, name, y.cols());
      x.block(idxs.head_.max_ - 1, idxs.tail_.head_.max_ - 1, row_size,
              col_size)
          = y.reverse();
      return;
    }
  }
}

/**
 * Assign the specified Eigen matrix at the specified pair of
 * single indexes to the specified scalar value.
 *
 * Types:  mat[single, single] = scalar
 *
 * @tparam Mat Eigen type with dynamic rows and columns.
 * @tparam U Scalar type.
 * @param[in] x Matrix variable to be assigned.
 * @param[in] idxs Sequence of two single indexes (from 1).
 * @param[in] y Value scalar.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If either of the indices are out of bounds.
 */
template <typename Mat, typename U,
          stan::internal::require_eigen_dense_dynamic_t<Mat>* = nullptr>
inline void assign(
    Mat&& x,
    const cons_index_list<index_uni,
                          cons_index_list<index_uni, nil_index_list>>& idxs,
    const U& y, const char* name = "ANON", int depth = 0) {
  const int m = idxs.head_.n_;
  const int n = idxs.tail_.head_.n_;
  math::check_range("matrix[uni,uni] assign range", name, x.rows(), m);
  math::check_range("matrix[uni,uni] assign range", name, x.cols(), n);
  stan::math::to_ref(x).coeffRef(m - 1, n - 1) = y;
}

/**
 * Random access assign of a vector's cells to a row of an eigen matrix.
 * Types:  mat[uni, multi] = vector
 *
 * @tparam Mat1 Eigen type with dynamic rows and columns.
 * @tparam Vec Eigen type with dynamic rows and columns.
 * @param[in] x Matrix variable to be assigned.
 * @param[in] idxs Pair of multiple indexes (from 1).
 * @param[in] y Vector
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the dimensions of the indexed
 * matrix and value matrix do not match.
 */
template <typename Mat1, typename Vec,
          stan::internal::require_eigen_dense_dynamic_t<Mat1>* = nullptr,
          require_eigen_vector_t<Vec>* = nullptr>
inline void assign(
    Mat1&& x,
    const cons_index_list<index_uni,
                          cons_index_list<index_multi, nil_index_list>>& idxs,
    const Vec& y, const char* name = "ANON", int depth = 0) {
  const auto& y_ref = stan::math::to_ref(y);
  math::check_range("matrix[uni, multi] assign range", name, x.cols(),
                    idxs.head_.n_);
  math::check_size_match("matrix[uni, multi] assign sizes", "lhs",
                         idxs.tail_.head_.ns_.size(), name, y_ref.size());
  for (int i = 0; i < idxs.tail_.head_.ns_.size(); ++i) {
    math::check_range("matrix[uni, multi] assign range", name, x.cols(),
                      idxs.tail_.head_.ns_[i]);
    x.coeffRef(idxs.head_.n_ - 1, idxs.tail_.head_.ns_[i] - 1)
        = y_ref.coeffRef(i);
  }
}

/**
 * Assign the specified Eigen matrix at the specified pair of
 * multiple indexes to the specified matrix.
 *
 * Types:  mat[multi, multi] = mat
 *
 * @tparam Mat1 Eigen type with dynamic rows and columns.
 * @tparam Mat2 Eigen type with dynamic rows and columns.
 * @param[in] x Matrix variable to be assigned.
 * @param[in] idxs Pair of multiple indexes (from 1).
 * @param[in] y Value matrix.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the dimensions of the indexed
 * matrix and value matrix do not match.
 */
template <
    typename Mat1, typename Mat2,
    stan::internal::require_all_eigen_dense_dynamic_t<Mat1, Mat2>* = nullptr>
inline void assign(
    Mat1&& x,
    const cons_index_list<index_multi,
                          cons_index_list<index_multi, nil_index_list>>& idxs,
    const Mat2& y, const char* name = "ANON", int depth = 0) {
  const auto& y_ref = stan::math::to_ref(y);
  math::check_size_match("matrix[multi,multi] assign sizes", "lhs",
                         idxs.head_.ns_.size(), name, y_ref.rows());
  math::check_size_match("matrix[multi,multi] assign sizes", "lhs",
                         idxs.tail_.head_.ns_.size(), name, y_ref.cols());
  for (int j = 0; j < y_ref.cols(); ++j) {
    const int n = idxs.tail_.head_.ns_[j];
    math::check_range("matrix[multi,multi] assign range", name, x.cols(), n);
    for (int i = 0; i < y_ref.rows(); ++i) {
      const int m = idxs.head_.ns_[i];
      math::check_range("matrix[multi,multi] assign range", name, x.rows(), m);
      x.coeffRef(m - 1, n - 1) = y_ref.coeffRef(i, j);
    }
  }
}

/**
 * Random access assignment to an eigen matrix.
 *
 * Types:  mat[general] = mat
 *
 * @tparam Mat1 Eigen type with dynamic rows and columns.
 * @tparam L Multiple index type.
 * @tparam Mat2 Eigen type with dynamic rows and columns.
 * @param[in] x Matrix variable to be assigned.
 * @param[in] idxs Any of the index types.
 * @param[in] y Value matrix.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the dimensions of the indexed
 * matrix and right-hand side matrix do not match.
 */
template <
    typename Mat1, typename L, typename Mat2,
    stan::internal::require_all_eigen_dense_dynamic_t<Mat2, Mat2>* = nullptr>
inline void assign(Mat1&& x, const cons_index_list<L, nil_index_list>& idxs,
                   const Mat2& y, const char* name = "ANON", int depth = 0) {
  const int x_idx_rows = rvalue_index_size(idxs.head_, x.rows());
  const auto& y_ref = stan::math::to_ref(y);
  math::check_size_match("matrix[multi] assign row sizes", "lhs", x_idx_rows,
                         name, y_ref.rows());
  math::check_size_match("matrix[multi] assign col sizes", "lhs", x.cols(),
                         name, y_ref.cols());
  for (int i = 0; i < y_ref.rows(); ++i) {
    const int m = rvalue_at(i, idxs.head_);
    math::check_range("matrix[multi] assign range", name, x.rows(), m);
    x.row(m - 1) = y_ref.row(i);
  }
}

/**
 * Assign the specified Eigen matrix at the specified min
 * index to the specified matrix value.
 *
 * Types:  mat[L, min] = mat
 *
 * @tparam Mat1 Eigen type with dynamic rows and columns.
 * @tparam Mat2 Eigen type with dynamic rows and columns.
 * @tparam L An index.
 * @param[in] x Matrix variable to be assigned.
 * @param[in] idxs Container holding a row index and an index from a minimum
 * index (inclusive) to the end of a container.
 * @param[in] y Value matrix.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the dimensions of the indexed
 * matrix and right-hand side matrix do not match.
 */
template <
    typename Mat1, typename Mat2, typename L,
    stan::internal::require_all_eigen_dense_dynamic_t<Mat1, Mat2>* = nullptr>
inline void assign(
    Mat1&& x,
    const cons_index_list<L, cons_index_list<index_min, nil_index_list>>& idxs,
    const Mat2& y, const char* name = "ANON", int depth = 0) {
  const int x_idx_rows = rvalue_index_size(idxs.head_, x.rows());
  const auto start_col = idxs.tail_.head_.min_ - 1;
  const auto col_size = x.cols() - start_col;
  math::check_size_match("matrix[L, min] assign col sizes", "lhs", col_size,
                         name, y.cols());
  assign(x.block(0, start_col, x.rows(), col_size), index_list(idxs.head_), y,
         name, depth + 1);
}

/**
 * Assign the specified Eigen matrix at the specified max
 * index to the specified matrix value.
 *
 * Types:  mat[L, max] = mat
 *
 * @tparam Mat1 Eigen type with dynamic rows and columns.
 * @tparam Mat2 Eigen type with dynamic rows and columns.
 * @tparam An index.
 * @param[in] x Matrix variable to be assigned.
 * @param[in] idxs Index holding a row index and an index from the start of the
 * container up to the specified maximum index (inclusive).
 * @param[in] y Value matrix.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the dimensions of the indexed
 * matrix and right-hand side matrix do not match.
 */
template <
    typename Mat1, typename Mat2, typename L,
    stan::internal::require_all_eigen_dense_dynamic_t<Mat1, Mat2>* = nullptr>
inline void assign(
    Mat1&& x,
    const cons_index_list<L, cons_index_list<index_max, nil_index_list>>& idxs,
    const Mat2& y, const char* name = "ANON", int depth = 0) {
  math::check_size_match("matrix[L, max] assign row sizes", "lhs",
                         idxs.tail_.head_.max_, name, y.rows());
  assign(x.block(0, 0, x.rows(), idxs.tail_.head_.max_), index_list(idxs.head_),
         y, name, depth + 1);
}

/**
 * Assign the specified Eigen matrix at the min_max indice for the assignee.
 *
 * Types:  mat[L, min_max] = mat
 *
 * @tparam Mat1 Eigen type with dynamic rows and columns.
 * @tparam Mat2 Eigen type with dynamic rows and columns.
 * @tparam L The row index type
 * @param[in] x Matrix variable to be assigned.
 * @param[in] idxs Container holding row index and a min_max index.
 * @param[in] y Matrix variable to assign from.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the dimensions of the indexed
 * matrix and right-hand side matrix do not match.
 */
template <typename Mat1, typename Mat2, typename L,
          stan::internal::require_eigen_dense_dynamic_t<Mat1>* = nullptr,
          require_eigen_t<Mat2>* = nullptr>
inline void assign(
    Mat1&& x,
    const cons_index_list<L, cons_index_list<index_min_max, nil_index_list>>&
        idxs,
    const Mat2& y, const char* name = "ANON", int depth = 0) {
  if (idxs.tail_.head_.positive_idx_) {
    const auto col_start = idxs.tail_.head_.min_ - 1;
    const auto col_size = idxs.tail_.head_.max_ - col_start;
    math::check_range("matrix[L, min_max] assign range", name, x.cols(),
                      idxs.tail_.head_.min_);
    math::check_range("matrix[L, min_max] assign range", name,
                      idxs.tail_.head_.max_, x.cols());
    math::check_size_match("matrix[L, min_max] assign col sizes", "lhs",
                           col_size, name, y.cols());
    assign(x.block(0, col_start, x.rows(), col_size), index_list(idxs.head_), y,
           name, depth + 1);
    return;
  } else {
    const auto col_start = idxs.tail_.head_.max_ - 1;
    const auto col_size = idxs.tail_.head_.min_ - col_start;
    math::check_range("matrix[L, reverse_min_max] assign range", name, x.cols(),
                      idxs.tail_.head_.max_);
    math::check_range("matrix[L, reverse_min_max] assign range", name,
                      idxs.tail_.head_.min_, x.cols());
    math::check_size_match("matrix[L, reverse_min_max] assign col sizes", "lhs",
                           col_size, name, y.cols());
    assign(x.block(0, col_start, x.rows(), col_size), index_list(idxs.head_),
           y.rowwise().reverse(), name, depth + 1);
    return;
  }
}

/**
 * Assign an eigen matrix with two indices without a specialization.
 *
 * Types:  mat[general, general] = mat
 *
 * @tparam Mat1 Eigen type with dynamic rows and columns.
 * @tparam I1 First index
 * @tparam I2 Second index
 * @tparam Mat2 Eigen type with dynamic rows and columns.
 * @param[in] x Matrix variable to be assigned.
 * @param[in] idxs Pair of general indexes (from 1).
 * @param[in] y Value matrix.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the dimensions of the indexed
 * matrix and value matrix do not match.
 */
template <
    typename Mat1, typename I1, typename I2, typename Mat2,
    stan::internal::require_all_eigen_dense_dynamic_t<Mat1, Mat2>* = nullptr>
inline void assign(
    Mat1&& x,
    const cons_index_list<I1, cons_index_list<I2, nil_index_list>>& idxs,
    const Mat2& y, const char* name = "ANON", int depth = 0) {
  const int x_idxs_rows = rvalue_index_size(idxs.head_, x.rows());
  const int x_idxs_cols = rvalue_index_size(idxs.tail_.head_, x.cols());
  const auto& y_ref = stan::math::to_ref(y);
  math::check_size_match("matrix[multi,multi] assign sizes", "lhs", x_idxs_rows,
                         name, y_ref.rows());
  math::check_size_match("matrix[multi,multi] assign sizes", "lhs", x_idxs_cols,
                         name, y_ref.cols());
  for (int j = 0; j < y_ref.cols(); ++j) {
    const int n = rvalue_at(j, idxs.tail_.head_);
    math::check_range("matrix[multi,multi] assign range", name, x.cols(), n);
    for (int i = 0; i < y_ref.rows(); ++i) {
      const int m = rvalue_at(i, idxs.head_);
      math::check_range("matrix[multi,multi] assign range", name, x.rows(), m);
      x.coeffRef(m - 1, n - 1) = y_ref.coeffRef(i, j);
    }
  }
}

/**
 * Random access assign of a vector's cells to a column of an eigen matrix.
 * Types:  mat[uni, multi] = vector
 *
 * @tparam Mat1 Eigen type with dynamic rows and columns.
 * @tparam Vec Eigen type with dynamic rows and columns.
 * @param[in] x Matrix variable to be assigned.
 * @param[in] idxs Pair of multiple indexes (from 1).
 * @param[in] y Vector
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the dimensions of the indexed
 * matrix and value matrix do not match.
 */
template <typename Mat1, typename Vec,
          stan::internal::require_eigen_dense_dynamic_t<Mat1>* = nullptr,
          require_eigen_vector_t<Vec>* = nullptr>
inline void assign(
    Mat1&& x,
    const cons_index_list<index_multi,
                          cons_index_list<index_uni, nil_index_list>>& idxs,
    const Vec& y, const char* name = "ANON", int depth = 0) {
  const auto& y_ref = stan::math::to_ref(y);
  math::check_range("matrix[multi, uni] assign range", name, x.cols(),
                    idxs.tail_.head_.n_);
  math::check_size_match("matrix[multi, uni] assign sizes", "lhs",
                         idxs.head_.ns_.size(), name, y_ref.size());
  for (int i = 0; i < idxs.head_.ns_.size(); ++i) {
    math::check_range("matrix[multi, uni] assign range", name, x.rows(),
                      idxs.head_.ns_[i]);
    x.coeffRef(idxs.head_.ns_[i] - 1, idxs.tail_.head_.n_ - 1)
        = y_ref.coeffRef(i);
  }
}

/**
 * Assign the specified array (standard vector) at the specified
 * index list beginning with a single index to the specified value.
 *
 * This function operates recursively to carry out the tail
 * indexing.
 *
 * Types:  x[uni | L] = y
 *
 * @tparam StdVec A standard vector
 * @tparam L Type of tail of index list.
 * @tparam U A type assignable to the value type of `StdVec`
 * @param[in] x Array variable to be assigned.
 * @param[in] idxs List of indexes beginning with single index
 * (from 1).
 * @param[in] y Value.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the dimensions do not match in the
 * tail assignment.
 */
template <typename StdVec, typename L, typename U,
          require_std_vector_t<StdVec>* = nullptr>
inline void assign(StdVec&& x, const cons_index_list<index_uni, L>& idxs, U&& y,
                   const char* name = "ANON", int depth = 0) {
  math::check_range("vector[uni,...] assign range", name, x.size(),
                    idxs.head_.n_);
  assign(x[idxs.head_.n_ - 1], idxs.tail_, y, name, depth + 1);
}

/**
 * Assign the specified array (standard vector) at the specified
 * index list beginning with a multiple index to the specified value.
 *
 * This function operates recursively to carry out the tail
 * indexing.
 *
 * Types:  x[multi | L] = y
 *
 * @tparam T A standard vector.
 * @tparam I Type of multiple index heading index list.
 * @tparam L Type of tail of index list.
 * @tparam U A standard vector
 * @param[in] x Array variable to be assigned.
 * @param[in] idxs List of indexes beginning with multiple index
 * (from 1).
 * @param[in] y Value.
 * @param[in] name Name of variable (default "ANON").
 * @param[in] depth Indexing depth (default 0).
 * @throw std::out_of_range If any of the indices are out of bounds.
 * @throw std::invalid_argument If the size of the multiple indexing
 * and size of first dimension of value do not match, or any of
 * the recursive tail assignment dimensions do not match.
 */
template <typename T, typename I, typename L, typename U,
          require_all_std_vector_t<T, U>* = nullptr>
inline void assign(T&& x, const cons_index_list<I, L>& idxs, U&& y,
                   const char* name = "ANON", int depth = 0) {
  int x_idx_size = rvalue_index_size(idxs.head_, x.size());
  math::check_size_match("vector[multi,...] assign sizes", "lhs", x_idx_size,
                         name, y.size());
  for (size_t n = 0; n < y.size(); ++n) {
    int i = rvalue_at(n, idxs.head_);
    math::check_range("vector[multi,...] assign range", name, x.size(), i);
    assign(x[i - 1], idxs.tail_, y[n], name, depth + 1);
  }
}

}  // namespace model
}  // namespace stan
#endif
