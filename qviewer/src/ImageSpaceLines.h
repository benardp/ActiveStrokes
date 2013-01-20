/*****************************************************************************\

ImageSpaceLines.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

qviever is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _STEERABLE_IMAGE_LINES_H_
#define _STEERABLE_IMAGE_LINES_H_

#include "GQFramebufferObject.h"
#include "Scene.h"
#include "ASClipPath.h"

enum buffer_indices {
    SHADING_BUFFER_NUM = 0,
    POS_BUFFER_NUM,
    FLOW_BUFFER_NUM,
    Z_BUFFER_NUM,
    COLOR_BUFFER_NUM,
    BUFFER_INDICES_NUM
};

class ImageSpaceLines
{
  public:

    ImageSpaceLines();
    ~ImageSpaceLines();

    void drawScene(Scene& scene, bool visualize=false);
    
    void readbackSamples(xform &proj_xf, xform &mv_xf, bool read_motion, bool useDepth);

    ASClipPathSet* clipPathSet() { return &_clip_path_set; }

    GQFramebufferObject* colors_fbo() { return &_colors_fbo; }
    GQTexture2D*    offscreenTexture();
    GQTexture2D*    energyTexture() { return _energy_fbo.colorTexture(0); }
    GQTexture2D*    colorTexture() { return _colors_fbo.colorTexture(0); }

    GQFloatImage*   geometricFlowBuffer();

    GQFloatImage*   denseFlowBuffer();

    void nextGeomFlowBuffer();
    void initGeomFlowBuffer();

    void setNextCameraTransform(const xform& xf) { _next_camera_matrix = xf; }

    void visualizeBuffer();

    // Steerable filters
    void blurColors(const GQTexture2D* tex, float radius = -1);
    void computeDerivatives();
    void applyFilterPair(int buffer, float blur_radius = -1);
    void extractLines();

    // Line drawings via abstract shading, Lee et al. 2007
    void extractLeeLines();

protected:
    bool _initialized;

    GQFramebufferObject _colors_fbo;
    GQFramebufferObject _blurred_colors_fbo;
    GQFramebufferObject _d1_fbo;
    GQFramebufferObject _d2_fbo;
    GQFramebufferObject _d3_fbo;
    GQFramebufferObject _half_blur_fbo;
    GQFramebufferObject _energy_fbo;
    GQFramebufferObject _lines_fbo;
    GQFramebufferObject _offscreen_fbo;
    GQFramebufferObject _depth_buffer;

    ASClipPathSet _clip_path_set;

    GQFloatImage* _geomFlow;
    GQFloatImage* _prevGeomFlow;

    GQFloatImage* _motion_img;
    GQFloatImage* _prev_motion_img;

    xform _next_camera_matrix;
};

#endif // _STEERABLE_IMAGE_LINES_H_

