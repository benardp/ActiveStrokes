/*****************************************************************************\

ASRenderer.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef ASRENDERER_H
#define ASRENDERER_H

#include "NPRPathRenderer.h"
#include "NPRStyle.h"

#include "ASSnakes.h"

class ASRenderer
{
public:
    ASRenderer();
    void init(ASSnakes* snakes);

    void draw() const;
    void drawClosestEdge() const;
    void drawClosestEdge3D(const GLdouble *model, const GLdouble *proj, const GLint *view) const;
    void drawBrushPaths(bool black) const;

    void renderStrokes();
    void drawSpine();

protected:
    bool makePathVertexFBO();
    void updateStyle();

    ASSnakes* _snakes;

    typedef enum {
        BRUSHPATH_VERTEX_0_ID,
        BRUSHPATH_VERTEX_1_ID,
        BRUSHPATH_START_END_ID,
        BRUSHPATH_OFFSET_ID,
        BRUSHPATH_PARAM_ID,
        NUM_BRUSHPATH_BUFFERS
    } BrushPathBufferId;

    NPRPathRenderer     _pathRenderer;
    NPRStyle            _globalStyle;
    GQFramebufferObject _path_verts_fbo;
    int   _clip_buf_width;
    int   _total_segments;

    GQVertexBufferSet  _vertex_buffer_set;
    bool _vbo_initialized;
};

#endif // ASRENDERER_H
