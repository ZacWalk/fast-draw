#include "file_io.h"

#include "platform.h"

namespace {

pf::file_path PlatformPath(const std::filesystem::path& path) {
    return pf::file_path(pf::utf16_to_utf8(path.wstring()));
}

[[noreturn]] void ThrowFileOperation(const char* operation,
                                     const pf::file_operation_result& result) {
    std::string message = operation;
    message += ": ";
    message += result.message.empty() ? "file operation failed" : result.message;
    throw FileError(std::move(message));
}

}  // namespace

std::string ReadFileText(const std::filesystem::path& path) {
    const auto opened = pf::open_for_read_result(PlatformPath(path), true);
    if (!opened.success()) ThrowFileOperation("cannot open the file", opened.result);

    const std::uint32_t size = opened.file->size();
    std::string text(size, '\0');
    std::uint32_t offset = 0;
    while (offset < size) {
        std::uint32_t read = 0;
        if (!opened.file->read(reinterpret_cast<std::uint8_t*>(text.data()) + offset,
                               size - offset, &read) ||
            read == 0)
            ThrowFileOperation("cannot read the file", opened.result);
        offset += read;
    }
    return text;
}

void WriteFileBytes(const std::filesystem::path& path,
                    const std::span<const std::uint8_t> bytes) {
    const auto opened =
        pf::create_atomic_file_writer_result(PlatformPath(path));
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
