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
extern const double cam_speed;
extern const double mouse_sensitivity;
extern const int NUM_THREADS;

vector3 world_to_screen(const vector3 &point, Transform transform, Camera cam,
                        int width, int height);

void render_chunk(Model &model, Image &image, Camera cam, int start, int end);

void render_multithread(Model &model, Image &image, Camera cam);

void render_basic(Model &model, Image &image, Transform transform, Camera cam,
                  double fov);

void write_frame_rows(int startY, int endY, const Image &image,
                      uint32_t *pixels);

void frame_writer_multithread(const Image &image, uint32_t *pixels);

vector3 vertex_to_view(vector3 p, Transform transform, Camera cam);

vector3 view_to_world(vector3 p_view, Camera cam);

struct ClipVertex {
  vector3 pos_view;
  vector3 normal_world;
  vector2 uv;
};

ClipVertex lerp_clip_vertex(const ClipVertex &v1, const ClipVertex &v2,
                            double t);

Model process_model(const Model &model, Camera cam);

Scene create_main_scene();

Scene create_rotation_scene();

void real_time_render();