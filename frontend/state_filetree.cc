/**
 * Raw2Raw
 * frontend/state_filetree.cc
 * Author: Jonah Chen
 * Last updated in rev 0.1
 *
 * This file contains the implementation of the file tree in the GUI interface. It is a tree view of the file system
 * that allows the user to select a folder to view the images in that folder.
 *
 * This project is licensed under the GPL v3.0 license. Please see the LICENSE file for more information.
 */

#include "state.h"
#include <iostream>
#include <format>
#ifdef _WIN32
#include <Windows.h>
#endif

class TreeNode {
    std::filesystem::path path;
    std::string name;
    std::vector<TreeNode> children;
    bool is_directory;
    std::shared_ptr<std::filesystem::path> selected_path;   // the path that is selected, which will be propagated to the root of the tree
    int image_files;
public:
    TreeNode(std::filesystem::path _path, std::shared_ptr<std::filesystem::path> _selected_path, bool _leaf = false) :
            path(std::move(_path)),
            name(path.filename().string()),
            is_directory(!_leaf && std::filesystem::is_directory(path)),
            selected_path(std::move(_selected_path)),
            image_files(0)
    {
        if (name.empty())
            name = this->path.string();
    }

    void change_name(const std::string &new_name) {
        name = new_name;
    }

    void expand() {
#ifdef _WIN32 // if we are on Windows, we must allow the user to select any of the available drives
        if (path == "/" && children.empty()) {
            auto drives = GetLogicalDrives();
            for (int i = 0; i < 26; i++) {
                if (drives & (1 << i)) {
                    std::string drive = "A:\\";
                    drive[0] += i;
                    children.emplace_back(drive, selected_path);
                }
            }
            return;
        }
#endif
        if (is_directory && children.empty()) {
            try {
                for (const auto &entry: std::filesystem::directory_iterator(path)) {
                    if (entry.is_directory()) {
                        children.emplace_back(entry, selected_path);
                    } else if (entry.is_regular_file() and r2r::recognized_raw(entry.path())) {
                        image_files++;
                    }
                }
                // sort the children
                if (image_files > 0) {
                    children.emplace_back(path, selected_path, true);
                    children.back().change_name(std::to_string(image_files) + " images");
                }
                std::sort(children.begin(), children.end(), [](const TreeNode &a, const TreeNode &b) {
                    return a.name < b.name;
                });
            } catch (std::filesystem::filesystem_error &e) {
                // If we don't have permission to access the directory, we will just add a node that says "No Permission"
                // that cannot be selected
                children.emplace_back("NoPermission", nullptr, true);
            }
        }
    }

    bool render() {
        bool is_updated = false;
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (is_directory) {
            bool open = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanAllColumns);
            if (open) {
                expand();
                for (auto &child : children) {
                    is_updated = child.render() || is_updated;
                }
                ImGui::TreePop();
            }
        } else {
            ImGuiTreeNodeFlags leaf_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAllColumns;
            if (selected_path && *selected_path == path) {
                leaf_flags |= ImGuiTreeNodeFlags_Selected;
            }
            ImGui::TreeNodeEx(name.c_str(), leaf_flags);
            if (selected_path && ImGui::IsItemClicked()) {
                *selected_path = path;
                is_updated = true;
            }
            ImGui::TreePop();
        }
        return is_updated;
    }
};


void State::render_file_tree() {
    constexpr ImGuiTableFlags flags =   ImGuiTableFlags_BordersV |
                                        ImGuiTableFlags_BordersOuterH |
                                        ImGuiTableFlags_Resizable |
                                        ImGuiTableFlags_RowBg |
                                        ImGuiTableFlags_NoBordersInBody;

    updates.selected_path = false;
    ImGui::Begin(sFileTree, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginTable("File Tree Table", 1, flags)) {
        ImGui::TableSetupColumn("File Tree Columns", ImGuiTableColumnFlags_NoHide);
        ImGui::TableHeadersRow();
        if ((updates.selected_path = root_node->render())) {
            ImGui::SetWindowFocus(sGridView); // if the selected path has changed, we will switch to the grid view
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

void State::setup_file_tree() {
    std::filesystem::path root_path {"/"};
    root_node = new TreeNode(root_path, selected_path);
}

void State::cleanup_file_tree() {
    delete root_node;
}

void State::update_images_in_path() {
    if (updates.selected_path) { // if the selected path has changed
        images_in_path.clear();
        missing_thumbs.clear();
        updates.selected_images.clear();
        updates.last_selected_image = -1;

        for (const auto &entry: std::filesystem::directory_iterator(*selected_path)) {
            if (entry.is_regular_file() and r2r::recognized_raw(entry.path())) {
                images_in_path.push_back(entry.path());
            }
        }
        std::sort(images_in_path.begin(), images_in_path.end());
        missing_thumbs = images_in_path;

        logs += std::format("Selected Path: {}\n", selected_path->string());
        logs += std::format("Number of images: {}\n", images_in_path.size());

        // load the new thumbnails
        auto purged = thumbnails.purge([this](const fspath &path) {
            // if the folder the image is in is the selected path, we don't purge it
            return path.parent_path() != *selected_path;
        });
        logs += std::format("Purged {} thumbnails.\n", purged);

        temp.clear(); // stop previous async loading
    }
}
