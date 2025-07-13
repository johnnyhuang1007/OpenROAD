#ifndef POINT_H
#define POINT_H

#include <algorithm>
#include <cmath>

/**
 * @brief Template class for 2D point
 *
 * @tparam float Type of the point
 */
struct FloatPoint {
    /////////////////////////////////
    // Constructors
    /////////////////////////////////

    /// Default constructor
    FloatPoint() : x(0), y(0) {}

    /// Scalar constructor
    explicit FloatPoint(const float &u) : x(u), y(u) {}

    /// Construct from two values
    FloatPoint(const float &u, const float &v) : x(u), y(v) {}

    /////////////////////////////////
    // Assignment operators
    /////////////////////////////////

    /// Copy from scalar
    FloatPoint &operator=(const float &u) {
        x = u;
        y = u;
        return *this;
    }

    /////////////////////////////////
    // Indexing operators
    /////////////////////////////////

    /// Access by index
    float &operator[](const size_t &i) { return array_[i]; }

    /// Access by index
    const float &operator[](const size_t &i) const { return array_[i]; }

    /////////////////////////////////
    // Math operators
    /////////////////////////////////

    /// Scalar add and assign
    FloatPoint &operator+=(const float &rhs) {
        x += rhs;
        y += rhs;
        return *this;
    }

    /// Element-wise add and assign
    FloatPoint &operator+=(const FloatPoint &rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    /// Scalar subtract and assign
    FloatPoint &operator-=(const float &rhs) {
        x -= rhs;
        y -= rhs;
        return *this;
    }

    /// Element-wise subtract and assign
    FloatPoint &operator-=(const FloatPoint &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    /// Scalar multiply and assign
    FloatPoint &operator*=(const float &rhs) {
        x *= rhs;
        y *= rhs;
        return *this;
    }

    /// Element-wise multiply and assign
    FloatPoint &operator*=(const FloatPoint &rhs) {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    /// Scalar divide and assign
    FloatPoint &operator/=(const float &rhs) {
        x /= rhs;
        y /= rhs;
        return *this;
    }

    /// Element-wise divide and assign
    FloatPoint &operator/=(const FloatPoint &rhs) {
        x /= rhs.x;
        y /= rhs.y;
        return *this;
    }

    /// Unary negate
    FloatPoint operator-() const { return FloatPoint(-x, -y); }

    /// Scalar(right) add
    friend FloatPoint operator+(const FloatPoint lhs, const float &rhs) {
        return FloatPoint(lhs) += rhs;
    }

    /// Scalar(left) add
    friend FloatPoint operator+(const float &lhs, const FloatPoint rhs) {
        return FloatPoint(lhs) += rhs;
    }

    /// Element-wise add
    friend FloatPoint operator+(const FloatPoint lhs, const FloatPoint &rhs) {
        return FloatPoint(lhs) += rhs;
    }

    /// Scalar(right) subtract
    friend FloatPoint operator-(const FloatPoint lhs, const float &rhs) {
        return FloatPoint(lhs) -= rhs;
    }

    /// Scalar(left) subtract
    friend FloatPoint operator-(const float &lhs, const FloatPoint rhs) {
        return FloatPoint(lhs) -= rhs;
    }

    /// Element-wise subtract
    friend FloatPoint operator-(const FloatPoint lhs, const FloatPoint &rhs) {
        return FloatPoint(lhs) -= rhs;
    }

    /// Scalar(right) multiply
    friend FloatPoint operator*(const FloatPoint lhs, const float &rhs) {
        return FloatPoint(lhs) *= rhs;
    }

    /// Scalar(left) multiply
    friend FloatPoint operator*(const float &lhs, const FloatPoint rhs) {
        return FloatPoint(lhs) *= rhs;
    }

    /// Element-wise multiply
    friend FloatPoint operator*(const FloatPoint lhs, const FloatPoint &rhs) {
        return FloatPoint(lhs) *= rhs;
    }

    /// Scalar(right) divide
    friend FloatPoint operator/(const FloatPoint lhs, const float &rhs) {
        return FloatPoint(lhs) /= rhs;
    }

    /// Scalar(left) divide
    friend FloatPoint operator/(const float &lhs, const FloatPoint rhs) {
        return FloatPoint(lhs) /= rhs;
    }

    /// Element-wise divide
    friend FloatPoint operator/(const FloatPoint lhs, const FloatPoint &rhs) {
        return FloatPoint(lhs) /= rhs;
    }

    /////////////////////////////////
    // Comparison operators
    /////////////////////////////////

    /// Equality
    friend bool operator==(const FloatPoint &lhs, const FloatPoint &rhs) {
        return (lhs.x == rhs.x) && (lhs.y == rhs.y);
    }

    /// Inequality
    friend bool operator!=(const FloatPoint &lhs, const FloatPoint &rhs) {
        return !(lhs == rhs);
    }

    /////////////////////////////////
    // Math functions
    /////////////////////////////////

    /// Dot product
    friend float Dot(const FloatPoint &lhs, const FloatPoint &rhs) {
        return lhs.x * rhs.x + lhs.y * rhs.y;
    }

    /// Cross product
    friend float Cross(const FloatPoint &lhs, const FloatPoint &rhs) {
        return lhs.x * rhs.y - lhs.y * rhs.x;
    }

    /// 2-norm
    friend float Norm2(const FloatPoint &u) { return std::sqrt(u.x * u.x + u.y * u.y); }

    /// Element-wise exponential
    friend FloatPoint Exp(const FloatPoint &u) {
        return FloatPoint(std::exp(u.x), std::exp(u.y));
    }

    /// Element-wise minimum
    friend FloatPoint Min(const FloatPoint &u, const FloatPoint &v) {
        return FloatPoint(std::min(u.x, v.x), std::min(u.y, v.y));
    }

    /// Element-wise maximum
    friend FloatPoint Max(const FloatPoint &u, const FloatPoint &v) {
        return FloatPoint(std::max(u.x, v.x), std::max(u.y, v.y));
    }

    /// Element-wise clamp
    friend FloatPoint Clamp(const FloatPoint &u, const FloatPoint &lo, const FloatPoint &hi) {
        return FloatPoint(std::clamp(u.x, lo.x, hi.x), std::clamp(u.y, lo.y, hi.y));
    }

    /// Is finite
    friend bool IsFinite(const FloatPoint &u) {
        return std::isfinite(u.x) && std::isfinite(u.y);
    }

    /////////////////////////////////
    // Data members
    /////////////////////////////////

    // With unnamed union, we can access x and y directly
    // For example:
    //
    // FloatPoint p;
    // p.x = 1; // This will work
    // p.y = 1; // This will work
    // p[0] = 1; // This also will work, which is equivalent to p.x = 1
    // p[1] = 1; // This also will work, which is equivalent to p.y = 1
    //
    // Here, p.x and p[0] shared the same memory location, so does p.y and p[1]
    union {
        struct
        {
            float x;
            float y;
        };
        float array_[2];
    };
};

inline float abs(const FloatPoint &u) {
    return std::sqrt(u.x * u.x + u.y * u.y);
}

#endif  // POINT_H
