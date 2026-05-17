#pragma once

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <stdexcept>
#include <thread>
#include "math.hpp"
#include "util.hpp"

#include "object_loader.hpp"
#include "rasterizer.hpp"

void write_image_to_file(Image image, const std::string &filename);
