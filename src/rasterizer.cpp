#include "rasterizer.hpp"
#include <iostream>
#include <mutex>
#include <atomic>

const int WIDTH = 1920;
const int HEIGHT = 1080;
const float cam_speed = 0.5f;
const float mouse_sensitivity = 0.001f;
const int NUM_THREADS = std::thread::hardware_concurrency();

vector3 world_to_screen(const vector3 &point, const Transform &transform, const Camera &cam,
                        int width, int height) {
  vector3 vertex_world = transform.to_world_point(point);
  vector3 vertex_view = cam.transform.to_local_point(vertex_world);

  float screen_height = tanf(cam.fov / 2.0f) * 2.0f;
  float pixels_per_unit = height / screen_height / vertex_view.getZ();
  
  return vector3(vertex_view.getX() * pixels_per_unit + width / 2.0f,
                 height / 2.0f - vertex_view.getY() * pixels_per_unit, 
                 vertex_view.getZ());
}

inline float edge_function(const vector2 &a, const vector2 &b, const vector2 &c) {
    return (c.getX() - a.getX()) * (b.getY() - a.getY()) - (c.getY() - a.getY()) * (b.getX() - a.getX());
}

void render_tile(int start_x, int start_y, int end_x, int end_y, 
                 std::vector<ModelData> &models, Image &image) {
    for (auto &model : models) {
        auto &vertices = model.vertices;
        for (size_t i = 0; i < vertices.size(); i += 3) {
            const RenderVertex &v0 = vertices[i];
            const RenderVertex &v1 = vertices[i+1];
            const RenderVertex &v2 = vertices[i+2];

            // Bounding box of triangle
            float min_x = std::min({v0.screen_pos.getX(), v1.screen_pos.getX(), v2.screen_pos.getX()});
            float max_x = std::max({v0.screen_pos.getX(), v1.screen_pos.getX(), v2.screen_pos.getX()});
            float min_y = std::min({v0.screen_pos.getY(), v1.screen_pos.getY(), v2.screen_pos.getY()});
            float max_y = std::max({v0.screen_pos.getY(), v1.screen_pos.getY(), v2.screen_pos.getY()});

            // Intersection with tile
            int tri_start_x = std::max(start_x, (int)floorf(min_x));
            int tri_end_x = std::min(end_x, (int)ceilf(max_x));
            int tri_start_y = std::max(start_y, (int)floorf(min_y));
            int tri_end_y = std::min(end_y, (int)ceilf(max_y));

            if (tri_start_x >= tri_end_x || tri_start_y >= tri_end_y) continue;

            vector2 p0(v0.screen_pos.getX(), v0.screen_pos.getY());
            vector2 p1(v1.screen_pos.getX(), v1.screen_pos.getY());
            vector2 p2(v2.screen_pos.getX(), v2.screen_pos.getY());

            float area = edge_function(p0, p1, p2);
            if (area >= 0) continue; // Cull backfaces (CW in Y-down)
            float inv_area = 1.0f / area;

            // Incremental edge setup
            float dw0_dx = p2.getY() - p1.getY(); float dw0_dy = p1.getX() - p2.getX();
            float dw1_dx = p0.getY() - p2.getY(); float dw1_dy = p2.getX() - p0.getX();
            float dw2_dx = p1.getY() - p0.getY(); float dw2_dy = p0.getX() - p1.getX();

            float w0_row = edge_function(p1, p2, vector2(tri_start_x + 0.5f, tri_start_y + 0.5f));
            float w1_row = edge_function(p2, p0, vector2(tri_start_x + 0.5f, tri_start_y + 0.5f));
            float w2_row = edge_function(p0, p1, vector2(tri_start_x + 0.5f, tri_start_y + 0.5f));

            for (int y = tri_start_y; y < tri_end_y; ++y) {
                float w0 = w0_row;
                float w1 = w1_row;
                float w2 = w2_row;
                int row_idx = y * image.width;
                for (int x = tri_start_x; x < tri_end_x; ++x) {
                    if (w0 <= 0 && w1 <= 0 && w2 <= 0) {
                        float b0 = w0 * inv_area;
                        float b1 = w1 * inv_area;
                        float b2 = w2 * inv_area;

                        float z_inv = v0.inv_z * b0 + v1.inv_z * b1 + v2.inv_z * b2;
                        float z = 1.0f / z_inv;

                        int pixel_idx = row_idx + x;
                        if (z < image.depth[pixel_idx]) {
                            image.depth[pixel_idx] = z;
                            
                            vector2 uv = (v0.uv * (v0.inv_z * b0) + v1.uv * (v1.inv_z * b1) + v2.uv * (v2.inv_z * b2)) * z;
                            vector3 normal = (v0.normal * (v0.inv_z * b0) + v1.normal * (v1.inv_z * b1) + v2.normal * (v2.inv_z * b2)) * z;
                            
                            image.pixels[pixel_idx] = model.shader->get_colour(uv, normal);
                        }
                    }
                    w0 += dw0_dx; w1 += dw1_dx; w2 += dw2_dx;
                }
                w0_row += dw0_dy; w1_row += dw1_dy; w2_row += dw2_dy;
            }
        }
    }
}

void render_tiles(std::vector<ModelData> &models, Image &image) {
    const int TILE_SIZE = 64;
    int tiles_x = (WIDTH + TILE_SIZE - 1) / TILE_SIZE;
    int tiles_y = (HEIGHT + TILE_SIZE - 1) / TILE_SIZE;
    int total_tiles = tiles_x * tiles_y;

    std::atomic<int> next_tile(0);
    std::vector<std::thread> threads;

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&]() {
            int tile_idx;
            while ((tile_idx = next_tile.fetch_add(1)) < total_tiles) {
                int tx = tile_idx % tiles_x;
                int ty = tile_idx / tiles_x;
                int sx = tx * TILE_SIZE;
                int sy = ty * TILE_SIZE;
                int ex = std::min(sx + TILE_SIZE, WIDTH);
                int ey = std::min(sy + TILE_SIZE, HEIGHT);
                render_tile(sx, sy, ex, ey, models, image);
            }
        });
    }

    for (auto &thread : threads) thread.join();
}

vector3 vertex_to_view(const vector3 &p, const Transform &transform, const Camera &cam) {
  return cam.transform.to_local_point(transform.to_world_point(p));
}

vector3 view_to_world(const vector3 &p_view, const Camera &cam) {
  return cam.transform.to_world_point(p_view);
}

ClipVertex lerp_clip_vertex(const ClipVertex &v1, const ClipVertex &v2, float t) {
  return {v1.pos_view.lerp(v2.pos_view, t),
          v1.normal_world.lerp(v2.normal_world, t).normalize(),
          v1.uv.lerp(v2.uv, t)};
}

ModelData process_model(const Model &model, const Camera &cam) {
    ModelData data;
    data.shader = model.shader;
    data.transform = model.transform;

    float clipping_distance = 0.1f;
    float screen_height = tanf(cam.fov / 2.0f) * 2.0f;
    float pixels_per_unit = HEIGHT / screen_height;

    for (size_t i = 0; i + 2 < model.points.size(); i += 3) {
        ClipVertex v[3];
        for (int j = 0; j < 3; ++j) {
            v[j].pos_view = vertex_to_view(model.points[i + j], model.transform, cam);
            v[j].normal_world = model.transform.transform_normal(model.normals[i + j]);
            v[j].uv = model.shader->has_texture ? model.texture_coords[i + j] : vector2(0,0);
        }

        auto emit_triangle = [&](const ClipVertex &v0, const ClipVertex &v1, const ClipVertex &v2) {
            auto project = [&](const ClipVertex &cv) {
                float ppu_z = pixels_per_unit / cv.pos_view.getZ();
                RenderVertex rv;
                rv.screen_pos = vector3(cv.pos_view.getX() * ppu_z + WIDTH / 2.0f,
                                        HEIGHT / 2.0f - cv.pos_view.getY() * ppu_z,
                                        cv.pos_view.getZ());
                rv.normal = cv.normal_world;
                rv.uv = cv.uv;
                rv.inv_z = 1.0f / cv.pos_view.getZ();
                return rv;
            };
            data.vertices.push_back(project(v0));
            data.vertices.push_back(project(v1));
            data.vertices.push_back(project(v2));
        };

        bool inside[3];
        int inside_count = 0;
        for (int j = 0; j < 3; ++j) {
            inside[j] = v[j].pos_view.getZ() >= clipping_distance;
            if (inside[j]) inside_count++;
        }

        if (inside_count == 3) {
            emit_triangle(v[0], v[1], v[2]);
        } else if (inside_count == 1) {
            int i0 = inside[0] ? 0 : (inside[1] ? 1 : 2);
            int i1 = (i0 + 1) % 3; int i2 = (i0 + 2) % 3;
            float t1 = (clipping_distance - v[i0].pos_view.getZ()) / (v[i1].pos_view.getZ() - v[i0].pos_view.getZ());
            float t2 = (clipping_distance - v[i0].pos_view.getZ()) / (v[i2].pos_view.getZ() - v[i0].pos_view.getZ());
            emit_triangle(v[i0], lerp_clip_vertex(v[i0], v[i1], t1), lerp_clip_vertex(v[i0], v[i2], t2));
        } else if (inside_count == 2) {
            int i0 = !inside[0] ? 0 : (!inside[1] ? 1 : 2);
            int i1 = (i0 + 1) % 3; int i2 = (i0 + 2) % 3;
            float t1 = (clipping_distance - v[i0].pos_view.getZ()) / (v[i1].pos_view.getZ() - v[i0].pos_view.getZ());
            float t2 = (clipping_distance - v[i0].pos_view.getZ()) / (v[i2].pos_view.getZ() - v[i0].pos_view.getZ());
            ClipVertex v01 = lerp_clip_vertex(v[i0], v[i1], t1);
            ClipVertex v02 = lerp_clip_vertex(v[i0], v[i2], t2);
            emit_triangle(v[i1], v[i2], v02);
            emit_triangle(v[i1], v02, v01);
        }
    }
    return data;
}

Scene create_main_scene() {
  std::vector<Model> models;
  Model dragon = load_object("objects/dragon.obj", "_no_texture", vector3(80, 255, 200));
  Model cube = load_object("objects/cube.obj", "textures/grass_block.bmp");
  Model fox = load_object("objects/fox.obj", "textures/colMap.bytes");
  Model dave = load_object("objects/dave.obj", "textures/daveTex.bytes");
  Model floor = load_object("objects/floor.obj", "textures/tile.bmp");
  Model tree_1 = load_object("objects/tree.obj", "textures/colMap.bytes");
  Model tree_2 = load_object("objects/tree.obj", "textures/colMap.bytes");

  dragon.transform = Transform(0, 0, 0, vector3(0, 0, 7));
  cube.transform = Transform(degrees_to_radians(75), degrees_to_radians(20), 0, vector3(7, 0.5, 3));
  fox.transform = Transform(0, 0, 0, vector3(0.5, 0, 3), vector3(0.2, 0.2, 0.2));
  dave.transform = Transform(0, 0, 0, vector3(0, 0, 3));
  floor.transform = Transform(0, 0, 0, vector3(0, 0, 5));
  tree_1.transform = Transform(0, 0, 0, vector3(-4, 0, 3));
  tree_2.transform = Transform(0, 0, 0, vector3(4, 0, 7));

  models.push_back(dragon); models.push_back(cube); models.push_back(fox);
  models.push_back(dave); models.push_back(floor); models.push_back(tree_1); models.push_back(tree_2);

  return Scene(models, Camera(60.0f, Transform(0, 0, 0, vector3(0, 2, -2))));
  }

Scene create_rotation_scene() {
  std::vector<Model> models;
  Model dragon = load_object("objects/dragon.obj", "_no_texture", vector3(80, 255, 200));
  dragon.transform = Transform(0, 0, 0, vector3(0, 0, 7));
  models.push_back(dragon);
  return Scene(models, Camera(60.0f, Transform(0, 0, 0, vector3(0, 2, -2))));
  }

void real_time_render() {
  Scene scene = create_main_scene();
  Image image(WIDTH, HEIGHT);

  if (SDL_Init(SDL_INIT_VIDEO) < 0) return;

  SDL_Window *window = SDL_CreateWindow("Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
  SDL_SetRelativeMouseMode(SDL_TRUE);

  bool running = true;
  SDL_Event e;
  uint32_t sky_color = (255u << 24) | (135 << 16) | (206 << 8) | 235;

  while (running) {
    image.clearDepth();
    image.clearPixels(sky_color);

    int deltaX = 0, deltaY = 0;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDLK_ESCAPE || e.type == SDL_QUIT) running = false;
      else if (e.type == SDL_MOUSEMOTION) { 
          deltaX += e.motion.xrel; 
          deltaY += e.motion.yrel; 
      }
    }

    scene.camera.transform.rotate(-deltaX * mouse_sensitivity, -deltaY * mouse_sensitivity, 0);
    // Clamp pitch
    if (scene.camera.transform.pitch > M_PI/2) scene.camera.transform.pitch = M_PI/2;
    if (scene.camera.transform.pitch < -M_PI/2) scene.camera.transform.pitch = -M_PI/2;

    scene.camera.transform.update_cache(); // Ensure axes are up to date

    const Uint8 *state = SDL_GetKeyboardState(nullptr);
    vector3 move_dir(0, 0, 0);
    if (state[SDL_SCANCODE_W]) move_dir = move_dir + scene.camera.transform.cached_base[2];
    if (state[SDL_SCANCODE_S]) move_dir = move_dir - scene.camera.transform.cached_base[2];
    if (state[SDL_SCANCODE_A]) move_dir = move_dir - scene.camera.transform.cached_base[0];
    if (state[SDL_SCANCODE_D]) move_dir = move_dir + scene.camera.transform.cached_base[0];
    if (state[SDL_SCANCODE_SPACE]) move_dir = move_dir + vector3(0, 1, 0);
    if (state[SDL_SCANCODE_LCTRL]) move_dir = move_dir - vector3(0, 1, 0);

    if (move_dir.magnitude() > 0)
        scene.camera.transform.position = scene.camera.transform.position + move_dir.normalize() * cam_speed;

    std::vector<ModelData> processed_models;
    for (auto &m : scene.models) processed_models.push_back(process_model(m, scene.camera));

    render_tiles(processed_models, image);

    scene.models[0].transform.rotate(degrees_to_radians(1.0f), 0, 0);

    SDL_UpdateTexture(texture, nullptr, image.pixels.data(), WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
