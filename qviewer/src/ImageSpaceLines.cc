/*****************************************************************************\

ImageSpaceLines.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

qviever is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "ImageSpaceLines.h"
#include "DialsAndKnobs.h"

#include "GQShaderManager.h"
#include "GQDraw.h"
#include "Stats.h"

#ifndef isinf
using std::isinf;
#endif

#include "TriMesh.h"

static QStringList k_shading_list = QStringList() << "Phong" << "Toon";
dkStringList k_shading("Current->Shader",k_shading_list);
extern dkStringList k_model;

static dkFloat k_blur_radius("Image Lines->Steerable->Blur Radius", 2.0, 0.1, 100.0, 0.2);
static dkFloat k_line_threshold("Image Lines->Steerable->Threshold", 0.003, 0.0, 100.0, 0.0001);
static dkFloat k_line_thresh_range("Image Lines->Steerable->Thresh. Range", 0.01, 0.0, 100.0, 0.0001);
static dkFloat k_visualization_gain("Image Lines->Viz.->Gain", 1, 0.0, 10e10, 1);
static dkFloat k_added_z_scale("Image Lines->Steerable->Added Z Scale", 2);

static dkBool k_separate_xyz_derivs("Image Lines->Steerable->Separate XYZ Derivs.", true);
static QStringList k_source_types = QStringList() << "Single Light" << "Depth Map";
static dkStringList k_source_type("Image Lines->Steerable->Source Type", k_source_types);
static QStringList k_lines = QStringList() << "Steerable filters" << "Lee";
static dkStringList k_line("Image Lines-> Line Type", k_lines);

static dkFloat k_half_width("Image Lines->Lee->Half width",1.0f);
static dkFloat k_upper_curv_thd("Image Lines->Lee->Upper curv. th.",0.01);
static dkFloat k_lower_curv_thd("Image Lines->Lee->Lower curv. th.",0.007);
static dkFloat k_moving_factor("Image Lines->Lee->Moving factor", 0.3);
static QStringList k_opacity_controls = QStringList() << "None" << "Curvature" << "Distance"<< "Tone" ;
static dkStringList k_opacity_ctrl("Image Lines->Lee->Opacity ctrl",k_opacity_controls);

ImageSpaceLines::ImageSpaceLines()
{
    _prevGeomFlow = new GQFloatImage();
    _geomFlow = new GQFloatImage();
    _motion_img = new GQFloatImage();
    _prev_motion_img = new GQFloatImage();
    _initialized = false;
}

ImageSpaceLines::~ImageSpaceLines()
{
    delete _prevGeomFlow;
    delete _geomFlow;
    delete _prev_motion_img;
    delete _motion_img;
}

void ImageSpaceLines::drawScene(Scene& scene, bool visualize)
{
    __TIME_CODE_BLOCK("Draw Scene (img)");

    if (GQShaderManager::status() == GQ_SHADERS_NOT_LOADED)
    {
        GQShaderManager::initialize();
    }

    GQDraw::clearGLState();

    if (!GQShaderManager::status() == GQ_SHADERS_OK)
        return;

    vec background_color = vec(0.0, 0.0, 0.0);

    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(background_color[0], background_color[1], background_color[2], 0.0);
    glClearDepth(1.0);

    _colors_fbo.initFullScreen(BUFFER_INDICES_NUM,GQ_ATTACH_DEPTH_TEXTURE,GQ_COORDS_PIXEL,GQ_FORMAT_RGBA_FLOAT);
    _colors_fbo.bind(GQ_CLEAR_BUFFER);

    GQDraw::clearGLState();

    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
    GQShaderRef shader;

    shader = GQShaderManager::bindProgram("deferredShading");
    shader.setUniform1i("toon_mode",k_shading.index());

    scene.drawScene(shader,(ModelType)k_model.index());

    _colors_fbo.unbind();

    int type = 0;

    if(k_source_type == "Single Light")
        type = SHADING_BUFFER_NUM;
    else if(k_source_type == "Depth Map")
        type = Z_BUFFER_NUM;

    if (k_line == k_lines[0]) { // Steerable filters
        blurColors(_colors_fbo.colorTexture(type));
        computeDerivatives();
        applyFilterPair(-1);
        extractLines();
    }
    else if(k_line == k_lines[1]){ // Line drawings via abstract shading
        extractLeeLines();
    }

    if(visualize){
        GQDraw::clearGLScreen(background_color, 1);
        visualizeBuffer();
    }

    GQDraw::clearGLState();
}

void ImageSpaceLines::blurColors(const GQTexture2D* tex, float radius)
{
    __TIME_CODE_BLOCK("Blur Colors");
    
    _blurred_colors_fbo.initFullScreen(2);
    _blurred_colors_fbo.bind(GQ_CLEAR_BUFFER);

    if (radius < 0)
        radius = k_blur_radius;

    GQShaderRef shader = GQShaderManager::bindProgram("gaussian_blur");
    shader.setUniform1f("kernel_radius", radius);

    shader.setUniform2f("filter_direction", 1, 0);
    shader.bindNamedTexture("colors", tex);
    _blurred_colors_fbo.drawBuffer(0);

    GQDraw::drawFullScreenQuad(_blurred_colors_fbo);

    shader.setUniform2f("filter_direction", 0, 1);
    shader.bindNamedTexture("colors", _blurred_colors_fbo.colorTexture(0));
    _blurred_colors_fbo.drawBuffer(1);

    GQDraw::drawFullScreenQuad(_blurred_colors_fbo);

    _blurred_colors_fbo.unbind();
}

void ImageSpaceLines::computeDerivatives()
{
    __TIME_CODE_BLOCK("Compute Derivatives");
    int num_derivative_channels = 1;
    if (k_separate_xyz_derivs)
        num_derivative_channels = 3;

    // Allocate enough room in the FBOs for the shader to output three buffers
    // no matter what.
    _d1_fbo.initFullScreen(3);
    _d2_fbo.initFullScreen(3);
    _d3_fbo.initFullScreen(3);
    
    GQShaderRef shader = GQShaderManager::bindProgram("derivative_filter_rgb");
    shader.bindNamedTexture("colors", _blurred_colors_fbo.colorTexture(1));    

    _d1_fbo.bind();
    _d1_fbo.drawToAllBuffers();
    GQDraw::drawFullScreenQuad(_d1_fbo);
    _d1_fbo.unbind();

    shader = GQShaderManager::bindProgram("derivative_filter");

    for (int i = 0; i < num_derivative_channels; i++) {

        shader.bindNamedTexture("colors", _d1_fbo.colorTexture(i));
        shader.setUniform4f("dx_read_mask", 1, 0, 1, 0);
        shader.setUniform4f("dy_read_mask", 0, 1, 0, 1);
	
        _d2_fbo.bind();
        _d2_fbo.drawBuffer(i);
        GQDraw::drawFullScreenQuad(_d2_fbo);
        _d2_fbo.unbind();
	
        shader.bindNamedTexture("colors", _d2_fbo.colorTexture(i));
        shader.setUniform4f("dx_read_mask", 1, 1, 0, 0);
        shader.setUniform4f("dy_read_mask", 0, 0, 1, 1);
	
        _d3_fbo.bind();
        _d3_fbo.drawBuffer(i);
        GQDraw::drawFullScreenQuad(_d3_fbo);
        _d3_fbo.unbind();
    }
}

static dkFloat k_filter_blur_multiplier("Image Lines->Steerable->Blur Multiplier", 1, 0, 10, 0.1);
static dkFloat k_g2_selection_scale("Image Lines->Steerable->G2 Scale", 1, 0, 10, 0.1);
static dkFloat k_h2_selection_scale("Image Lines->Steerable->H2 Scale", 1, 0, 10, 0.1);

void ImageSpaceLines::applyFilterPair(int buffer, float blur_radius)
{
    __TIME_CODE_BLOCK("Apply Filter Pair");
    
    _energy_fbo.initFullScreen(3);

    GQShaderRef shader;
    if (!k_separate_xyz_derivs) {
        shader = GQShaderManager::bindProgram("filter_pair");
        shader.bindNamedTexture("d1_image", _d1_fbo.colorTexture(0));
        shader.bindNamedTexture("d2_image", _d2_fbo.colorTexture(0));
        shader.bindNamedTexture("d3_image", _d3_fbo.colorTexture(0));
    } else {
        shader = GQShaderManager::bindProgram("filter_pair_xyz");
        shader.bindNamedTexture("d1_x_image", _d1_fbo.colorTexture(0));
        shader.bindNamedTexture("d2_x_image", _d2_fbo.colorTexture(0));
        shader.bindNamedTexture("d3_x_image", _d3_fbo.colorTexture(0));
        shader.bindNamedTexture("d1_y_image", _d1_fbo.colorTexture(1));
        shader.bindNamedTexture("d2_y_image", _d2_fbo.colorTexture(1));
        shader.bindNamedTexture("d3_y_image", _d3_fbo.colorTexture(1));
        shader.bindNamedTexture("d1_z_image", _d1_fbo.colorTexture(2));
        shader.bindNamedTexture("d2_z_image", _d2_fbo.colorTexture(2));
        shader.bindNamedTexture("d3_z_image", _d3_fbo.colorTexture(2));
    }
    
    if (blur_radius < 0)
        blur_radius = k_blur_radius;

    float effective_blur = blur_radius * k_filter_blur_multiplier;
    

    shader.setUniform1f("blur_radius", effective_blur);
    shader.setUniform1f("g2_selection_scale", k_g2_selection_scale);
    shader.setUniform1f("h2_selection_scale", k_h2_selection_scale);

    _energy_fbo.bind();
    if (buffer >= 0)
        _energy_fbo.drawBuffer(buffer);
    else
        _energy_fbo.drawToAllBuffers();
    GQDraw::drawFullScreenQuad(_energy_fbo);
    _energy_fbo.unbind();
}

void ImageSpaceLines::extractLines()
{
    _lines_fbo.initFullScreen(2);

    GLfloat proj[16];
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    
    GQShaderRef shader = GQShaderManager::bindProgram("image_lines");
    shader.bindNamedTexture("energy", _energy_fbo.colorTexture(0));
    shader.setUniform1f("threshold", k_line_threshold);
    shader.setUniform1f("thresh_range", k_line_thresh_range);

    shader.setUniformMatrix4fv("projection", proj);
    shader.bindNamedTexture("camera_pos", _colors_fbo.colorTexture(POS_BUFFER_NUM));
    shader.bindNamedTexture("geom_flow", _colors_fbo.colorTexture(FLOW_BUFFER_NUM));

    GLfloat viewport[4];
    glGetFloatv(GL_VIEWPORT, viewport);
    int viewport_id = shader.uniformLocation("viewport");
    glUniform4fv(viewport_id, 1, viewport);

    _lines_fbo.bind(GQ_CLEAR_BUFFER);
    _lines_fbo.drawToAllBuffers();
    GQDraw::drawFullScreenQuad(_lines_fbo);
    _lines_fbo.unbind();
}

void ImageSpaceLines::extractLeeLines()
{
    _lines_fbo.initFullScreen(2);

    GQShaderRef shader = GQShaderManager::bindProgram("Lee_lines");

    GLfloat viewport[4];
    glGetFloatv(GL_VIEWPORT, viewport);
    shader.setUniform4fv("viewport",viewport);

    GLfloat proj[16];
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    shader.setUniformMatrix4fv("projection", proj);

    shader.bindNamedTexture("tone_map",   _colors_fbo.colorTexture(SHADING_BUFFER_NUM));
    shader.bindNamedTexture("camera_pos", _colors_fbo.colorTexture(POS_BUFFER_NUM));
    shader.bindNamedTexture("geom_flow",  _colors_fbo.colorTexture(FLOW_BUFFER_NUM));
    shader.setUniform1f("half_width",k_half_width);
    shader.setUniform1f("upper_curv_thd",k_upper_curv_thd);
    shader.setUniform1f("lower_curv_thd",k_lower_curv_thd);
    shader.setUniform1f("moving_factor", k_moving_factor);
    if(k_opacity_ctrl == k_opacity_controls[0]){
        shader.setUniform1i("curv_opacity_ctrl",0);
        shader.setUniform1i("dist_opacity_ctrl",0);
        shader.setUniform1i("tone_opacity_ctrl",0);
    }else if(k_opacity_ctrl == k_opacity_controls[1]){
        shader.setUniform1i("curv_opacity_ctrl",1);
        shader.setUniform1i("dist_opacity_ctrl",0);
        shader.setUniform1i("tone_opacity_ctrl",0);
    }else if(k_opacity_ctrl == k_opacity_controls[2]){
        shader.setUniform1i("curv_opacity_ctrl",0);
        shader.setUniform1i("dist_opacity_ctrl",1);
        shader.setUniform1i("tone_opacity_ctrl",0);
    }else if(k_opacity_ctrl == k_opacity_controls[3]){
        shader.setUniform1i("curv_opacity_ctrl",0);
        shader.setUniform1i("dist_opacity_ctrl",0);
        shader.setUniform1i("tone_opacity_ctrl",1);
    }

    _lines_fbo.bind(GQ_CLEAR_BUFFER);
    GQDraw::drawFullScreenQuad(_lines_fbo);
    _lines_fbo.unbind();
}

static dkBool k_viz_use_colors("Image Lines->Viz.->Colors", false);
static QStringList k_buffer_types = QStringList() << "Lines Buffer" << "Blurred Color" << "Source Color"
                                    << "D1 Buffer" << "D2 Buffer" << "D3 Buffer"
                                    << "Energy Buffer" << "Filter Components" << "Sample Positions" ;
static dkStringList k_viz_buffer("Image Lines->Viz.->Buffer", k_buffer_types);

void ImageSpaceLines::visualizeBuffer()
{
    GQShaderRef shader;
    float viz_gain = k_visualization_gain;
    float offset = 0.0;
    bool use_colors = k_viz_use_colors;
    
    const GQTexture* source_tex = 0;

    if (k_viz_buffer == "Source Color")
    {
        source_tex = _colors_fbo.colorTexture(SHADING_BUFFER_NUM);
        viz_gain = 1.0f;
        use_colors = true;
    }
    else if (k_viz_buffer == "Blurred Color" && k_line == k_lines[0])
    {
        source_tex = _blurred_colors_fbo.colorTexture(1);
        viz_gain = 1.0f;
        use_colors = true;
    }
    else if (k_viz_buffer == "D1 Buffer" && k_line == k_lines[0])
    {
        source_tex = _d1_fbo.colorTexture(0);
        offset = 0.5;
    }
    else if (k_viz_buffer == "D2 Buffer" && k_line == k_lines[0])
    {
        source_tex = _d2_fbo.colorTexture(0);
        offset = 0.5;
    }
    else if (k_viz_buffer == "D3 Buffer" && k_line == k_lines[0])
    {
        source_tex = _d3_fbo.colorTexture(0);
        offset = 0.5;
    }
    else if (k_viz_buffer == "Energy Buffer" && k_line == k_lines[0])
    {
        source_tex = _energy_fbo.colorTexture(2);
    }
    else if (k_viz_buffer == "Filter Components" && k_line == k_lines[0])
    {
        source_tex = _energy_fbo.colorTexture(1);
    }
    else if (k_viz_buffer == "Lines Buffer")
    {
        source_tex = _lines_fbo.colorTexture(0);
        offset = 0.0;
        viz_gain = 1.0;
        use_colors = true;
    }
    else if (k_viz_buffer == "Sample Positions")
    {
        source_tex = _lines_fbo.colorTexture(1);
        offset = 0.0;
        use_colors = true;
    }

    shader  = GQShaderManager::bindProgram("imagesc");
    shader.setUniform1i("use_color", use_colors);
    shader.setUniform1f("gain", viz_gain);
    shader.setUniform1f("offset", offset);
    shader.bindNamedTexture("source", source_tex);
    GQDraw::drawFullScreenQuad(_colors_fbo);

}

void ImageSpaceLines::readbackSamples(xform &proj_xf, xform &mv_xf, bool read_motion, bool useDepth)
{
    __TIME_CODE_BLOCK("Image Lines Readback");

    GLint viewport[4];
    GLdouble depthRange[2];
    glGetDoublev(GL_DEPTH_RANGE, depthRange);
    glGetIntegerv(GL_VIEWPORT, viewport);

    int prevClipPathSize = _clip_path_set.size();

    if(!read_motion){
        // Computation of the motion of the previous samples (assuming camera motion only)
        _prevGeomFlow->resize(prevClipPathSize,1,3);

        for(int i=0; i<prevClipPathSize; i++){
            const vec& prevPos_world = (*_clip_path_set[i])[0]->position3D();

            vec newPos = proj_xf * mv_xf * prevPos_world;
            vec3 clipPos = ASClipPathSet::clipToViewport(vec4(newPos[0],newPos[1],newPos[2],1.f),viewport,depthRange);

            if(isnan(clipPos[0]) || isnan(clipPos[1]) || isnan(clipPos[2])){
                qWarning("Nan reprojection");
            }

            _prevGeomFlow->setPixelChannel(i,0,0,clipPos[0]);
            _prevGeomFlow->setPixelChannel(i,0,1,clipPos[1]);
            _prevGeomFlow->setPixelChannel(i,0,2,clipPos[2]);
        }
    }

    GQFloatImage max_img;
    _lines_fbo.readColorTexturef(0, max_img);
    if (read_motion) {
        _lines_fbo.readColorTexturef(1, *_motion_img);
    }

    // Read back the new samples
    static QVector<vec>  sample_positions2D;
    static QVector<vec>  sample_positions;
    static QVector<vec>  sample_motions;
    static QVector<vec2> sample_tangents;
    static QVector<float> sample_strengths;
    sample_positions2D.clear();
    sample_positions.clear();
    sample_motions.clear();
    sample_tangents.clear();
    sample_strengths.clear();

    xform inv_mvp = inv(proj_xf*mv_xf);

    for (int y = 0; y < max_img.height(); y++) {
        for (int x = 0; x < max_img.width(); x++) {
            float strength = max_img.pixel(x,y,0);
            if (strength < 10e-7)
                continue;

            float tan_x = max_img.pixel(x,y,1);
            float tan_y = max_img.pixel(x,y,2);
            float sz    = max_img.pixel(x,y,3);
            if(!useDepth || sz < -1 || sz > 1)
                sz = 0.f;
            vec projPos;
            projPos[0] = ((float)(x+0.5)/(float)(viewport[2]-1))*2-1;
            projPos[1] = ((float)(y+0.5)/(float)(viewport[3]-1))*2-1;
            projPos[2] = sz;

            vec worldPos = inv_mvp * projPos;

            if(isnan(worldPos[0]) || isnan(worldPos[1]) || isnan(worldPos[2]) ||
                    isinf(worldPos[0]) || isinf(worldPos[1]) || isinf(worldPos[2])){
                qWarning("Nan back projection");
                continue;
            }

            sample_positions2D.push_back(projPos);
            sample_positions.push_back(worldPos);
            sample_tangents.push_back(vec2(tan_x, tan_y));
            sample_strengths.push_back(strength);

            if (read_motion) {
                vec motion = vec(_motion_img->pixel(x,y,0), _motion_img->pixel(x,y,1), _motion_img->pixel(x,y,2));
                sample_motions.push_back(motion);
            }
        }
    }

    _clip_path_set.initFromPoints(sample_positions2D, sample_positions,
                                  sample_tangents, sample_strengths);

    if(read_motion){
        _geomFlow->resize(sample_motions.size(),1,3);
        for(int i=0; i<_clip_path_set.size(); i++){
            vec3 nextPos = sample_motions.at(i);
            _geomFlow->setPixel(i,0,nextPos);
        }
    }else{
        if(prevClipPathSize==0 && _clip_path_set.size()!=0 ){
            initGeomFlowBuffer();
        }
    }
}


GQTexture2D* ImageSpaceLines::offscreenTexture()
{
    _offscreen_fbo.initFullScreen(1,GQ_ATTACH_NONE,GQ_COORDS_PIXEL,GQ_FORMAT_RGBA_BYTE);

    _offscreen_fbo.bind(GQ_CLEAR_BUFFER);

    GQShaderRef shader = GQShaderManager::bindProgram("imagesc");

    shader.setUniform1i("use_color", 0);
    shader.setUniform1f("gain", 1.0);
    shader.setUniform1f("offset", 0.0);
    shader.bindNamedTexture("source", _lines_fbo.colorTexture(0));

    GQDraw::drawFullScreenQuad(_colors_fbo);

    _offscreen_fbo.unbind();

    return _offscreen_fbo.colorTexture(0);
}

GQFloatImage* ImageSpaceLines::geometricFlowBuffer() {
    return _prevGeomFlow;
}

GQFloatImage* ImageSpaceLines::denseFlowBuffer() {
    return _prev_motion_img;
}

void ImageSpaceLines::nextGeomFlowBuffer() {
    if(!_prevGeomFlow)
        delete _prevGeomFlow;
    _prevGeomFlow = _geomFlow;
    _geomFlow = new GQFloatImage();

    if(_prev_motion_img)
        delete _prev_motion_img;
    _prev_motion_img = _motion_img;
    _motion_img = new GQFloatImage();
}

void ImageSpaceLines::initGeomFlowBuffer() {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    _prevGeomFlow->resize(_clip_path_set.size(),1,3);
    for(int i=0; i<_clip_path_set.size(); i++){
        vec3 newPos = (*_clip_path_set[i])[0]->position();
        _prevGeomFlow->setPixel(i,0,newPos);
    }
    _prev_motion_img->resize(viewport[2],viewport[3],3);
    float zero[3] = {0.f,0.f,0.f};
    for(int x=0; x<viewport[2]; ++x)
        for(int y=0; y<viewport[3]; ++y)
            _prev_motion_img->setPixel(x,y,zero);
}
