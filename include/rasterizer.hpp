#pragma once

#include "image_creator.hpp"
#include "math.hpp"
#include "object_loader.hpp"
#include "util.hpp"
#include <SDL2/SDL.h>
#include <algorithm>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

extern const int WIDTH;
extern const int HEIGHT;
extern const float cam_speed;
extern const float mouse_sensitivity;
extern const int NUM_THREADS;

// Pre-transformed vertex for the rasterizer
struct RenderVertex {
    vector3 screen_pos; // x, y, z (depth)
    vector3 normal;
    vector2 uv;
    float inv_z;
};

struct ModelData {
    std::vector<RenderVertex> vertices;
    std::shared_ptr<Shader> shader;
    Transform transform;
};

vector3 world_to_screen(const vector3 &point, const Transform &transform, const Camera &cam,
                        int width, int height);

void render_tiles(std::vector<ModelData> &models, Image &image);

vector3 vertex_to_view(const vector3 &p, const Transform &transform, const Camera &cam);
vector3 view_to_world(const vector3 &p_view, const Camera &cam);

struct ClipVertex {
  vector3 pos_view;
  vector3 normal_world;
  vector2 uv;
};

ClipVertex lerp_clip_vertex(const ClipVertex &v1, const ClipVertex &v2, float t);

ModelData process_model(const Model &model, const Camera &cam);

Scene create_main_scene();
Scene create_rotation_scene();

void real_time_render();