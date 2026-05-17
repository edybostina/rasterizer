#include "object_loader.hpp"

std::vector<std::string> split(std::string s, std::string delimiter) {
  size_t pos_start = 0, pos_end, delim_len = delimiter.length();
  std::string token;
  std::vector<std::string> res;

  while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
    token = s.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + delim_len;
    res.push_back(token);
  }

  res.push_back(s.substr(pos_start));
  return res;
}

Model load_object(const std::string &obj, const std::string &texture_filename,
                  vector3 base_color) {
  std::vector<vector3> all_points;
  std::vector<vector2> all_uvs;
  std::vector<vector3> all_normals;

  std::vector<vector3> triangle_points;
  std::vector<vector3> triangle_normals;
  std::vector<vector2> triangle_uvs;

  std::ifstream file(obj);
  if (!file.is_open())
    throw std::runtime_error("Failed to open file: " + obj);

  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#')
      continue;

    if (line.rfind("v ", 0) == 0) {
      double x, y, z;
      if (sscanf(line.c_str(), "v %lf %lf %lf", &x, &y, &z) == 3)
        all_points.emplace_back(x, y, z);
    } else if (line.rfind("vt ", 0) == 0) {
      double u, v;
      if (sscanf(line.c_str(), "vt %lf %lf", &u, &v) == 2)
        all_uvs.emplace_back(u, v);
    } else if (line.rfind("vn ", 0) == 0) {
      double nx, ny, nz;
      if (sscanf(line.c_str(), "vn %lf %lf %lf", &nx, &ny, &nz) == 3)
        all_normals.emplace_back(nx, ny, nz);
    } else if (line.rfind("f ", 0) == 0) {
      std::vector<std::string> tokens = split(line, " ");
      struct VertexIndices {
        int p = -1, t = -1, n = -1;
      };
      std::vector<VertexIndices> face;

      for (size_t i = 1; i < tokens.size(); ++i) {
        if (tokens[i].empty())
          continue;
        std::vector<std::string> parts = split(tokens[i], "/");
        VertexIndices vi;
        if (parts.size() >= 1 && !parts[0].empty())
          vi.p = std::stoi(parts[0]) - 1;
        if (parts.size() >= 2 && !parts[1].empty()) {
          if (all_uvs.empty() && !all_normals.empty() && parts.size() == 2) {
            vi.n = std::stoi(parts[1]) - 1;
          } else {
            vi.t = std::stoi(parts[1]) - 1;
          }
        }
        if (parts.size() >= 3 && !parts[2].empty())
          vi.n = std::stoi(parts[2]) - 1;
        face.push_back(vi);
      }

      // Triangle fan
      for (size_t i = 1; i + 1 < face.size(); ++i) {
        int idx[3] = {0, (int)i, (int)i + 1};
        for (int k = 0; k < 3; ++k) {
          VertexIndices vi = face[idx[k]];
          if (vi.p >= 0 && vi.p < (int)all_points.size())
            triangle_points.push_back(all_points[vi.p]);
          else
            triangle_points.push_back(vector3(0, 0, 0));

          if (vi.t >= 0 && vi.t < (int)all_uvs.size())
            triangle_uvs.push_back(all_uvs[vi.t]);
          else
            triangle_uvs.push_back(vector2(0, 0));

          if (vi.n >= 0 && vi.n < (int)all_normals.size())
            triangle_normals.push_back(all_normals[vi.n]);
          else
            triangle_normals.push_back(vector3(0, 1, 0));
        }
      }
    }
  }

  file.close();

  Transform identity_transform;
  Model model(triangle_points, triangle_normals, triangle_uvs,
              identity_transform, Shader(texture_filename));
  model.shader->has_texture = all_uvs.empty() ? false : true;
  model.shader->texture.base_color = base_color;
  return model;
}
