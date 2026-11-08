#include "file_io.h"

#include "libdesktop/platform.h"

#include <limits>

namespace {

core::file_path PlatformPath(const std::filesystem::path& path) {
    return core::file_path(core::utf16_to_utf8(path.wstring()));
}

[[noreturn]] void ThrowFileOperation(const char* operation,
                                     const platform::file_op_result& result) {
    std::string message = operation;
    message += ": ";
    message += result.error_message.empty() ? "file operation failed" : result.error_message;
    throw FileError(std::move(message));
}

[[noreturn]] void ThrowFileOperation(const char* operation) {
    throw FileError(std::string(operation) + ": file operation failed");
}

}  // namespace

std::string ReadFileText(const std::filesystem::path& path) {
    const auto file = platform::open_file(PlatformPath(path), platform::file_open_mode::read_shared);
    if (!file) ThrowFileOperation("cannot open the file");

    const std::uint64_t size64 = file->size();
    if (size64 > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
        throw FileError("cannot read the file: it is too large");
    const std::size_t size = static_cast<std::size_t>(size64);
    std::string text(size, '\0');
    std::size_t offset = 0;
    while (offset < size) {
        const std::uint64_t read = file->read(reinterpret_cast<std::uint8_t*>(text.data()) + offset,
                                              size - offset);
        if (read == 0) ThrowFileOperation("cannot read the file");
        offset += static_cast<std::size_t>(read);
    }
    return text;
}

void WriteFileBytes(const std::filesystem::path& path,
                    const std::span<const std::uint8_t> bytes) {
    const auto opened = platform::create_atomic_file_writer_result(PlatformPath(path));
    if (!opened.success()) ThrowFileOperation("cannot open the file for writing", opened.result);

    auto result = opened.writer->write(bytes);
    if (!result.success()) ThrowFileOperation("cannot write the file", result);
    result = opened.writer->flush();
    if (!result.success()) ThrowFileOperation("cannot flush the file", result);
    result = opened.writer->commit();
    if (!result.success()) ThrowFileOperation("cannot commit the file", result);
}

void WriteFileText(const std::filesystem::path& path, const std::string_view text) {
    WriteFileBytes(path, {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()});
}
