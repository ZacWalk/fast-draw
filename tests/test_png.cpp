#include "test_framework.h"

#include "canvas.h"
#include "file_io.h"
#include "png_export.h"
#include "platform.h"

#include <libfiles/encode.h>
#include <png.h>

#include <filesystem>
#include <vector>

namespace {

struct DecodedImage {
    int width = 0;
    int height = 0;
    std::vector<std::uint32_t> pixels;  // 0xAARRGGBB

    std::uint32_t At(int x, int y) const {
        return pixels[static_cast<std::size_t>(y) * width + x];
    }
};

// A minimal reader so the tests verify what was actually encoded. The
// libpng target here is the copy built and owned by libfiles; fast-draw's
// production code only reaches it through libfiles::encode_png.
DecodedImage DecodePng(const std::filesystem::path& path) {
   const std::string encoded = ReadFileText(path);
   png_image png{};
   png.version = PNG_IMAGE_VERSION;
   if (!::png_image_begin_read_from_memory(&png, encoded.data(), encoded.size()))
       throw FileError("libpng failed to read the image");
   png.format = PNG_FORMAT_RGBA;

   std::vector<png_byte> rgba(PNG_IMAGE_SIZE(png));
   if (!::png_image_finish_read(&png, nullptr, rgba.data(), 0, nullptr)) {
       ::png_image_free(&png);
       throw FileError("libpng failed to decode the image");
   }
   DecodedImage result;
   result.width = static_cast<int>(png.width);
   result.height = static_cast<int>(png.height);
   result.pixels.resize(static_cast<std::size_t>(result.width) * result.height);
   for (std::size_t i = 0; i < result.pixels.size(); ++i) {
       const auto* source = rgba.data() + i * 4;
       result.pixels[i] = (static_cast<std::uint32_t>(source[3]) << 24) |
                          (static_cast<std::uint32_t>(source[0]) << 16) |
                          (static_cast<std::uint32_t>(source[1]) << 8) |
                          static_cast<std::uint32_t>(source[2]);
   }
   return result;
}

std::filesystem::path TempImage(const char* name) {
    return std::filesystem::temp_directory_path() / name;
}

}  // namespace

TEST(CanvasStartsInvalidAndResizes) {
    Canvas canvas;
    CHECK(!canvas.IsValid());

    CHECK(canvas.Resize(16, 8));
    CHECK(canvas.IsValid());
    CHECK_EQ(canvas.Width(), 16);
    CHECK_EQ(canvas.Height(), 8);

    // A zero size releases the surface rather than leaving a stale one.
    CHECK(!canvas.Resize(0, 10));
    CHECK(!canvas.IsValid());
}

TEST(CanvasClearFillsEveryPixel) {
    Canvas canvas;
    CHECK(canvas.Resize(37, 5));  // deliberately not a multiple of the SIMD width
    canvas.Clear(0xFF204060);

    bool allMatch = true;
    for (int y = 0; y < canvas.Height(); ++y) {
        for (int x = 0; x < canvas.Width(); ++x) {
            if (canvas.Row(y)[x] != 0xFF204060) allMatch = false;
        }
    }
    CHECK(allMatch);
}

TEST(CanvasSetOpaqueForcesAlpha) {
    Canvas canvas;
    CHECK(canvas.Resize(11, 3));
    canvas.Clear(0x00123456);
    canvas.SetOpaque();

    bool allOpaque = true;
    for (int y = 0; y < canvas.Height(); ++y) {
        for (int x = 0; x < canvas.Width(); ++x) {
            if (canvas.Row(y)[x] != 0xFF123456) allOpaque = false;
        }
    }
    CHECK(allOpaque);
}

TEST(PngRoundTripPreservesPixels) {
    Canvas canvas;
    CHECK(canvas.Resize(8, 4));
    canvas.Clear(0xFF102030);

    canvas.Row(0)[0] = 0xFFFF0000;  // red
    canvas.Row(1)[2] = 0xFF00FF00;  // green
    canvas.Row(3)[7] = 0xFF0000FF;  // blue

    const auto path = TempImage("fast-draw-test.png");
    WritePng(canvas, path);

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

TEST(PngRoundTripPreservesAlpha) {
    Canvas canvas;
    CHECK(canvas.Resize(4, 1));
    canvas.Clear(0x00000000);
    canvas.Row(0)[0] = 0x80FF8040;

    const auto path = TempImage("fast-draw-test-alpha.png");
    WritePng(canvas, path);

    const DecodedImage image = DecodePng(path);
    CHECK_EQ(image.At(0, 0), 0x80FF8040u);
    CHECK_EQ(image.At(1, 0), 0x00000000u);

    std::filesystem::remove(path);
}

TEST(LibfilesPngBoundaryAcceptsCanvasBgra) {
    Canvas canvas;
    CHECK(canvas.Resize(2, 1));
    canvas.Row(0)[0] = 0x80402010;
    canvas.Row(0)[1] = 0xFF102030;

    libfiles::image_view image;
    image.pixels = reinterpret_cast<const std::uint8_t*>(canvas.Pixels());
    image.stride = sizeof(std::uint32_t) * 2;
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

TEST(SharedPixelSurfaceDrawContextWritesPixels) {
    Canvas canvas;
    CHECK(canvas.Resize(20, 20));
    canvas.Clear(0xFF000000);

    {
        auto draw = pf::create_pixel_surface_context(canvas.Surface());
        CHECK(draw != nullptr);
        draw->fill_solid_rect({0, 0, 20, 20}, pf::color_t{255, 255, 255});
    }

    CHECK_EQ(canvas.Row(10)[10] & 0x00FFFFFFu, 0x00FFFFFFu);
}

TEST(ArgbFromColorPreservesChannels) {
    CHECK_EQ(ArgbFromColor(Rgb(0x12, 0x34, 0x56)), 0xFF123456u);
    CHECK_EQ(ArgbFromColor(Rgb(255, 0, 0)), 0xFFFF0000u);
    CHECK_EQ(ArgbFromColor(Rgb(0, 0, 255), 0x80), 0x800000FFu);
}

TEST(ExportingAnEmptyCanvasFails) {
    Canvas canvas;
    CHECK_THROWS(WritePng(canvas, TempImage("fast-draw-test-empty.png")), FileError);
}

TEST(ExportingToAnUnavailableFileFails) {
    Canvas canvas;
    CHECK(canvas.Resize(2, 2));
    const auto path = std::filesystem::temp_directory_path() /
                      "fast-draw-directory-that-does-not-exist" / "image.png";
    CHECK_THROWS(WritePng(canvas, path), FileError);
}

TEST(AtomicPngWriteReportsSharedWriterFailures) {
    Canvas canvas;
    CHECK(canvas.Resize(2, 2));
    const auto path = TempImage("fast-draw-atomic-failure.png");
    const pf::atomic_file_test_failure failures[] = {
        pf::atomic_file_test_failure::open,
        pf::atomic_file_test_failure::write,
        pf::atomic_file_test_failure::flush,
        pf::atomic_file_test_failure::commit,
    };

    for (const auto failure : failures) {
        pf::test_set_atomic_file_failure(failure);
        CHECK_THROWS(WritePng(canvas, path), FileError);
        pf::test_set_atomic_file_failure(pf::atomic_file_test_failure::none);
        std::filesystem::remove(path);
    }
}
