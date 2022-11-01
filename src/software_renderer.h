#ifndef CMU462_SOFTWARE_RENDERER_H
#define CMU462_SOFTWARE_RENDERER_H

#include <stdio.h>
#include <vector>
#include <functional>

#include "CMU462.h"
#include "texture.h"
#include "svg_renderer.h"

namespace CMU462 { // CMU462

class SoftwareRenderer : public SVGRenderer {
 public:

  SoftwareRenderer() : sample_rate(1), render_target(nullptr) {}

  // Free used resources
  virtual ~SoftwareRenderer() {}

  // Draw an svg input to render target
  virtual void draw_svg(SVG &svg) = 0;

  // Set sample rate
  virtual void set_sample_rate(size_t sample_rate) = 0;

  // Set render target
  virtual void set_render_target(unsigned char *render_target,
                                 size_t width, size_t height) = 0;

  // Clear render target
  inline void clear_target() {
    memset(render_target, 255, 4 * target_w * target_h);
  }

  // Set texture sampler
  inline void set_tex_sampler(Sampler2D *sampler) {
    this->sampler = sampler;
  }

  // Set svg to screen transformation
  inline void set_svg_2_screen(Matrix3x3 svg_2_screen) {
    this->svg_2_screen = svg_2_screen;
  }

 protected:

  // Sample rate (square root of samples per pixel)
  size_t sample_rate;

  // Render target memory location
  unsigned char *render_target;

  // Target buffer dimension (in pixels)
  size_t target_w;
  size_t target_h;

  // Texture sampler being used
  Sampler2D *sampler;

  // SVG coordinates to screen space coordinates
  Matrix3x3 svg_2_screen;

}; // class SoftwareRenderer


class SoftwareRendererImp : public SoftwareRenderer {
 public:

  SoftwareRendererImp() : SoftwareRenderer() {
    update_sample_buffer();
  }

  // draw an svg input to render target
  void draw_svg(SVG &svg);

  // set sample rate
  void set_sample_rate(size_t sample_rate);

  // set render target
  void set_render_target(unsigned char *target_buffer,
                         size_t width, size_t height);

 private:

  // supersampling
  std::vector<u_int8_t> sample_buffer;
  size_t sample_w;
  size_t sample_h;
  void update_sample_buffer();

  // Primitive Drawing //

  // Draws an SVG element
  void draw_element(SVGElement *element);

  // Draws a point
  void draw_point(Point &p);

  // Draw a line
  void draw_line(Line &line);

  // Draw a polyline
  void draw_polyline(Polyline &polyline);

  // Draw a rectangle
  void draw_rect(Rect &rect);

  // Draw a polygon
  void draw_polygon(Polygon &polygon);

  // Draw a ellipse
  void draw_ellipse(Ellipse &ellipse);

  // Draws a bitmap image
  void draw_image(Image &image);

  // Draw a group
  void draw_group(Group &group);

  // Rasterization //

  // rasterize a point
  void rasterize_point(float x, float y, Color color);

  // rasterize a line
  void rasterize_line(float x0, float y0,
                      float x1, float y1,
                      const Color &color);

  void rasterize_line_DDA(float x0, float y0,
                          float x1, float y1,
                          const Color &color);

  void rasterize_line_midpoint(float x0, float y0,
                               float x1, float y1,
                               const Color &color);

  void rasterize_line_bresenham(float x0, float y0,
                                float x1, float y1,
                                const Color &color);

  // rasterize a triangle
  void rasterize_triangle(float x0, float y0,
                          float x1, float y1,
                          float x2, float y2,
                          const Color &color);

  // rasterize a triangle with horizontal base
  void rasterize_triangle(
      float xTip, float yTip,
      float xBase0, float xBase1, float yBase,
      const Color &color
  );

  // rasterize an image
  void rasterize_image(float x0, float y0,
                       float x1, float y1,
                       Texture &tex);

  // resolve samples to render target
  void resolve();

  // helpers
  static inline int i_floor(float f) {
    return int(std::floor(f));
  }

  static inline int i_ceil(float f) {
    return int(std::ceil(f));
  }

  [[nodiscard]] inline bool valid_sx(int sx) const {
    return 0 <= sx && sx < sample_w;
  }

  [[nodiscard]] inline bool valid_sy(int sy) const {
    return 0 <= sy && sy < sample_h;
  }

  inline bool overflow(int sx, int sy) {
    return !(valid_sx(sx) && valid_sy(sy));
  }

  // set pixel color
  inline void put_pixel(int sx, int sy, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    auto base = 4 * (sx + sy * target_w);
    render_target[base] = r;
    render_target[base + 1] = g;
    render_target[base + 2] = b;
    render_target[base + 3] = a;
  }

  inline void put_pixel(int sx, int sy, const Color &color) {
    put_pixel(
        sx, sy,
        static_cast<uint8_t>(color.r * 255),
        static_cast<uint8_t>(color.g * 255),
        static_cast<uint8_t>(color.b * 255),
        static_cast<uint8_t>(color.a * 255)
    );
  }

  inline void put_sample(int sx, int sy, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    auto base = 4 * (sx + sy * sample_w);
    sample_buffer[base] = r;
    sample_buffer[base + 1] = g;
    sample_buffer[base + 2] = b;
    sample_buffer[base + 3] = a;
  }

  inline void put_sample(int sx, int sy, const Color &color) {
    put_sample(
        sx, sy,
        static_cast<uint8_t>(color.r * 255),
        static_cast<uint8_t>(color.g * 255),
        static_cast<uint8_t>(color.b * 255),
        static_cast<uint8_t>(color.a * 255)
    );
  }

  // determine axis to move along
  static inline bool slope_le_1(float x0, float y0, float x1, float y1) {
    return std::abs(x1 - x0) > std::abs(y1 - y0);
  }

  static inline void sort_by_x(float &x0, float &y0, float &x1, float &y1) {
    if (x0 > x1) {
      std::swap(x0, x1);
      std::swap(y0, y1);
    }
  }

  static inline void sort_by_y(float &x0, float &y0, float &x1, float &y1) {
    if (y0 > y1) {
      std::swap(x0, x1);
      std::swap(y0, y1);
    }
  }

  [[nodiscard]] inline std::pair<int, int> truncated_x_range(float x0, float x1) const {
    return {
        std::max(0, i_floor(x0)),
        std::min(int(sample_w) - 1, i_floor(x1))
    };
  }

  [[nodiscard]] inline std::pair<int, int> truncated_y_range(float y0, float y1) const {
    return {
        std::max(0, i_floor(y0)),
        std::min(int(sample_h) - 1, i_floor(y1))
    };
  }

}; // class SoftwareRendererImp


class SoftwareRendererRef : public SoftwareRenderer {
 public:

  SoftwareRendererRef() : SoftwareRenderer() {}

  // draw an svg input to render target
  void draw_svg(SVG &svg);

  // set sample rate
  void set_sample_rate(size_t sample_rate);

  // set render target
  void set_render_target(unsigned char *target_buffer,
                         size_t width, size_t height);

 private:

  // Primitive Drawing //

  // Draws an SVG element
  void draw_element(SVGElement *element);

  // Draws a point
  void draw_point(Point &p);

  // Draw a line
  void draw_line(Line &line);

  // Draw a polyline
  void draw_polyline(Polyline &polyline);

  // Draw a rectangle
  void draw_rect(Rect &rect);

  // Draw a polygon
  void draw_polygon(Polygon &polygon);

  // Draw a ellipse
  void draw_ellipse(Ellipse &ellipse);

  // Draws a bitmap image
  void draw_image(Image &image);

  // Draw a group
  void draw_group(Group &group);

  // Rasterization //

  // rasterize a point
  void rasterize_point(float x, float y, Color color);

  // rasterize a line
  void rasterize_line(float x0, float y0,
                      float x1, float y1,
                      Color color);

  // rasterize a triangle
  void rasterize_triangle(float x0, float y0,
                          float x1, float y1,
                          float x2, float y2,
                          Color color);

  // rasterize an image
  void rasterize_image(float x0, float y0,
                       float x1, float y1,
                       Texture &tex);

  // resolve samples to render target
  void resolve(void);

  // Helpers //
  // HINT: you may want to have something similar //
  std::vector<unsigned char> sample_buffer;
  int w;
  int h;
  void fill_sample(int sx, int sy, const Color &c);
  void fill_pixel(int x, int y, const Color &c);

}; // class SoftwareRendererRef

} // namespace CMU462

#endif // CMU462_SOFTWARE_RENDERER_H
