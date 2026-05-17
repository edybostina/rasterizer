#include "image_creator.hpp"

void write_image_to_file(Image image, const std::string &filename)
{
    std::string full_filename = filename + ".bmp";
    uint32_t header_size = 54;                                 // BMP header size
    uint32_t pixel_data_size = image.width * image.height * 3; // 3 bytes per pixel (RGB)
    uint32_t file_size = header_size + pixel_data_size;
    std::vector<unsigned char> bmp_file(file_size);
    unsigned char *bmp_data = bmp_file.data();
    unsigned char *pixel_data = bmp_data + header_size;

    // https://en.wikipedia.org/wiki/BMP_file_format
    // BMP header
    bmp_data[0] = 'B';
    bmp_data[1] = 'M';
    *reinterpret_cast<uint32_t *>(bmp_data + 2) = file_size;        // file size
    *reinterpret_cast<uint32_t *>(bmp_data + 10) = header_size;     // offset to pixel data
    *reinterpret_cast<uint32_t *>(bmp_data + 14) = 40;              // DIB header size
    *reinterpret_cast<uint32_t *>(bmp_data + 18) = image.width;     // image width
    *reinterpret_cast<uint32_t *>(bmp_data + 22) = image.height;    // image height
    *reinterpret_cast<uint16_t *>(bmp_data + 26) = 1;               // number of color planes
    *reinterpret_cast<uint16_t *>(bmp_data + 28) = 24;              // bits per pixel
    *reinterpret_cast<uint32_t *>(bmp_data + 30) = 0;               // compression (none)
    *reinterpret_cast<uint32_t *>(bmp_data + 34) = pixel_data_size; // size of pixel data
    *reinterpret_cast<uint32_t *>(bmp_data + 38) = 2835;            // horizontal resolution (72 DPI)
    *reinterpret_cast<uint32_t *>(bmp_data + 42) = 2835;            // vertical resolution (72 DPI)
    *reinterpret_cast<uint32_t *>(bmp_data + 46) = 0;               // number of colors in palette
    *reinterpret_cast<uint32_t *>(bmp_data + 50) = 0;               // important colors

    // fill the data
    for (int j = 0; j < image.height; ++j)
    {
        for (int i = 0; i < image.width; ++i)
        {
            uint32_t pixel = image.pixels[get_index(i, j, image.width)];
            // Extract from ARGB8888
            unsigned char r = (pixel >> 16) & 0xFF;
            unsigned char g = (pixel >> 8) & 0xFF;
            unsigned char b = pixel & 0xFF;
            
            pixel_data[(j * image.width + i) * 3 + 0] = b; // BGR format
            pixel_data[(j * image.width + i) * 3 + 1] = g;
            pixel_data[(j * image.width + i) * 3 + 2] = r;
        }
    }

    FILE *file = fopen(full_filename.c_str(), "wb");
    if (file)
    {
        fwrite(bmp_data, 1, file_size, file);
        fclose(file);
    }
    else
    {
        throw std::runtime_error("Failed to open file for writing: " + full_filename);
    }
}
