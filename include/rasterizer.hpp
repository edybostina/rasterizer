#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <stdexcept>
#include <thread>
#include "math.hpp"
#include "object_loader.hpp"
#include "util.hpp"
#include "image_creator.hpp"

const int WIDTH = 720;
const int HEIGHT = 480;
const double cam_speed = 0.5;
const double mouse_sensitivity = 0.001;
const int NUM_THREADS = std::thread::hardware_concurrency();

vector3 world_to_screen(const vector3 &point, Transform transform, Camera cam, int width, int height)
{
    vector3 vertex_world = transform.to_world_point(point);
    vector3 vertex_view = cam.transform.to_local_point(vertex_world);

    double screen_height = tan(cam.fov / 2) * 2;
    double pixels_per_unit = height / screen_height / vertex_view.getZ();
    vector2 pixel_offset = vector2(
        vertex_view.getX() * pixels_per_unit + width / 2.0,
        vertex_view.getY() * pixels_per_unit + height / 2.0);

    return vector3(pixel_offset.getX(), pixel_offset.getY(), vertex_view.getZ());
}

void render_chunk(Model &model, Image &image, Camera cam, int start, int end)
{
    for (int i = start; i < end; i += 3)
    {

        vector3 a = world_to_screen(model.points[i], model.transform, cam, image.width, image.height);
        vector3 b = world_to_screen(model.points[i + 1], model.transform, cam, image.width, image.height);
        vector3 c = world_to_screen(model.points[i + 2], model.transform, cam, image.width, image.height);
        if (a.getZ() < 0 || b.getZ() < 0 || c.getZ() < 0)
        {
            continue; // skip triangles that are behind the camera (crude fix)
        }

        double min_x = std::min({a.getX(), b.getX(), c.getX()});
        double max_x = std::max({a.getX(), b.getX(), c.getX()});

        double min_y = std::min({a.getY(), b.getY(), c.getY()});
        double max_y = std::max({a.getY(), b.getY(), c.getY()});

        int start_x = clamp(static_cast<int>(min_x), 0, static_cast<int>(image.width - 1));
        int end_x = clamp(static_cast<int>(ceil(max_x)), 0, static_cast<int>(image.width - 1));

        int start_y = clamp(static_cast<int>(min_y), 0, static_cast<int>(image.height - 1));
        int end_y = clamp(static_cast<int>(ceil(max_y)), 0, static_cast<int>(image.height - 1));

        for (int y = start_y; y < end_y; ++y)
        {
            for (int x = start_x; x < end_x; ++x)
            {
                vector2 point(x, y);
                vector3 weights(0, 0, 0);

                if (point.insideTriangle(vector2(a.getX(), a.getY()), vector2(b.getX(), b.getY()), vector2(c.getX(), c.getY()), weights))
                {
                    vector3 depths(a.getZ(), b.getZ(), c.getZ());
                    vector3 depths_inv(1 / a.getZ(), 1 / b.getZ(), 1 / c.getZ());
                    double depth = 1 / weights.dot(depths_inv);

                    if (depth > image.depth[get_index(x, y, image.width)])
                    {
                        continue; // skip if the depth is not closer
                    }

                    double w0 = weights.getX() * depths_inv.getX();
                    double w1 = weights.getY() * depths_inv.getY();
                    double w2 = weights.getZ() * depths_inv.getZ();
                    double w_sum = w0 + w1 + w2;

                    // interpolate texture coordinates
                    vector2 texture_coord(0, 0);
                    if (model.shader.has_texture)
                    {
                        texture_coord = (model.texture_coords[i] * w0 +
                                         model.texture_coords[i + 1] * w1 +
                                         model.texture_coords[i + 2] * w2) *
                                        (1 /
                                         w_sum);
                    }
                    // interpolate normals
                    vector3 normal = (model.normals[i] * w0 +
                                      model.normals[i + 1] * w1 +
                                      model.normals[i + 2] * w2) *
                                     (1 / w_sum);
                    // TODO: add normals into calculations
                    image.pixels[get_index(x, y, image.width)] = model.shader.get_colour(texture_coord, normal);
                    image.depth[get_index(x, y, image.width)] = depth;
                }
            }
        }
    }
}

void render_multithread(Model &model, Image &image, Camera cam)
{

    std::vector<std::thread> threads;
    int total_points = model.points.size();
    int points_per_thread = total_points / NUM_THREADS;
    if (points_per_thread % 3 != 0)
    {
        points_per_thread -= points_per_thread % 3; // ensure we have complete triangles
    }

    for (int t = 0; t < NUM_THREADS; ++t)
    {
        int start = t * points_per_thread;
        int end = (t == NUM_THREADS - 1) ? total_points : start + points_per_thread;
        if (end % 3 != 0)
        {
            end -= end % 3;
        }

        threads.emplace_back(render_chunk, std::ref(model), std::ref(image), cam, start, end);
    }

    for (auto &thread : threads)
        thread.join();
}

void render_basic(Model &model, Image &image, Transform transform, Camera cam, double fov)
{
    for (int i = 0; i < model.points.size(); i += 3)
    {
        vector3 a = world_to_screen(model.points[i], transform, cam, image.width, image.height);
        vector3 b = world_to_screen(model.points[i + 1], transform, cam, image.width, image.height);
        vector3 c = world_to_screen(model.points[i + 2], transform, cam, image.width, image.height);

        double min_x = std::min({a.getX(), b.getX(), c.getX()});
        double max_x = std::max({a.getX(), b.getX(), c.getX()});
        double min_y = std::min({a.getY(), b.getY(), c.getY()});
        double max_y = std::max({a.getY(), b.getY(), c.getY()});

        // ensure triangle within the bounds of the pixel array
        int start_x = clamp(static_cast<int>(min_x), 0, static_cast<int>(image.width - 1));
        int end_x = clamp(static_cast<int>(ceil(max_x)), 0, static_cast<int>(image.width - 1));

        int start_y = clamp(static_cast<int>(min_y), 0, static_cast<int>(image.height - 1));
        int end_y = clamp(static_cast<int>(ceil(max_y)), 0, static_cast<int>(image.height - 1));

        for (int y = start_y; y < end_y; ++y)
        {
            for (int x = start_x; x < end_x; ++x)
            {
                vector2 point(x, y);
                vector3 weights(0, 0, 0);

                if (point.insideTriangle(vector2(a.getX(), a.getY()), vector2(b.getX(), b.getY()), vector2(c.getX(), c.getY()), weights))
                {
                    vector3 depths(a.getZ(), b.getZ(), c.getZ());
                    vector3 depths_inv(1 / a.getZ(), 1 / b.getZ(), 1 / c.getZ());
                    double depth = 1 / weights.dot(depths_inv);

                    if (depth > image.depth[get_index(x, y, image.width)])
                    {
                        continue; // skip if the depth is not closer
                    }

                    double w0 = weights.getX() * depths_inv.getX();
                    double w1 = weights.getY() * depths_inv.getY();
                    double w2 = weights.getZ() * depths_inv.getZ();
                    double w_sum = w0 + w1 + w2;
                    vector2 texture_coord(0, 0);
                    if (model.shader.has_texture)
                    {
                        texture_coord = (model.texture_coords[i] * w0 +
                                         model.texture_coords[i + 1] * w1 +
                                         model.texture_coords[i + 2] * w2) *
                                        (1 /
                                         w_sum);
                    }

                    // interpolate normals
                    vector3 normal = (model.normals[i] * w0 +
                                      model.normals[i + 1] * w1 +
                                      model.normals[i + 2] * w2) *
                                     (1 / w_sum);
                    // TODO: add normals into calculations
                    image.pixels[get_index(x, y, image.width)] = model.shader.get_colour(texture_coord, normal);
                    image.depth[get_index(x, y, image.width)] = depth;
                }
            }
        }
    }
}

void write_frame_rows(int startY, int endY, const Image &image, uint32_t *pixels)
{
    for (int y = startY; y < endY; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            vector3 color = image.pixels[get_index(x, y, image.width)];
            uint8_t r = color.getX();
            uint8_t g = color.getY();
            uint8_t b = color.getZ();
            pixels[(HEIGHT - y - 1) * WIDTH + x] = (255 << 24) | (r << 16) | (g << 8) | b; // ARGB
        }
    }
}

void frame_writer_multithread(const Image &image, uint32_t *pixels)
{
    std::vector<std::thread> threads;
    int rowsPerThread = HEIGHT / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; ++i)
    {
        int startY = i * rowsPerThread;
        int endY = (i == NUM_THREADS - 1) ? HEIGHT : startY + rowsPerThread;
        threads.emplace_back(write_frame_rows, startY, endY, std::ref(image), pixels);
    }

    for (auto &t : threads)
        t.join();
}

vector3 vertex_to_view(vector3 p, Transform transform, Camera cam)
{
    vector3 vertex_world = transform.to_world_point(p);
    vector3 vertex_view = cam.transform.to_local_point(vertex_world);
    return vertex_view;
}

void process_model(Model model, Camera cam)
{
    const size_t triangle_count = model.points.size() / 3;
    const int num_threads = std::thread::hardware_concurrency();
    const size_t triangles_per_thread = (triangle_count + num_threads - 1) / num_threads;

    std::vector<std::thread> threads;
    std::vector<std::vector<vector3>> thread_local_results(num_threads);

    auto worker = [&](int thread_id)
    {
        size_t start = thread_id * triangles_per_thread * 3;
        size_t end = std::min(start + triangles_per_thread * 3, model.points.size());

        std::vector<vector3> transformed_points(3);

        for (size_t i = start; i + 2 < end; i += 3)
        {
            transformed_points[0] = vertex_to_view(model.points[i], model.transform, cam);
            transformed_points[1] = vertex_to_view(model.points[i + 1], model.transform, cam);
            transformed_points[2] = vertex_to_view(model.points[i + 2], model.transform, cam);

            double clipping_distance = 0.01;
            bool clip_1 = transformed_points[0].getZ() < clipping_distance;
            bool clip_2 = transformed_points[1].getZ() < clipping_distance;
            bool clip_3 = transformed_points[2].getZ() < clipping_distance;

            int clipped_count = (int)clip_1 + (int)clip_2 + (int)clip_3;

            switch (clipped_count)
            {
            case 0:
                thread_local_results[thread_id].push_back(transformed_points[0]);
                thread_local_results[thread_id].push_back(transformed_points[1]);
                thread_local_results[thread_id].push_back(transformed_points[2]);
                break;
            case 1:
                // TODO: add clipping for one point and 2 points behind the camera
                // int clipped_index = clip_1 ? 0 : (clip_2 ? 1 : 2);
                // int other_index_1 = (clipped_index + 1) % 3;
                // int other_index_2 = (clipped_index + 2) % 3;

                // vector3 clipped_point = transformed_points[clipped_index];
                // vector3 other_point_1 = transformed_points[other_index_1];
                // vector3 other_point_2 = transformed_points[other_index_2];

                // double a = (clipping_distance - clipped_point.getZ()) / (other_point_1.getZ() - clipped_point.getZ());
                // double b = (clipping_distance - clipped_point.getZ()) / (other_point_2.getZ() - clipped_point.getZ());

                // vector3 new_clipped_point_a = clipped_point.lerp(other_point_1, a);
                // vector3 new_clipped_point_b = clipped_point.lerp(other_point_2, b);

                break;
            case 2:
                // clipping logic (optional)
                break;
            default:
                break;
            }
        }
    };

    for (int i = 0; i < num_threads; ++i)
        threads.emplace_back(worker, i);

    for (auto &t : threads)
        t.join();

    // Merge all thread-local results into final new_points
    std::vector<vector3> new_points;
    for (auto &thread_result : thread_local_results)
        new_points.insert(new_points.end(), thread_result.begin(), thread_result.end());

    // You now have all processed points in new_points
}

Scene create_scene()
{
    std::vector<Model> models;
    vector3 SUN(0.3, 1, 0.6); // position of the sun in the scene

    Model cube = load_object("objects/cube.obj", "textures/grass_block.bmp");
    Model fox = load_object("objects/fox.obj", "textures/colMap.bytes");
    Model dave = load_object("objects/dave.obj", "textures/daveTex.bytes");
    Model floor = load_object("objects/floor.obj", "textures/tile.bmp");
    Model tree_1 = load_object("objects/tree.obj", "textures/colMap.bytes");
    Model tree_2 = load_object("objects/tree.obj", "textures/colMap.bytes");
    Model dragon = load_object("objects/dragon.obj");

    Transform cube_transform(degrees_to_radians(75), degrees_to_radians(20), 0, vector3(20, 0.5, 5), vector3(1, 1, 1));
    Transform fox_transform(degrees_to_radians(180), 0, 0, vector3(-0.5, 0, 5), vector3(1, 1, 1) * 0.2);
    Transform dave_transform(degrees_to_radians(180), 0, 0, vector3(0, 0, 5));
    Transform floor_transform(0, 0, 0, vector3(0, 0, 5));
    Transform tree_1_transform(0, 0, 0, vector3(-4, 0, 3));
    Transform tree_2_transform(0, 0, 0, vector3(4, 0, 7));
    Transform dragon_transform(0, 0, 0, vector3(-10, 0, 5));

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

    Camera camera(60.0, Transform(0, 0, 0, vector3(0, 2, -2))); // camera with a field of view of 60 degrees
    Scene scene(models, camera);
    return scene;
}

void real_time_render()
{

    Scene scene = create_scene();
    Image image(WIDTH, HEIGHT);

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL init failed: " << SDL_GetError() << "\n";
        return;
    }

    SDL_Window *window = SDL_CreateWindow("Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    SDL_SetRelativeMouseMode(SDL_TRUE); // enable relative mouse mode for better camera control

    bool running = true;
    SDL_Event e;

    while (running)
    {
        image.clearDepth();                        // clear depth buffer for the next frame
        image.clearPixels(vector3(135, 206, 235)); // clear pixel buffer for the next frame

        int deltaX = 0, deltaY = 0;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDLK_ESCAPE || e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_q))
                running = false;
            else if (e.type == SDL_MOUSEMOTION)
            {
                deltaX = e.motion.xrel;
                deltaY = e.motion.yrel;
                // deltaX and deltaY are the movement deltas since the last event
            }
        }

        // handle camera movement
        scene.camera.transform.set_rotation(
            scene.camera.transform.yaw - deltaX * mouse_sensitivity,
            clamp(scene.camera.transform.pitch - deltaY * mouse_sensitivity, -M_PI / 2, M_PI / 2),
            scene.camera.transform.roll);

        std::vector<vector3> base_vectors = scene.camera.transform.get_base_vectors();
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

        scene.camera.transform.position = scene.camera.transform.position + move_delta.normalize() * cam_speed;

        for (size_t i = 0; i < scene.models.size(); ++i)
        {
            // this lags the whole thing lol
            // process_model(scene.models[i], scene.camera);

            render_multithread(scene.models[i], image, scene.camera);
        }

        std::vector<uint32_t> pixels(WIDTH * HEIGHT);
        frame_writer_multithread(image, pixels.data());

        SDL_UpdateTexture(texture, nullptr, pixels.data(), WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}