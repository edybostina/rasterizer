#include "rasterizer.hpp"
#include <iostream>

const int WIDTH = 1920;
const int HEIGHT = 1080;
const double cam_speed = 0.5;
const double mouse_sensitivity = 0.001;
const int NUM_THREADS = std::thread::hardware_concurrency();

vector3 world_to_screen(const vector3 &point, Transform transform, Camera cam,
                        int width, int height) {
  vector3 vertex_world = transform.to_world_point(point);
  vector3 vertex_view = cam.transform.to_local_point(vertex_world);

  double screen_height = tan(cam.fov / 2) * 2;
  double pixels_per_unit = height / screen_height / vertex_view.getZ();
  vector2 pixel_offset =
      vector2(vertex_view.getX() * pixels_per_unit + width / 2.0,
              vertex_view.getY() * pixels_per_unit + height / 2.0);

  return vector3(pixel_offset.getX(), pixel_offset.getY(), vertex_view.getZ());
}

void render_chunk(Model &model, Image &image, Camera cam, int start, int end) {
  for (int i = start; i < end; i += 3) {

    vector3 a = world_to_screen(model.points[i], model.transform, cam,
                                image.width, image.height);
    vector3 b = world_to_screen(model.points[i + 1], model.transform, cam,
                                image.width, image.height);
    vector3 c = world_to_screen(model.points[i + 2], model.transform, cam,
                                image.width, image.height);
    if (a.getZ() < 0 || b.getZ() < 0 || c.getZ() < 0) {
      continue; // skip triangles that are behind the camera (crude fix)
    }

    double min_x = std::min({a.getX(), b.getX(), c.getX()});
    double max_x = std::max({a.getX(), b.getX(), c.getX()});

    double min_y = std::min({a.getY(), b.getY(), c.getY()});
    double max_y = std::max({a.getY(), b.getY(), c.getY()});

    int start_x =
        clamp(static_cast<int>(min_x), 0, static_cast<int>(image.width - 1));
    int end_x = clamp(static_cast<int>(ceil(max_x)), 0,
                      static_cast<int>(image.width - 1));

    int start_y =
        clamp(static_cast<int>(min_y), 0, static_cast<int>(image.height - 1));
    int end_y = clamp(static_cast<int>(ceil(max_y)), 0,
                      static_cast<int>(image.height - 1));

    for (int y = start_y; y < end_y; ++y) {
      for (int x = start_x; x < end_x; ++x) {
        vector2 point(x, y);
        vector3 weights(0, 0, 0);

        if (point.insideTriangle(vector2(a.getX(), a.getY()),
                                 vector2(b.getX(), b.getY()),
                                 vector2(c.getX(), c.getY()), weights)) {
          vector3 depths(a.getZ(), b.getZ(), c.getZ());
          vector3 depths_inv(1 / a.getZ(), 1 / b.getZ(), 1 / c.getZ());
          double depth = 1 / weights.dot(depths_inv);

          if (depth > image.depth[get_index(x, y, image.width)]) {
            continue; // skip if the depth is not closer
          }

          double w0 = weights.getX() * depths_inv.getX();
          double w1 = weights.getY() * depths_inv.getY();
          double w2 = weights.getZ() * depths_inv.getZ();
          double w_sum = w0 + w1 + w2;

          // interpolate texture coordinates
          vector2 texture_coord(0, 0);
          if (model.shader->has_texture) {
            texture_coord = (model.texture_coords[i] * w0 +
                             model.texture_coords[i + 1] * w1 +
                             model.texture_coords[i + 2] * w2) *
                            (1 / w_sum);
          }
          // interpolate normals
          vector3 normal = (model.normals[i] * w0 + model.normals[i + 1] * w1 +
                            model.normals[i + 2] * w2) *
                           (1 / w_sum);

          image.pixels[get_index(x, y, image.width)] = model.shader->get_colour(
              texture_coord, model.transform.transform_normal(normal));
          image.depth[get_index(x, y, image.width)] = depth;
        }
      }
    }
  }
}

void render_multithread(Model &model, Image &image, Camera cam) {
  model.transform.get_base_vectors();
  model.transform.get_inverse_base_vectors();
  cam.transform.get_base_vectors();
  cam.transform.get_inverse_base_vectors();

  std::vector<std::thread> threads;
  int total_points = model.points.size();
  int points_per_thread = total_points / NUM_THREADS;
  if (points_per_thread % 3 != 0) {
    points_per_thread -=
        points_per_thread % 3; // ensure we have complete triangles
  }

  for (int t = 0; t < NUM_THREADS; ++t) {
    int start = t * points_per_thread;
    int end = (t == NUM_THREADS - 1) ? total_points : start + points_per_thread;
    if (end % 3 != 0) {
      end -= end % 3;
    }

    threads.emplace_back(render_chunk, std::ref(model), std::ref(image), cam,
                         start, end);
  }

  for (auto &thread : threads)
    thread.join();
}

void render_basic(Model &model, Image &image, Transform transform, Camera cam,
                  double fov) {
  for (int i = 0; i < model.points.size(); i += 3) {
    vector3 a = world_to_screen(model.points[i], transform, cam, image.width,
                                image.height);
    vector3 b = world_to_screen(model.points[i + 1], transform, cam,
                                image.width, image.height);
    vector3 c = world_to_screen(model.points[i + 2], transform, cam,
                                image.width, image.height);

    double min_x = std::min({a.getX(), b.getX(), c.getX()});
    double max_x = std::max({a.getX(), b.getX(), c.getX()});
    double min_y = std::min({a.getY(), b.getY(), c.getY()});
    double max_y = std::max({a.getY(), b.getY(), c.getY()});

    // ensure triangle within the bounds of the pixel array
    int start_x =
        clamp(static_cast<int>(min_x), 0, static_cast<int>(image.width - 1));
    int end_x = clamp(static_cast<int>(ceil(max_x)), 0,
                      static_cast<int>(image.width - 1));

    int start_y =
        clamp(static_cast<int>(min_y), 0, static_cast<int>(image.height - 1));
    int end_y = clamp(static_cast<int>(ceil(max_y)), 0,
                      static_cast<int>(image.height - 1));

    for (int y = start_y; y < end_y; ++y) {
      for (int x = start_x; x < end_x; ++x) {
        vector2 point(x, y);
        vector3 weights(0, 0, 0);

        if (point.insideTriangle(vector2(a.getX(), a.getY()),
                                 vector2(b.getX(), b.getY()),
                                 vector2(c.getX(), c.getY()), weights)) {
          vector3 depths(a.getZ(), b.getZ(), c.getZ());
          vector3 depths_inv(1 / a.getZ(), 1 / b.getZ(), 1 / c.getZ());
          double depth = 1 / weights.dot(depths_inv);

          if (depth > image.depth[get_index(x, y, image.width)]) {
            continue; // skip if the depth is not closer
          }

          double w0 = weights.getX() * depths_inv.getX();
          double w1 = weights.getY() * depths_inv.getY();
          double w2 = weights.getZ() * depths_inv.getZ();
          double w_sum = w0 + w1 + w2;
          vector2 texture_coord(0, 0);
          if (model.shader->has_texture) {
            texture_coord = (model.texture_coords[i] * w0 +
                             model.texture_coords[i + 1] * w1 +
                             model.texture_coords[i + 2] * w2) *
                            (1 / w_sum);
          }

          // interpolate normals
          vector3 normal = (model.normals[i] * w0 + model.normals[i + 1] * w1 +
                            model.normals[i + 2] * w2) *
                           (1 / w_sum);

          image.pixels[get_index(x, y, image.width)] =
              model.shader->get_colour(texture_coord, normal);
          image.depth[get_index(x, y, image.width)] = depth;
        }
      }
    }
  }
}

void write_frame_rows(int startY, int endY, const Image &image,
                      uint32_t *pixels) {
  for (int y = startY; y < endY; ++y) {
    for (int x = 0; x < WIDTH; ++x) {
      vector3 color = image.pixels[get_index(x, y, image.width)];
      uint8_t r = color.getX();
      uint8_t g = color.getY();
      uint8_t b = color.getZ();
      pixels[(HEIGHT - y - 1) * WIDTH + x] =
          (255 << 24) | (r << 16) | (g << 8) | b; // ARGB
    }
  }
}

void frame_writer_multithread(const Image &image, uint32_t *pixels) {
  std::vector<std::thread> threads;
  int rowsPerThread = HEIGHT / NUM_THREADS;

  for (int i = 0; i < NUM_THREADS; ++i) {
    int startY = i * rowsPerThread;
    int endY = (i == NUM_THREADS - 1) ? HEIGHT : startY + rowsPerThread;
    threads.emplace_back(write_frame_rows, startY, endY, std::ref(image),
                         pixels);
  }

  for (auto &t : threads)
    t.join();
}

vector3 vertex_to_view(vector3 p, Transform transform, Camera cam) {
  vector3 vertex_world = transform.to_world_point(p);
  vector3 vertex_view = cam.transform.to_local_point(vertex_world);
  return vertex_view;
}

vector3 view_to_world(vector3 p_view, Camera cam) {
  return cam.transform.to_world_point(p_view);
}

ClipVertex lerp_clip_vertex(const ClipVertex &v1, const ClipVertex &v2,
                            double t) {
  return {v1.pos_view.lerp(v2.pos_view, t),
          v1.normal_world.lerp(v2.normal_world, t).normalize(),
          v1.uv.lerp(v2.uv, t)};
}

Model process_model(const Model &model, Camera cam) {
  const size_t triangle_count = model.points.size() / 3;
  if (triangle_count == 0)
    return model;

  model.transform.get_base_vectors();
  model.transform.get_inverse_base_vectors();
  cam.transform.get_base_vectors();
  cam.transform.get_inverse_base_vectors();

  int num_threads = std::thread::hardware_concurrency();
  if (num_threads <= 0)
    num_threads = 1;
  const size_t triangles_per_thread =
      (triangle_count + num_threads - 1) / num_threads;

  struct ThreadResult {
    std::vector<vector3> points;
    std::vector<vector3> normals;
    std::vector<vector2> uvs;
  };
  std::vector<ThreadResult> thread_local_results(num_threads);

  auto worker = [&](int thread_id) {
    size_t start = thread_id * triangles_per_thread * 3;
    if (start >= model.points.size())
      return;
    size_t end =
        std::min(start + triangles_per_thread * 3, model.points.size());

    // pre-reserve to reduce reallocations
    size_t expected_triangles = (end - start) / 3;
    thread_local_results[thread_id].points.reserve(expected_triangles * 3);
    thread_local_results[thread_id].normals.reserve(expected_triangles * 3);
    thread_local_results[thread_id].uvs.reserve(expected_triangles * 3);

    for (size_t i = start; i + 2 < end; i += 3) {
      ClipVertex v[3];
      for (int j = 0; j < 3; ++j) {
        v[j].pos_view =
            vertex_to_view(model.points[i + j], model.transform, cam);

        if (i + j < model.normals.size())
          v[j].normal_world =
              model.transform.transform_normal(model.normals[i + j]);
        else
          v[j].normal_world = vector3(0, 1, 0);

        if (model.shader->has_texture &&
            (i + j) < model.texture_coords.size()) {
          v[j].uv = model.texture_coords[i + j];
        } else {
          v[j].uv = vector2(0, 0);
        }
      }

      double clipping_distance = 0.1;
      bool inside[3];
      int inside_count = 0;
      for (int j = 0; j < 3; ++j) {
        inside[j] = v[j].pos_view.getZ() >= clipping_distance;
        if (inside[j])
          inside_count++;
      }

      auto add_triangle = [&](const ClipVertex &v0, const ClipVertex &v1,
                              const ClipVertex &v2) {
        thread_local_results[thread_id].points.push_back(
            view_to_world(v0.pos_view, cam));
        thread_local_results[thread_id].points.push_back(
            view_to_world(v1.pos_view, cam));
        thread_local_results[thread_id].points.push_back(
            view_to_world(v2.pos_view, cam));
        thread_local_results[thread_id].normals.push_back(v0.normal_world);
        thread_local_results[thread_id].normals.push_back(v1.normal_world);
        thread_local_results[thread_id].normals.push_back(v2.normal_world);
        thread_local_results[thread_id].uvs.push_back(v0.uv);
        thread_local_results[thread_id].uvs.push_back(v1.uv);
        thread_local_results[thread_id].uvs.push_back(v2.uv);
      };

      if (inside_count == 3) {
        add_triangle(v[0], v[1], v[2]);
      } else if (inside_count == 1) {
        int i0 = inside[0] ? 0 : (inside[1] ? 1 : 2);
        int i1 = (i0 + 1) % 3;
        int i2 = (i0 + 2) % 3;

        double t1 = (clipping_distance - v[i0].pos_view.getZ()) /
                    (v[i1].pos_view.getZ() - v[i0].pos_view.getZ());
        double t2 = (clipping_distance - v[i0].pos_view.getZ()) /
                    (v[i2].pos_view.getZ() - v[i0].pos_view.getZ());

        ClipVertex v01 = lerp_clip_vertex(v[i0], v[i1], t1);
        ClipVertex v02 = lerp_clip_vertex(v[i0], v[i2], t2);

        add_triangle(v[i0], v01, v02);
      } else if (inside_count == 2) {
        int i0 = !inside[0] ? 0 : (!inside[1] ? 1 : 2);
        int i1 = (i0 + 1) % 3;
        int i2 = (i0 + 2) % 3;

        double t1 = (clipping_distance - v[i0].pos_view.getZ()) /
                    (v[i1].pos_view.getZ() - v[i0].pos_view.getZ());
        double t2 = (clipping_distance - v[i0].pos_view.getZ()) /
                    (v[i2].pos_view.getZ() - v[i0].pos_view.getZ());

        ClipVertex v01 = lerp_clip_vertex(v[i0], v[i1], t1);
        ClipVertex v02 = lerp_clip_vertex(v[i0], v[i2], t2);

        add_triangle(v[i1], v[i2], v02);
        add_triangle(v[i1], v02, v01);
      }
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i)
    threads.emplace_back(worker, i);

  for (auto &t : threads)
    t.join();

  size_t total_points = 0;
  for (const auto &res : thread_local_results)
    total_points += res.points.size();

  std::vector<vector3> new_points, new_normals;
  std::vector<vector2> new_uvs;
  new_points.reserve(total_points);
  new_normals.reserve(total_points);
  new_uvs.reserve(total_points);

  for (const auto &res : thread_local_results) {
    new_points.insert(new_points.end(), res.points.begin(), res.points.end());
    new_normals.insert(new_normals.end(), res.normals.begin(),
                       res.normals.end());
    new_uvs.insert(new_uvs.end(), res.uvs.begin(), res.uvs.end());
  }

  Model new_model(new_points, new_normals, new_uvs, Transform(), model.shader);
  new_model.shader->has_texture = model.shader->has_texture;
  return new_model;
}

Scene create_main_scene() {
  std::vector<Model> models;
  vector3 SUN(0.3, 1, 0.6); // position of the sun in the scene

  Model cube = load_object("objects/cube.obj", "textures/grass_block.bmp");
  Model fox = load_object("objects/fox.obj", "textures/colMap.bytes");
  Model dave = load_object("objects/dave.obj", "textures/daveTex.bytes");
  Model floor = load_object("objects/floor.obj", "textures/tile.bmp");
  Model tree_1 = load_object("objects/tree.obj", "textures/colMap.bytes");
  Model tree_2 = load_object("objects/tree.obj", "textures/colMap.bytes");
  Model dragon =
      load_object("objects/dragon.obj", "_no_texture", vector3(80, 255, 200));

  Transform cube_transform(degrees_to_radians(75), degrees_to_radians(20), 0,
                           vector3(7, 0.5, 3), vector3(1, 1, 1));
  Transform fox_transform(0, 0, 0, vector3(0.5, 0, 3), vector3(1, 1, 1) * 0.2);
  Transform dave_transform(0, 0, 0, vector3(0, 0, 3));
  Transform floor_transform(0, 0, 0, vector3(0, 0, 5));
  Transform tree_1_transform(0, 0, 0, vector3(-4, 0, 3));
  Transform tree_2_transform(0, 0, 0, vector3(4, 0, 7));
  Transform dragon_transform(0, 0, 0, vector3(0, 0, 7));

  cube.transform = cube_transform;
  fox.transform = fox_transform;
  dave.transform = dave_transform;
  floor.transform = floor_transform;
  tree_1.transform = tree_1_transform;
  tree_2.transform = tree_2_transform;
  dragon.transform = dragon_transform;

  models.push_back(dragon);
  models.push_back(cube);
  models.push_back(fox);
  models.push_back(dave);
  models.push_back(floor);
  models.push_back(tree_1);
  models.push_back(tree_2);

  Camera camera(
      60.0,
      Transform(
          0, 0, 0,
          vector3(0, 2, -2))); // camera with a field of view of 60 degrees
  Scene scene(models, camera);
  return scene;
}

Scene create_rotation_scene() {
  std::vector<Model> models;
  vector3 SUN(0.3, 1, 0.6);

  Model dragon =
      load_object("objects/dragon.obj", "_no_texture", vector3(80, 255, 200));

  Transform dragon_transform(0, 0, 0, vector3(0, 0, 7));

  dragon.transform = dragon_transform;

  models.push_back(dragon);

  Camera camera(60.0, Transform(0, 0, 0, vector3(0, 2, -2)));

  Scene scene(models, camera);
  return scene;
}

void real_time_render() {

  Scene scene = create_main_scene();
  Image image(WIDTH, HEIGHT);

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL init failed: " << SDL_GetError() << "\n";
    return;
  }

  SDL_Window *window =
      SDL_CreateWindow("Renderer", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
  SDL_SetRelativeMouseMode(
      SDL_TRUE); // enable relative mouse mode for better camera control

  bool running = true;
  SDL_Event e;

  while (running) {
    image.clearDepth(); // clear depth buffer for the next frame
    image.clearPixels(vector3(
        135, 206,
        235)); // clear pixel buffer for the next frame (with a sky color)

    int deltaX = 0, deltaY = 0;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDLK_ESCAPE || e.type == SDL_QUIT ||
          (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_q))
        running = false;
      else if (e.type == SDL_MOUSEMOTION) {
        deltaX = e.motion.xrel;
        deltaY = e.motion.yrel;
        // deltaX and deltaY are the movement deltas since the last event
      }
    }

    // handle camera movement
    scene.camera.transform.set_rotation(
        scene.camera.transform.yaw - deltaX * mouse_sensitivity,
        clamp(scene.camera.transform.pitch - deltaY * mouse_sensitivity,
              -M_PI / 2, M_PI / 2),
        scene.camera.transform.roll);

    std::vector<vector3> base_vectors =
        scene.camera.transform.get_base_vectors();
    vector3 move_delta(0, 0, 0);

    const Uint8 *state = SDL_GetKeyboardState(nullptr);
    if (state[SDL_SCANCODE_UP] || state[SDL_SCANCODE_W])
      move_delta = move_delta + base_vectors[2]; // move forward
    if (state[SDL_SCANCODE_DOWN] || state[SDL_SCANCODE_S])
      move_delta = move_delta - base_vectors[2]; // move backward

    if (state[SDL_SCANCODE_LEFT] || state[SDL_SCANCODE_A])
      move_delta = move_delta - base_vectors[0]; // move left
    if (state[SDL_SCANCODE_RIGHT] || state[SDL_SCANCODE_D])
      move_delta = move_delta + base_vectors[0]; // move right

    if (state[SDL_SCANCODE_SPACE])
      move_delta = move_delta + base_vectors[1]; // move up
    if (state[SDL_SCANCODE_LCTRL])
      move_delta = move_delta - base_vectors[1]; // move down

    scene.camera.transform.position =
        scene.camera.transform.position + move_delta.normalize() * cam_speed;

    for (size_t i = 0; i < scene.models.size(); ++i) {
      Model clipped_model = process_model(scene.models[i], scene.camera);
      render_multithread(clipped_model, image, scene.camera);
    }

    scene.models[0].transform.rotate(degrees_to_radians(1), 0, 0);

    std::vector<uint32_t> pixels(WIDTH * HEIGHT);
    frame_writer_multithread(image, pixels.data());

    SDL_UpdateTexture(texture, nullptr, pixels.data(),
                      WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
