/*****************************************************************************\

Sphere.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
Copyright (c) 2012 Pierre Benard

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "Sphere.h"

#include "GQDraw.h"
using namespace GQDraw;

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/** 
* Sphere constructor
* 
*/
Sphere::Sphere(float radius, int nU, int nV) : _radius(radius)
{
	int nVertices  = (nU + 1) * (nV + 1);
	int nTriangles =  nU * nV * 2;

	_vertices.resize(nVertices);
	_normalsV.resize(nVertices);
	_tangentsV.resize(nVertices);
	_texCoordsV.resize(nVertices);
	_colors.resize(nVertices);
	_triangles.resize(3*nTriangles);

	for(int v=0;v<=nV;++v)
	{
		for(int u=0;u<=nU;++u)
		{

			float theta = u / float(nU) * M_PI;
			float phi 	= v / float(nV) * M_PI * 2;

			int index 	= u +(nU+1)*v;

			vec  vertex;
			vec  tangent, normale;
			vec2 texCoord;

			// position
			vertex[0] = sin(theta) * cos(phi) * _radius;
			vertex[1] = sin(theta) * sin(phi) * _radius;
			vertex[2] = cos(theta) * _radius;

			// normal
			normale[0] = sin(theta) * cos(phi);
			normale[1] = sin(theta) * sin(phi);
			normale[2] = cos(theta);
			
			// tangent
			tangent = vec(-normale[1], normale[0], 0);
			tangent = normalize(tangent);

			// texture coordinates
			texCoord[1] = u / float(nU);
			texCoord[0] = v / float(nV);

			_vertices[index] = vertex;
			_normalsV[index] = normale;
			_tangentsV [index] = tangent;
			_texCoordsV [index] = texCoord;
			_colors[index] = vec(0.6f,0.2f,0.8f);
		}    
	}

	int index = 0;
	for(int v=0;v<nV;++v)
	{
		for(int u=0;u<nU;++u)
		{
			int vindex 	= u + (nU+1)*v;

			_triangles[index+0] = vindex;
			_triangles[index+1] = vindex+1 + (nU+1);
			_triangles[index+2] = vindex+1;

			_triangles[index+3] = vindex;
			_triangles[index+4] = vindex  + (nU+1);
			_triangles[index+5] = vindex+1 + (nU+1);

			index += 6;
		}
	}
}

void Sphere::setupVertexBufferSet()
{
	_vertex_buffer_set.clear();
	_vertex_buffer_set.add(GQ_VERTEX,   _vertices);
	_vertex_buffer_set.add(GQ_NORMAL,   _normalsV);
	_vertex_buffer_set.add(GQ_COLOR,	_colors);
	_vertex_buffer_set.add(GQ_TEXCOORD, _texCoordsV);
	_vertex_buffer_set.add(GQ_TANGENT,  _tangentsV);
	_vertex_buffer_set.add(GQ_INDEX, 1, _triangles);

	_vertex_buffer_set.copyToVBOs();
}

void Sphere::draw(GQShaderRef& shader)
{
    if (_vertex_buffer_set.numBuffers() == 0)
		setupVertexBufferSet();

	_vertex_buffer_set.bind(shader);

	drawElements(_vertex_buffer_set, GL_TRIANGLES, 0, _triangles.size()); 
	
    _vertex_buffer_set.unbind();
}
