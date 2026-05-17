#include "util.hpp"

Texture::Texture(const std::string &filename) : filename(filename) {
  if (filename == "_no_texture") {
    width = 1;
    height = 1;
    pixels.resize(1, vector3(255, 255, 255));
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
  int32_t w, h;
  file.read(reinterpret_cast<char *>(&w), 4);
  file.read(reinterpret_cast<char *>(&h), 4);
  width = w;
  height = h;

  file.seekg(28);
  uint16_t bpp;
  file.read(reinterpret_cast<char *>(&bpp), 2);
  bool hasAlpha = (bpp == 32);

  file.seekg(54);
  pixels.resize(width * height);

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
      pixels[y * width + x] = vector3(r, g, b);
    }
    if (padding > 0)
      file.ignore(padding);
  }
}

void Texture::from_bytes(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file)
    throw std::runtime_error("Failed to open: " + filename);

  uint16_t w, h;
  file.read(reinterpret_cast<char *>(&w), 2);
  file.read(reinterpret_cast<char *>(&h), 2);
  width = w;
  height = h;
  pixels.resize(width * height);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      unsigned char r, g, b;
      file.read(reinterpret_cast<char *>(&r), 1);
      file.read(reinterpret_cast<char *>(&g), 1);
      file.read(reinterpret_cast<char *>(&b), 1);
      pixels[y * width + x] = vector3(r, g, b);
    }
  }
}

Shader::Shader(const std::string &texture_filename,
               const vector3 directional_light)
    : texture(texture_filename), directional_light(directional_light) {
  if (texture.width == 0 || texture.height == 0)
    throw std::runtime_error("Failed to load texture: " + texture_filename);
  has_texture = (texture_filename != "_no_texture");
}

vector3 Transform::transform(const vector3 base[3], const vector3 &p) {
  return vector3(base[0].getX() * p.getX() + base[1].getX() * p.getY() +
                     base[2].getX() * p.getZ(),
                 base[0].getY() * p.getX() + base[1].getY() * p.getY() +
                     base[2].getY() * p.getZ(),
                 base[0].getZ() * p.getX() + base[1].getZ() * p.getY() +
                     base[2].getZ() * p.getZ());
}

void Transform::update_cache() const {
  float cy = cosf(yaw), sy = sinf(yaw);
  float cp = cosf(pitch), sp = sinf(pitch);
  float cr = cosf(roll), sr = sinf(roll);

  vector3 i_yaw(cy, 0, sy);
  vector3 j_yaw(0, 1, 0);
  vector3 k_yaw(-sy, 0, cy);
  vector3 base_yaw[3] = {i_yaw, j_yaw, k_yaw};

  vector3 i_pitch(1, 0, 0);
  vector3 j_pitch(0, cp, -sp);
  vector3 k_pitch(0, sp, cp);

  vector3 i = transform(base_yaw, i_pitch);
  vector3 j = transform(base_yaw, j_pitch);
  vector3 k = transform(base_yaw, k_pitch);
  vector3 base_ip[3] = {i, j, k};

  vector3 i_roll(cr, sr, 0);
  vector3 j_roll(-sr, cr, 0);
  vector3 k_roll(0, 0, 1);

  cached_base[0] = transform(base_ip, i_roll);
  cached_base[1] = transform(base_ip, j_roll);
  cached_base[2] = transform(base_ip, k_roll);

  cached_inverse[0] = vector3(cached_base[0].getX(), cached_base[1].getX(), cached_base[2].getX());
  cached_inverse[1] = vector3(cached_base[0].getY(), cached_base[1].getY(), cached_base[2].getY());
  cached_inverse[2] = vector3(cached_base[0].getZ(), cached_base[1].getZ(), cached_base[2].getZ());

  cache_valid = true;
}
