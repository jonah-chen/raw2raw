/**
 * Raw2Raw
 * frontend/loupe.h
 * Author: Jonah Chen
 * Last updated in rev 0.1
 *
 * This file contains the definition of the Image8 class, which represents an 8-bit image that can be displayed in the
 * loupe. It is used to load and display raw images using OpenGL.
 *
 * This project is licensed under the GPL v3.0 license. Please see the LICENSE file for more information.
 */

#pragma once
#include "libraw/libraw_types.h"
#include <filesystem>
#include "glad/glad.h"
#include <future>

using fspath = std::filesystem::path;

class Image8 {
public:
    Image8() = default;
    ~Image8();
    void load_opengl();
    void open_file(const fspath &filename);
    std::future<void> open_async(const fspath &filename);
    operator bool() const;
    void render(float mag_pct = 0.0f) const; // if mag_pct is 0, then it is fit
private:
    libraw_processed_image_t *image {nullptr};
    int width{0}, height{0};
    GLuint texture_id { -1u };
    bool loaded { false };
};
