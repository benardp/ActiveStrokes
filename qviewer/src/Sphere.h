/*****************************************************************************\

Sphere.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
Copyright (c) 2012 Pierre Benard

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _SPHERE_H
#define _SPHERE_H

#include <vector>

#include "GQVertexBufferSet.h"
#include "GQShaderManager.h"
#include "Vec.h"

class Sphere {

public:
  Sphere(float radius=1.f, int nU=40, int nV=40);
  
  void draw(GQShaderRef& shader);

  float radius() const { return _radius; }

protected:
  void setupVertexBufferSet();

private :
  GQVertexBufferSet		_vertex_buffer_set;

  std::vector<int>	    _triangles;  /**< vertex indices */
  std::vector<vec>		_vertices;   /**< 3D positions */
  std::vector<vec>		_colors;     /**< colors */
  std::vector<vec>		_normalsV;   /**< 3D normals */
  std::vector<vec>		_tangentsV;  /**< 3D tangent to surface */
  std::vector<vec2>		_texCoordsV; /**< 2D texture coordinates */

  float _radius;
};

#endif
