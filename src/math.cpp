#include "math.hpp"

float vector2::signed_triangle_area(const vector2 &a, const vector2 &b,
                                     const vector2 &c) {
  vector2 ac = c - a;
  vector2 ab_perpendicular = (b - a).orthogonal();
  float area = ac.dot(ab_perpendicular);
  return area / 2.0f;
}

bool vector2::insideTriangle(vector2 a, vector2 b, vector2 c,
                             vector3 &weights) {
  float area_1 = signed_triangle_area(a, b, *this);
  float area_2 = signed_triangle_area(b, c, *this);
  float area_3 = signed_triangle_area(c, a, *this);

  bool is_inside = (area_1 >= 0 && area_2 >= 0 && area_3 >= 0);
  float total_area = area_1 + area_2 + area_3;
  if (total_area == 0.0f) return false;
  
  float inverse_area = 1.0f / total_area;
  float weight_a = area_2 * inverse_area;
  float weight_b = area_3 * inverse_area;
  float weight_c = area_1 * inverse_area;
  weights.setX(weight_a);
  weights.setY(weight_b);
  weights.setZ(weight_c);
  return is_inside;
}

float clamp(float value, float min, float max) {
  return std::max(min, std::min(value, max));
}

float degrees_to_radians(float degrees) { return degrees * (M_PI / 180.0f); }

vector3 get_random_colour() {
  return vector3(static_cast<float>(rand()) / RAND_MAX * 255.0f,
                 static_cast<float>(rand()) / RAND_MAX * 255.0f,
                 static_cast<float>(rand()) / RAND_MAX * 255.0f);
}

int get_index(int x, int y, int width) { return y * width + x; }
