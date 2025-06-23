#pragma once

#include <string>
#include <vector>
#include <memory>
#include <list>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include "math.hpp"

// ==================== Image Class ====================
class Image
{
public:
    int width, height;
    std::vector<vector3> pixels;
    std::vector<double> depth;

    Image(int width = 0, int height = 0) : width(width), height(height)
    {
        pixels.resize(width * height, vector3(0, 0, 0));
        depth.resize(width * height, std::numeric_limits<float>::max());
    }

    void clearPixels(const vector3 &color = vector3(0, 0, 0))
    {
        std::fill(pixels.begin(), pixels.end(), color);
    }

    void clearDepth(float val = std::numeric_limits<float>::max())
    {
        std::fill(depth.begin(), depth.end(), val);
    }
};

// ==================== Texture Class ====================
class Texture
{
public:
    std::string filename;
    Image image;

    Texture(const std::string &filename) : filename(filename)
    {
        std::string ext = filename.substr(filename.find_last_of(".") + 1);
        if (ext == "bytes")
            from_bytes(filename);
        else if (ext == "bmp")
            from_bmp(filename);
        else
            throw std::runtime_error("Unsupported texture format: " + filename);
    }

    void from_bmp(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file)
            throw std::runtime_error("Failed to open: " + filename);

        file.seekg(18);
        int32_t width, height;
        file.read(reinterpret_cast<char *>(&width), 4);
        file.read(reinterpret_cast<char *>(&height), 4);

        file.seekg(28);
        uint16_t bpp;
        file.read(reinterpret_cast<char *>(&bpp), 2);
        bool hasAlpha = (bpp == 32);

        file.seekg(54);
        image = Image(width, height);

        int rowSize = ((bpp * width + 31) / 32) * 4;
        int pixelSize = hasAlpha ? 4 : 3;
        int padding = rowSize - (pixelSize * width);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                unsigned char r, g, b, a = 255;
                file.read(reinterpret_cast<char *>(&b), 1);
                file.read(reinterpret_cast<char *>(&g), 1);
                file.read(reinterpret_cast<char *>(&r), 1);
                if (hasAlpha)
                    file.read(reinterpret_cast<char *>(&a), 1);
                image.pixels[y * width + x] = vector3(r, g, b);
            }
            if (padding > 0)
                file.ignore(padding);
        }

        if (image.pixels.empty())
            throw std::runtime_error("Failed to load texture: " + filename);
    }

    void from_bytes(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file)
            throw std::runtime_error("Failed to open: " + filename);

        uint16_t w, h;
        file.read(reinterpret_cast<char *>(&w), 2);
        file.read(reinterpret_cast<char *>(&h), 2);
        image = Image(w, h);

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                unsigned char r, g, b;
                file.read(reinterpret_cast<char *>(&r), 1);
                file.read(reinterpret_cast<char *>(&g), 1);
                file.read(reinterpret_cast<char *>(&b), 1);
                image.pixels[y * w + x] = vector3(r, g, b);
            }
        }
    }

    inline vector3 get_color(double u, double v) const
    {
        u = clamp(u, 0.0, 1.0);
        v = clamp(v, 0.0, 1.0);
        int x = static_cast<int>(u * (image.width - 1));
        int y = static_cast<int>(v * (image.height - 1));
        return image.pixels[y * image.width + x];
    }
};

// ==================== Shader Class ====================
class Shader
{
public:
    Texture texture;

    Shader(const std::string &texture_filename) : texture(texture_filename)
    {
        if (texture.image.width == 0 || texture.image.height == 0)
            throw std::runtime_error("Failed to load texture: " + texture_filename);
    }

    inline vector3 get_colour(const vector2 &uv) const
    {
        return texture.get_color(uv.getX(), uv.getY());
    }
};

// ==================== Transform Class ====================
class Transform
{
public:
    double yaw, pitch, roll;
    vector3 position;
    vector3 scale;

    mutable bool cache_valid = false;
    mutable std::vector<vector3> cached_base, cached_inverse;

    Transform(double yaw = 0, double pitch = 0, double roll = 0, vector3 position = vector3(0, 0, 0), vector3 scale = vector3(1, 1, 1))
        : yaw(yaw), pitch(pitch), roll(roll), position(position), scale(scale) {}

    static vector3 transform(const std::vector<vector3> &base, const vector3 &p)
    {
        return vector3(
            base[0].getX() * p.getX() + base[1].getX() * p.getY() + base[2].getX() * p.getZ(),
            base[0].getY() * p.getX() + base[1].getY() * p.getY() + base[2].getY() * p.getZ(),
            base[0].getZ() * p.getX() + base[1].getZ() * p.getY() + base[2].getZ() * p.getZ());
    }

    const std::vector<vector3> &get_base_vectors() const
    {
        if (cache_valid)
            return cached_base;

        vector3 i_yaw = vector3(cos(yaw), 0, sin(yaw));
        vector3 j_yaw = vector3(0, 1, 0);
        vector3 k_yaw = vector3(-sin(yaw), 0, cos(yaw));

        vector3 i_pitch = vector3(1, 0, 0);
        vector3 j_pitch = vector3(0, cos(pitch), -sin(pitch));
        vector3 k_pitch = vector3(0, sin(pitch), cos(pitch));

        vector3 i_roll = vector3(cos(roll), sin(roll), 0);
        vector3 j_roll = vector3(-sin(roll), cos(roll), 0);
        vector3 k_roll = vector3(0, 0, 1);

        vector3 i = transform({i_yaw, j_yaw, k_yaw}, i_pitch);
        vector3 j = transform({i_yaw, j_yaw, k_yaw}, j_pitch);
        vector3 k = transform({i_yaw, j_yaw, k_yaw}, k_pitch);

        vector3 final_i = transform({i, j, k}, i_roll);
        vector3 final_j = transform({i, j, k}, j_roll);
        vector3 final_k = transform({i, j, k}, k_roll);

        cached_base = {final_i, final_j, final_k};
        cached_inverse = {
            vector3(final_i.getX(), final_j.getX(), final_k.getX()),
            vector3(final_i.getY(), final_j.getY(), final_k.getY()),
            vector3(final_i.getZ(), final_j.getZ(), final_k.getZ())};
        cache_valid = true;
        return cached_base;
    }

    const std::vector<vector3> &get_inverse_base_vectors() const
    {
        if (!cache_valid)
            get_base_vectors(); // triggers both caches
        return cached_inverse;
    }

    inline vector3 to_world_point(const vector3 &p) const
    {
        std::vector<vector3> base_vectors = get_base_vectors();
        base_vectors[0] = base_vectors[0] * scale.getX();
        base_vectors[1] = base_vectors[1] * scale.getY();
        base_vectors[2] = base_vectors[2] * scale.getZ();
        return transform(base_vectors, p) + position;
    }

    inline vector3 to_local_point(const vector3 &p) const
    {
        std::vector<vector3> inverse_base_vectors = get_inverse_base_vectors();
        vector3 local_point = p - position;
        local_point = transform(inverse_base_vectors, local_point);
        local_point.setX(local_point.getX() / scale.getX());
        local_point.setY(local_point.getY() / scale.getY());
        local_point.setZ(local_point.getZ() / scale.getZ());
        return local_point;
    }

    void set_rotation(double new_yaw, double new_pitch, double new_roll)
    {
        yaw = new_yaw;
        pitch = new_pitch;
        roll = new_roll;
        cache_valid = false;
    }

    void rotate(double delta_yaw, double delta_pitch, double delta_roll)
    {
        yaw += delta_yaw;
        pitch += delta_pitch;
        roll += delta_roll;
        cache_valid = false;
    }
};
// ==================== Model Class ====================
class Model
{
public:
    std::vector<vector3> points;
    std::vector<vector3> normals;
    std::vector<vector2> texture_coords;
    Transform transform;
    Shader shader;

    Model(const std::vector<vector3> &pts,
          const std::vector<vector3> &norms,
          const std::vector<vector2> &uvs,
          const Transform &trans,
          const Shader &shader)
        : points(pts), normals(norms), texture_coords(uvs), transform(trans), shader(shader) {}

    inline vector2 get_texture_coord(int idx) const
    {
        if (idx < 0 || idx >= static_cast<int>(texture_coords.size()))
            throw std::out_of_range("Texture coordinate index out of range");
        return texture_coords[idx];
    }
};

// ==================== Camera Class ====================
class Camera
{
public:
    double fov;
    Transform transform;

    Camera(double fov = 60.0, const Transform &transform = Transform())
        : fov(degrees_to_radians(fov)), transform(transform) {}
};

// ==================== Scene Class ====================
class Scene
{
public:
    std::vector<Model> models;
    Camera camera;

    Scene(const std::vector<Model> &models = {},
          const Camera &camera = Camera())
        : models(models), camera(camera) {}

    void addModel(const Model &model)
    {
        models.push_back(model);
    }

    void setCamera(const Camera &cam)
    {
        camera = cam;
    }
};
