#include "texture.h"
#include "color.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CMU462 {

inline void uint8_to_float(float dst[4], unsigned char *src) {
  uint8_t *src_uint8 = (uint8_t *) src;
  dst[0] = src_uint8[0] / 255.f;
  dst[1] = src_uint8[1] / 255.f;
  dst[2] = src_uint8[2] / 255.f;
  dst[3] = src_uint8[3] / 255.f;
}

inline void float_to_uint8(unsigned char *dst, float src[4]) {
  uint8_t *dst_uint8 = (uint8_t *) dst;
  dst_uint8[0] = (uint8_t) (255.f * max(0.0f, min(1.0f, src[0])));
  dst_uint8[1] = (uint8_t) (255.f * max(0.0f, min(1.0f, src[1])));
  dst_uint8[2] = (uint8_t) (255.f * max(0.0f, min(1.0f, src[2])));
  dst_uint8[3] = (uint8_t) (255.f * max(0.0f, min(1.0f, src[3])));
}

void Sampler2DImp::generate_mips(Texture &tex, int startLevel) {

  // NOTE: 
  // This starter code allocates the mip levels and generates a level 
  // map by filling each level with placeholder data in the form of a 
  // color that differs from its neighbours'. You should instead fill
  // with the correct data!

  // Task 7: Implement this

  // check start level
  if (startLevel >= tex.mipmap.size()) {
    std::cerr << "Invalid start level";
  }

  // allocate sublevels
  int baseWidth = tex.mipmap[startLevel].width;
  int baseHeight = tex.mipmap[startLevel].height;
  int numSubLevels = (int) (log2f((float) max(baseWidth, baseHeight)));

  numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
  tex.mipmap.resize(startLevel + numSubLevels + 1);

  int width = baseWidth;
  int height = baseHeight;
  for (int i = 1; i <= numSubLevels; i++) {

    MipLevel &level = tex.mipmap[startLevel + i];

    // handle odd size texture by rounding down
    width = max(1, width / 2);
    assert(width > 0);
    height = max(1, height / 2);
    assert(height > 0);

    level.width = width;
    level.height = height;
    level.texels = vector<unsigned char>(4 * width * height);

  }

  // fill all 0 sub levels with interchanging colors (JUST AS A PLACEHOLDER)
  for (size_t i = 1; i < tex.mipmap.size(); ++i) {

    MipLevel &mip = tex.mipmap[i];
    auto &last = tex.mipmap[i - 1];

    for (size_t tv = 0; tv < mip.height; tv++)
      for (size_t tu = 0; tu < mip.width; tu++) {
        auto c = (last.color(tu * 2, tv * 2) + last.color(tu * 2 + 1, tv * 2)
            + last.color(tu * 2, tv * 2 + 1) + last.color(tu * 2 + 1, tv * 2 + 1)) * 0.25f;
        c.render(&mip.texels[(tv * mip.width + tu) * 4]);
      }
  }

}

Color Sampler2DImp::sample_nearest(Texture &tex,
                                   float u, float v,
                                   int level) {

  // Task 6: Implement nearest neighbour interpolation
  int tu = floor(u * float(tex.width));
  int tv = floor(v * float(tex.height));

  // return magenta for invalid level
  if (0 > tu || tu >= tex.width || 0 > tv || tv >= tex.height)
    return Color(1, 0, 1, 1);

  size_t base = 4 * (tu * tex.width + tv);
  return Color(
      tex.mipmap[0].texels[base] / 255.0f,
      tex.mipmap[0].texels[base + 1] / 255.0f,
      tex.mipmap[0].texels[base + 2] / 255.0f,
      tex.mipmap[0].texels[base + 3] / 255.0f
  );

}

Color Sampler2DImp::sample_bilinear(Texture &tex,
                                    float u, float v,
                                    int level) {

  // Task 6: Implement bilinear filtering
  auto mipmap = tex.mipmap[level];
  u *= float(mipmap.width);
  v *= float(mipmap.height);
  if (0 > u || u >= mipmap.width || 0 > v || v >= mipmap.height)
    return Color(1, 0, 1, 1);
  int tu = floor(u - 0.5);
  int tv = floor(v - 0.5);
  auto cTL = mipmap.valid(tu, tv) ? mipmap.color(tu, tv) : Color::Black;
  auto cTR = mipmap.valid(tu, tv + 1) ? mipmap.color(tu, tv + 1) : Color::Black;
  auto cBL = mipmap.valid(tu + 1, tv) ? mipmap.color(tu + 1, tv) : Color::Black;
  auto cBR = mipmap.valid(tu + 1, tv + 1) ? mipmap.color(tu + 1, tv + 1) : Color::Black;
  float s = u - float(tu) - 0.5f;
  float t = v - float(tv) - 0.5f;
  return (cTL * (1 - t) + cTR * t) * (1 - s) + (cBL * (1 - t) + cBR * t) * s;

}

Color Sampler2DImp::sample_trilinear(Texture &tex,
                                     float u, float v,
                                     float u_scale, float v_scale) {

  // Task 7: Implement trilinear filtering
  float level = -log2(u_scale);
  if (level <= 0) return sample_bilinear(tex, u, v, 0);
  if (level >= tex.mipmap.size() - 1) return sample_bilinear(tex, u, v, tex.mipmap.size() - 1);
  int curr_level = int(floor(level));
  int next_level = curr_level + 1;
  float r = level - float(curr_level);
  auto curr_color = sample_bilinear(tex, u, v, curr_level);
  auto next_color = sample_bilinear(tex, u, v, next_level);
  return curr_color * (1 - r) + next_color * r;

}

Color MipLevel::color(size_t tu, size_t tv) {
  size_t base = 4 * (tv * width + tu);
  return Color(
      texels[base] / 255.0f,
      texels[base + 1] / 255.0f,
      texels[base + 2] / 255.0f,
      texels[base + 3] / 255.0f
  );
}

} // namespace CMU462
