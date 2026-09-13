#include "test_framework.h"

#include "canvas.h"
#include "file_io.h"
#include "png_export.h"

#include <png.h>

#include <cstdio>
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

// A minimal reader so the tests verify what was actually encoded.
DecodedImage DecodePng(const std::filesystem::path& path) {
    std::FILE* file = nullptr;
    if (::_wfopen_s(&file, path.c_str(), L"rb") != 0 || !file)
        throw FileError("cannot open the PNG for reading");

    png_structp png = ::png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info = ::png_create_info_struct(png);

    if (setjmp(png_jmpbuf(png))) {
        ::png_destroy_read_struct(&png, &info, nullptr);
        std::fclose(file);
        throw FileError("libpng failed to read the image");
    }

    ::png_init_io(png, file);
    ::png_read_info(png, info);

    DecodedImage image;
    image.width = static_cast<int>(::png_get_image_width(png, info));
    image.height = static_cast<int>(::png_get_image_height(png, info));

    ::png_set_expand(png);
    ::png_set_strip_16(png);
    ::png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    ::png_set_gray_to_rgb(png);
    ::png_read_update_info(png, info);

    std::vector<png_byte> row(static_cast<std::size_t>(image.width) * 4);
    image.pixels.resize(static_cast<std::size_t>(image.width) * image.height);

    for (int y = 0; y < image.height; ++y) {
        png_bytep rowPointer = row.data();
        ::png_read_row(png, rowPointer, nullptr);
        for (int x = 0; x < image.width; ++x) {
            const png_byte* source = row.data() + static_cast<std::size_t>(x) * 4;
            image.pixels[static_cast<std::size_t>(y) * image.width + x] =
                (static_cast<std::uint32_t>(source[3]) << 24) |
                (static_cast<std::uint32_t>(source[0]) << 16) |
                (static_cast<std::uint32_t>(source[1]) << 8) |
                static_cast<std::uint32_t>(source[2]);
        }
    }

    ::png_destroy_read_struct(&png, &info, nullptr);
    std::fclose(file);
    return image;
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
    CHECK(canvas.Dc() != nullptr);

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

TEST(GdiCanDrawIntoTheCanvas) {
    // The canvas is a DIB section so text can still be rendered by GDI.
    Canvas canvas;
    CHECK(canvas.Resize(20, 20));
    canvas.Clear(0xFF000000);

    RECT rect{0, 0, 20, 20};
    HBRUSH brush = ::CreateSolidBrush(RGB(255, 255, 255));
    ::FillRect(canvas.Dc(), &rect, brush);
    ::DeleteObject(brush);

    CHECK_EQ(canvas.Row(10)[10] & 0x00FFFFFFu, 0x00FFFFFFu);
}

TEST(ArgbFromColorRefSwapsChannels) {
    // COLORREF is 0x00BBGGRR; the canvas stores 0xAARRGGBB.
    CHECK_EQ(ArgbFromColorRef(RGB(0x12, 0x34, 0x56)), 0xFF123456u);
    CHECK_EQ(ArgbFromColorRef(RGB(255, 0, 0)), 0xFFFF0000u);
    CHECK_EQ(ArgbFromColorRef(RGB(0, 0, 255), 0x80), 0x800000FFu);
}

TEST(ExportingAnEmptyCanvasFails) {
    Canvas canvas;
    CHECK_THROWS(WritePng(canvas, TempImage("fast-draw-test-empty.png")), FileError);
}
