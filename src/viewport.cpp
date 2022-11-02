#include "viewport.h"

#include "CMU462.h"

namespace CMU462 {

void ViewportImp::set_viewbox(float centerX, float centerY, float vspan) {

  // Task 5 (part 2): 
  // Set svg coordinate to normalized device coordinate transformation. Your input
  // arguments are defined as normalized SVG canvas coordinates.
  this->centerX = centerX;
  this->centerY = centerY;
  this->vspan = vspan;
  Matrix3x3 svg2norm;
  svg2norm(0, 0) = 1 / vspan / 2;
  svg2norm(0, 1) = 0;
  svg2norm(0, 2) = (vspan - centerX) / vspan / 2;
  svg2norm(1, 0) = 0;
  svg2norm(1, 1) = 1 / vspan / 2;
  svg2norm(1, 2) = (vspan - centerY) / vspan / 2;
  svg2norm(2, 0) = 0;
  svg2norm(2, 1) = 0;
  svg2norm(2, 2) = 1;
  set_svg_2_norm(svg2norm);
}

void ViewportImp::update_viewbox(float dx, float dy, float scale) {

  this->centerX -= dx;
  this->centerY -= dy;
  this->vspan *= scale;
  set_viewbox(centerX, centerY, vspan);
}

} // namespace CMU462
