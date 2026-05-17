// math.hpp
#pragma once
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

class vector3
{
private:
    double x, y, z;

public:
    vector3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}

    double magnitude() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    vector3 normalize() const
    {
        double mag = magnitude();
        if (mag == 0)
            return vector3(0, 0, 0);
        return vector3(x / mag, y / mag, z / mag);
    }

    friend std::ostream &operator<<(std::ostream &os, const vector3 &v)
    {
        os << "vector3(" << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }

    double getX() const { return x; }
    double getY() const { return y; }
    double getZ() const { return z; }

    void setX(double newX) { x = newX; }
    void setY(double newY) { y = newY; }
    void setZ(double newZ) { z = newZ; }

    vector3 operator+(const vector3 &other) const
    {
        return vector3(x + other.x, y + other.y, z + other.z);
    }

    vector3 operator-(const vector3 &other) const
    {
        return vector3(x - other.x, y - other.y, z - other.z);
    }
    vector3 operator*(double scalar) const
    {
        return vector3(x * scalar, y * scalar, z * scalar);
    }

    double dot(const vector3 &other) const
    {
        return x * other.x + y * other.y + z * other.z;
    }
    vector3 cross(const vector3 &other) const
    {
        return vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x);
    }

    vector3 lerp(const vector3 &other, double t) const
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
    double x, y;

public:
    vector2(double x = 0, double y = 0) : x(x), y(y) {}

    double magnitude() const
    {
        return std::sqrt(x * x + y * y);
    }

    vector2 normalize() const
    {
        double mag = magnitude();
        if (mag == 0)
            return vector2(0, 0);
        return vector2(x / mag, y / mag);
    }

    friend std::ostream &operator<<(std::ostream &os, const vector2 &v)
    {
        os << "vector2(" << v.x << ", " << v.y << ")";
        return os;
    }

    double getX() const { return x; }
    double getY() const { return y; }

    void setX(double newX) { x = newX; }
    void setY(double newY) { y = newY; }

    vector2 operator+(const vector2 &other) const
    {
        return vector2(x + other.x, y + other.y);
    }
    vector2 operator-(const vector2 &other) const
    {
        return vector2(x - other.x, y - other.y);
    }
    vector2 operator*(double scalar) const
    {
        return vector2(x * scalar, y * scalar);
    }

    double dot(const vector2 &other) const
    {
        return x * other.x + y * other.y;
    }

    vector2 lerp(const vector2 &other, double t) const
    {
        return vector2(
            x + (other.x - x) * t,
            y + (other.y - y) * t);
    }

    // clockwise 90 degrees rotation
    // for counter-clockwise, use vector2(-y, x)
    vector2 orthogonal() const
    {
        return vector2(y, -x);
    }

    double signed_triangle_area(const vector2 &a, const vector2 &b, const vector2 &c);
    bool insideTriangle(vector2 a, vector2 b, vector2 c, vector3 &weights);
};

double clamp(double value, double min, double max);
double degrees_to_radians(double degrees);
vector3 get_random_colour();
int get_index(int x, int y, int width);
