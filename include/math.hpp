// math.hpp
#pragma once
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

class vector3
{
private:
    float x, y, z;

public:
    inline vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    inline float magnitude() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    inline vector3 normalize() const
    {
        float mag = magnitude();
        if (mag == 0)
            return vector3(0, 0, 0);
        return vector3(x / mag, y / mag, z / mag);
    }

    friend std::ostream &operator<<(std::ostream &os, const vector3 &v)
    {
        os << "vector3(" << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }

    inline float getX() const { return x; }
    inline float getY() const { return y; }
    inline float getZ() const { return z; }

    inline void setX(float newX) { x = newX; }
    inline void setY(float newY) { y = newY; }
    inline void setZ(float newZ) { z = newZ; }

    inline vector3 operator+(const vector3 &other) const
    {
        return vector3(x + other.x, y + other.y, z + other.z);
    }

    inline vector3 operator-(const vector3 &other) const
    {
        return vector3(x - other.x, y - other.y, z - other.z);
    }
    inline vector3 operator*(float scalar) const
    {
        return vector3(x * scalar, y * scalar, z * scalar);
    }

    inline float dot(const vector3 &other) const
    {
        return x * other.x + y * other.y + z * other.z;
    }
    inline vector3 cross(const vector3 &other) const
    {
        return vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x);
    }

    inline vector3 lerp(const vector3 &other, float t) const
    {
        return vector3(
            x + (other.x - x) * t,
            y + (other.y - y) * t,
            z + (other.z - z) * t);
    }
};

class vector2
{
private:
    float x, y;

public:
    inline vector2(float x = 0, float y = 0) : x(x), y(y) {}

    inline float magnitude() const
    {
        return std::sqrt(x * x + y * y);
    }

    inline vector2 normalize() const
    {
        float mag = magnitude();
        if (mag == 0)
            return vector2(0, 0);
        return vector2(x / mag, y / mag);
    }

    friend std::ostream &operator<<(std::ostream &os, const vector2 &v)
    {
        os << "vector2(" << v.x << ", " << v.y << ")";
        return os;
    }

    inline float getX() const { return x; }
    inline float getY() const { return y; }

    inline void setX(float newX) { x = newX; }
    inline void setY(float newY) { y = newY; }

    inline vector2 operator+(const vector2 &other) const
    {
        return vector2(x + other.x, y + other.y);
    }
    inline vector2 operator-(const vector2 &other) const
    {
        return vector2(x - other.x, y - other.y);
    }
    inline vector2 operator*(float scalar) const
    {
        return vector2(x * scalar, y * scalar);
    }

    inline float dot(const vector2 &other) const
    {
        return x * other.x + y * other.y;
    }

    inline vector2 lerp(const vector2 &other, float t) const
    {
        return vector2(
            x + (other.x - x) * t,
            y + (other.y - y) * t);
    }

    // clockwise 90 degrees rotation
    // for counter-clockwise, use vector2(-y, x)
    inline vector2 orthogonal() const
    {
        return vector2(y, -x);
    }

    float signed_triangle_area(const vector2 &a, const vector2 &b, const vector2 &c);
    bool insideTriangle(vector2 a, vector2 b, vector2 c, vector3 &weights);
};

float clamp(float value, float min, float max);
float degrees_to_radians(float degrees);
vector3 get_random_colour();
int get_index(int x, int y, int width);
