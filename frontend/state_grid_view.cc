/**
 * Raw2Raw
 * frontend/state_grid_view.cc
 * Author: Jonah Chen
 * Last updated in rev 0.1
 *
 * This file contains the implementation of the grid view in the GUI interface. The grid view displays thumbnails of
 * images in the current directory. The user can click on them to select them, and double click to open them in the
 * loupe view. The grid view also allows the user to zoom in and out of the grid.
 *
 * This project is licensed under the GPL v3.0 license. Please see the LICENSE file for more information.
 */

#include "state.h"
#include <omp.h>

namespace {
template<typename T> // Returns c = a \ b
std::vector<T> ordered_sub(const std::vector<T> &a, const std::vector<T> &b) {
    std::vector<T> c;
    int i, j;
    for (i=0,j=0;i<a.size();i++) {
        if (j < b.size() && a[i] == b[j]) {
            j++;
        } else {
            c.push_back(a[i]);
        }
    }
    return c;
}

struct GridViewStyle {
    constexpr static float r = 0.4f;
    constexpr static float g = 0.4f;
    constexpr static float b = 0.4f;
    constexpr static float a_active = 0.8f;
    GridViewStyle() {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r, g, b, 0.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r, g, b, a_active / 2));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(r, g, b, a_active));
    }
    ~GridViewStyle() {
        ImGui::PopStyleColor(4);
    }
};

struct SelectedImageStyle {
    constexpr static float r = GridViewStyle::r;
    constexpr static float g = GridViewStyle::g;
    constexpr static float b = GridViewStyle::b;
    explicit SelectedImageStyle(bool _selected) : selected(_selected) {
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r, g, b, GridViewStyle::a_active));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r, g, b, (1.f + GridViewStyle::a_active) / 2));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(r, g, b, 1.f));
        }
    }
    ~SelectedImageStyle() {
        if (selected) {
            ImGui::PopStyleColor(3);
        }
    }
private:
    bool selected;
};

} // anonymous namespace

void State::update_thumbnail_async(int chunk_size) {
    if (thumb_future.valid()) {
        if (thumb_future.wait_for(kZero) != std::future_status::ready) {
            return;
        }
        auto *thumbs = thumb_future.get();
        if (temp.empty()) { // we have moved on to a different folder, so we don't need to do anymore work here
            delete[] thumbs;
            return;
        }
        thumbnails.read_thumbs_to_opengl(thumbs, temp, thumb_size, 8);
        delete[] thumbs;
        log(std::format("{} thumbnails loaded in {:0.2f}s.\n", temp.size(), omp_get_wtime() - thumb_future_submit_time));

        // TODO: to save on resources (for weaker systems), we may want to purge some thumbnails here with openGL.
        // TODO: Come up with a policy to decide which thumbnails to purge.

        // erase temp from missing_thumbs, assuming that temp is a subset of missing_thumbs and that the order is preserved
        missing_thumbs = ordered_sub(missing_thumbs, temp);
        temp.clear();
    }
    if (missing_thumbs.empty()) return;

    if (!update_thumbnails) return;

    // find which images are currently shown using the scroll position
    // this will be the default behavior
    temp.clear();
    if (scroll.last_idx > scroll.first_idx && scroll.first_idx >= 0 && scroll.last_idx < images_in_path.size()) {
        for (int i = scroll.first_idx; i <= scroll.last_idx; i++) {
            if (!thumbnails.contains(images_in_path[i])) {
                temp.push_back(images_in_path[i]);
            }
        }
    }
    if (temp.empty()) {
        for (const auto &missing_thumb : missing_thumbs) {
            if (!thumbnails.contains(missing_thumb)) {
                temp.push_back(missing_thumb);
            }
            if (temp.size() >= chunk_size) {
                break;
            }
        }
    }
    if (temp.empty()) return;

    thumb_future = std::async(std::launch::async, read_thumbs, temp, thumb_size, thumb_size);
    thumb_future_submit_time = omp_get_wtime();
    log(std::format("{}/{} thumbnails remaining to load. Queued {} thumbnails.\n", missing_thumbs.size(), images_in_path.size(), temp.size()));
}

void State::render_grid_view() {
    GridViewStyle _grid_view_style;
    ImGui::Begin(sGridView, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
    float screen_height = ImGui::GetContentRegionAvail().y;
    ImGui::Columns(images_per_row, nullptr, false);

    // get the scroll position
    scroll.pos = ImGui::GetScrollY();
    scroll.first_idx = std::numeric_limits<int>::max();
    scroll.last_idx = std::numeric_limits<int>::min();

    for (int i = 0; i < images_in_path.size(); i++) {
        SelectedImageStyle _selected_image_style { updates.selected_images.contains(i) };

        if (ImGui::GetCursorPosY() >= scroll.pos) {
            // this means that index i is on the screen
            scroll.first_idx = std::min(scroll.first_idx, i);
        }

        auto &thumb = thumbnails[images_in_path[i]];
        auto event = thumb.button(ImGui::GetContentRegionAvail().x);
        if (event & kDoubleClick) { // double click -> open in single image view
            log(std::format("Opening {} in loupe view.\n", images_in_path[i].filename().string()));
            if (loupe_path != images_in_path[i]) {
                single_future = loupe_image.open_async((loupe_path = images_in_path[i]));
            } else {
                ImGui::SetWindowFocus(sLoupeView);
            }
        }
        if (event & kSingleClick) { // single click -> select image
            auto &io = ImGui::GetIO();
            if (io.KeyCtrl) {
                updates.selected_images.insert(i);
            } else if (io.KeyShift) {
                for (int j = std::min(i, updates.last_selected_image); j <= std::max(i, updates.last_selected_image); j++) {
                    updates.selected_images.insert(j);
                }
            } else {
                updates.selected_images.clear();
                updates.selected_images.insert(i);
            }
            updates.last_selected_image = i;

            if (output.follow_selection) {
                const auto &path = images_in_path[i];
                auto path_wo_ext = path; path_wo_ext.replace_extension();
                std::string output_path = std::format("{}_STACKED{}", path_wo_ext.string(), path.extension().string());
                strncpy(output.path, output_path.c_str(), kPathMaxLen);
            }
        }
        ImGui::TextWrapped(images_in_path[i].filename().string().c_str());

        if (ImGui::GetCursorPosY() <= scroll.pos + screen_height) {
            scroll.last_idx = i; // this means that image is on the screen
        }

        ImGui::NextColumn();
    }

    // introduce one row above and one row below
    scroll.first_idx = std::max(0, scroll.first_idx - images_per_row);
    scroll.last_idx = std::min((int)images_in_path.size() - 1, scroll.last_idx + images_per_row);

    ImGui::Columns(1);
    ImGui::End();
}

void State::render_grid_zooming_slider() {
    ImGui::Begin("Grid Zoom Slider");
    ImGui::PushItemWidth(-1.f);
    ImGui::SliderInt("##Images Per Row", &images_per_row, 3, 13);
    ImGui::PopItemWidth();
    ImGui::End();
}
