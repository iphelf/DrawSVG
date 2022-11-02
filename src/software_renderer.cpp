#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

#include <cassert>
#define ASSERT_ENABLED
#ifdef ASSERT_ENABLED
#define ASSERT(expr) assert(expr)
#endif
#ifndef ASSERT_ENABLED
#define ASSERT(expr)
#endif

using namespace std;

namespace CMU462 {


// Implements SoftwareRenderer //

void SoftwareRendererImp::draw_svg(SVG &svg) {

  // set top level transformation
  transformation = svg_2_screen;

  // draw all elements
  for (size_t i = 0; i < svg.elements.size(); ++i) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(0, 0));
  a.x--;
  a.y--;
  Vector2D b = transform(Vector2D(svg.width, 0));
  b.x++;
  b.y--;
  Vector2D c = transform(Vector2D(0, svg.height));
  c.x--;
  c.y++;
  Vector2D d = transform(Vector2D(svg.width, svg.height));
  d.x++;
  d.y++;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  resolve();

}

void SoftwareRendererImp::set_sample_rate(size_t sample_rate) {

  // Task 4: 
  // You may want to modify this for supersampling support
  this->sample_rate = sample_rate;
  update_sample_buffer();

}

void SoftwareRendererImp::set_render_target(unsigned char *render_target,
                                            size_t width, size_t height) {

  // Task 4: 
  // You may want to modify this for supersampling support
  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;
  update_sample_buffer();

}

void SoftwareRendererImp::update_sample_buffer() {
  if (!this->render_target) return;
  this->sample_h = this->target_h * this->sample_rate;
  this->sample_w = this->target_w * this->sample_rate;
  this->sample_buffer.assign(this->sample_h * this->sample_w * 4, 255);
}

void SoftwareRendererImp::draw_element(SVGElement *element) {

  // Task 5 (part 1):
  // Modify this to implement the transformation stack

  switch (element->type) {
    case POINT:draw_point(static_cast<Point &>(*element));
      break;
    case LINE:draw_line(static_cast<Line &>(*element));
      break;
    case POLYLINE:draw_polyline(static_cast<Polyline &>(*element));
      break;
    case RECT:draw_rect(static_cast<Rect &>(*element));
      break;
    case POLYGON:draw_polygon(static_cast<Polygon &>(*element));
      break;
    case ELLIPSE:draw_ellipse(static_cast<Ellipse &>(*element));
      break;
    case IMAGE:draw_image(static_cast<Image &>(*element));
      break;
    case GROUP:draw_group(static_cast<Group &>(*element));
      break;
    default:break;
  }

}


// Primitive Drawing //

void SoftwareRendererImp::draw_point(Point &point) {

  Vector2D p = transform(point.position);
  rasterize_point(p.x, p.y, point.style.fillColor);

}

void SoftwareRendererImp::draw_line(Line &line) {

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line(p0.x, p0.y, p1.x, p1.y, line.style.strokeColor);

}

void SoftwareRendererImp::draw_polyline(Polyline &polyline) {

  Color c = polyline.style.strokeColor;

  if (c.a != 0) {
    int nPoints = polyline.points.size();
    for (int i = 0; i < nPoints - 1; i++) {
      Vector2D p0 = transform(polyline.points[(i + 0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i + 1) % nPoints]);
      rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
    }
  }
}

void SoftwareRendererImp::draw_rect(Rect &rect) {

  Color c;

  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(x, y));
  Vector2D p1 = transform(Vector2D(x + w, y));
  Vector2D p2 = transform(Vector2D(x, y + h));
  Vector2D p3 = transform(Vector2D(x + w, y + h));

  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0) {
    rasterize_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c);
    rasterize_triangle(p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c);
  }

  // draw outline
  c = rect.style.strokeColor;
  if (c.a != 0) {
    rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
    rasterize_line(p1.x, p1.y, p3.x, p3.y, c);
    rasterize_line(p3.x, p3.y, p2.x, p2.y, c);
    rasterize_line(p2.x, p2.y, p0.x, p0.y, c);
  }

}

void SoftwareRendererImp::draw_polygon(Polygon &polygon) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if (c.a != 0) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate(polygon, triangles);

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c);
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if (c.a != 0) {
    int nPoints = polygon.points.size();
    for (int i = 0; i < nPoints; i++) {
      Vector2D p0 = transform(polygon.points[(i + 0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i + 1) % nPoints]);
      rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
    }
  }
}

void SoftwareRendererImp::draw_ellipse(Ellipse &ellipse) {

  // Extra credit 

}

void SoftwareRendererImp::draw_image(Image &image) {

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image(p0.x, p0.y, p1.x, p1.y, image.tex);
}

void SoftwareRendererImp::draw_group(Group &group) {

  for (size_t i = 0; i < group.elements.size(); ++i) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point(float x, float y, Color color) {

  x *= float(sample_rate);
  y *= float(sample_rate);
  // fill in the nearest pixel
  int sx = (int) floor(x);
  int sy = (int) floor(y);

  // check bounds
  if (overflow(sx, sy)) return;

  // fill sample - NOT doing alpha blending!
  put_sample(sx, sy, color);

}

void SoftwareRendererImp::rasterize_line(float x0, float y0,
                                         float x1, float y1,
                                         const Color &color) {

  // Task 2:
  // Implement line rasterization
  x0 *= float(sample_rate);
  y0 *= float(sample_rate);
  x1 *= float(sample_rate);
  y1 *= float(sample_rate);
//  rasterize_line_DDA(x0, y0, x1, y1, color);
//  rasterize_line_midpoint(x0, y0, x1, y1, color);
  rasterize_line_bresenham(x0, y0, x1, y1, color);

}

void SoftwareRendererImp::rasterize_line_DDA(
    float x0, float y0, float x1, float y1, const Color &color) {
  if (slope_le_1(x0, y0, x1, y1)) {
    sort_by_x(x0, y0, x1, y1);
    float y = y0, k = (y1 - y0) / (x1 - x0);
    int sy;
    for (auto [sx, to] = truncated_x_range(x0, x1);
         sx <= to; y += k, ++sx) {
      sy = int(floor(y));
      if (!valid_sy(sy)) continue;
      put_sample(sx, sy, color);
    }
  } else {
    sort_by_y(x0, y0, x1, y1);
    float x = x0, k = (x1 - x0) / (y1 - y0);
    int sx;
    for (auto [sy, to] = truncated_y_range(y0, y1);
         sy <= to; x += k, ++sy) {
      sx = int(floor(x));
      if (!valid_sx(sx)) continue;
      put_sample(sx, sy, color);
    }
  }
}

void SoftwareRendererImp::rasterize_line_midpoint(
    float x0, float y0, float x1, float y1, const Color &color) {
  if (slope_le_1(x0, y0, x1, y1)) {
    sort_by_x(x0, y0, x1, y1);
    int a = i_floor(y0) - i_floor(y1), b = i_floor(x1) - i_floor(x0);
    auto [sx, to] = truncated_x_range(x0, x1);
    int sy = i_floor(y0);
    for (int a2 = 2 * a, b2 = 2 * b,
             d = a2 + b,
             a2pb2 = a2 + b2,
             a2mb2 = a2 - b2;
         sx <= to; ++sx) {
      if (valid_sy(sy)) put_sample(sx, sy, color);
      if ((d < 0) ^ (a <= 0))
        d += a2;
      else if (d < 0) {
        ++sy;
        d += a2pb2;
      } else {
        --sy;
        d += a2mb2;
      }
    }
  } else {
    sort_by_y(x0, y0, x1, y1);
    int a = i_floor(x0) - i_floor(x1), b = i_floor(y1) - i_floor(y0);
    auto [sy, to] = truncated_y_range(y0, y1);
    int sx = i_floor(x0);
    for (int a2 = 2 * a, b2 = 2 * b,
             d = a2 + b,
             a2pb2 = a2 + b2,
             a2mb2 = a2 - b2;
         sy <= to; ++sy) {
      if (valid_sx(sx)) put_sample(sx, sy, color);
      if ((d < 0) ^ (a <= 0))
        d += a2;
      else if (d < 0) {
        ++sx;
        d += a2pb2;
      } else {
        --sx;
        d += a2mb2;
      }
    }
  }
}

void SoftwareRendererImp::rasterize_line_bresenham(
    float x0, float y0, float x1, float y1, const Color &color) {
  if (slope_le_1(x0, y0, x1, y1)) {
    sort_by_x(x0, y0, x1, y1);
    int dx = i_floor(x1) - i_floor(x0), dy = i_floor(y1) - i_floor(y0);
    int e_dx2 = dy > 0 ? -dx : dx;
    auto [sx, to] = truncated_x_range(x0, x1);
    for (int sy = i_floor(y0), dy2 = 2 * dy, dx2 = 2 * dx; sx <= to; ++sx) {
      if (valid_sy(sy)) put_sample(sx, sy, color);
      e_dx2 += dy2;
      if (dy < 0 && e_dx2 < 0) {
        --sy;
        e_dx2 += dx2;
      } else if (dy > 0 && e_dx2 > 0) {
        ++sy;
        e_dx2 -= dx2;
      }
    }
  } else {
    sort_by_y(x0, y0, x1, y1);
    int dx = i_floor(x1) - i_floor(x0), dy = i_floor(y1) - i_floor(y0);
    int e_dy2 = dx > 0 ? -dy : dy;
    auto [sy, to] = truncated_y_range(y0, y1);
    for (int sx = i_floor(x0), dy2 = 2 * dy, dx2 = 2 * dx; sy <= to; ++sy) {
      if (valid_sx(sx)) put_sample(sx, sy, color);
      e_dy2 += dx2;
      if (dx < 0 && e_dy2 < 0) {
        --sx;
        e_dy2 += dy2;
      } else if (dx > 0 && e_dy2 > 0) {
        ++sx;
        e_dy2 -= dy2;
      }
    }
  }
}

void SoftwareRendererImp::rasterize_triangle(float x0, float y0,
                                             float x1, float y1,
                                             float x2, float y2,
                                             const Color &color) {
  // Task 3: 
  // Implement triangle rasterization

  x0 *= float(sample_rate);
  y0 *= float(sample_rate);
  x1 *= float(sample_rate);
  y1 *= float(sample_rate);
  x2 *= float(sample_rate);
  y2 *= float(sample_rate);
  // make sure (x0,y0) is the top
  if (y1 > y0 && y1 > y2) {
    swap(x0, x1);
    swap(y0, y1);
  } else if (y2 > y0 && y2 > y1) {
    swap(x0, x2);
    swap(y0, y2);
  }

  // make sure (x0,y0) is the only top
  if (y0 == y1) {
    rasterize_triangle(x2, y2, x0, x1, y0, color);
    return;
  } else if (y0 == y2) {
    rasterize_triangle(x1, y1, x0, x2, y0, color);
    return;
  }

  // make sure p0->p1->p2 is counter-clockwise
  if ((x1 == x2 && (x1 < x0) == (y1 < y2)) || x1 > x2) {
    swap(x1, x2);
    swap(y1, y2);
  }

  // horizontally cut into two parts - upper and lower
  if (y1 == y2) {
    rasterize_triangle(x0, y0, x1, x2, y1, color);
  } else if (y1 < y2) {
    float x = (x1 - x0) / (y1 - y0) * (y2 - y0) + x0;
    rasterize_triangle(x0, y0, x, x2, y2, color);
    rasterize_triangle(x1, y1, x, x2, y2, color);
  } else {
    float x = (x2 - x0) / (y2 - y0) * (y1 - y0) + x0;
    rasterize_triangle(x0, y0, x1, x, y1, color);
    rasterize_triangle(x2, y2, x1, x, y1, color);
  }
}

void SoftwareRendererImp::rasterize_triangle(
    float xTip, float yTip,
    float xBase0, float xBase1, float yBase,
    const Color &color
) {
  if (xBase0 > xBase1)
    swap(xBase0, xBase1);
  float kl = (xBase0 - xTip) / (yBase - yTip);
  float bl = xTip - kl * yTip;
  float kr = (xBase1 - xTip) / (yBase - yTip);
  float br = xTip - kr * yTip;
  if (yBase > yTip) swap(yBase, yTip);
  int y_from = max(0, i_floor(yBase + 0.5f));
  int y_to = min(int(sample_h) - 1, i_floor(yTip - 0.5f));
  int x_from, x_to;
  for (int sy = y_from, sx; sy <= y_to; ++sy) {
    x_from = max(0, i_floor(kl * (0.5f + float(sy)) + bl + 0.5f));
    x_to = min(int(sample_w) - 1, i_floor(kr * (0.5f + float(sy)) + br - 0.5f));
    for (sx = x_from; sx <= x_to; ++sx)
      put_sample(sx, sy, color);
  }
}

void SoftwareRendererImp::rasterize_image(float x0, float y0,
                                          float x1, float y1,
                                          Texture &tex) {
  // Task 6: 
  // Implement image rasterization

}

// resolve samples to render target
void SoftwareRendererImp::resolve() {

  // Task 4:
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 4".
  int r, g, b, a;
  size_t base;
  size_t sample_squared = sample_rate * sample_rate;
  for (int sy = 0; sy < target_h; ++sy)
    for (int sx = 0; sx < target_w; ++sx) {
      r = g = b = a = 0;
      for (int i = 0; i < sample_squared; ++i) {
        base = 4 * (
            (sy * sample_rate + i / sample_rate) * sample_w + (sx * sample_rate + i % sample_rate)
        );
        r += sample_buffer[base];
        g += sample_buffer[base + 1];
        b += sample_buffer[base + 2];
        a += sample_buffer[base + 3];
      }
      put_pixel(sx, sy, r / sample_squared, g / sample_squared, b / sample_squared, a / sample_squared);
    }

}

} // namespace CMU462
