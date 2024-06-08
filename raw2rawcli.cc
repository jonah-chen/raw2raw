/**
 * Raw2Raw
 * raw2rawcli.cc
 * Author: Jonah Chen
 * Last updated in rev 0.1
 *
 * This file contains the implementation of the command line interface for raw2raw. It is a simple program that takes
 * in a list of files and an algorithm, and then processes the files using the algorithm. The output is then written to
 * a file.
 *
 * Usage: raw2rawcli <algorithm> <directory or list of files> [-o <output path>]
 *
 * Supported algorithms:
 * - mean
 * - median
 * - summation
 * - maximum
 * - minimum
 * - range
 *
 * This project is licensed under the GPL v3.0 license. Please see the LICENSE file for more information.
 */

#include "core/raw2raw.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <unordered_map>

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

    std::unordered_map<std::string, r2r::pReduction> reduction_algos = {
        {"mean",        r2r::pReduction::MEAN},
        {"average",     r2r::pReduction::MEAN},
        {"avg",         r2r::pReduction::MEAN},
        {"median",      r2r::pReduction::MEDIAN},
        {"summation",   r2r::pReduction::SUMMATION},
        {"sum",         r2r::pReduction::SUMMATION},
        {"maximum",     r2r::pReduction::MAXIMUM},
        {"max",         r2r::pReduction::MAXIMUM},
        {"minimum",     r2r::pReduction::MINIMUM},
        {"min",         r2r::pReduction::MINIMUM},
        {"range",       r2r::pReduction::RANGE},
    };

    if (reduction_algos.find(algorithm) != reduction_algos.end()) {
        // reduction algo will have the same output

        timer.start();
        r2r::io_t *ans = r2r::p_reduce(task, reduction_algos[algorithm]);

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

        auto e = r2r::write_image(files[0], output_path, ans, task.data, task.width, task.height);
        if (e == r2r::ParserErrors::PARSE_SUCCESS) {
            std::cout <<  "Output written to " << output_path << "\n";
        } else {
            std::cout << "Failed to write the output due to " << (int)e << ".\n";
            delete[] ans;
            return 1;
        }
        delete[] ans;
    } else {
        std::cout << algorithm << " is not supported. Only the following [ ";
        for (auto &&[algo,_] : reduction_algos) {
            std::cout << algo << " ";
        }
        std::cout << "] algorithms are currently supported. If you would like to suggest another algorithm, feel"
        << " free to raise the suggestion as an issue on https://github.com/jonah-chen/raw2raw.\n";
        return 1;
    }

    return 0;
}
