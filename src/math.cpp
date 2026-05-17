#include "math.hpp"

double vector2::signed_triangle_area(const vector2 &a, const vector2 &b,
                                     const vector2 &c) {
  vector2 ac = c - a;
  vector2 ab_perpendicular = (b - a).orthogonal();
  double area = ac.dot(ab_perpendicular);
  return area / 2.0;
}

bool vector2::insideTriangle(vector2 a, vector2 b, vector2 c,
                             vector3 &weights) {
  double area_1 = signed_triangle_area(a, b, *this);
  double area_2 = signed_triangle_area(b, c, *this);
  double area_3 = signed_triangle_area(c, a, *this);

  bool is_inside = (area_1 >= 0 && area_2 >= 0 && area_3 >= 0);
  double total_area = area_1 + area_2 + area_3;
  double inverse_area = 1.0 / (area_1 + area_2 + area_3);
  double weight_a = area_2 * inverse_area;
  double weight_b = area_3 * inverse_area;
  double weight_c = area_1 * inverse_area;
  weights.setX(weight_a);
  weights.setY(weight_b);
  weights.setZ(weight_c);
  return is_inside && (total_area != 0.0);
}

double clamp(double value, double min, double max) {
  return std::max(min, std::min(value, max));
}

double degrees_to_radians(double degrees) { return degrees * (M_PI / 180.0); }

vector3 get_random_colour() {
  return vector3(static_cast<double>(rand()) / RAND_MAX * 255.0,
                 static_cast<double>(rand()) / RAND_MAX * 255.0,
                 static_cast<double>(rand()) / RAND_MAX * 255.0);
}

int get_index(int x, int y, int width) { return y * width + x; }
