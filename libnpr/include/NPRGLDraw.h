/*****************************************************************************\

NPRGLDraw.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Static utility functions for OpenGL drawing.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef NPR_GL_DRAW_H_
#define NPR_GL_DRAW_H_

#include <QList>
#include "NPRGeometry.h"

class NPRStyle;
class GQTexture2D;
class GQTexture3D;
class GQFramebufferObject;
class GQShaderRef;

const int NPR_OPAQUE = 0x1;
const int NPR_TRANSLUCENT = 0x2;
const int NPR_DRAW_POLYGONS = 0x4;
const int NPR_DRAW_LINES = 0x8;
const int NPR_DRAW_PROFILES = 0x10;
const int NPR_DRAW_ALL_POLYGONS = NPR_DRAW_POLYGONS | NPR_OPAQUE | NPR_TRANSLUCENT;
const int NPR_DRAW_EVERYTHING = NPR_DRAW_POLYGONS | NPR_DRAW_LINES | NPR_OPAQUE | NPR_TRANSLUCENT;

const int NPR_SUCCESS = 1;
const int NPR_FAILURE = 0;

class NPRGLDraw
{
public:

    static void clearGLState();
    static void clearGLScreen(const vec& color, float depth);

    static void setUniformViewParams(const GQShaderRef& shader);

    static void handleGLError(const char* file = 0, int line = 0);

	static void drawFullScreenQuad( int texture_mode );
	static void visualizeFBO(const GQFramebufferObject& fbo, int which);
	static void visualizeTexture(const GQTexture* texture);

protected:
    static int _cur_index;
};

#endif /*NPR_GL_DRAW_H_*/
