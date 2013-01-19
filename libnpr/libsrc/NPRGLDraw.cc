/************************************************************************
NPRGLDraw.cc
Copyright (c) 2009 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "NPRGLDraw.h"
#include "NPRStyle.h"
#include "NPRSettings.h"
#include "GQTexture.h"
#include "GQShaderManager.h"
#include "GQFramebufferObject.h"
#include "Stats.h"

#include <assert.h>

int NPRGLDraw::_cur_index = 0;

void NPRGLDraw::handleGLError(const char* file, int line)
{
#ifndef NDEBUG
    GLint error = glGetError();
    if (error != 0)
    {
        QString errormsg = "NPRGLDraw::handleGLError: " +
                           QString((const char*)gluErrorString(error));
        if (file)
            errormsg = errormsg + QString(" (in %1:%2)").arg(file).arg(line);

        qCritical("%s\n", qPrintable(errormsg));
        qFatal("%s\n", qPrintable(errormsg));
    }
#endif
} 

void NPRGLDraw::clearGLScreen(const vec& color, float depth)
{
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(color[0], color[1], color[2], 0.0);
    glClearDepth(depth);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void NPRGLDraw::clearGLState()
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
    glUseProgram(0);

    for (int i = 0; i < 8; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_3D, 0);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
    }
    glActiveTexture(GL_TEXTURE0);
}

void NPRGLDraw::setUniformViewParams(const GQShaderRef& shader)
{
    GLfloat viewport[4];
    GLfloat mv[16],proj[16];
    glGetFloatv(GL_VIEWPORT, viewport);
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    glGetFloatv(GL_PROJECTION_MATRIX, proj);

    int viewport_id = shader.uniformLocation("viewport");
    if (viewport_id >= 0)
        glUniform4fv(viewport_id, 1, viewport);

    int modelview_id = shader.uniformLocation("modelview");
    if (modelview_id >= 0)
        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, mv);

    int projection_id = shader.uniformLocation("projection");
    if (projection_id >= 0)
        glUniformMatrix4fv(projection_id, 1, GL_FALSE, proj);

    int inverse_projection_id = shader.uniformLocation("inverse_projection");
    if (inverse_projection_id >= 0)
    {
        XForm<float> inverse_projection = XForm<float>(proj);
        invert(inverse_projection);
        glUniformMatrix4fv(inverse_projection_id, 1, GL_FALSE, inverse_projection);
    }
}

void NPRGLDraw::drawFullScreenQuad( int texture_mode )
{
    NPRGLDraw::handleGLError();

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
    /*    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);*/

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

    NPRGLDraw::handleGLError();
}

void NPRGLDraw::visualizeFBO(const GQFramebufferObject& fbo, int which)
{
    fbo.colorTexture(which)->bind();
    glEnable(fbo.colorTexture(which)->target());

    drawFullScreenQuad(fbo.glTarget());

    glDisable(fbo.colorTexture(which)->target());
}

void NPRGLDraw::visualizeTexture(const GQTexture* texture)
{
    texture->bind();
    glEnable(texture->target());

    drawFullScreenQuad(texture->target());

    glDisable(texture->target());
}
