/*****************************************************************************\

GQDraw.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Utility drawing calls that wrap OpenGL calls.

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "GQDraw.h"

#include <assert.h>

void GQDraw::drawElements(const GQVertexBufferSet& vb, int gl_mode, int offset, 
                          int length, const int* indices)
{
    assert(vb.isBound());

    if (indices != nullptr)
    {
        assert(!vb.vbosLoaded());
        const int* start = indices + offset;
        glDrawElements(gl_mode, length, GL_UNSIGNED_INT, (const GLvoid*)(start));
    }
    else
    {
        assert(vb.vbosLoaded());
        assert(vb.hasBuffer(GQ_INDEX));
        int byte_offset = offset * sizeof(GLint);
        glDrawElements(gl_mode, length, GL_UNSIGNED_INT, (const GLvoid*)(byte_offset));
    }
}

void GQDraw::clearGLState()
{
    glDisable(GL_DITHER);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_NORMALIZE);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_MULTISAMPLE);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_RECTANGLE_ARB);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());
    glFuncs.glUseProgram(0);

    for (int i = 0; i < 8; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_3D, 0);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
    }
    glActiveTexture(GL_TEXTURE0);
}

void GQDraw::clearGLScreen(const vec& color, float depth)
{
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(color[0], color[1], color[2], 0.0);
    glClearDepth(depth);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GQDraw::clearGLDepth(float depth)
{
    glDepthMask(GL_TRUE);
    glClearDepth(depth);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void GQDraw::drawFullScreenQuad( int texture_mode )
{
    reportGLError();

    GLint main_viewport[4];
    glGetIntegerv (GL_VIEWPORT, main_viewport);
    int width  = main_viewport[2];
    int height = main_viewport[3];

    // draw full screen quad. start by setting projection and modelview to identity
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // turn off depth testing
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);

    if (texture_mode == GL_TEXTURE_2D)
    {
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1, 0);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2f(1, 1);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0, 1);
        glVertex2f(-1.0f, 1.0f);
        glEnd();
    }
    else
    {
        glColor4f(1.0, 1.0, 1.0, 1.0);

        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2i(width, 0);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2i(width, height);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2i(0, height);
        glVertex2f(-1.0f, 1.0f);
        glEnd();
    }

    // restore matrix state
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    reportGLError();
}


void GQDraw::drawFullScreenQuad( const GQFramebufferObject& fbo )
{
    reportGLError();

    // draw full screen quad. start by setting projection and modelview to identity
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    if (fbo.glTarget() == GL_TEXTURE_2D)
    {
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1, 0);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2f(1, 1);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0, 1);
        glVertex2f(-1.0f, 1.0f);
        glEnd();
    }
    else
    {
        int width = fbo.width();
        int height = fbo.height();
        glColor4f(1.0, 1.0, 1.0, 1.0);
        glPointSize(20.f);

        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2i(width, 0);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2i(width, height);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2i(0, height);
        glVertex2f(-1.0f, 1.0f);
        glEnd();
    }

    // restore matrix state

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    reportGLError();
}

void GQDraw::startScreenCoordinatesSystem(bool upward, int width, int height)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    if (upward)
        glOrtho(0, width, 0, height, 0.0, -1.0);
    else
        glOrtho(0, width, height, 0, 0.0, -1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void GQDraw::stopScreenCoordinatesSystem()
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void GQDraw::visualizeTexture(const GQTexture* texture)
{
    texture->bind();
    glEnable(texture->target());

    drawFullScreenQuad(texture->target());

    glDisable(texture->target());
}
