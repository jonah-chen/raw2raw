/**
 * Raw2Raw
 * frontend/thumbnail.cc
 * Author: Jonah Chen
 * Last updated in rev 0.1
 *
 * This file contains the implementation of the thumbnail class, which is used to display thumbnails in the GUI
 * interface. It also contains the implementation of the thumbnail store, which is used to store and manage the loading
 * of thumbnails.
 *
 * This project is licensed under the GPL v3.0 license. Please see the LICENSE file for more information.
 */

#include <algorithm>
#include "thumbnail.h"
#include "imgui.h"
#include "core/raw2raw.h"
#include "libraw/libraw.h"
#include <iostream>

#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP

#ifdef _WIN32
#define STBI_WINDOWS_UTF8
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize2.h"

#include "glad/glad.h"

constexpr int nchannels = 4; // RGBA

class GLuintTexWrapper {
    GLuint tex_id;
public:
    constexpr GLuintTexWrapper() : tex_id(-1u) {}
    explicit GLuintTexWrapper(GLuint x) : tex_id(x) {}
    ~GLuintTexWrapper() {
        if (tex_id != -1u)
            glDeleteTextures(1, &tex_id);
    }
    constexpr operator GLuint() const {
        return tex_id;
    }
    constexpr operator bool() const {
        return tex_id != -1u;
    }
};

static std::shared_ptr<GLuintTexWrapper> load_thumbnail_opengl(r2r::u8 *data, int width, int height, int n) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height * n, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return std::make_shared<GLuintTexWrapper>(texture);
}

void read_thumb(const fspath &filename, int width, int height, r2r::u8 *output) {
    LibRaw rawProcessor;
    rawProcessor.open_file(filename.string().c_str());
    rawProcessor.unpack_thumb();
    auto &&thumbnail = rawProcessor.imgdata.thumbnail;
    int iwidth = thumbnail.twidth;
    int iheight = thumbnail.theight;

    if (thumbnail.tformat != LIBRAW_THUMBNAIL_JPEG && thumbnail.tformat != LIBRAW_THUMBNAIL_BITMAP) {
        std::cerr << "Thumbnail format not supported" << std::endl;
        return;
    }

    int jwidth, jheight, jcomp;
    auto *thumb_data = reinterpret_cast<const stbi_uc *>(thumbnail.thumb);
    auto *img = stbi_load_from_memory(thumb_data, thumbnail.tlength, &jwidth, &jheight, &jcomp, 3);
    rawProcessor.recycle();

    if (img == nullptr) {
        std::cerr << "Failed to load thumbnail" << std::endl;
        return;
    }
    if (jwidth != iwidth || jheight != iheight || jcomp != 3) {
        std::cerr << "Thumbnail size mismatch " << jwidth << "x" << jheight << "x" << jcomp << std::endl;
        stbi_image_free(img);
        return;
    }

    // first find the output width and length. We keep aspect ratio but make sure the output is not larger than width x height
    int owidth, oheight;
    if (iwidth * height > iheight * width) {
        owidth = width;
        oheight = width * iheight / iwidth;
    } else {
        oheight = height;
        owidth = height * iwidth / iheight;
    }
    auto *resized_img = stbir_resize_uint8_srgb(img, iwidth, iheight, 0, nullptr, owidth, oheight, 0, STBIR_RGB);
    stbi_image_free(img);

    // now we want to write the resized image to RGBA output. We should center the image (as our output is a square)
    // and make the non-image pixels transparent

    // Calculate the starting point (offset) for both x and y coordinates
    int xOffset = (width - owidth) / 2;
    int yOffset = (height - oheight) / 2;

    // Modify the loop to start from the offset and end at the offset plus the resized image size
    for (int i = 0; i < oheight; i++) {
        for (int j = 0; j < owidth; j++) {
            int lidx = ((i + yOffset) * width + j + xOffset) * 4;
            int ridx = (i * owidth + j) * 3;
            output[lidx + 0] = resized_img[ridx + 0];
            output[lidx + 1] = resized_img[ridx + 1];
            output[lidx + 2] = resized_img[ridx + 2];
            output[lidx + 3] = 255;
        }
    }
    stbi_image_free(resized_img);
}

r2r::u8 *read_thumbs(path_vec files, int width, int height) {
    auto n = files.size();
    auto image_size = width * height * nchannels;
    r2r::u8 *output = new r2r::u8[n * image_size];
    #pragma omp parallel for default(shared) schedule(auto)
    for (int i = 0; i < n; i++) {
        read_thumb(files[i], width, height, &output[i * image_size]);
    }
    return output;
}

Thumbnail::Thumbnail(std::shared_ptr<GLuintTexWrapper> tex, int i, int total_images, std::string str_id) : texture(std::move(tex)), str_id(std::move(str_id)) {
    // always 1 row
    if (this->str_id.empty()) {
        this->str_id = "##defaultThumbnail";
    }
    float size = 1.f / total_images;
    uv0 = ImVec2(0.f, i * size);
    uv1 = ImVec2(1.f, (i + 1) * size);
}

unsigned char Thumbnail::button(float size) const {
    if (texture) {
        ImGui::ImageButton(str_id.c_str(), (void*)(intptr_t)(GLuint)*texture, ImVec2(size, size), uv0, uv1);
        unsigned char event = 0;
        if (ImGui::IsItemClicked()) {
            event |= kSingleClick;
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            event |= kDoubleClick;
        }
        return event;
    }
    return 0;
}


Thumbnail &ThumbnailStore::operator[](const fspath &key) {
    if (contains(key)) {
        return data.at(key);
    }
    return data.at(fspath());
}

const Thumbnail &ThumbnailStore::operator[](const fspath &key) const {
    if (contains(key)) {
        return data.at(key);
    }
    return data.at(fspath());
}

bool ThumbnailStore::contains(const fspath &key) const {
    return data.find(key) != data.end();
}

void ThumbnailStore::read_thumbs_to_opengl(r2r::u8 *thumbs, const path_vec &files, int size, int thumbs_per_tex) {
    int n = (int)files.size();
    int image_size = size * size * nchannels;
    for (int i = 0; i < n; i += thumbs_per_tex) {
        int m = std::min(thumbs_per_tex, (int)n - i);
        auto tex = load_thumbnail_opengl(thumbs + i * image_size, size, size, m);
        for (int j = 0; j < m; j++) {
            auto &&file = files[i + j];
            data.emplace(file, Thumbnail(tex, j, m, file.filename().string()));
        }
    }
}

void ThumbnailStore::load_default(const fspath &default_thumb_path) {
    auto output = read_thumbs({default_thumb_path}, thumb_size, thumb_size);
    read_thumbs_to_opengl(output, {fspath()}, thumb_size, 1);
}

size_t ThumbnailStore::purge(const std::function<bool(const fspath &)>& pred) {
    return std::erase_if(data, [&pred](const auto &p) {
        const auto &fn = p.first;
        if (fn == fspath())
            return false;
        return pred(fn);
    });
}
