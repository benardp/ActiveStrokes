/*****************************************************************************\

GQGPUImageProcessing.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "GQGPUImageProcessing.h"
#include "GQDraw.h"
#include "GQFramebufferObject.h"
#include "GQShaderManager.h"

GQFramebufferObject GQGPUImageProcessing::_blur_fbo;

GQGPUImageProcessing::GQGPUImageProcessing() {}

GQGPUImageProcessing::~GQGPUImageProcessing() {}

void GQGPUImageProcessing::drawQuad(){
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor4f(1.0, 1.0, 1.0, 1.0);

    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex2f(-1.0f, -1.0f);
    glTexCoord2i(1, 0);
    glVertex2f(1.0f, -1.0f);
    glTexCoord2i(1, 1);
    glVertex2f(1.0f, 1.0f);
    glTexCoord2i(0, 1);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void GQGPUImageProcessing::convolve(GQTexture2D* kernel, GQTexture2D* image, GQFloatImage& result)
{
    GQFramebufferObject fbo;
    GLint viewport[4];
    glGetIntegerv (GL_VIEWPORT, viewport);
    fbo.initGL(GL_TEXTURE_RECTANGLE_ARB, GL_RGBA32F_ARB,
               GQ_ATTACH_NONE, 1, viewport[2], viewport[3]);

    GQShaderRef shader = GQShaderManager::bindProgram("vfc");

    fbo.bind();
    fbo.drawBuffer(0);

    glViewport(0,0,fbo.width(),fbo.height());
    glClearColor(0.0,0.0,0.0,1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    shader.setUniform1f("kernel_size",kernel->width());
    shader.setUniform2f("image_size",image->width(),image->height());
    shader.bindNamedTexture("kernel",kernel);
    shader.bindNamedTexture("src_tex",image);

    glActiveTexture(GL_TEXTURE0);

    drawQuad();

    fbo.unbind();

    fbo.readColorTexturef(0,result);
}

void GQGPUImageProcessing::convolve(GQTexture2D* kernel, GQTexture2D* image, GQImage& result)
{
    GQFramebufferObject fbo;
    GLint viewport[4];
    glGetIntegerv (GL_VIEWPORT, viewport);
    fbo.initGL(GL_TEXTURE_RECTANGLE_ARB, GL_RGBA32F_ARB,
               GQ_ATTACH_NONE, 1, viewport[2], viewport[3]);

    GQShaderRef shader = GQShaderManager::bindProgram("vfc");

    fbo.bind();
    fbo.drawBuffer(0);

    glViewport(0,0,fbo.width(),fbo.height());
    glClearColor(0.0,0.0,0.0,1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    shader.setUniform1f("kernel_size",kernel->width());
    shader.setUniform2f("image_size",image->width(),image->height());
    shader.bindNamedTexture("kernel",kernel);
    shader.bindNamedTexture("src_tex",image);

    glActiveTexture(GL_TEXTURE0);

    drawQuad();

    fbo.unbind();

    fbo.readColorTexturei(0,result);
}

void GQGPUImageProcessing::diff(GQTexture2D* image1, GQTexture2D* image2, GQImage& result) {
    GQFramebufferObject fbo;
    GLint viewport[4];
    glGetIntegerv (GL_VIEWPORT, viewport);
    fbo.initGL(GL_TEXTURE_RECTANGLE_ARB, GL_RGBA32F_ARB,
               GQ_ATTACH_NONE, 1, viewport[2], viewport[3]);

    GQShaderRef shader = GQShaderManager::bindProgram("diff");

    fbo.bind();
    fbo.drawBuffer(0);

    glViewport(0,0,fbo.width(),fbo.height());
    glClearColor(0.0,0.0,0.0,1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    shader.setUniform2f("image_size",image1->width(),image1->height());
    shader.bindNamedTexture("image1",image1);
    shader.bindNamedTexture("image2",image2);

    glActiveTexture(GL_TEXTURE0);

    drawQuad();

    fbo.unbind();

    fbo.readColorTexturei(0,result);
}

void GQGPUImageProcessing::morpho(GQTexture2D* image, float blur, GQImage& result) {
    GQFramebufferObject fbo, fbo2;
    GLint viewport[4];
    glGetIntegerv (GL_VIEWPORT, viewport);
    fbo.initGL(GL_TEXTURE_RECTANGLE_ARB, GL_RGBA32F_ARB,
               GQ_ATTACH_NONE, 1, viewport[2], viewport[3]);

    // Erosion
    GQShaderRef shader = GQShaderManager::bindProgram("erode");

    shader.setUniform2f("image_size",image->width(),image->height());
    shader.setUniform1f("blur",blur);
    shader.bindNamedTexture("image",image);

    fbo.bind();
    fbo.drawBuffer(0);

    glViewport(0,0,fbo.width(),fbo.height());
    glClearColor(0.0,0.0,0.0,1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);

    drawQuad();

    fbo.unbind();

    fbo.readColorTexturei(0,result);
}


void GQGPUImageProcessing::blurAndGrad(int iter, GQTexture2D* image,
                                     GQFloatImage& output) {

    _blur_fbo.initFullScreen(2);

    GQShaderRef shader;

    glClearColor(0,0,0,1);
    glDisable(GL_DEPTH_TEST);

    _blur_fbo.bind(GQ_CLEAR_BUFFER);

    for(int i=0; i<iter; i++) {
        // Vertical pass
        shader = GQShaderManager::bindProgram("vblur");

        _blur_fbo.drawBuffer(1);

        if(i==0)
            shader.bindNamedTexture("src_tex",image);
        else
            shader.bindNamedTexture("src_tex",_blur_fbo.colorTexture(0));

        GQDraw::drawFullScreenQuad(_blur_fbo);

        // Horizontal pass
        shader = GQShaderManager::bindProgram("hblur");

        _blur_fbo.drawBuffer(0);

        shader.bindNamedTexture("src_tex",_blur_fbo.colorTexture(1));

        GQDraw::drawFullScreenQuad(_blur_fbo);
    }

    //_blur_fbo.saveColorTextureToFile(0,"blur.bmp");

    shader = GQShaderManager::bindProgram("gradient");

    _blur_fbo.drawBuffer(1);

    glClear(GL_COLOR_BUFFER_BIT);

    if(iter>0)
        shader.bindNamedTexture("image",_blur_fbo.colorTexture(0));
    else
        shader.bindNamedTexture("image",image);
    shader.setUniform1f("weight",1.0);

    reportGLError();

    drawQuad();

    _blur_fbo.unbind();

    reportGLError();

    shader.unbind();

    _blur_fbo.readColorTexturef(1,output);
}
