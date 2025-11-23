#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace SimpleBase64 {

    static const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                      "abcdefghijklmnopqrstuvwxyz"
                                      "0123456789+/";

    // 编码
    inline std::string encode(const std::uint8_t* data, std::size_t len) {
        std::string ret;
        ret.reserve((len + 2) / 3 * 4);
        int val = 0, valb = -6;
        for (std::size_t i = 0; i < len; ++i) {
            val = (val << 8) + data[i];
            valb += 8;
            while (valb >= 0) {
                ret.push_back(base64_chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6)
            ret.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
        while (ret.size() % 4)
            ret.push_back('=');
        return ret;
    }

    inline std::string encode(const std::vector<std::uint8_t>& data) { return encode(data.data(), data.size()); }

    // 解码
    inline std::vector<std::uint8_t> decode(const std::string& str) {
        std::vector<std::uint8_t> ret;
        std::vector<int> T(256, -1);
        for (int i = 0; i < 64; i++)
            T[static_cast<unsigned char>(base64_chars[i])] = i;

        int val = 0, valb = -8;
        for (unsigned char c : str) {
            if (T[c] == -1) {
                if (c == '=')
                    break; // padding
                else
                    continue; // skip invalid chars
            }
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                ret.push_back(std::uint8_t((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return ret;
    }

} // namespace SimpleBase64
