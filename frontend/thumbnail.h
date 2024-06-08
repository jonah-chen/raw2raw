/**
 * Raw2Raw
 * frontend/thumbnail.h
 * Author: Jonah Chen
 * Last updated in rev 0.1
 *
 * This file contains the definition of the Thumbnail and ThumbnailStore classes used to manage thumbnails in the GUI.
 *
 * This project is licensed under the GPL v3.0 license. Please see the LICENSE file for more information.
 */

#pragma once
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>
#include "imgui.h"
#include "core/raw2raw.h"
#include <future>
#include <functional>
#include <forward_list>

using fspath = std::filesystem::path;
using path_vec = std::vector<fspath>;

r2r::u8 *read_thumbs(path_vec files, int width, int height);
void read_thumb(const fspath &filename, int width, int height, r2r::u8 *output);

constexpr unsigned char kSingleClick = 1;
constexpr unsigned char kDoubleClick = 2;
constexpr int thumb_size = 256; // TODO: this should be part of a config file, not hardcoded.

class GLuintTexWrapper;

/* A thumbnail class that contains the texture and UV coordinates for rendering */
class Thumbnail {
    std::shared_ptr<GLuintTexWrapper> texture; // when all the thumbnails are gone, the ref count will drop to 0 so the texture will be cleaned up
    ImVec2 uv0, uv1;
    std::string str_id;
public:
    Thumbnail(std::shared_ptr<GLuintTexWrapper> tex, int i, int total_images, std::string str_id);
    unsigned char button(float size) const;
};

/*
 * The thumbnail store class acts like a dictionary cache, and thus implements similar functions as a dictionary.
 * Note that to add a thumbnail to the store, you must use read_thumbs_to_opengl, not the [] operator.
 */
class ThumbnailStore {
public:
    ThumbnailStore() = default;

    Thumbnail &operator[](const fspath &key);
    const Thumbnail &operator[](const fspath &key) const;
    bool contains(const fspath &key) const;

    void load_default(const fspath &default_thumb_path);

    void read_thumbs_to_opengl(r2r::u8 *thumbs, const path_vec &files, int size, int thumbs_per_tex);

    /* Return the number of thumbnails purged */
    size_t purge(const std::function<bool(const fspath &)>& pred = [](const fspath &){ return true; });

private:
    std::unordered_map<fspath, Thumbnail> data;
};

