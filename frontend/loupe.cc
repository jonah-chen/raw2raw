/**
 * Raw2Raw
 * frontend/loupe.cc
 * Author: Jonah Chen
 * Last updated in rev 0.1
 *
 * This file contains the implementation of the loupe, which is a tool that allows the user to view a single image at
 * full size. As of now, it is not able to zoom in or scroll around the image, but that will be added in the future.
 *
 * This project is licensed under the GPL v3.0 license. Please see the LICENSE file for more information.
 */

#include "loupe.h"
#include "libraw/libraw.h"
#include <iostream>
#include <format>
#include "imgui.h"

/* Open a raw file and process it with LibRaw's dcraw_process, and load it into memory */
void Image8::open_file(const fspath &filename) {
    LibRaw rawProcessor;
    rawProcessor.open_file(filename.string().c_str());
    rawProcessor.unpack();
    rawProcessor.dcraw_process();

    int ec;
    image = rawProcessor.dcraw_make_mem_image(&ec);
    if (ec != LIBRAW_SUCCESS) {
        std::cerr << std::format("Failed to load image {}: {}", filename.string(), libraw_strerror(ec)) << std::endl;
        image = nullptr;
    }
    rawProcessor.recycle();
}

/* Perform the open_file function asynchronously, used in the GUI so that the program doesn't temporarily freeze */
std::future<void> Image8::open_async(const fspath &filename) {
    return std::async(std::launch::async, [this, filename] {
        open_file(filename);
    });
}

/* Load the image into OpenGL memory */
void Image8::load_opengl() {
    if (image == nullptr) {
        return;
    }
    if (!loaded) {
        loaded = true;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    auto *data = image->data;
    width = image->width;
    height = image->height;

    if (image->colors != 3 || image->bits != 8) {
        std::cerr << std::format("Image has {} colors and {} bits per pixel, only 3 colors and 8 bits per pixel are supported", image->colors, image->bits) << std::endl;
        goto cleanup;
    }
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

cleanup:
    LibRaw::dcraw_clear_mem(image);
}

Image8::~Image8() {
    if (*this) {
        glDeleteTextures(1, &texture_id);
    }
}

Image8::operator bool() const {
    return texture_id != -1u;
}

/* Render the image in the GUI */
void Image8::render(float mag_pct) const {
    if (texture_id == -1u)
        return;

    auto [x_size, y_size] = ImGui::GetContentRegionAvail();
    float x, y;
    float aspect_ratio = (float)width / (float)height;
    if (mag_pct == 0.0f) {
        if (x_size > y_size * aspect_ratio) {
            x = y_size * aspect_ratio;
            y = y_size;
        } else {
            x = x_size;
            y = x_size / aspect_ratio;
        }
    } else {
        x = x_size * mag_pct;
        y = y_size * mag_pct;
    }

    // center the image
    ImGui::SetCursorPosX((x_size - x) / 2);
    ImGui::SetCursorPosY((y_size - y) / 2);

    ImGui::Image((void*)(intptr_t)texture_id, ImVec2(x, y));
}
