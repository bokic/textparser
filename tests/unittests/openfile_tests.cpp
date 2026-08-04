#include <gtest/gtest.h>
#include <textparser.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#ifndef TEXTPARSER_TEST_TMP_DIR
#define TEXTPARSER_TEST_TMP_DIR "."
#endif

namespace {

std::string write_temp_file(const std::string &name, const std::vector<char> &content) {
    std::filesystem::path dir(TEXTPARSER_TEST_TMP_DIR);
    std::filesystem::create_directories(dir);

    std::string path = (dir / name).string();
    FILE *f = fopen(path.c_str(), "wb");
    if (!f) {
        return "";
    }
    if (!content.empty()) {
        fwrite(content.data(), 1, content.size(), f);
    }
    fclose(f);
    return path;
}

std::vector<char> to_bytes(const std::string &s) {
    return std::vector<char>(s.begin(), s.end());
}

const std::vector<char> UTF8_BOM   = { (char)0xEF, (char)0xBB, (char)0xBF };
const std::vector<char> UTF16LE_BOM = { (char)0xFF, (char)0xFE };
const std::vector<char> UTF16BE_BOM = { (char)0xFE, (char)0xFF };
const std::vector<char> UTF32LE_BOM = { (char)0xFF, (char)0xFE, 0x00, 0x00 };
const std::vector<char> UTF7_BOM    = { (char)0x2B, (char)0x2F, (char)0x76, (char)0x38 };

std::vector<char> concat(const std::vector<char> &a, const std::vector<char> &b) {
    std::vector<char> out(a);
    out.insert(out.end(), b.begin(), b.end());
    return out;
}

void expect_text(textparser_t handle, const std::vector<char> &expected) {
    ASSERT_NE(handle, nullptr);
    const char *text = textparser_get_text(handle);
    size_t size = textparser_get_text_size(handle);
    ASSERT_EQ(size, expected.size());
    if (!expected.empty()) {
        ASSERT_NE(text, nullptr);
        EXPECT_EQ(memcmp(text, expected.data(), expected.size()), 0);
    }
}

} // namespace

TEST(openfile_bom_mask, utf8_bom_detected_when_in_mask) {
    std::string path = write_temp_file("bom_utf8_in_mask.txt", concat(UTF8_BOM, to_bytes("hello")));
    ASSERT_FALSE(path.empty());

    textparser_t handle = nullptr;
    int err = textparser_openfile(path.c_str(), TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_BOM_UTF_8, &handle);
    ASSERT_EQ(err, 0);
    expect_text(handle, to_bytes("hello"));
    textparser_close(handle);
}

TEST(openfile_bom_mask, utf8_bom_not_detected_when_mask_zero) {
    std::string path = write_temp_file("bom_utf8_mask_zero.txt", concat(UTF8_BOM, to_bytes("hello")));
    ASSERT_FALSE(path.empty());

    textparser_t handle = nullptr;
    int err = textparser_openfile(path.c_str(), TEXTPARSER_ENCODING_LATIN1, 0, &handle);
    ASSERT_EQ(err, 0);
    expect_text(handle, concat(UTF8_BOM, to_bytes("hello")));
    textparser_close(handle);
}

TEST(openfile_bom_mask, utf8_bom_not_detected_when_not_in_mask) {
    std::string path = write_temp_file("bom_utf8_not_in_mask.txt", concat(UTF8_BOM, to_bytes("hello")));
    ASSERT_FALSE(path.empty());

    textparser_t handle = nullptr;
    int err = textparser_openfile(path.c_str(), TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_BOM_UTF_16_LE, &handle);
    ASSERT_EQ(err, 0);
    expect_text(handle, concat(UTF8_BOM, to_bytes("hello")));
    textparser_close(handle);
}

TEST(openfile_bom_mask, utf8_bom_detected_when_all_mask) {
    std::string path = write_temp_file("bom_utf8_all_mask.txt", concat(UTF8_BOM, to_bytes("hello")));
    ASSERT_FALSE(path.empty());

    textparser_t handle = nullptr;
    int err = textparser_openfile(path.c_str(), TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_BOM_ALL, &handle);
    ASSERT_EQ(err, 0);
    expect_text(handle, to_bytes("hello"));
    textparser_close(handle);
}

TEST(openfile_bom_mask, utf16le_bom_detected_when_in_mask) {
    std::vector<char> content = concat(UTF16LE_BOM, std::vector<char>({ 'A', 0x00, 'B', 0x00 }));
    std::string path = write_temp_file("bom_utf16le_in_mask.txt", content);
    ASSERT_FALSE(path.empty());

    textparser_t handle = nullptr;
    int err = textparser_openfile(path.c_str(), TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_BOM_UTF_16_LE, &handle);
    ASSERT_EQ(err, 0);
    expect_text(handle, std::vector<char>({ 'A', 0x00, 'B', 0x00 }));
    textparser_close(handle);
}

TEST(openfile_bom_mask, utf32le_bom_detected_when_in_mask) {
    std::vector<char> content = concat(UTF32LE_BOM, std::vector<char>({ 'A', 0x00, 0x00, 0x00 }));
    std::string path = write_temp_file("bom_utf32le_in_mask.txt", content);
    ASSERT_FALSE(path.empty());

    textparser_t handle = nullptr;
    int err = textparser_openfile(path.c_str(), TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_BOM_UTF_32_LE, &handle);
    ASSERT_EQ(err, 0);
    expect_text(handle, std::vector<char>({ 'A', 0x00, 0x00, 0x00 }));
    textparser_close(handle);
}

TEST(openfile_bom_mask, utf32le_bom_with_only_utf16le_mask_strips_two_bytes) {
    std::vector<char> content = concat(UTF32LE_BOM, std::vector<char>({ 'A', 0x00, 0x00, 0x00 }));
    std::string path = write_temp_file("bom_utf32le_utf16le_mask.txt", content);
    ASSERT_FALSE(path.empty());

    textparser_t handle = nullptr;
    int err = textparser_openfile(path.c_str(), TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_BOM_UTF_16_LE, &handle);
    ASSERT_EQ(err, 0);
    expect_text(handle, std::vector<char>({ 0x00, 0x00, 'A', 0x00, 0x00, 0x00 }));
    textparser_close(handle);
}

TEST(openfile_bom_mask, unsupported_bom_in_mask_returns_error) {
    std::vector<char> content = concat(UTF16BE_BOM, std::vector<char>({ 'A', 0x00 }));
    std::string path = write_temp_file("bom_utf16be_in_mask.txt", content);
    ASSERT_FALSE(path.empty());

    textparser_t handle = nullptr;
    int err = textparser_openfile(path.c_str(), TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_BOM_UTF_16_BE, &handle);
    EXPECT_EQ(err, 5);
    textparser_close(handle);
}

TEST(openfile_bom_mask, utf7_bom_in_mask_returns_error) {
    std::string path = write_temp_file("bom_utf7_in_mask.txt", concat(UTF7_BOM, to_bytes("hello")));
    ASSERT_FALSE(path.empty());

    textparser_t handle = nullptr;
    int err = textparser_openfile(path.c_str(), TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_BOM_UTF_7_1, &handle);
    EXPECT_EQ(err, 5);
    textparser_close(handle);
}

TEST(openfile_bom_mask, unsupported_bom_ignored_when_mask_zero) {
    std::vector<char> content = concat(UTF16BE_BOM, std::vector<char>({ 'A', 0x00 }));
    std::string path = write_temp_file("bom_utf16be_mask_zero.txt", content);
    ASSERT_FALSE(path.empty());

    textparser_t handle = nullptr;
    int err = textparser_openfile(path.c_str(), TEXTPARSER_ENCODING_LATIN1, 0, &handle);
    ASSERT_EQ(err, 0);
    expect_text(handle, content);
    textparser_close(handle);
}

TEST(openfile_bom_mask, unsupported_bom_ignored_when_mask_has_other_boms) {
    std::vector<char> content = concat(UTF16BE_BOM, std::vector<char>({ 'A', 0x00 }));
    std::string path = write_temp_file("bom_utf16be_other_mask.txt", content);
    ASSERT_FALSE(path.empty());

    textparser_t handle = nullptr;
    int err = textparser_openfile(path.c_str(), TEXTPARSER_ENCODING_LATIN1, TEXTPARSER_BOM_UTF_8, &handle);
    ASSERT_EQ(err, 0);
    expect_text(handle, content);
    textparser_close(handle);
}

TEST(openfile_bom_mask, plain_text_no_bom) {
    std::string path = write_temp_file("plain_text.txt", to_bytes("hello"));
    ASSERT_FALSE(path.empty());

    textparser_t handle = nullptr;
    int err = textparser_openfile(path.c_str(), TEXTPARSER_ENCODING_LATIN1, 0, &handle);
    ASSERT_EQ(err, 0);
    expect_text(handle, to_bytes("hello"));
    textparser_close(handle);
}

TEST(openfile_bom_mask, empty_file_no_bom) {
    std::string path = write_temp_file("empty_file.txt", {});
    ASSERT_FALSE(path.empty());

    textparser_t handle = nullptr;
    int err = textparser_openfile(path.c_str(), TEXTPARSER_ENCODING_LATIN1, 0, &handle);
    ASSERT_EQ(err, 0);
    expect_text(handle, {});
    textparser_close(handle);
}
