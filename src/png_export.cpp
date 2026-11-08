#include "png_export.h"

#include "canvas.h"
#include "file_io.h"

#include <libfiles/encode.h>

#include <new>
#include <string>

void WritePng(const Canvas& canvas, const std::filesystem::path& path) {
    if (!canvas.IsValid()) throw FileError("there is nothing to save");

    // Canvas is a top-down straight-alpha BGRA8 core::surface, which is the
    // shared libfiles encoder contract on Windows.
    libfiles::image_view image;
    image.pixels = reinterpret_cast<const std::uint8_t*>(canvas.Pixels());
    image.stride = canvas.Surface()->stride();
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
    for (const auto& note : encoded.notes) core::trace("fast-draw PNG encoder: " + note);
    WriteFileBytes(path, encoded.data);
}
