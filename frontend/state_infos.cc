/**
 * Raw2Raw
 * frontend/state_infos.cc
 * Author: Jonah Chen
 * Last updated in rev 0.1
 *
 * This file contains the implementation of the various info boxes on the GUI interface.
 *
 * This project is licensed under the GPL v3.0 license. Please see the LICENSE file for more information.
 */

#include "state.h"

namespace {
/* Display a popup that the user can close by clicking OK, usually for error, warning, or info messages */
void error_popup(const char *name, const char *msg) {
    if (ImGui::BeginPopupModal(name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(msg);
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

} // anonymous namespace

/* Render the info box, which contains metadata or other image info */
void State::render_info_box() {
    // TODO: Implement metadata information. Some can already be extracted from LibRaw
    std::string info_str = "In the future, metadata information will be displayed here. Currently it is just some debug info\n\n";
    info_str += "Last Clicked: %d\nSelected Path: %s\nScroll Position: %f\nFirst Image: %d\nLast Image: %d\nSelected Images: ";
    for (int i : updates.selected_images) {
        info_str += std::to_string(i) + ", ";
    }
    ImGui::Begin(sInfoBox);
    ImGui::TextWrapped(info_str.c_str(), updates.last_selected_image, selected_path->string().c_str(), scroll.pos, scroll.first_idx, scroll.last_idx);
    ImGui::End();
}

/* Log a message to the log box */
void State::log(std::string &&msg) {
    logs += msg;
    ImGui::SetScrollHereY(1.f);
}

/* Render the logs box, which contains logs of actions taken */
void State::render_logs_box() {
    ImGui::Begin(sLogBox);
    ImGui::TextWrapped(logs.c_str());
    ImGui::End();
}

/* Render the output specifier box, which contains the output path and other options */
void State::render_output_specifier() {
    ImGui::Begin(sOutputBox);
    ImGui::Text("Output:");
    ImGui::SameLine();
    ImGui::InputText("##Output Path", output.path, kPathMaxLen);
    ImGui::SameLine();
    ImGui::Checkbox("Follow Selection", &output.follow_selection);
    ImGui::End();
}

/* Specify the different error popups that can be displayed */
void State::error_popups() {
    error_popup("Error", "An error occurred.");
    error_popup("Compressed", "Compressed raw files are currently not supported.");
    error_popup("Invalid Path", "The path you are trying to save to is invalid. Make sure that the path exists.");
    error_popup("No Selection", "To stack, you must select at least two images.");
    error_popup("No Shortcut", "Shortcuts like Ctrl+C, Ctrl+V, Ctrl+Z are currently not supported.");
    error_popup("No MATLAB", "This is not MATLAB! Wait...why would you use MATLAB in the first place?");
    error_popup("Only One Image", "You may only select one image for full view at a time. Not zero or 2+.");
    error_popup("Already Running", "An algorithm is already running. Please wait for it to finish before starting another instance.");

    if (err) {
        ImGui::OpenPopup(err);
        err = nullptr;
    }
}
