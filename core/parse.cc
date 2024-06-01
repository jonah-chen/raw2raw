#include "raw2raw.h"
#include "libraw/libraw.h"
#include <omp.h>

namespace {
template<typename T>
void to_little_endian_16(const T *input, char *output, size_t count)
// 0x2010 -> \x10\x20
// 0x2010 >> 8 = 0x20
// 0x2010 & 0xFF = 0x10
{
    static_assert(sizeof(T) >= 2, "Size of T must be at least 2 bytes");
    for (size_t i = 0; i < count; i++) {
        output[i * 2] = input[i] & 0xFF;
        output[i * 2 + 1] = input[i] >> 8;
    }
}

}

namespace r2r {
ParserErrors get_dimensions (const char *filename, size_t &width, size_t &height)
{
    LibRaw RawProcessor;
    if (RawProcessor.open_file(filename) != LIBRAW_SUCCESS) {
        RawProcessor.recycle();
        return ParserErrors::CANNOT_OPEN_FILE;
    }
    width = RawProcessor.imgdata.sizes.raw_width;
    height = RawProcessor.imgdata.sizes.raw_height;
    RawProcessor.recycle();
    return ParserErrors::PARSE_SUCCESS;
}

ParserErrors parse_image(const char *filename, 
                 io_t *output,
                 size_t width,
                 size_t height)
{
    LibRaw RawProcessor;
    if (RawProcessor.open_file(filename) != LIBRAW_SUCCESS) {
        RawProcessor.recycle();
        return ParserErrors::CANNOT_OPEN_FILE;
    }
    if (RawProcessor.unpack() != LIBRAW_SUCCESS) {
        RawProcessor.recycle();
        return ParserErrors::CANNOT_UNPACK_FILE;
    }
    if (width != RawProcessor.imgdata.sizes.raw_width ||
        height != RawProcessor.imgdata.sizes.raw_height) 
    {
        RawProcessor.recycle();
        return ParserErrors::SIZE_MISMATCH;
    }
    if (output) {
        static_assert(sizeof(*RawProcessor.imgdata.rawdata.raw_image) == sizeof(*output),
                    "Size mismatch between LibRaw and u16");

        memcpy(output, RawProcessor.imgdata.rawdata.raw_image, width * height * sizeof(*output));
    }
    RawProcessor.recycle();
    return ParserErrors::PARSE_SUCCESS;
}

/* Note: we are assuming width and height are correct. */
ParserErrors write_image(const char *ref_file,
                         const char *out_file,
                         const u16 *output_data,
                         const u16 *raw_data,
                         size_t width,
                         size_t height)
{
    size_t img_size = width * height * sizeof(u16);
    // Open reference file and read its bytes
    std::ifstream ref(ref_file, std::ios::binary);
    if (!ref.is_open()) {
        return ParserErrors::CANNOT_OPEN_FILE;
    }

    // read the bytes into a buffer
    // first find the size of ref
    ref.seekg(0, std::ios::end);
    size_t ref_size = ref.tellg();
    ref.seekg(0, std::ios::beg);
    char *ref_bytes = new char[ref_size];
    ref.read(ref_bytes, ref_size);
    ref.close();

    // convert raw_data to binary, little endian
    // copy in little endian
    char *raw_bytes = new char[img_size];
    to_little_endian_16(raw_data, raw_bytes, width * height);

    // find the offset of raw_data in ref_data
    size_t offset = -1ul;
    for (size_t i = 0; i < ref_size - img_size; i++) {
        if (memcmp(ref_bytes + i, raw_bytes, img_size) == 0) {
            offset = i;
            break;
        }
    }
    if (offset == -1ul) {
        delete[] ref_bytes;
        delete[] raw_bytes;
        return ParserErrors::MAY_BE_COMPRESSED;
    }

    // write the output data *RawProcessor.imgdata.rawdata.raw_image
    // overwrite the ref bytes with the output data bytes
    to_little_endian_16(output_data, ref_bytes + offset, width * height); 

    // write the updated ref bytes to out_file
    std::ofstream out(out_file, std::ios::binary);
    out.write(ref_bytes, ref_size);
    out.close();

    delete[] ref_bytes;
    delete[] raw_bytes;

    return ParserErrors::PARSE_SUCCESS; 
}


Task::Task(const std::vector<std::filesystem::path> &files)
    : n_images(files.size())
{   
    get_dimensions(files[0].c_str(), width, height);
    wh = width * height; whn = wh * n_images;
    data = new u16[whn];
    
    std::vector<ParserErrors> errs(n_images);
    // parse each image in parallel
    #pragma omp parallel for
    for (size_t i = 0; i < n_images; i++) {
        errs[i] = parse_image(files[i].c_str(), 
            data + (i * wh), width, height);
    }
    for (ParserErrors e : errs) {
        if (e != ParserErrors::PARSE_SUCCESS) {
            delete[] data;
            throw e;
        }
    }
}
Task::Task(const std::filesystem::path &root) : Task(
    std::vector<std::filesystem::path>(std::filesystem::directory_iterator(root), 
                                       std::filesystem::directory_iterator())) {}

Task::~Task()
{
    delete[] data;
}

}
