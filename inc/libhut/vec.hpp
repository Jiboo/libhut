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

namespace hut {

    template<typename _Type, size_t _Size>
    struct vec {
        _Type data[_Size];

        _Type &operator[](const size_t i) {
            return data[i];
        }

        constexpr const _Type &operator[](const size_t i) const {
            return data[i];
        }

        constexpr _Type mag() const {
            _Type sq = 0;
            for (size_t i = 0; i < _Size; i++)
                sq += data[i] * data[i];
            return std::sqrt(sq);
        }
    };

    template<typename _Type>
    struct vec<_Type, 2> {
        union {
            _Type data[2];
            struct {
                _Type x, y;
            };
        };

        _Type &operator[](const size_t i) {
            return data[i];
        }

        constexpr const _Type &operator[](const size_t i) const {
            return data[i];
        }

        constexpr _Type mag() const {
            return std::sqrt(x * x + y * y);
        }
    };

    template<typename _Type>
    struct vec<_Type, 3> {
        union {
            _Type data[3];
            struct {
                _Type x, y, z;
            };
            struct {
                _Type r, g, b;
            };
            vec<_Type, 2> xy;
        };

        _Type &operator[](const size_t i) {
            return data[i];
        }

        constexpr const _Type &operator[](const size_t i) const {
            return data[i];
        }

        constexpr _Type mag() const {
            return std::sqrt(x * x + y * y + z * z);
        }
    };

    template<typename _Type>
    struct vec<_Type, 4> {
        union {
            _Type data[4];
            struct {
                _Type x, y, z, w;
            };
            struct {
                _Type r, g, b, a;
            };
            vec<_Type, 2> xy;
            vec<_Type, 3> xyz;
            vec<_Type, 3> rgb;
        };

        _Type &operator[](const size_t i) {
            return data[i];
        }

        constexpr const _Type &operator[](const size_t i) const {
            return data[i];
        }

        constexpr _Type mag() const {
            return std::sqrt(x * x + y * y + z * z + w * w);
        }
    };

    using vec2 = vec<float, 2>;
    using vec3 = vec<float, 3>;
    using vec4 = vec<float, 4>;

    using dvec2 = vec<double, 2>;
    using dvec3 = vec<double, 3>;
    using dvec4 = vec<double, 4>;

    using ivec2 = vec<double, 2>;
    using ivec3 = vec<double, 3>;
    using ivec4 = vec<double, 4>;

    template<typename _Type, size_t _Size>
    std::ostream &operator<<(std::ostream &os, const vec<_Type, _Size> &v) {
        os << '{';
        for (size_t i = 0; i < _Size - 1; i++)
            os << v.data[i] << ", ";
        return os << v.data[_Size - 1] << '}';
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    inline bool operator==(const vec<_TypeA, _Size> &a, const vec<_TypeB, _Size> &b) {
        return std::equal(a.data, a.data + _Size, b.data);
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    inline bool operator!=(const vec<_TypeA, _Size> &a, const vec<_TypeB, _Size> &b) {
        return !(a == b);
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    inline bool operator<(const vec<_TypeA, _Size> &a, const vec<_TypeB, _Size> &b) {
        return std::lexicographical_compare(a.data, a.data + _Size, b.data, b.data + _Size);
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    inline bool operator>(const vec<_TypeA, _Size> &a, const vec<_TypeB, _Size> &b) {
        return b < a;
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    inline bool operator<=(const vec<_TypeA, _Size> &a, const vec<_TypeB, _Size> &b) {
        return !(b < a);
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    inline bool operator>=(const vec<_TypeA, _Size> &a, const vec<_TypeB, _Size> &b) {
        return !(a < b);
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    vec<typename std::common_type<_TypeA, typename std::enable_if<std::is_arithmetic<_TypeB>::value, _TypeB>::type>::type, _Size>
    operator+(const vec<_TypeA, _Size> &a, const _TypeB &b) {
        vec<typename std::common_type<_TypeA, _TypeB>::type, _Size> result;
        for (size_t i = 0; i < _Size; i++)
            result[i] = a[i] + b;
        return result;
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    vec<typename std::common_type<_TypeA, _TypeB>::type, _Size>
    operator+(const vec<_TypeA, _Size> &a, const vec<_TypeB, _Size> &b) {
        vec<typename std::common_type<_TypeA, _TypeB>::type, _Size> result;
        for (size_t i = 0; i < _Size; i++)
            result[i] = a[i] + b[i];
        return result;
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    vec<typename std::common_type<_TypeA, typename std::enable_if<std::is_arithmetic<_TypeB>::value, _TypeB>::type>::type, _Size>
    operator+=(vec<_TypeA, _Size> &a, const _TypeB &b) {
        for (size_t i = 0; i < _Size; i++)
            a[i] += b;
        return a;
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    vec<typename std::common_type<_TypeA, _TypeB>::type, _Size>
    operator+=(vec<_TypeA, _Size> &a, const vec<_TypeB, _Size> &b) {
        for (size_t i = 0; i < _Size; i++)
            a[i] += b[i];
        return a;
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    vec<typename std::common_type<_TypeA, typename std::enable_if<std::is_arithmetic<_TypeB>::value, _TypeB>::type>::type, _Size>
    operator-(const vec<_TypeA, _Size> &a, const _TypeB &b) {
        vec<typename std::common_type<_TypeA, _TypeB>::type, _Size> result;
        for (size_t i = 0; i < _Size; i++)
            result[i] = a[i] - b;
        return result;
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    vec<typename std::common_type<_TypeA, _TypeB>::type, _Size>
    operator-(const vec<_TypeA, _Size> &a, const vec<_TypeB, _Size> &b) {
        vec<typename std::common_type<_TypeA, _TypeB>::type, _Size> result;
        for (size_t i = 0; i < _Size; i++)
            result[i] = a[i] - b[i];
        return result;
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    vec<typename std::common_type<_TypeA, typename std::enable_if<std::is_arithmetic<_TypeB>::value, _TypeB>::type>::type, _Size>
    operator-=(vec<_TypeA, _Size> &a, const _TypeB &b) {
        for (size_t i = 0; i < _Size; i++)
            a[i] -= b;
        return a;
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    vec<typename std::common_type<_TypeA, _TypeB>::type, _Size>
    operator-=(vec<_TypeA, _Size> &a, const vec<_TypeB, _Size> &b) {
        for (size_t i = 0; i < _Size; i++)
            a[i] -= b[i];
        return a;
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    vec<typename std::common_type<_TypeA, typename std::enable_if<std::is_arithmetic<_TypeB>::value, _TypeB>::type>::type, _Size>
    operator*(const vec<_TypeA, _Size> &a, const _TypeB &b) {
        vec<typename std::common_type<_TypeA, _TypeB>::type, _Size> result;
        for (size_t i = 0; i < _Size; i++)
            result[i] = a[i] * b;
        return result;
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    vec<typename std::common_type<_TypeA, _TypeB>::type, _Size>
    operator*(const vec<_TypeA, _Size> &a, const vec<_TypeB, _Size> &b) {
        vec<typename std::common_type<_TypeA, _TypeB>::type, _Size> result;
        for (size_t i = 0; i < _Size; i++)
            result[i] = a[i] * b[i];
        return result;
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    vec<typename std::common_type<_TypeA, typename std::enable_if<std::is_arithmetic<_TypeB>::value, _TypeB>::type>::type, _Size>
    operator*=(vec<_TypeA, _Size> &a, const _TypeB &b) {
        for (size_t i = 0; i < _Size; i++)
            a[i] *= b;
        return a;
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    vec<typename std::common_type<_TypeA, _TypeB>::type, _Size>
    operator*=(vec<_TypeA, _Size> &a, const vec<_TypeB, _Size> &b) {
        for (size_t i = 0; i < _Size; i++)
            a[i] *= b[i];
        return a;
    }

    template<typename _TypeA, typename _TypeB, size_t _Size>
    typename std::common_type<_TypeA, _TypeB>::type
    dot(const vec<_TypeA, _Size> &a, const vec<_TypeB, _Size> &b) {
        typename std::common_type<_TypeA, _TypeB>::type result = 0;
        for (size_t i = 0; i < _Size; i++)
            result += a[i] * b[i];
        return result;
    }

    template<typename _TypeA, typename _TypeB>
    constexpr vec<typename std::common_type<_TypeA, _TypeB>::type, 3>
    cross(const vec<_TypeA, 3> &a, const vec<_TypeB, 3> &b) {
        return vec<typename std::common_type<_TypeA, _TypeB>::type, 3> {{
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
        }};
    }

} // namespace hut
