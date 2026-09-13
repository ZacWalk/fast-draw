#include "file_io.h"

#include <fstream>

std::string ReadFileText(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) throw FileError("cannot open the file for reading");

    const auto size = stream.tellg();
    if (size < 0) throw FileError("cannot determine the file size");
    stream.seekg(0);

    std::string text(static_cast<std::size_t>(size), '\0');
    if (!text.empty() && !stream.read(text.data(), size)) throw FileError("cannot read the file");

    return text;
}

void WriteFileText(const std::filesystem::path& path, std::string_view text) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) throw FileError("cannot open the file for writing");

    if (!text.empty() &&
        !stream.write(text.data(), static_cast<std::streamsize>(text.size())))
        throw FileError("cannot write the file");
}
