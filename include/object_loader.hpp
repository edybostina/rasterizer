#pragma once

#include <string>
#include <vector>
#include <memory>
#include <list>
#include <fstream>
#include <stdexcept>
#include "math.hpp"
#include "util.hpp"

std::vector<std::string> split(std::string s, std::string delimiter);
Model load_object(const std::string &obj, const std::string &texture_filename = "_no_texture", vector3 base_color = vector3(255, 255, 255));
