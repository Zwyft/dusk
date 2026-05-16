#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>

// GX tiled texture decoder — adapted from aurora::gfx::convert_texture
// Correctly handles Nintendo Wii/GC tile-block encoding

struct RGBA8 { uint8_t r, g, b, a; };

static size_t compute_mipped_texel_count(uint32_t w, uint32_t h, uint32_t mips) {
    size_t ret = w * h;
    for (uint32_t i = mips; i > 1; --i) {
        if (w > 1) w /= 2;
        if (h > 1) h /= 2;
        ret += w * h;
    }
    return ret;
}

template <uint8_t v>
static constexpr uint8_t ExpandTo8(uint8_t n) {
    if constexpr (v == 3) return (n << 5) | (n << 2) | (n >> 1);
    else return (n << (8 - v)) | (n >> ((v * 2) - 8));
}

static constexpr uint16_t bswap16_val(uint16_t v) {
    return (v >> 8) | (v << 8);
}

template <typename T>
concept TextureDecoder = requires(T) {
    typename T::Source;
    typename T::Target;
    { T::Frac } -> std::convertible_to<uint32_t>;
    { T::BlockWidth } -> std::convertible_to<uint32_t>;
    { T::BlockHeight } -> std::convertible_to<uint32_t>;
    { T::decode_texel(std::declval<typename T::Target*>(), std::declval<const typename T::Source*>(), 0u) };
};

template <TextureDecoder T>
static std::vector<uint8_t> DecodeTiled(uint32_t width, uint32_t height, const uint8_t* data) {
    const size_t texelCount = width * height;
    std::vector<uint8_t> buf(texelCount * sizeof(typename T::Target));
    auto* targetMip = reinterpret_cast<typename T::Target*>(buf.data());
    const auto* in = reinterpret_cast<const typename T::Source*>(data);

    const uint32_t bwidth = (width + (T::BlockWidth - 1)) / T::BlockWidth;
    const uint32_t bheight = (height + (T::BlockHeight - 1)) / T::BlockHeight;
    for (uint32_t by = 0; by < bheight; ++by) {
        const uint32_t baseY = by * T::BlockHeight;
        const uint32_t numRows = std::min(height - baseY, T::BlockHeight);
        for (uint32_t bx = 0; bx < bwidth; ++bx) {
            const uint32_t baseX = bx * T::BlockWidth;
            for (uint32_t y = 0; y < numRows; ++y) {
                auto* target = targetMip + (baseY + y) * width + baseX;
                const auto n = std::min(width - baseX, T::BlockWidth);
                for (uint32_t x = 0; x < n; ++x)
                    T::decode_texel(target, in, x);
                in += T::BlockWidth / T::Frac;
            }
            const uint32_t extraY = T::BlockHeight - numRows;
            in += T::BlockWidth * extraY / T::Frac;
        }
    }
    return buf;
}

// --- I4 ---
struct DecoderI4 {
    using Source = uint8_t; using Target = RGBA8;
    static constexpr uint32_t Frac = 2, BlockWidth = 8, BlockHeight = 8;
    static void decode_texel(Target* target, const Source* in, uint32_t x) {
        uint8_t i = ExpandTo8<4>(in[x / 2] >> (x & 1 ? 0 : 4) & 0xf);
        target[x] = {i, i, i, i};
    }
};

// --- IA4 ---
struct DecoderIA4 {
    using Source = uint8_t; using Target = RGBA8;
    static constexpr uint32_t Frac = 1, BlockWidth = 8, BlockHeight = 4;
    static void decode_texel(Target* target, const Source* in, uint32_t x) {
        uint8_t i = ExpandTo8<4>(in[x] & 0xf);
        target[x] = {i, i, i, ExpandTo8<4>(in[x] >> 4)};
    }
};

// --- IA8 ---
struct DecoderIA8 {
    using Source = uint16_t; using Target = RGBA8;
    static constexpr uint32_t Frac = 1, BlockWidth = 4, BlockHeight = 4;
    static void decode_texel(Target* target, const Source* in, uint32_t x) {
        uint16_t v = bswap16_val(in[x]);
        target[x] = {uint8_t(v >> 8), uint8_t(v >> 8), uint8_t(v >> 8), uint8_t(v & 0xff)};
    }
};

// --- RGB565 ---
struct DecoderRGB565 {
    using Source = uint16_t; using Target = RGBA8;
    static constexpr uint32_t Frac = 1, BlockWidth = 4, BlockHeight = 4;
    static void decode_texel(Target* target, const Source* in, uint32_t x) {
        uint16_t v = bswap16_val(in[x]);
        target[x] = {ExpandTo8<5>(uint8_t((v >> 11) & 0x1f)),
                     ExpandTo8<6>(uint8_t((v >> 5) & 0x3f)),
                     ExpandTo8<5>(uint8_t(v & 0x1f)), 0xff};
    }
};

// --- RGB5A3 ---
struct DecoderRGB5A3 {
    using Source = uint16_t; using Target = RGBA8;
    static constexpr uint32_t Frac = 1, BlockWidth = 4, BlockHeight = 4;
    static void decode_texel(Target* target, const Source* in, uint32_t x) {
        uint16_t v = bswap16_val(in[x]);
        if (v & 0x8000) {
            target[x] = {ExpandTo8<5>(uint8_t((v >> 10) & 0x1f)),
                         ExpandTo8<5>(uint8_t((v >> 5) & 0x1f)),
                         ExpandTo8<5>(uint8_t(v & 0x1f)), 0xff};
        } else {
            target[x] = {ExpandTo8<4>(uint8_t((v >> 8) & 0xf)),
                         ExpandTo8<4>(uint8_t((v >> 4) & 0xf)),
                         ExpandTo8<4>(uint8_t(v & 0xf)),
                         ExpandTo8<3>(uint8_t((v >> 12) & 0x7))};
        }
    }
};

// --- RGBA8 (GCN) ---
static std::vector<uint8_t> BuildRGBA8FromGCN(uint32_t width, uint32_t height, const uint8_t* data) {
    std::vector<uint8_t> buf(width * height * 4, 0);
    auto* targetMip = reinterpret_cast<RGBA8*>(buf.data());
    const uint8_t* in = data;
    const uint32_t bwidth = (width + 3) / 4;
    const uint32_t bheight = (height + 3) / 4;
    for (uint32_t by = 0; by < bheight; ++by) {
        const uint32_t baseY = by * 4;
        for (uint32_t bx = 0; bx < bwidth; ++bx) {
            const uint32_t baseX = bx * 4;
            for (uint32_t c = 0; c < 2; ++c) {
                for (uint32_t y = 0; y < 4 && baseY + y < height; ++y) {
                    RGBA8* target = targetMip + (baseY + y) * width + baseX;
                    for (uint32_t x = 0; x < 4 && baseX + x < width; ++x) {
                        if (c != 0) { target[x].g = in[x * 2]; target[x].b = in[x * 2 + 1]; }
                        else        { target[x].a = in[x * 2]; target[x].r = in[x * 2 + 1]; }
                    }
                    in += 8;
                }
            }
        }
    }
    return buf;
}

// --- Main decode entry point ---
static std::vector<uint8_t> decode_gx_to_rgba(uint8_t format, uint32_t w, uint32_t h, const uint8_t* data) {
    switch (format) {
        case 0: /* GX_TF_I4 */      return DecodeTiled<DecoderI4>(w, h, data);
        case 2: /* GX_TF_IA4 */     return DecodeTiled<DecoderIA4>(w, h, data);
        case 3: /* GX_TF_IA8 */     return DecodeTiled<DecoderIA8>(w, h, data);
        case 4: /* GX_TF_RGB565 */  return DecodeTiled<DecoderRGB565>(w, h, data);
        case 5: /* GX_TF_RGB5A3 */  return DecodeTiled<DecoderRGB5A3>(w, h, data);
        case 6: /* GX_TF_RGBA8 */   return BuildRGBA8FromGCN(w, h, data);
        default: return {};
    }
}
