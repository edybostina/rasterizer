#pragma once

#include <string>
#include <vector>
#include <memory>
#include <list>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cstdint>
#include "math.hpp"

// ==================== Image Class ====================
class Image
{
public:
    int width, height;
    std::vector<uint32_t> pixels;
    std::vector<float> depth;

    Image(int width = 0, int height = 0) : width(width), height(height)
    {
        pixels.resize(width * height, 0);
        depth.resize(width * height, std::numeric_limits<float>::max());
    }

    inline void clearPixels(uint32_t color = 0)
    {
        std::fill(pixels.begin(), pixels.end(), color);
    }

    inline void clearDepth(float val = std::numeric_limits<float>::max())
    {
        std::fill(depth.begin(), depth.end(), val);
    }
};

// ==================== Texture Class ====================
class Texture
{
public:
    std::string filename;
    std::vector<vector3> pixels; // Internal texture storage as float vectors for easier blending
    int width, height;
    vector3 base_color;
    
    Texture(const std::string &filename);

    void from_bmp(const std::string &filename);
    void from_bytes(const std::string &filename);

    inline vector3 get_color(float u, float v) const
    {
        u = clamp(u, 0.0f, 1.0f);
        v = clamp(v, 0.0f, 1.0f);
        int x = static_cast<int>(u * (width - 1));
        int y = static_cast<int>(v * (height - 1));
        return pixels[y * width + x];
    }
};

// ==================== Shader Class ====================
class Shader
{
public:
    Texture texture;
    vector3 directional_light;
    bool has_texture = false;
    Shader(const std::string &texture_filename = "_no_texture", const vector3 directional_light = vector3(0.3f, 1.0f, 0.6f).normalize());

    inline uint32_t get_colour(const vector2 &uv, vector3 normal) const
    {
        normal = normal.normalize();
        float light_intensity = (normal.dot(directional_light) + 1.0f) * 0.5f;
        vector3 color = has_texture ? texture.get_color(uv.getX(), uv.getY()) : texture.base_color;

        uint8_t r = static_cast<uint8_t>(clamp(color.getX() * light_intensity, 0.0f, 255.0f));
        uint8_t g = static_cast<uint8_t>(clamp(color.getY() * light_intensity, 0.0f, 255.0f));
        uint8_t b = static_cast<uint8_t>(clamp(color.getZ() * light_intensity, 0.0f, 255.0f));
        
        // Return packed ARGB8888
        return (255u << 24) | (r << 16) | (g << 8) | b;
    }
};

// ==================== Transform Class ====================
class Transform
{
public:
    float yaw, pitch, roll;
    vector3 position;
    vector3 scale;

    mutable bool cache_valid = false;
    mutable vector3 cached_base[3];
    mutable vector3 cached_inverse[3];

    Transform(float yaw = 0, float pitch = 0, float roll = 0, vector3 position = vector3(0, 0, 0), vector3 scale = vector3(1, 1, 1))
        : yaw(yaw), pitch(pitch), roll(roll), position(position), scale(scale) {}

    static vector3 transform(const vector3 base[3], const vector3 &p);
    void update_cache() const;

    inline vector3 to_world_point(const vector3 &p) const
    {
        if (!cache_valid) update_cache();
        vector3 scaled_base[3] = {
            cached_base[0] * scale.getX(),
            cached_base[1] * scale.getY(),
            cached_base[2] * scale.getZ()
        };
        return transform(scaled_base, p) + position;
    }

    inline vector3 to_local_point(const vector3 &p) const
    {
        if (!cache_valid) update_cache();
        vector3 local_point = p - position;
        local_point = transform(cached_inverse, local_point);
        return vector3(local_point.getX() / scale.getX(),
                       local_point.getY() / scale.getY(),
                       local_point.getZ() / scale.getZ());
    }

    void set_rotation(float new_yaw, float new_pitch, float new_roll)
    {
        yaw = new_yaw;
        pitch = new_pitch;
        roll = new_roll;
        cache_valid = false;
    }

    void rotate(float delta_yaw, float delta_pitch, float delta_roll)
    {
        yaw += delta_yaw;
        pitch += delta_pitch;
        roll += delta_roll;
        cache_valid = false;
    }

    inline vector3 transform_normal(const vector3 &n) const
    {
        if (!cache_valid) update_cache();
        return transform(cached_base, n).normalize();
    }
};

// =| Model Class =|
class Model
{
public:
    std::vector<vector3> points;
    std::vector<vector3> normals;
    std::vector<vector2> texture_coords;
    Transform transform;
    std::shared_ptr<Shader> shader;

    Model(const std::vector<vector3> &pts,
          const std::vector<vector3> &norms,
          const std::vector<vector2> &uvs,
          const Transform &trans,
          const std::shared_ptr<Shader> &shader)
        : points(pts), normals(norms), texture_coords(uvs), transform(trans), shader(shader) {}

    Model(const std::vector<vector3> &pts,
          const std::vector<vector3> &norms,
          const std::vector<vector2> &uvs,
          const Transform &trans,
          const Shader &sh)
        : points(pts), normals(norms), texture_coords(uvs), transform(trans), shader(std::make_shared<Shader>(sh)) {}

    inline vector2 get_texture_coord(int idx) const
    {
        return texture_coords[idx];
    }
};

// ==================== Camera Class ====================
class Camera
{
public:
    float fov;
    Transform transform;

    Camera(float fov = 60.0f, const Transform &transform = Transform())
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