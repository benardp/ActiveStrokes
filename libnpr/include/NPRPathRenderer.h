/*****************************************************************************\

NPRPathRenderer.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Stroke rendering functions for the various rendering methods supported.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef NPR_PATH_RENDERER_H_
#define NPR_PATH_RENDERER_H_

#include "GQImage.h"
#include "GQTexture.h"
#include "GQVertexBufferSet.h"
#include "GQShaderManager.h"

class NPRScene;
class NPRStyle;

class NPRPathRenderer
{
    public:
        NPRPathRenderer();

        void clear();

        void setUniformPenStyleParams(const GQShaderRef& shader,
                        const NPRStyle& style, bool noStyle=false);

        void setGeomFlowPenStyleParams(const GQShaderRef& shader,
        		const NPRStyle& style);

        void setUniformOffsetParams(const GQShaderRef& shader,
                        const NPRStyle& style, bool noStyle=false);

        void init();

        void bindQuadVerticesVBO(const GQShaderRef& current_shader)   { _quad_vertices_vbo.bind(current_shader); }
        void unbindQuadVerticesVBO() { _quad_vertices_vbo.unbind(); }

    protected:

        void makeQuadRenderingVBO();

        void makeBlankTextures();

    protected:
        bool         _is_initialized;

        GQTexture3D  _blank_white_texture;
        GQTexture2D  _blank_white_texture_rect;
        GQVertexBufferSet _quad_vertices_vbo;
};

#endif /*NPR_PATH_RENDERER_H_*/
