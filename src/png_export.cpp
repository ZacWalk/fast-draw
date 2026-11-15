#include "png_export.h"

#include "file_io.h"

#include <libfiles/encode.h>

#include <new>
#include <string>

void WritePng(const core::const_surface_ptr& surface, const std::filesystem::path& path)
{
	if (!surface || surface->empty()) throw FileError("there is nothing to save");
	if (surface->format() != core::texture_format::ARGB) throw FileError("the image is not a BGRA surface");

	// A surface the software renderer drew is top-down BGRA8, which is the
	// shared libfiles encoder contract on Windows.
	libfiles::image_view image;
	image.pixels = surface->pixels();
	image.stride = surface->stride();
	image.width = surface->width();
	image.height = surface->height();
	image.format = libfiles::pixel_format::bgra8;

	libfiles::encoded encoded;
	try
	{
		encoded = libfiles::encode_png(image, 1, {});
	}
	catch (const libfiles::encode_error& error)
	{
		throw FileError(std::string("PNG encode failed: ") + error.what());
	}
	catch (const std::bad_alloc&)
	{
		throw FileError("PNG encode failed: out of memory");
	}

	if (encoded.data.empty()) throw FileError("PNG encoder returned no data");
	for (const auto& note : encoded.notes) core::trace("fast-draw PNG encoder: " + note);
	WriteFileBytes(path, encoded.data);
}
