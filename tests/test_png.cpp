#include "test_framework.h"

#include "document.h"
#include "view.h"
#include "file_io.h"
#include "png_export.h"
#include "libdesktop/platform.h"
#include "libdesktop/render.h"
#include "libdesktop/render/software.h"

#include <libfiles/encode.h>
#include <png.h>

#include <cstdlib>
#include <filesystem>
#include <vector>

namespace
{

	struct DecodedImage
	{
		int width = 0;
		int height = 0;
		std::vector<std::uint32_t> pixels; // 0xAARRGGBB

		std::uint32_t At(int x, int y) const { return pixels[static_cast<std::size_t>(y) * width + x]; }
	};

	// A minimal reader so the tests verify what was actually encoded. The
	// libpng target here is the copy built and owned by libfiles; fast-draw's
	// production code only reaches it through libfiles::encode_png.
	DecodedImage DecodePng(const std::filesystem::path& path)
	{
		const std::string encoded = ReadFileText(path);
		png_image png{};
		png.version = PNG_IMAGE_VERSION;
		if (!::png_image_begin_read_from_memory(&png, encoded.data(), encoded.size()))
			throw FileError("libpng failed to read the image");
		png.format = PNG_FORMAT_RGBA;

		std::vector<png_byte> rgba(PNG_IMAGE_SIZE(png));
		if (!::png_image_finish_read(&png, nullptr, rgba.data(), 0, nullptr))
		{
			::png_image_free(&png);
			throw FileError("libpng failed to decode the image");
		}
		DecodedImage result;
		result.width = static_cast<int>(png.width);
		result.height = static_cast<int>(png.height);
		result.pixels.resize(static_cast<std::size_t>(result.width) * result.height);
		for (std::size_t i = 0; i < result.pixels.size(); ++i)
		{
			const auto* source = rgba.data() + i * 4;
			result.pixels[i] = (static_cast<std::uint32_t>(source[3]) << 24) |
			                   (static_cast<std::uint32_t>(source[0]) << 16) |
			                   (static_cast<std::uint32_t>(source[1]) << 8) | static_cast<std::uint32_t>(source[2]);
		}
		return result;
	}

	std::filesystem::path TempImage(const char* name)
	{
		return std::filesystem::temp_directory_path() / name;
	}

	// A BGRA surface of one colour, 0xAARRGGBB.
	core::surface_ptr Surface(int width, int height, std::uint32_t colour)
	{
		auto surface = std::make_shared<core::surface>();
		surface->alloc(width, height, core::texture_format::ARGB, core::orientation::top_left);
		for (int y = 0; y < height; ++y)
			for (int x = 0; x < width; ++x) surface->set_pixel(x, y, colour);
		return surface;
	}

	// Each channel of two 0xAARRGGBB colours within `tolerance` of the other.
	bool Near(std::uint32_t actual, std::uint32_t expected, int tolerance)
	{
		for (int shift = 0; shift < 32; shift += 8)
		{
			const int a = static_cast<int>((actual >> shift) & 0xFF);
			const int b = static_cast<int>((expected >> shift) & 0xFF);
			if (std::abs(a - b) > tolerance) return false;
		}
		return true;
	}

} // namespace

TEST(PngRoundTripPreservesPixels)
{
	const auto surface = Surface(8, 4, 0xFF102030);
	surface->set_pixel(0, 0, 0xFFFF0000); // red
	surface->set_pixel(2, 1, 0xFF00FF00); // green
	surface->set_pixel(7, 3, 0xFF0000FF); // blue

	const auto path = TempImage("fast-draw-test.png");
	WritePng(surface, path);

	// The file must start with the PNG signature.
	const std::string bytes = ReadFileText(path);
	CHECK(bytes.size() > 8);
	CHECK_EQ(static_cast<unsigned char>(bytes[0]), 0x89u);
	CHECK_EQ(bytes.substr(1, 3), std::string("PNG"));

	const DecodedImage image = DecodePng(path);
	CHECK_EQ(image.width, 8);
	CHECK_EQ(image.height, 4);
	CHECK_EQ(image.At(0, 0), 0xFFFF0000u);
	CHECK_EQ(image.At(2, 1), 0xFF00FF00u);
	CHECK_EQ(image.At(7, 3), 0xFF0000FFu);
	CHECK_EQ(image.At(4, 2), 0xFF102030u);

	std::filesystem::remove(path);
}

TEST(PngRoundTripPreservesAlpha)
{
	const auto surface = Surface(4, 1, 0x00000000);
	surface->set_pixel(0, 0, 0x80FF8040);

	const auto path = TempImage("fast-draw-test-alpha.png");
	WritePng(surface, path);

	const DecodedImage image = DecodePng(path);
	CHECK_EQ(image.At(0, 0), 0x80FF8040u);
	CHECK_EQ(image.At(1, 0), 0x00000000u);

	std::filesystem::remove(path);
}

TEST(LibfilesPngBoundaryAcceptsSurfaceBgra)
{
	const auto surface = Surface(2, 1, 0xFF102030);
	surface->set_pixel(0, 0, 0x80402010);

	libfiles::image_view image;
	image.pixels = surface->pixels();
	image.stride = surface->stride();
	image.width = 2;
	image.height = 1;
	image.format = libfiles::pixel_format::bgra8;

	const libfiles::encoded encoded = libfiles::encode_png(image, 1, {});
	CHECK(encoded.data.size() > 8);
	CHECK_EQ(encoded.data[0], static_cast<std::uint8_t>(0x89));
	CHECK_EQ(encoded.data[1], static_cast<std::uint8_t>('P'));
	CHECK_EQ(encoded.data[2], static_cast<std::uint8_t>('N'));
	CHECK_EQ(encoded.data[3], static_cast<std::uint8_t>('G'));
}

TEST(SharedPixelSurfaceDrawContextWritesPixels)
{
	const auto surface = Surface(20, 20, 0xFF000000);

	{
		auto draw = render::create_surface_context(surface);
		CHECK(draw != nullptr);
		draw->draw_rect({0, 0, 20, 20}, core::color(core::rgba(255, 255, 255, 255)));
		(void)draw->render();
	}

	CHECK_EQ(surface->get_pixel(10, 10) & 0x00FFFFFFu, 0x00FFFFFFu);
}

TEST(AnExportedPageHoldsTheDrawing)
{
	Document document;
	document.SetPaperColour(Rgb(0xFF, 0xFF, 0xFF));

	Style solid;
	solid.fill = Rgb(0xC0, 0x20, 0x20);
	solid.hasLine = false;
	DrawObj left(ShapeKind::Rectangle, Rect(-500, 300, -100, -300), solid);
	left.text = L"label";
	document.Add(std::move(left));

	Style gradient;
	gradient.fill = Rgb(0x00, 0x00, 0xFF);
	gradient.fillEnd = Rgb(0x00, 0xFF, 0x00);
	gradient.gradient = true;
	gradient.hasLine = false;
	document.Add(DrawObj(ShapeKind::Rectangle, Rect(100, 300, 500, -300), gradient));

	Style framed;
	framed.hasFill = false;
	framed.line = Rgb(0x10, 0x10, 0x10);
	framed.lineWidth = 8;
	document.Add(DrawObj(ShapeKind::Rectangle, Rect(-50, 50, 50, -50), framed));

	Style line;
	line.hasFill = false;
	line.line = Rgb(0x20, 0x40, 0x80);
	line.lineWidth = 4;
	document.Add(DrawObj(ShapeKind::Line, Rect(-500, -380, 500, -380), line));

	// At 100 dpi a document unit is a pixel, and the page centre is (550, 425).
	const core::surface_ptr page = RenderDocument(document, 100);
	CHECK(page != nullptr);
	if (!page) return;
	CHECK_EQ(page->width(), 1100u);
	CHECK_EQ(page->height(), 850u);

	const auto path = TempImage("fast-draw-test-export.png");
	WritePng(page, path);
	const DecodedImage image = DecodePng(path);
	std::filesystem::remove(path);

	CHECK_EQ(image.At(5, 5), 0xFFFFFFFFu);           // paper
	CHECK(Near(image.At(250, 200), 0xFFC02020u, 1)); // the solid fill
	CHECK(Near(image.At(850, 126), 0xFF0000FFu, 3)); // the gradient's top...
	CHECK(Near(image.At(850, 425), 0xFF00807Fu, 3)); // ...middle...
	CHECK(Near(image.At(850, 724), 0xFF00FF00u, 3)); // ...and bottom
	CHECK(Near(image.At(500, 425), 0xFF101010u, 1)); // the frame's border
	CHECK_EQ(image.At(550, 425), 0xFFFFFFFFu);       // and its unfilled middle
	CHECK(Near(image.At(300, 805), 0xFF204080u, 1)); // the line, four pixels wide
	CHECK_EQ(image.At(300, 809), 0xFFFFFFFFu);
}

TEST(ExportingNothingFails)
{
	CHECK_THROWS(WritePng(nullptr, TempImage("fast-draw-test-empty.png")), FileError);
	CHECK_THROWS(WritePng(std::make_shared<core::surface>(), TempImage("fast-draw-test-empty.png")), FileError);
}

TEST(ExportingToAnUnavailableFileFails)
{
	const auto surface = Surface(2, 2, 0xFF000000);
	const auto path = std::filesystem::temp_directory_path() / "fast-draw-directory-that-does-not-exist" / "image.png";
	CHECK_THROWS(WritePng(surface, path), FileError);
}

TEST(AtomicPngWriteReportsSharedWriterFailures)
{
	const auto surface = Surface(2, 2, 0xFF000000);
	const auto path = TempImage("fast-draw-atomic-failure.png");
	const platform::atomic_file_test_failure failures[] = {
		platform::atomic_file_test_failure::open,
		platform::atomic_file_test_failure::write,
		platform::atomic_file_test_failure::flush,
		platform::atomic_file_test_failure::commit,
	};

	for (const auto failure : failures)
	{
		platform::test_set_atomic_file_failure(failure);
		CHECK_THROWS(WritePng(surface, path), FileError);
		platform::test_set_atomic_file_failure(platform::atomic_file_test_failure::none);
		std::filesystem::remove(path);
	}
}
