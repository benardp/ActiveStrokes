/*****************************************************************************\

GQGPUImageProcessing.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef GPUIMAGEPROCESSING_H_
#define GPUIMAGEPROCESSING_H_

#include "GQInclude.h"
#include "GQTexture.h"
#include "GQImage.h"
#include "GQFramebufferObject.h"

class GQGPUImageProcessing {
public:
    GQGPUImageProcessing();
    virtual ~GQGPUImageProcessing();

    static void convolve(GQTexture2D* kernel, GQTexture2D* image, GQFloatImage& result);

    static void convolve(GQTexture2D* kernel, GQTexture2D* image, GQImage& result);

    static void diff(GQTexture2D* image1, GQTexture2D* image2, GQImage& result);

    static void morpho(GQTexture2D* image, float blur, GQImage& result);

    static void blurAndGrad(int iter, GQTexture2D* image, GQFloatImage& output);

    static void drawQuad();

    static GQTexture* blurredImage() { return _blur_fbo.colorTexture(0); }
    static GQTexture* gradientImage() { return _blur_fbo.colorTexture(1); }

protected:
    static GQFramebufferObject _blur_fbo;
};

#endif /* GPUIMAGEPROCESSING_H_ */
