#include "png_export.h"

#include "canvas.h"
#include "file_io.h"

#include <png.h>

#include <cstdio>
#include <memory>
#include <vector>

namespace {

struct FileCloser {
    void operator()(std::FILE* file) const {
        if (file) std::fclose(file);
    }
};

using FilePtr = std::unique_ptr<std::FILE, FileCloser>;

[[noreturn]] void OnPngError(png_structp, png_const_charp message) {
    throw FileError(std::string("libpng: ") + message);
}

void OnPngWarning(png_structp, png_const_charp) {}

}  // namespace

void WritePng(const Canvas& canvas, const std::filesystem::path& path) {
    if (!canvas.IsValid()) throw FileError("there is nothing to save");

    std::FILE* rawFile = nullptr;
    if (::_wfopen_s(&rawFile, path.c_str(), L"wb") != 0 || !rawFile)
        throw FileError("cannot open the file for writing");
    FilePtr file(rawFile);

    png_structp png =
        ::png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, &OnPngError, &OnPngWarning);
    if (!png) throw FileError("cannot create the PNG writer");

    png_infop info = ::png_create_info_struct(png);
    if (!info) {
        ::png_destroy_write_struct(&png, nullptr);
        throw FileError("cannot create the PNG info block");
    }

    try {
        ::png_init_io(png, file.get());
        ::png_set_IHDR(png, info, static_cast<png_uint_32>(canvas.Width()),
                       static_cast<png_uint_32>(canvas.Height()), 8, PNG_COLOR_TYPE_RGB_ALPHA,
                       PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        ::png_write_info(png, info);

        // The canvas stores 0xAARRGGBB; PNG wants R,G,B,A byte order.
        std::vector<png_byte> row(static_cast<std::size_t>(canvas.Width()) * 4);
        for (int y = 0; y < canvas.Height(); ++y) {
            const std::uint32_t* source = canvas.Row(y);
            for (int x = 0; x < canvas.Width(); ++x) {
                const std::uint32_t pixel = source[x];
                png_byte* target = row.data() + static_cast<std::size_t>(x) * 4;
                target[0] = static_cast<png_byte>((pixel >> 16) & 0xFF);
                target[1] = static_cast<png_byte>((pixel >> 8) & 0xFF);
                target[2] = static_cast<png_byte>(pixel & 0xFF);
                target[3] = static_cast<png_byte>((pixel >> 24) & 0xFF);
            }
            ::png_write_row(png, row.data());
        }

        ::png_write_end(png, nullptr);
    } catch (...) {
        ::png_destroy_write_struct(&png, &info);
        throw;
    }

    ::png_destroy_write_struct(&png, &info);
}
