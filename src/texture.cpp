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
  Color colors[3] = {Color(1, 0, 0, 1), Color(0, 1, 0, 1), Color(0, 0, 1, 1)};
  for (size_t i = 1; i < tex.mipmap.size(); ++i) {

    Color c = colors[i % 3];
    MipLevel &mip = tex.mipmap[i];

    for (size_t i = 0; i < 4 * mip.width * mip.height; i += 4) {
      float_to_uint8(&mip.texels[i], &c.r);
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
  u *= float(tex.width);
  v *= float(tex.height);
  if (0 > u || u >= tex.width || 0 > v || v >= tex.height)
    return Color(1, 0, 1, 1);
  int tu = floor(u - 0.5);
  int tv = floor(v - 0.5);
  auto cTL = tex.valid(tu, tv) ? tex.mipmap[0].color(tu, tv) : Color::Black;
  auto cTR = tex.valid(tu, tv + 1) ? tex.mipmap[0].color(tu, tv + 1) : Color::Black;
  auto cBL = tex.valid(tu + 1, tv) ? tex.mipmap[0].color(tu + 1, tv) : Color::Black;
  auto cBR = tex.valid(tu + 1, tv + 1) ? tex.mipmap[0].color(tu + 1, tv + 1) : Color::Black;
  float s = u - float(tu) - 0.5f;
  float t = v - float(tv) - 0.5f;
  return (cTL * (1 - t) + cTR * t) * (1 - s) + (cBL * (1 - t) + cBR * t) * s;

}

Color Sampler2DImp::sample_trilinear(Texture &tex,
                                     float u, float v,
                                     float u_scale, float v_scale) {

  // Task 7: Implement trilinear filtering

  // return magenta for invalid level
  return Color(1, 0, 1, 1);

}

Color MipLevel::color(size_t tu, size_t tv) {
  size_t base = 4 * (tu * width + tv);
  return Color(
      texels[base] / 255.0f,
      texels[base + 1] / 255.0f,
      texels[base + 2] / 255.0f,
      texels[base + 3] / 255.0f
  );
}

} // namespace CMU462
