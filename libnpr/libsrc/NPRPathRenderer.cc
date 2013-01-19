/***************************************************************************
NPRPathRenderer.cc
Copyright (c) 2009 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "NPRPathRenderer.h"
#include "NPRStyle.h"
#include "NPRSettings.h"
#include "NPRGLDraw.h"

#include "GQShaderManager.h"
#include "DialsAndKnobs.h"
#include "Stats.h"
#include <assert.h>

#if 1
#define __MY_START_TIMER __START_TIMER
#define __MY_STOP_TIMER __STOP_TIMER
#define __MY_TIME_CODE_BLOCK __TIME_CODE_BLOCK
#else
#define __MY_START_TIMER __START_TIMER_AFTER_GL_FINISH
#define __MY_STOP_TIMER __STOP_TIMER_AFTER_GL_FINISH
#define __MY_TIME_CODE_BLOCK __TIME_CODE_BLOCK_AFTER_GL_FINISH
#endif

const int DRAW_STROKES_WITH_PRIORITY = 0;
const int DRAW_STROKES_NO_PRIORITY = 1;
const int DRAW_PRIORITY_WITH_VISIBILITY = 2;
const int DRAW_PRIORITY_NO_VISIBILITY = 3;

NPRPathRenderer::NPRPathRenderer()
{
    clear();
}

void NPRPathRenderer::clear()
{
    _blank_white_texture.clear();
    _quad_vertices_vbo.clear();
    
    _is_initialized = false;
}

void NPRPathRenderer::init()
{
    makeBlankTextures();
    makeQuadRenderingVBO();
    
    _is_initialized = true;
}

void NPRPathRenderer::setGeomFlowPenStyleParams(const GQShaderRef& shader,
                                                const NPRStyle& style)
{
    const NPRPenStyle* vis_focus = style.penStyle("Base Style");
    const GQTexture* vis_focus_tex = &_blank_white_texture;
    if (vis_focus->texture()) vis_focus_tex = vis_focus->texture();
    shader.setUniform1f("pen_width", 3);
}


void NPRPathRenderer::setUniformOffsetParams(const GQShaderRef& shader,
                                             const NPRStyle& style,                                      
                                             bool noStyle)
{
    const NPRPenStyle* vis_focus = style.penStyle("Base Style");
    const GQTexture* offset_tex = &_blank_white_texture;
    if (!noStyle && vis_focus->normalTexture()) offset_tex = vis_focus->normalTexture();
    if (shader.uniformLocation("normal_texture") >= 0)
        shader.bindNamedTexture("normal_texture", offset_tex);
    if (!noStyle && vis_focus->tangentTexture()) offset_tex = vis_focus->tangentTexture();
    if (shader.uniformLocation("tangent_texture") >= 0)
        shader.bindNamedTexture("tangent_texture", offset_tex);
    if(shader.uniformLocation("offset_scale")>=0)
        shader.setUniform1f("offset_scale", vis_focus->offsetScale() * vis_focus->userOffsetScale());
}

void NPRPathRenderer::setUniformPenStyleParams(const GQShaderRef& shader, 
                                               const NPRStyle& style,
                                               bool noStyle)
{
    const NPRPenStyle* vis_focus = style.penStyle("Base Style");
    const NPRPenStyle* vis_defocus = style.penStyle("Defocused");
    const NPRPenStyle* invis_focus = style.penStyle("Invisible");
    const NPRPenStyle* invis_defocus = style.penStyle("Defocused Invis.");
    
    // We have to set some texture for the pen even if the user hasn't
    // selected anything to avoid a crash on mac.
    const GQTexture* vis_focus_tex = &_blank_white_texture;
    const GQTexture* vis_defocus_tex = &_blank_white_texture;
    const GQTexture* invis_focus_tex = &_blank_white_texture;
    const GQTexture* invis_defocus_tex = &_blank_white_texture;
    
	if (!noStyle && vis_focus->texture()) vis_focus_tex = vis_focus->texture();
	if (!noStyle && vis_defocus->texture()) vis_defocus_tex = vis_defocus->texture();
	if (!noStyle && invis_focus->texture()) invis_focus_tex = invis_focus->texture();
	if (!noStyle && invis_defocus->texture()) invis_defocus_tex = invis_defocus->texture();

    if(shader.uniformLocation("use_artmaps")>=0){
        if (!noStyle && vis_focus_tex->depth()>1)
            shader.setUniform1i("use_artmaps",1);
        else
            shader.setUniform1i("use_artmaps",0);
    }
    
    shader.bindNamedTexture("vis_focus_texture", vis_focus_tex);
    if(shader.uniformLocation("vis_defocus_texture")>=0)
        shader.bindNamedTexture("vis_defocus_texture", vis_defocus_tex);

    if(shader.uniformLocation("invis_focus_texture")>=0){
    	shader.bindNamedTexture("invis_focus_texture", invis_focus_tex);
    	shader.bindNamedTexture("invis_defocus_texture", invis_defocus_tex);
    }

    float invis_opacity_factor = style.drawInvisibleLines() ? 1.0f : 0.0f;
    
    if(noStyle){
        vec4f white(1.f,1.f,1.f,1.f);
        if(shader.uniformLocation("vis_focus_color")>=0)
            shader.setUniform4fv("vis_focus_color", white);
        if(shader.uniformLocation("vis_defocus_color")>=0)
            shader.setUniform4fv("vis_defocus_color", white);
        if(shader.uniformLocation("invis_focus_color")>=0){
            shader.setUniform4fv("invis_focus_color", white);
            shader.setUniform4fv("invis_defocus_color", white);
        }
        shader.setUniform1f("pen_width", 1.f);
    }else{
        
        vec4f vis_focus_color(vis_focus->color()[0], vis_focus->color()[1],
                              vis_focus->color()[2], vis_focus->opacity());
        vec4f vis_defocus_color(vis_defocus->color()[0], vis_defocus->color()[1],
                                vis_defocus->color()[2], vis_defocus->opacity());
        vec4f invis_focus_color(invis_focus->color()[0], invis_focus->color()[1],
                                invis_focus->color()[2],
                                invis_focus->opacity()*invis_opacity_factor);
        vec4f invis_defocus_color(invis_defocus->color()[0], invis_defocus->color()[1],
                                  invis_defocus->color()[2],
                                  invis_defocus->opacity()*invis_opacity_factor);
        
        if(shader.uniformLocation("vis_focus_color")>=0)
            shader.setUniform4fv("vis_focus_color", vis_focus_color);
        if(shader.uniformLocation("vis_defocus_color")>=0)
            shader.setUniform4fv("vis_defocus_color", vis_defocus_color);

        if(shader.uniformLocation("invis_focus_color")>=0){
            shader.setUniform4fv("invis_focus_color", invis_focus_color);
            shader.setUniform4fv("invis_defocus_color", invis_defocus_color);
        }
        
        shader.setUniform1f("pen_width", vis_focus->stripWidth());
    }
    
    if(shader.uniformLocation("texture_length")>=0)
        shader.setUniform1f("texture_length", vis_focus_tex->width());
    shader.setUniform1f("length_scale", vis_focus->lengthScale());
    //shader.setUniform1f("texture_depth", vis_focus_tex->depth());
    
    float color_trans[4], opacity_trans[4], texture_trans[4];
    style.transfer(NPRStyle::LINE_COLOR).toArray(color_trans);
    style.transfer(NPRStyle::LINE_OPACITY).toArray(opacity_trans);
    style.transfer(NPRStyle::LINE_TEXTURE).toArray(texture_trans);
    if(shader.uniformLocation("color_transfer")>=0){
        shader.setUniform4fv("color_transfer", color_trans);
        shader.setUniform4fv("opacity_transfer", opacity_trans);
        shader.setUniform4fv("texture_transfer", texture_trans);
    }
}

// Creates a blank dummy texture for rendering quads when the user
// hasn't selected a texture.
void NPRPathRenderer::makeBlankTextures()
{
    int size = 16;
    GQImage blank_img(size, size, 4);
    for (int i = 0; i < size*size*4; i++)
    {
        blank_img.raster()[i] = 255;
    }
    _blank_white_texture.create(blank_img);
    _blank_white_texture_rect.create(blank_img, GL_TEXTURE_RECTANGLE_ARB);
}

// Creates a string of vertices for rendering quads along line segments.
// The vertex positions will be adjusted in a shader.
void NPRPathRenderer::makeQuadRenderingVBO()
{
    QVector<float> vertices;
    int count = GQFramebufferObject::maxFramebufferSize();
    
    for (int i = 0; i < count; i++)
    {
        vertices.push_back(i);
        vertices.push_back(0);
    }
    _quad_vertices_vbo.clear();
    _quad_vertices_vbo.add(GQ_VERTEX, 2, vertices);
    _quad_vertices_vbo.copyToVBOs();
    
    NPRGLDraw::handleGLError();
}

