#include "core/raw2raw.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <unordered_set>

int main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <algorithm> <directory or list of files> [-o <output path>]\n";
        return 0;
    }
    std::string algorithm = argv[1];
    std::filesystem::path output_path = "output";

    std::vector<std::filesystem::path> files;
    {
        int i;
        for (i = 2; i < argc && std::string(argv[i]) != "-o"; i++) {
            files.emplace_back(argv[i]);
        }
        if (i < argc) // which means we have a -o
        {
            if (++i == argc) {
                std::cout << "Need an output path\n";
                return 1;
            }
            output_path = argv[i];
        }
    }
    if (output_path == "output") {
        output_path += files[0].extension();
    }
    
    if (files.empty()) {
        std::cout << "No file is specified\n";
        return 1;
    }
    if (files.size() == 1) {
        files = {std::filesystem::directory_iterator(files[0]), std::filesystem::directory_iterator()};
    }
    if (files.size() < 2) {
        std::cout << "Need at least two files to process\n";
        return 1;
    }

    std::cout << "Reading " << files.size() << " files...\n";
    r2r::Timer timer;
    r2r::Task task(files);
    std::cout << "...finished in " << std::setprecision(5) << timer.stop() << "ms\n\n";

    std::unordered_set<std::string> reduction_algos = {
        "mean", "median", "summation", "maximum", "minimum", "range",
        
        // we can have some aliases too
        "average", "avg", "sum", "max", "min"
    };

    if (reduction_algos.find(algorithm) != reduction_algos.end()) {
        // reduction algo will have the same output
        r2r::io_t *ans = nullptr;

        timer.start();
        if (algorithm == "mean" || algorithm == "average" || algorithm == "avg") {
            ans = r2r::p_mean(task);
        } else if (algorithm == "median") {
            ans = r2r::p_median(task);
        } else if (algorithm == "summation" || algorithm == "sum") {
            ans = r2r::p_summation(task);
        } else if (algorithm == "maximum" || algorithm == "max") {
            ans = r2r::p_maximum(task);
        } else if (algorithm == "minimum" || algorithm == "min") {
            ans = r2r::p_minimum(task);
        } else if (algorithm == "range") {
            ans = r2r::p_range(task);
        }
        std::cout << "Computing the " << algorithm << " took " << 
            std::setprecision(5) << timer.stop() << "ms\n\n" << 
            "------------------------------------------\n\nA small preview of the output:\n";
        
        // print a 10x10 square of the reduced image
        for (size_t i = 0; i < 10; i++) {
            for (size_t j = 0; j < 10; j++) {
                std::cout << std::setw(5) << ans[i * task.width + j] << " ";
            }
            std::cout << "\n";
        }
        std::cout << "------------------------------------------\n\n";

        if (r2r::write_image(files[0].c_str(), output_path.c_str(), ans, task.data, task.width, task.height) == r2r::ParserErrors::PARSE_SUCCESS) {
            std::cout <<  "Output written to " << output_path << "\n";
        } else {
            std::cout << "Failed to write the output\n";
            delete[] ans;
            return 1;
        }
        delete[] ans;
    } else {
        std::cout << algorithm << " is currently not supported\n";
        return 1;
    }

    return 0;
}
