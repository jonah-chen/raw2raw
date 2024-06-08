/**
 * Raw2Raw
 * frontend/state_primary.cc
 * Author: Jonah Chen
 * Last updated in rev 0.1
 *
 * This file contains the implementation of the primary state of the GUI interface, which includes the main render loop.
 * It also handles the buttons that allow the user to run the various algorithms. Currently, only pixel-wise reduction
 * algorithms are supported.
 *
 * This project is licensed under the GPL v3.0 license. Please see the LICENSE file for more information.
 */

#include "state.h"
#include "imgui.h"
#include <vector>
#include <algorithm>
#include <format>
#include <future>
#include "core/raw2raw.h"
#include <omp.h>


State::State() {
    thumbnails.load_default("../assets/default_thumbnail.ARW");
    setup_file_tree();
}

State::~State() {
    cleanup_file_tree();
}

void State::render() {
    render_file_tree();
    update_images_in_path();
    update_thumbnail_async(omp_get_max_threads());
    render_grid_view();
    render_grid_zooming_slider();
    render_single_image_view();

    render_info_box();
    render_logs_box();
    render_bottom_bar();
    render_output_specifier();


    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        updates.selected_images.clear();
        updates.last_selected_image = -1;
    }
    else if (ImGui::GetIO().KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_A)) {
            updates.selected_images.clear();
            for (int i = 0; i < 100; i++) {
                updates.selected_images.insert(i);
            }
        } else if (ImGui::IsKeyPressed(ImGuiKey_C) ||
                   ImGui::IsKeyPressed(ImGuiKey_V) ||
                   ImGui::IsKeyPressed(ImGuiKey_Z)) {
            err = "No Shortcut";
        }
    }
    else if (ImGui::GetIO().KeyAlt) {
        if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
            err = "No MATLAB";
        }
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_2)) {
        select_every_n(2);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_3)) {
        select_every_n(3);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_5)) {
        select_every_n(5);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_7)) {
        select_every_n(7);
    }

    error_popups();

}


struct LoupeViewStyle {
    LoupeViewStyle() {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    }
    ~LoupeViewStyle() {
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();
    }
};

void State::render_single_image_view() {
    LoupeViewStyle style;
    if (single_future.valid() && single_future.wait_for(kZero) == std::future_status::ready) {
        single_future.get();
        loupe_image.load_opengl();
        // select the single image view to be on top
        ImGui::SetWindowFocus(sLoupeView);
    }

    ImGui::Begin(sLoupeView, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
    loupe_image.render(0.0f); // TODO: allow the user to zoom in and browse around
    ImGui::End();
}

void State::render_bottom_bar() {
    constexpr float slider_width = 200.f;

    using r2r::pReduction;
    auto make_algo_button = [this](const char *name, r2r::pReduction algo) {
        if (ImGui::Button(name)) {
            run_algo(algo);
        }
        ImGui::SameLine();
    };

    ImGui::Begin("Pixel-wise Reduction");
    make_algo_button("Mean", pReduction::MEAN);
    make_algo_button("Median", pReduction::MEDIAN);
    make_algo_button("Summation", pReduction::SUMMATION);
    make_algo_button("Maximum", pReduction::MAXIMUM);
    make_algo_button("Minimum", pReduction::MINIMUM);
    make_algo_button("Range", pReduction::RANGE);
    make_algo_button("Variance", pReduction::VARIANCE);
    make_algo_button("Standard Deviation", pReduction::STANDARD_DEVIATION);
    ImGui::End();
}

void State::select_every_n(int n) {
    std::set<int> new_selected_images;
    int counter = 0;
    for (int i : updates.selected_images) {
        if (counter++ % n == 0) {
            new_selected_images.insert(i);
        }
    }
    updates.selected_images = new_selected_images;
}

void State::run_algo(r2r::pReduction algo) {
    if (updates.selected_images.size() < 2) {
        err = "No Selection";
        return;
    }
    if (!std::filesystem::exists(fspath(output.path).parent_path())) {
        err = "Invalid Path";
        return;
    }
    if (algo_future.valid()) {
        err = "Already Running";
        return;
    }

    update_thumbnails = false; // stop updating thumbnails to free up CPU time

    algo_future = std::async(std::launch::async, [this, algo] () {
        r2r::Timer timer;
        path_vec selected_images;
        for (int i: updates.selected_images) {
            selected_images.push_back(images_in_path[i]);
        }
        r2r::Task task(selected_images);
        r2r::io_t *ans = r2r::p_reduce(task, algo);
        auto e = r2r::write_image(selected_images[0], output.path, ans, task.data, task.width, task.height);
        if (e == r2r::ParserErrors::PARSE_SUCCESS) {
            log(std::format("Output written to {}.\n", output.path));
        } else {
            log(std::format("Failed to write the output due to {}.\n", (int)e));
        }
        delete[] ans;
        log(std::format("The algorithm took {:0.2f}s on {} images.\n", timer.stop() / 1000, selected_images.size()));
    });

    algo_future.get();

    single_future = loupe_image.open_async((loupe_path = output.path));

    update_thumbnails = true;
}
