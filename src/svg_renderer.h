#ifndef CMU462_SVG_RENDERER_H
#define CMU462_SVG_RENDERER_H

#include <stdio.h>

#include "CMU462.h"
#include "svg.h"
#include "viewport.h"
#include <iostream>
namespace CMU462 {

class SVGRenderer {
 public:

  SVGRenderer() : transformation(Matrix3x3::identity()) {}

  // Free used resources
  virtual ~SVGRenderer() {}

  // Draw an svg file
  virtual void draw_svg(SVG &svg) = 0;

  // Set viewport
  inline void set_viewport(Viewport *viewport) {
    this->viewport = viewport;
  }

 protected:

  // Viewport
  Viewport *viewport;

  // Projective space transformation
  Matrix3x3 transformation;

  // Transform object coordinate to screen coordinate
  inline Vector2D transform(const Vector2D &p) {
    return transform(transformation, p);
  }

  static inline Vector2D transform(const Matrix3x3 &t, const Vector2D &p) {
    // map point from 2D Euclidean plane to 3D projective space
    Vector3D u(p.x, p.y, 1.0);
    // apply projective space transformation
    u = t * u;
    // project back to 2D Euclidean plane
    return {u.x / u.z, u.y / u.z};
  }

};

} // namespace CMU462

#endif // CMU462_SVG_RENDERER_H
