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

Model load_object(const std::string &obj, const std::string &texture_filename)
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

            for (size_t i = 1; i < tokens.size(); ++i)
            {
                std::vector<std::string> parts = split(tokens[i], "/");
                if (parts.empty() || parts[0].empty())
                    throw std::runtime_error("Invalid face format in file: " + obj);

                int index = std::stoi(parts[0]) - 1;
                int texure_index = std::stoi(parts[1]) - 1;
                face_indices.push_back(index);
                face_textures.push_back(texure_index);
            }

            // Triangle fan
            for (size_t i = 1; i + 1 < face_indices.size(); ++i)
            {
                triangle_points.push_back(all_points[face_indices[0]]);
                triangle_points.push_back(all_points[face_indices[i]]);
                triangle_points.push_back(all_points[face_indices[i + 1]]);

                texture_coords.push_back(texture_coords_vt[face_textures[0]]);
                texture_coords.push_back(texture_coords_vt[face_textures[i]]);
                texture_coords.push_back(texture_coords_vt[face_textures[i + 1]]);

                normals.push_back(normals_vn[face_indices[0]]);
                normals.push_back(normals_vn[face_indices[i]]);
                normals.push_back(normals_vn[face_indices[i + 1]]);
            }
        }
    }

    file.close();
    Transform identity_transform;
    Model model(triangle_points, normals, texture_coords, identity_transform, Shader(texture_filename));
    return model;
}
