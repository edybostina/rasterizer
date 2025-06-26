#pragma once

#include <string>
#include <vector>
#include <memory>
#include <list>
#include <fstream>
#include <stdexcept>
#include "math.hpp"
#include "util.hpp"

// from https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
// this function splits a string by a delimiter and returns a vector of strings
std::vector<std::string> split(std::string s, std::string delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
    {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

// a basic .obj parser
Model load_object(const std::string &obj, const std::string &texture_filename = "_no_texture", vector3 base_color = vector3(255, 255, 255))
{
    std::vector<vector3> all_points;
    std::vector<vector3> triangle_points;
    std::vector<vector2> texture_coords_vt;
    std::vector<vector3> normals_vn;
    std::vector<vector3> normals;
    std::vector<vector2> texture_coords;

    std::ifstream file(obj);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + obj);

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        if (line.rfind("v ", 0) == 0)
        {
            double x, y, z;
            if (sscanf(line.c_str(), "v %lf %lf %lf", &x, &y, &z) == 3)
                all_points.emplace_back(x, y, z);
            else
                throw std::runtime_error("Invalid vertex format in file: " + obj);
        }
        else if (line.rfind("vt ", 0) == 0)
        {
            double u, v;
            if (sscanf(line.c_str(), "vt %lf %lf", &u, &v) == 2)
                texture_coords_vt.emplace_back(vector2(u, v));
            else
                throw std::runtime_error("Invalid texture coordinate format in file: " + obj);
        }
        else if (line.rfind("vn ", 0) == 0)
        {
            double nx, ny, nz;
            if (sscanf(line.c_str(), "vn %lf %lf %lf", &nx, &ny, &nz) == 3)
                normals_vn.emplace_back(vector3(nx, ny, nz));
            else
                throw std::runtime_error("Invalid normal format in file: " + obj);
        }
        else if (line.rfind("f ", 0) == 0)
        {
            std::vector<std::string> tokens = split(line, " ");
            std::vector<int> face_indices;
            std::vector<int> face_textures;
            std::vector<int> face_normals;

            std::string splitter = "/";
            if (line.find("//") != std::string::npos)
            {
                splitter = "//";
            }

            for (size_t i = 1; i < tokens.size(); ++i)
            {
                std::vector<std::string> parts = split(tokens[i], splitter);
                if (parts.empty() || parts[0].empty())
                    throw std::runtime_error("Invalid face format in file: " + obj);

                int input_size = parts.size();
                int face_index = 0;
                int texture_index = 0;
                int normal_index = 0;
                switch (input_size)
                {
                case 1:
                    face_index = std::stoi(parts[0]) - 1;
                    face_indices.push_back(face_index);
                    break;
                case 2:
                    face_index = std::stoi(parts[0]) - 1;
                    texture_index = std::stoi(parts[1]) - 1;
                    if (texture_filename != "_no_texture")
                    {
                        face_indices.push_back(face_index);
                        face_textures.push_back(texture_index);
                    }
                    else
                    {
                        face_indices.push_back(face_index);
                        face_normals.push_back(texture_index);
                    }
                    break;
                case 3:
                    face_index = std::stoi(parts[0]) - 1;
                    texture_index = std::stoi(parts[1]) - 1;
                    normal_index = std::stoi(parts[2]) - 1;
                    face_indices.push_back(face_index);
                    face_textures.push_back(texture_index);
                    face_normals.push_back(normal_index);
                    break;
                default:
                    throw std::runtime_error("Invalid face format in file: " + obj);
                    break;
                }
            }
            // Triangle fan
            for (size_t i = 1; i + 1 < face_indices.size(); ++i)
            {
                triangle_points.push_back(all_points[face_indices[0]]);
                triangle_points.push_back(all_points[face_indices[i]]);
                triangle_points.push_back(all_points[face_indices[i + 1]]);

                if (texture_coords_vt.size() > 0)
                {
                    texture_coords.push_back(texture_coords_vt[face_textures[0]]);
                    texture_coords.push_back(texture_coords_vt[face_textures[i]]);
                    texture_coords.push_back(texture_coords_vt[face_textures[i + 1]]);
                }

                if (normals_vn.size() > 0)
                {
                    normals.push_back(normals_vn[face_normals[0]]);
                    normals.push_back(normals_vn[face_normals[i]]);
                    normals.push_back(normals_vn[face_normals[i + 1]]);
                }
            }
        }
    }

    file.close();

    Transform identity_transform;
    Model model(triangle_points, normals, texture_coords, identity_transform, Shader(texture_filename));
    model.shader.has_texture = texture_coords.empty() ? false : true;
    model.shader.texture.base_color = base_color;
    return model;
}
