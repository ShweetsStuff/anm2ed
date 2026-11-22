#pragma once

#include <cmath>
#include <type_traits>

// Vectors and 4x4 matrices for 2D drawing, shaped like GLSL's: components x, y, z, w (r, g, b, a for colors), and
// matrices column-major, so m[column][row]. Formulas follow glm's, which the editor used before.

template <class T> struct TVec4;

template <class T> struct TVec2
{
    using Value = T;
    static constexpr int SIZE = 2;
    T x = 0;
    T y = 0;

    constexpr TVec2() = default;
    constexpr TVec2(T x, T y) : x(x), y(y)
    {}
    explicit constexpr TVec2(T value) : x(value), y(value)
    {}
    template <class U> constexpr TVec2(TVec2<U> v) : x((T)v.x), y((T)v.y)
    {}
    template <class V>
        requires(V::SIZE > 2)
    constexpr TVec2(V v) : x((T)v.x), y((T)v.y)
    {}
    constexpr T& operator[](int i)
    {
        return i == 0 ? x : y;
    }
    constexpr T operator[](int i) const
    {
        return i == 0 ? x : y;
    }
};

template <class T> struct TVec3
{
    using Value = T;
    static constexpr int SIZE = 3;
    union
    {
        T x = 0;
        T r;
    };
    union
    {
        T y = 0;
        T g;
    };
    union
    {
        T z = 0;
        T b;
    };

    constexpr TVec3() = default;
    constexpr TVec3(T x, T y, T z) : x(x), y(y), z(z)
    {}
    explicit constexpr TVec3(T value) : x(value), y(value), z(value)
    {}
    constexpr TVec3(TVec2<T> xy, T z) : x(xy.x), y(xy.y), z(z)
    {}
    template <class U> constexpr TVec3(TVec3<U> v) : x((T)v.x), y((T)v.y), z((T)v.z)
    {}
    template <class U> constexpr TVec3(TVec4<U> v) : x((T)v.x), y((T)v.y), z((T)v.z)
    {}
    constexpr T& operator[](int i)
    {
        return i == 0 ? x : i == 1 ? y : z;
    }
    constexpr T operator[](int i) const
    {
        return i == 0 ? x : i == 1 ? y : z;
    }
};

template <class T> struct TVec4
{
    using Value = T;
    static constexpr int SIZE = 4;
    union
    {
        T x = 0;
        T r;
    };
    union
    {
        T y = 0;
        T g;
    };
    union
    {
        T z = 0;
        T b;
    };
    union
    {
        T w = 0;
        T a;
    };

    constexpr TVec4() = default;
    constexpr TVec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w)
    {}
    explicit constexpr TVec4(T value) : x(value), y(value), z(value), w(value)
    {}
    constexpr TVec4(TVec2<T> xy, TVec2<T> zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y)
    {}
    constexpr TVec4(TVec2<T> xy, T z, T w) : x(xy.x), y(xy.y), z(z), w(w)
    {}
    constexpr TVec4(TVec3<T> xyz, T w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w)
    {}
    template <class U> constexpr TVec4(TVec4<U> v) : x((T)v.x), y((T)v.y), z((T)v.z), w((T)v.w)
    {}
    constexpr T& operator[](int i)
    {
        return i == 0 ? x : i == 1 ? y : i == 2 ? z : w;
    }
    constexpr T operator[](int i) const
    {
        return i == 0 ? x : i == 1 ? y : i == 2 ? z : w;
    }
};

using Vec2 = TVec2<float>;
using Vec3 = TVec3<float>;
using Vec4 = TVec4<float>;
using IVec2 = TVec2<int>;
using IVec4 = TVec4<int>;

template <class V>
concept Vector = requires {
    V::SIZE;
    typename V::Value;
};

#define VEC_OPERATOR(op, assign)                                                                                       \
    template <Vector V, Vector W>                                                                                      \
        requires(V::SIZE == W::SIZE)                                                                                   \
    constexpr V& operator assign(V& a, W b)                                                                            \
    {                                                                                                                  \
        for (int i = 0; i < V::SIZE; ++i)                                                                              \
            a[i] assign b[i];                                                                                          \
        return a;                                                                                                      \
    }                                                                                                                  \
    template <Vector V> constexpr V& operator assign(V& a, typename V::Value value)                                    \
    {                                                                                                                  \
        for (int i = 0; i < V::SIZE; ++i)                                                                              \
            a[i] assign value;                                                                                         \
        return a;                                                                                                      \
    }                                                                                                                  \
    template <Vector V> constexpr V operator op(V a, V b)                                                              \
    {                                                                                                                  \
        return a assign b;                                                                                             \
    }                                                                                                                  \
    template <Vector V> constexpr V operator op(V a, typename V::Value value)                                          \
    {                                                                                                                  \
        return a assign value;                                                                                         \
    }                                                                                                                  \
    template <Vector V> constexpr V operator op(typename V::Value value, V a)                                          \
    {                                                                                                                  \
        auto result = V(value);                                                                                        \
        return result assign a;                                                                                        \
    }
VEC_OPERATOR(+, +=)
VEC_OPERATOR(-, -=)
VEC_OPERATOR(*, *=)
VEC_OPERATOR(/, /=)
#undef VEC_OPERATOR

template <Vector V> constexpr V operator-(V v)
{
    for (int i = 0; i < V::SIZE; ++i)
        v[i] = -v[i];
    return v;
}

template <Vector V> constexpr bool operator==(V a, V b)
{
    for (int i = 0; i < V::SIZE; ++i)
        if (a[i] != b[i])
            return false;
    return true;
}

// Applies `function` to each component of a vector, or to a plain number.
template <class T, class F> constexpr T components_map(T a, F function)
{
    if constexpr (Vector<T>)
    {
        for (int i = 0; i < T::SIZE; ++i)
            a[i] = function(a[i]);
        return a;
    }
    else
        return function(a);
}

template <class T, class F> constexpr T components_map(T a, T b, F function)
{
    if constexpr (Vector<T>)
    {
        for (int i = 0; i < T::SIZE; ++i)
            a[i] = function(a[i], b[i]);
        return a;
    }
    else
        return function(a, b);
}

template <class T> struct VectorValue
{
    using Type = T;
};

template <Vector V> struct VectorValue<V>
{
    using Type = typename V::Value;
};

// A vector, or a number for every component.
template <class T> using Bound = std::type_identity_t<T>;

template <class T> constexpr T vec_min(T a, Bound<T> b)
{
    return components_map(a, b, [](auto x, auto y) { return y < x ? y : x; });
}

template <class T> constexpr T vec_max(T a, Bound<T> b)
{
    return components_map(a, b, [](auto x, auto y) { return x < y ? y : x; });
}

template <Vector V> constexpr V vec_min(V a, typename V::Value b)
{
    return vec_min(a, V(b));
}

template <Vector V> constexpr V vec_max(V a, typename V::Value b)
{
    return vec_max(a, V(b));
}

template <class T, class B> constexpr T vec_clamp(T value, B low, B high)
{
    return vec_min(vec_max(value, low), high);
}

template <class T> constexpr T vec_mix(T a, T b, typename VectorValue<T>::Type amount)
{
    return a * (1 - amount) + b * amount;
}

template <Vector V> constexpr V vec_mix(V a, V b, V amount)
{
    return a * (V(1) - amount) + b * amount;
}

template <class T> T vec_floor(T v)
{
    return components_map(v, [](auto x) { return std::floor(x); });
}

template <class T> T vec_ceil(T v)
{
    return components_map(v, [](auto x) { return std::ceil(x); });
}

template <class T> T vec_round(T v)
{
    return components_map(v, [](auto x) { return std::round(x); });
}

template <class T> T vec_abs(T v)
{
    return components_map(v, [](auto x) { return std::abs(x); });
}

template <class T> constexpr T vec_sign(T v)
{
    return components_map(v, [](auto x) { return (decltype(x))((0 < x) - (x < 0)); });
}

template <Vector V> constexpr typename V::Value vec_dot(V a, V b)
{
    typename V::Value sum = 0;
    for (int i = 0; i < V::SIZE; ++i)
        sum += a[i] * b[i];
    return sum;
}

template <Vector V> typename V::Value vec_length(V v)
{
    return std::sqrt(vec_dot(v, v));
}

template <Vector V> typename V::Value vec_distance(V a, V b)
{
    return vec_length(b - a);
}

template <Vector V> V vec_normalize(V v)
{
    return v * (1 / std::sqrt(vec_dot(v, v)));
}

// Whether `point` lies in [low, high), each axis.
template <Vector V> constexpr bool is_within(V point, V low, V high)
{
    for (int i = 0; i < V::SIZE; ++i)
        if (point[i] < low[i] || !(point[i] < high[i]))
            return false;
    return true;
}

// Whether every component of `a` is at most the one of `b`.
template <Vector V> constexpr bool is_all_less_equal(V a, V b)
{
    for (int i = 0; i < V::SIZE; ++i)
        if (b[i] < a[i])
            return false;
    return true;
}

// Whether any component of `a` is at most the one of `b`.
template <Vector V> constexpr bool is_any_less_equal(V a, V b)
{
    for (int i = 0; i < V::SIZE; ++i)
        if (!(b[i] < a[i]))
            return true;
    return false;
}

constexpr float radians(float degrees)
{
    return degrees * 0.01745329251994329576923690768489f;
}

constexpr float degrees(float radians)
{
    return radians * 57.295779513082320876798154814105f;
}

struct Mat4
{
    Vec4 columns[4];

    constexpr Mat4() = default;
    constexpr Mat4(Vec4 c0, Vec4 c1, Vec4 c2, Vec4 c3) : columns{c0, c1, c2, c3}
    {}
    explicit constexpr Mat4(float diagonal)
        : columns{{diagonal, 0, 0, 0}, {0, diagonal, 0, 0}, {0, 0, diagonal, 0}, {0, 0, 0, diagonal}}
    {}
    constexpr Vec4& operator[](int i)
    {
        return columns[i];
    }
    constexpr Vec4 operator[](int i) const
    {
        return columns[i];
    }
};

constexpr Vec4 operator*(const Mat4& m, Vec4 v)
{
    return m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w;
}

constexpr Mat4 operator*(const Mat4& a, const Mat4& b)
{
    Mat4 result;
    for (int i = 0; i < 4; ++i)
        result[i] = a * b[i];
    return result;
}

constexpr Mat4& operator*=(Mat4& a, const Mat4& b)
{
    return a = a * b;
}

constexpr bool operator==(const Mat4& a, const Mat4& b)
{
    for (int i = 0; i < 4; ++i)
        if (a[i] != b[i])
            return false;
    return true;
}

constexpr Mat4 mat4_translate(Mat4 m, Vec3 v)
{
    m[3] = m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3];
    return m;
}

constexpr Mat4 mat4_scale(Mat4 m, Vec3 v)
{
    m[0] *= v.x;
    m[1] *= v.y;
    m[2] *= v.z;
    return m;
}

// Turns `m` by `angle` radians around the z axis.
inline Mat4 mat4_rotate(Mat4 m, float angle)
{
    auto cosine = std::cos(angle);
    auto sine = std::sin(angle);
    auto column0 = m[0] * cosine + m[1] * sine;
    m[1] = m[0] * -sine + m[1] * cosine;
    m[0] = column0;
    return m;
}

constexpr Mat4 mat4_ortho(float left, float right, float bottom, float top, float near, float far)
{
    Mat4 result(1.0f);
    result[0][0] = 2 / (right - left);
    result[1][1] = 2 / (top - bottom);
    result[2][2] = -2 / (far - near);
    result[3][0] = -(right + left) / (right - left);
    result[3][1] = -(top + bottom) / (top - bottom);
    result[3][2] = -(far + near) / (far - near);
    return result;
}

// The 2x2 minors of the bottom two rows, shared by the determinant and the inverse.
struct Mat4Minors
{
    float s[6];
    float c[6];
};

constexpr Mat4Minors mat4_minors_get(const Mat4& m)
{
    return {{m[0][0] * m[1][1] - m[1][0] * m[0][1], m[0][0] * m[1][2] - m[1][0] * m[0][2],
             m[0][0] * m[1][3] - m[1][0] * m[0][3], m[0][1] * m[1][2] - m[1][1] * m[0][2],
             m[0][1] * m[1][3] - m[1][1] * m[0][3], m[0][2] * m[1][3] - m[1][2] * m[0][3]},
            {m[2][2] * m[3][3] - m[3][2] * m[2][3], m[2][1] * m[3][3] - m[3][1] * m[2][3],
             m[2][1] * m[3][2] - m[3][1] * m[2][2], m[2][0] * m[3][3] - m[3][0] * m[2][3],
             m[2][0] * m[3][2] - m[3][0] * m[2][2], m[2][0] * m[3][1] - m[3][0] * m[2][1]}};
}

constexpr float mat4_determinant(const Mat4& m)
{
    auto [s, c] = mat4_minors_get(m);
    return s[0] * c[0] - s[1] * c[1] + s[2] * c[2] + s[3] * c[3] - s[4] * c[4] + s[5] * c[5];
}

constexpr Mat4 mat4_inverse(const Mat4& m)
{
    auto [s, c] = mat4_minors_get(m);
    auto inverse = 1 / (s[0] * c[0] - s[1] * c[1] + s[2] * c[2] + s[3] * c[3] - s[4] * c[4] + s[5] * c[5]);
    Mat4 result;
    result[0] =
        Vec4(m[1][1] * c[0] - m[1][2] * c[1] + m[1][3] * c[2], -m[0][1] * c[0] + m[0][2] * c[1] - m[0][3] * c[2],
             m[3][1] * s[5] - m[3][2] * s[4] + m[3][3] * s[3], -m[2][1] * s[5] + m[2][2] * s[4] - m[2][3] * s[3]);
    result[1] =
        Vec4(-m[1][0] * c[0] + m[1][2] * c[3] - m[1][3] * c[4], m[0][0] * c[0] - m[0][2] * c[3] + m[0][3] * c[4],
             -m[3][0] * s[5] + m[3][2] * s[2] - m[3][3] * s[1], m[2][0] * s[5] - m[2][2] * s[2] + m[2][3] * s[1]);
    result[2] =
        Vec4(m[1][0] * c[1] - m[1][1] * c[3] + m[1][3] * c[5], -m[0][0] * c[1] + m[0][1] * c[3] - m[0][3] * c[5],
             m[3][0] * s[4] - m[3][1] * s[2] + m[3][3] * s[0], -m[2][0] * s[4] + m[2][1] * s[2] - m[2][3] * s[0]);
    result[3] =
        Vec4(-m[1][0] * c[2] + m[1][1] * c[4] - m[1][2] * c[5], m[0][0] * c[2] - m[0][1] * c[4] + m[0][2] * c[5],
             -m[3][0] * s[3] + m[3][1] * s[1] - m[3][2] * s[0], m[2][0] * s[3] - m[2][1] * s[1] + m[2][2] * s[0]);
    for (auto& column : result.columns)
        column *= inverse;
    return result;
}

// The floats behind a vector or matrix, for OpenGL and ImGui.
template <Vector V> constexpr typename V::Value* values_get(V& v)
{
    return &v.x;
}

template <Vector V> constexpr const typename V::Value* values_get(const V& v)
{
    return &v.x;
}

constexpr float* values_get(Mat4& m)
{
    return &m.columns[0].x;
}

constexpr const float* values_get(const Mat4& m)
{
    return &m.columns[0].x;
}
