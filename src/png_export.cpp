#include "png_export.h"

#include "canvas.h"
#include "file_io.h"
#include "platform.h"

#include <libfiles/encode.h>

#include <new>
#include <string>

void WritePng(const Canvas& canvas, const std::filesystem::path& path) {
    if (!canvas.IsValid()) throw FileError("there is nothing to save");

    // Canvas stores 0xAARRGGBB values. On Windows' little-endian machines the
    // byte view is exactly libfiles' straight-alpha BGRA8 contract:
    // blue, green, red, alpha. Do not swap the channels here.
    libfiles::image_view image;
    image.pixels = reinterpret_cast<const std::uint8_t*>(canvas.Pixels());
    image.stride = static_cast<std::size_t>(canvas.Width()) * sizeof(std::uint32_t);
    image.width = static_cast<std::uint32_t>(canvas.Width());
    image.height = static_cast<std::uint32_t>(canvas.Height());
    image.format = libfiles::pixel_format::bgra8;

    libfiles::encoded encoded;
    try {
        encoded = libfiles::encode_png(image, 1, {});
    } catch (const libfiles::encode_error& error) {
        throw FileError(std::string("PNG encode failed: ") + error.what());
    } catch (const std::bad_alloc&) {
        throw FileError("PNG encode failed: out of memory");
    }

    if (encoded.data.empty()) throw FileError("PNG encoder returned no data");
    for (const auto& note : encoded.notes)
        pf::debug_trace("fast-draw PNG encoder: " + note + "\n");
    WriteFileBytes(path, encoded.data);
}
