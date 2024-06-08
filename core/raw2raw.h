/**
 * Raw2Raw
 * core/raw2raw.h
 * Author: Jonah Chen
 * Last updated in rev 0.1
 *
 * This file outlines the core functionality of the raw2raw library, which is used to process raw images into other
 * raw images. This is the API of the library that is exposed, and contains the main types and functions that are used
 * in the library.
 *
 * This project is licensed under the GPL v3.0 license. Please see the LICENSE file for more information.
 */
#pragma once
#include <cstdint>
#include <vector>
#include <filesystem>

namespace r2r {

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using io_t = u16;
using interm_t = u32;

enum class ParserErrors {
    PARSE_SUCCESS = 0,
    CANNOT_OPEN_FILE,
    CANNOT_UNPACK_FILE,
    SIZE_MISMATCH,
    MAY_BE_COMPRESSED
};

bool recognized_raw(std::filesystem::path fp);

ParserErrors get_dimensions (const char *filename, size_t &width, size_t &height);

/* Parse a raw file using libraw, and return the raw data (as u16)
 */
ParserErrors parse_image(const char *filename,
                         io_t *output,
                         size_t width,
                         size_t height);

/* Write the image to an output file, that is in the spirit of a reference
 * file. As of now, the output file will have the same metadata as the reference
 * file.
 * 
 * Note that only uncompressed files are supported at the moment. Compressed
 * raw will require some reverse engineering of the compression algorithm.
 */
ParserErrors write_image(const std::filesystem::path &ref_file,
                         const std::filesystem::path &out_file,
                         const io_t *output_data,
                         const io_t *raw_data,
                         size_t width,
                         size_t height);

template<typename I, typename O>
void array_cast(const I *input, O *output, size_t count)
{
    #pragma omp parallel for default(none) shared(input, output, count) schedule(static)
    for (size_t i = 0; i < count; i++) {
        output[i] = (O)input[i];
    }
}

/* Cpu profiler using std::chrono. Stop will restart the timer and return the 
 * time elapsed since the last start in milliseconds. 
 */
struct Timer
{
    Timer(bool _start = true);
    void start();
    double stop();
private:
    double start_;
};

/* A task that holds a set of images to be processed together in a 
 * contiguous block.
 * 
 * The CLI tool will only support one task at a time, but the API will allow
 * for multiple tasks to be created at once.
 */
struct Task {
    Task(const std::filesystem::path &root);
    Task(const std::vector<std::filesystem::path> &files);
    ~Task();
    io_t *data;
    u32 max_val;
    size_t width, height, n_images;
    size_t wh, whn;
};

enum class pReduction {
    NONE = 0,
    MEAN,
    MEDIAN,
    SUMMATION,
    MAXIMUM,
    MINIMUM,
    RANGE,
    VARIANCE,
    STANDARD_DEVIATION,
    SKEWNESS,
    KURTOSIS,
    ENTROPY
};

io_t *p_reduce(const Task &task, pReduction reduction);

/* pixel-wise reduction functions (avail in photoshop stack modes)
 * but unlike photoshop, these functions can be done raw 
 */
io_t *p_mean(const Task &task);
io_t *p_median(const Task &task);
io_t *p_summation(const Task &task);
io_t *p_maximum(const Task &task);
io_t *p_minimum(const Task &task);
io_t *p_range(const Task &task);
/* these functions need to be renormalized, as they have vastly different ranges
 * compared with the input */
io_t *p_variance(const Task &task);
io_t *p_standard_deviation(const Task &task);

/** TODO: Implement all of these, and more
io_t *p_skewness(const Task &task);
io_t *p_kurtosis(const Task &task);
io_t *p_entropy(const Task &task);


// pixel-wise reduction functions (that are not in photoshop)
io_t *p_mean_remove_outlier(Task &task, int outliers);


// whole-image analysis functions
std::vector<double> noise(const Task &task); 
// TODO: a general renormalize function taking many parameters
*/

} // namespace r2r
