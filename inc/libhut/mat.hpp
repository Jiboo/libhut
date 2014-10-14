/*  _ _ _   _       _
 * | |_| |_| |_ _ _| |_
 * | | | . |   | | |  _|
 * |_|_|___|_|_|___|_|
 * Hobby graphics and GUI library under the MIT License (MIT)
 *
 * Copyright (c) 2014 Jean-Baptiste "Jiboo" Lepesme
 * github.com/jiboo/libhut @lepesmejb +JeanBaptisteLepesme
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <cmath>

#include <algorithm>
#include <iostream>
#include <type_traits>
#include <utility>

#include "libhut/vec.hpp"

namespace hut {

    template<typename _Type, size_t _RowCount, size_t _RowSize>
    struct mat {
        typedef vec<_Type, _RowSize> col_type;

        col_type data[_RowCount];

        col_type &operator[](const size_t i) {
            return data[i];
        }

        constexpr const col_type &operator[](const size_t i) const {
            return data[i];
        }
    };

    using mat2 = mat<float, 2, 2>;
    using mat3 = mat<float, 3, 3>;
    using mat4 = mat<float, 4, 4>;

    using dmat2 = mat<double, 2, 2>;
    using dmat3 = mat<double, 3, 3>;
    using dmat4 = mat<double, 4, 4>;

    template<typename _Type, size_t _RowCount, size_t _RowSize>
    std::ostream &operator<<(std::ostream &os, const mat<_Type, _RowCount, _RowSize> &m) {
        os << '{';
        for (size_t i = 0; i < _RowCount - 1; i++)
            os << m[i] << ", ";
        return os << m[_RowCount - 1] << '}';
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    inline bool operator==(const mat<_TypeA, _RowCount, _RowSize> &a, const mat<_TypeB, _RowCount, _RowSize> &b) {
        return std::equal(a.data, a.data + _RowCount, b.data);
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    inline bool operator!=(const mat<_TypeA, _RowCount, _RowSize> &a, const mat<_TypeB, _RowCount, _RowSize> &b) {
        return !(a == b);
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    inline bool operator<(const mat<_TypeA, _RowCount, _RowSize> &a, const mat<_TypeB, _RowCount, _RowSize> &b) {
        return std::lexicographical_compare(a.data, a.data + _RowCount, b.data, b.data + _RowCount);
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    inline bool operator>(const mat<_TypeA, _RowCount, _RowSize> &a, const mat<_TypeB, _RowCount, _RowSize> &b) {
        return b < a;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    inline bool operator<=(const mat<_TypeA, _RowCount, _RowSize> &a, const mat<_TypeB, _RowCount, _RowSize> &b) {
        return !(b < a);
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    inline bool operator>=(const mat<_TypeA, _RowCount, _RowSize> &a, const mat<_TypeB, _RowCount, _RowSize> &b) {
        return !(a < b);
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, typename std::enable_if<std::is_arithmetic<_TypeB>::value, _TypeB>::type>::type, _RowCount, _RowSize>
    operator+(const mat<_TypeA, _RowCount, _RowSize> &a, const _TypeB &b) {
        mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize> result;
        for (size_t i = 0; i < _RowCount; i++)
            result[i] = a[i] + b;
        return result;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize>
    operator+(const mat<_TypeA, _RowCount, _RowSize> &a, const mat<_TypeB, _RowCount, _RowSize> &b) {
        mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize> result;
        for (size_t i = 0; i < _RowCount; i++)
            result[i] = a[i] + b[i];
        return result;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, typename std::enable_if<std::is_arithmetic<_TypeB>::value, _TypeB>::type>::type, _RowCount, _RowSize>
    operator+=(mat<_TypeA, _RowCount, _RowSize> &a, const _TypeB &b) {
        for (size_t i = 0; i < _RowCount; i++)
            a[i] += b;
        return a;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize>
    operator+=(mat<_TypeA, _RowCount, _RowSize> &a, const mat<_TypeB, _RowCount, _RowSize> &b) {
        for (size_t i = 0; i < _RowCount; i++)
            a[i] += b[i];
        return a;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, typename std::enable_if<std::is_arithmetic<_TypeB>::value, _TypeB>::type>::type, _RowCount, _RowSize>
    operator-(const mat<_TypeA, _RowCount, _RowSize> &a, const _TypeB &b) {
        mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize> result;
        for (size_t i = 0; i < _RowCount; i++)
            result[i] = a[i] - b;
        return result;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize>
    operator-(const mat<_TypeA, _RowCount, _RowSize> &a, const mat<_TypeB, _RowCount, _RowSize> &b) {
        mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize> result;
        for (size_t i = 0; i < _RowCount; i++)
            result[i] = a[i] - b[i];
        return result;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, typename std::enable_if<std::is_arithmetic<_TypeB>::value, _TypeB>::type>::type, _RowCount, _RowSize>
    operator-=(mat<_TypeA, _RowCount, _RowSize> &a, const _TypeB &b) {
        for (size_t i = 0; i < _RowCount; i++)
            a[i] -= b;
        return a;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize>
    operator-=(mat<_TypeA, _RowCount, _RowSize> &a, const mat<_TypeB, _RowCount, _RowSize> &b) {
        for (size_t i = 0; i < _RowCount; i++)
            a[i] -= b[i];
        return a;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, typename std::enable_if<std::is_arithmetic<_TypeB>::value, _TypeB>::type>::type, _RowCount, _RowSize>
    operator*(const mat<_TypeA, _RowCount, _RowSize> &a, const _TypeB &b) {
        mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize> result;
        for (size_t i = 0; i < _RowCount; i++)
            result[i] = a[i] * b;
        return result;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize>
    matrixCompMult(const mat<_TypeA, _RowCount, _RowSize> &a, const mat<_TypeB, _RowCount, _RowSize> &b) {
        mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize> result;
        for (size_t i = 0; i < _RowCount; i++)
            result[i] = a[i] * b[i];
        return result;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, typename std::enable_if<std::is_arithmetic<_TypeB>::value, _TypeB>::type>::type, _RowCount, _RowSize>
    operator*=(mat<_TypeA, _RowCount, _RowSize> &a, const _TypeB &b) {
        for (size_t i = 0; i < _RowCount; i++)
            a[i] *= b;
        return a;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize>
    matrixCompMultInPlace(mat<_TypeA, _RowCount, _RowSize> &a, const mat<_TypeB, _RowCount, _RowSize> &b) {
        for (size_t i = 0; i < _RowCount; i++)
            a[i] *= b[i];
        return a;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, typename std::enable_if<std::is_arithmetic<_TypeB>::value, _TypeB>::type>::type, _RowCount, _RowSize>
    operator/(const mat<_TypeA, _RowCount, _RowSize> &a, const _TypeB &b) {
        mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize> result;
        for (size_t i = 0; i < _RowCount; i++)
            result[i] = a[i] / b;
        return result;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize>
    operator/(const mat<_TypeA, _RowCount, _RowSize> &a, const mat<_TypeB, _RowCount, _RowSize> &b) {
        mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize> result;
        for (size_t i = 0; i < _RowCount; i++)
            result[i] = a[i] / b[i];
        return result;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, typename std::enable_if<std::is_arithmetic<_TypeB>::value, _TypeB>::type>::type, _RowCount, _RowSize>
    operator/=(mat<_TypeA, _RowCount, _RowSize> &a, const _TypeB &b) {
        for (size_t i = 0; i < _RowCount; i++)
            a[i] /= b;
        return a;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCount, size_t _RowSize>
    mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCount, _RowSize>
    operator/=(mat<_TypeA, _RowCount, _RowSize> &a, const mat<_TypeB, _RowCount, _RowSize> &b) {
        for (size_t i = 0; i < _RowCount; i++)
            a[i] /= b[i];
        return a;
    }

    template<typename _TypeA, typename _TypeB, size_t _RowCountA, size_t _RowSizeA, size_t _RowCountB, size_t _RowSizeB>
    mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCountB, _RowSizeA>
    operator*(const mat<_TypeA, _RowCountA, _RowSizeA> &a, const mat<_TypeB, _RowCountB, _RowSizeB> &b) {
        static_assert(_RowCountA == _RowSizeB, "Incompatible matrices");
        mat<typename std::common_type<_TypeA, _TypeB>::type, _RowCountB, _RowSizeA> result;
        for (size_t i = 0; i < _RowCountB; i++) {
            for (size_t j = 0; j < _RowSizeA; j++) {
                result[i][j] = 0;
                for (size_t k = 0; k < _RowCountA; k++) {
                    result[i][j] += a[k][j] * b[i][k];
                }
            }
        }
        return result;
    }

} // namespace hut
