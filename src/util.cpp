#include "util.hpp"

Texture::Texture(const std::string &filename) : filename(filename) {
  if (filename == "_no_texture") {
    image = Image(1, 1);
    base_color = vector3(255, 255, 255); // default white color
    return;
  }
  std::string ext = filename.substr(filename.find_last_of(".") + 1);
  if (ext == "bytes")
    from_bytes(filename);
  else if (ext == "bmp")
    from_bmp(filename);
  else
    throw std::runtime_error("Unsupported texture format: " + filename);
}

void Texture::from_bmp(const std::string &filename) {
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

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
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

void Texture::from_bytes(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file)
    throw std::runtime_error("Failed to open: " + filename);

  uint16_t w, h;
  file.read(reinterpret_cast<char *>(&w), 2);
  file.read(reinterpret_cast<char *>(&h), 2);
  image = Image(w, h);

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      unsigned char r, g, b;
      file.read(reinterpret_cast<char *>(&r), 1);
      file.read(reinterpret_cast<char *>(&g), 1);
      file.read(reinterpret_cast<char *>(&b), 1);
      image.pixels[y * w + x] = vector3(r, g, b);
    }
  }
}

Shader::Shader(const std::string &texture_filename,
               const vector3 directional_light)
    : texture(texture_filename), directional_light(directional_light) {
  if (texture.image.width == 0 || texture.image.height == 0)
    throw std::runtime_error("Failed to load texture: " + texture_filename);
}

vector3 Transform::transform(const std::vector<vector3> &base,
                             const vector3 &p) {
  return vector3(base[0].getX() * p.getX() + base[1].getX() * p.getY() +
                     base[2].getX() * p.getZ(),
                 base[0].getY() * p.getX() + base[1].getY() * p.getY() +
                     base[2].getY() * p.getZ(),
                 base[0].getZ() * p.getX() + base[1].getZ() * p.getY() +
                     base[2].getZ() * p.getZ());
}

const std::vector<vector3> &Transform::get_base_vectors() const {
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
  cached_inverse = {vector3(final_i.getX(), final_j.getX(), final_k.getX()),
                    vector3(final_i.getY(), final_j.getY(), final_k.getY()),
                    vector3(final_i.getZ(), final_j.getZ(), final_k.getZ())};
  cache_valid = true;
  return cached_base;
}

const std::vector<vector3> &Transform::get_inverse_base_vectors() const {
  if (!cache_valid)
    get_base_vectors(); // triggers both caches
  return cached_inverse;
}
