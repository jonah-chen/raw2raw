/**
 * Raw2Raw
 * frontend/state.h
 * Author: Jonah Chen
 * Last updated in rev 0.1
 *
 * This file contains the definition of the State class, which is the class that encapsulates the entire GUI state.
 * It contains the file tree, grid view, loupe view, and other information boxes. It also contains the primary render
 * loop that is called by the main function.
 *
 * This project is licensed under the GPL v3.0 license. Please see the LICENSE file for more information.
 */

#pragma once
#include <memory>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <set>
#include "thumbnail.h"
#include "loupe.h"
#include <optional>
#include <chrono>

constexpr auto kZero = std::chrono::seconds(0);

constexpr const char *sAppTitle = "Raw to Raw Computational Photography Engine";
constexpr const char *sLogBox = "Logs";
constexpr const char *sInfoBox = "Info Box";
constexpr const char *sOutputBox = "Output Specifier";
constexpr const char *sFileTree = "File Tree";
constexpr const char *sGridView = "Grid View";
constexpr const char *sLoupeView = "Loupe View";



class TreeNode;

class State {
private: // file tree
    TreeNode *root_node;
    std::shared_ptr<std::filesystem::path> selected_path {std::make_shared<std::filesystem::path>()};
    std::vector<std::filesystem::path> images_in_path {};
    void render_file_tree();
    void setup_file_tree();
    void cleanup_file_tree();
    void update_images_in_path();
private: // grid view
    int images_per_row {5};
    bool update_thumbnails {true};
    ThumbnailStore thumbnails{};
    struct {
        int first_idx, last_idx;
        float pos;
    } scroll;
    struct {
        bool selected_path {false};
        std::set<int> selected_images;
        int last_selected_image {-1};
    } updates;
    std::vector<std::filesystem::path> missing_thumbs, temp;
    std::future<r2r::u8*> thumb_future;
    double thumb_future_submit_time;
    void update_thumbnail_async(int chunk_size);
    void render_grid_view();
    void render_grid_zooming_slider();
private: // infos
    const char *err {nullptr};
    std::string logs;
    constexpr static int kPathMaxLen = 256;
    struct {
        char path[kPathMaxLen] {0};
        bool follow_selection {true};
    } output;
    void log(std::string &&msg);
    void render_info_box();
    void render_logs_box();
    void render_output_specifier();
    void error_popups();
private: // primary
    fspath loupe_path;
    Image8 loupe_image;
    std::future<void> single_future;
    std::future<void> algo_future;
    void render_single_image_view();
    void render_bottom_bar();
    void run_algo(r2r::pReduction algo);
    // this is very useful for stuff like HDR or other bracketed images
    void select_every_n(int n);
public: // interface
    State();
    ~State();
    void render();
};
