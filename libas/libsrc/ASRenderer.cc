/*****************************************************************************\

ASRenderer.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "ASRenderer.h"

#include "NPRGLDraw.h"
#include "GQDraw.h"

static dkFloat k_lineWidth("Contours->Draw->Thickness", 1.0,0.1,100.0,1.0);
static dkBool  k_drawVerticies("Contours->Draw->Vertices", true);

extern dkFilename   k_texture;
extern dkFilename   k_offsets;
extern dkStringList k_fittingMode;
       dkFloat k_lengthScale("Style->Main->Length scale", 1.0, 0.0, 20.0, 0.5);
static dkFloat k_offsets_scale("Style->Main->Offsets scale",1.0, 0.0, 20.0, 0.5);
extern dkFloat k_targetLength;
extern dkFloat k_penWidth;
extern dkFloat k_taperStart;
extern dkFloat k_taperEnd;
extern dkFloat k_overshootMax;
extern dkFloat k_overshootMin;
extern dkFloat k_overshootTh;

ASRenderer::ASRenderer()
{
    _vbo_initialized = false;
}

void ASRenderer::init(ASSnakes* snakes)
{
    _snakes = snakes;
    _pathRenderer.init();
    _globalStyle.loadDefaults();
}

void ASRenderer::drawClosestEdge() const
{
    glLineWidth(1);

    for(int i=0; i<_snakes->nbContours(); i++){
        QColor qline_col =_snakes->at(i)->debugColor();
        vec3 color = vec3(qline_col.red(), qline_col.green(), qline_col.blue());
        color /= 255.0;
        glColor3fv(color);

        glBegin(GL_LINES);//POINTS
        _snakes->at(i)->drawClosestEdge();
        glEnd();
    }
}

void ASRenderer::drawClosestEdge3D(const GLdouble *model, const GLdouble *proj, const GLint *view) const
{
    glLineWidth(1);

    for(int i=0; i<_snakes->nbContours(); i++){
        QColor qline_col =_snakes->at(i)->debugColor();
        vec3 color = vec3(qline_col.red(), qline_col.green(), qline_col.blue());
        color /= 255.0;
        glColor3fv(color);

        glBegin(GL_LINES);
        _snakes->at(i)->drawClosestEdge3D(model,proj,view);
        glEnd();
    }
}

void ASRenderer::draw() const
{
    GQDraw::clearGLState();

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);

    glLineWidth(k_lineWidth);
    vec3 color = vec3(1.0,1.0,1.0);

    for(int i=0; i<_snakes->nbContours(); i++){
        QColor qline_col =_snakes->at(i)->debugColor();
        color = vec3(qline_col.red(), qline_col.green(), qline_col.blue());
        color /= 255.0;

        glBegin(GL_LINE_STRIP);
        _snakes->at(i)->draw(color);
        glEnd();

        if(k_drawVerticies){
            glPointSize(k_lineWidth*4.0);
            glBegin(GL_POINTS);
            _snakes->at(i)->draw(color);
            glEnd();
        }
    }
    glDisable(GL_LINE_SMOOTH);
}

void ASRenderer::drawBrushPaths(bool black) const
{
    GQDraw::clearGLState();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);

    for(int i=0; i<_snakes->nbContours(); ++i)
        _snakes->at(i)->drawBrushPaths(black);

    glDisable(GL_LINE_SMOOTH);
}

void ASRenderer::updateStyle()
{
    _globalStyle.penStyle("Base Style")->setTexture(k_texture);
    _globalStyle.penStyle("Base Style")->setOffsetTexture(k_offsets);
    _globalStyle.penStyle("Base Style")->setUserOffsetScale(k_offsets_scale);
    _globalStyle.penStyle("Base Style")->setStripWidth(k_penWidth);
    _globalStyle.penStyle("Base Style")->setUserOffsetScale(k_offsets_scale);
}

void ASRenderer::renderStrokes()
{
    makePathVertexFBO();
    reportGLError();
    updateStyle();

    GQShaderRef shader = GQShaderManager::bindProgram("stroke_render_snakes");

    _pathRenderer.setUniformPenStyleParams(shader, _globalStyle);
    _pathRenderer.setUniformOffsetParams(shader, _globalStyle);

    NPRGLDraw::setUniformViewParams(shader);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    GLfloat viewport[4];
    glGetFloatv(GL_VIEWPORT, viewport);
    gluOrtho2D(viewport[0], viewport[2], viewport[1], viewport[3]);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    shader.bindNamedTexture("path_start_end_ptrs",
                            _path_verts_fbo.colorTexture(BRUSHPATH_START_END_ID));
    shader.bindNamedTexture("clip_vert_0_buffer",
                            _path_verts_fbo.colorTexture(BRUSHPATH_VERTEX_0_ID));
    shader.bindNamedTexture("clip_vert_1_buffer",
                            _path_verts_fbo.colorTexture(BRUSHPATH_VERTEX_1_ID));

    if(shader.uniformLocation("offset_buffer")>=0)
        shader.bindNamedTexture("offset_buffer",
                                _path_verts_fbo.colorTexture(BRUSHPATH_OFFSET_ID));
    if(shader.uniformLocation("param_buffer")>=0)
        shader.bindNamedTexture("param_buffer",
                                _path_verts_fbo.colorTexture(BRUSHPATH_PARAM_ID));

    shader.setUniform1f("clip_buffer_width", _clip_buf_width);

    _pathRenderer.bindQuadVerticesVBO(shader);

    int segments_remaining = _total_segments;
    int row = 0;
    while(segments_remaining > 0)
    {
        int count = min(segments_remaining, _clip_buf_width);
        shader.setUniform1f("row", row);
        reportGLError();
        glDrawArrays(GL_POINTS, 0, count);
        reportGLError();
        segments_remaining -= count;
        row++;
    }

    _pathRenderer.unbindQuadVerticesVBO();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    NPRGLDraw::handleGLError(__FILE__,__LINE__);
}

void ASRenderer::drawSpine()
{
    NPRGLDraw::clearGLState();

    GQShaderRef shader = GQShaderManager::bindProgram("stroke_render_spine");

    shader.setUniform1f("check_at_spine", 1);
    _pathRenderer.setUniformPenStyleParams(shader, _globalStyle);
    _pathRenderer.setUniformOffsetParams(shader, _globalStyle);
    shader.setUniform1f("pen_width", k_penWidth);

    NPRGLDraw::setUniformViewParams(shader);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    GLfloat viewport[4];
    glGetFloatv(GL_VIEWPORT, viewport);
    gluOrtho2D(viewport[0], viewport[2], viewport[1], viewport[3]);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    for(int c=0; c<_snakes->nbContours(); ++c){
        ASContour* contour = _snakes->at(c);
        for(int idx=0; idx<contour->nbBrushPaths(); ++idx){
            ASBrushPath* brushPath = contour->brushPath(idx);

            glBegin(GL_LINE_STRIP);
            for(int j=0; j<brushPath->nbVertices()-1; j+=2){
                glVertex2fv(brushPath->at(j)->position());
                glVertex2fv(brushPath->at(j+1)->position());
            }
            if(brushPath->isClosed()){
                glVertex2fv(brushPath->last()->position());
                glVertex2fv(brushPath->first()->position());
            }
            glEnd();
        }
    }
    reportGLError();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

typedef struct {
    vec4 vertex_0;
    vec4 vertex_1;
    vec4 path_start_end;
    vec4 offset;
    vec4 param;
} Segment;

bool ASRenderer::makePathVertexFBO()
{
    QList<Segment> segments;

    // Load the snakes into the images.
    int segment_counter = 0;
    float num_samples_offset = 0.0f;
    float arc_length_offset = 0.0f;

    const NPRPenStyle* vis_focus = _globalStyle.penStyle("Base Style");
    float texture_length=16;
    if(vis_focus->texture()){
        const GQTexture* vis_focus_tex = vis_focus->texture();
        texture_length = vis_focus_tex->width();
    }
    float length_scale = k_lengthScale.value();

    for (int i = 0; i < _snakes->nbContours(); i++){
        ASContour* contour = _snakes->at(i);

        contour->cleanBrushPath();

        int nbBrushPaths = contour->nbBrushPaths();

        for(int j=0; j<nbBrushPaths; j++) {
            for(int k=0; k<contour->brushPath(j)->nbVertices(); k++){
                contour->brushPath(j)->at(k)->savePenWidth();
                contour->brushPath(j)->at(k)->setPenWidth(1);
            }
        }
        if(k_taperStart>0 || k_taperEnd>0)
            contour->updateTaper();

        for(int idx=0; idx<nbBrushPaths; ++idx){
            ASBrushPath* brushPath = contour->brushPath(idx);
            brushPath->computeArcLength();

            int nverts = brushPath->nbVertices();
            if(brushPath->isClosed())
                nverts+=1;

            int path_start = segment_counter;
            int path_end = segment_counter + nverts - 2;

            vec2 path_start_end_vec(path_start, path_end);

            //Per BP linear parametrization

            double slope = brushPath->slope();
            if(slope==0)
                slope = 0.001;
            double d = fabs(slope) * length_scale * texture_length;
            double t = ceil(log2(1.0/d));
            double l = pow(2.0,-t)/d;
            double fact = pow(2.0,t);

            double level = 2.0 * l - 1.0f;
            brushPath->setLevel(level);
            brushPath->setFact(fact);

            float bpLength = brushPath->last()->arcLength();
            float overshoot = 0.0;
            if(bpLength > k_targetLength){
                overshoot = k_overshootMax;
            }else if(bpLength < k_overshootTh){
                overshoot = k_overshootMin * bpLength / k_overshootTh;
            }else{
                overshoot = (k_overshootMax - k_overshootMin)*(bpLength - k_overshootTh)/(k_targetLength - k_overshootTh) + k_overshootMin;
            }
            ACBrushpathStyleMode mode = (ACBrushpathStyleMode) k_fittingMode.index();

            if(overshoot > 0.0)
            {
                if (mode == AS_BRUSHPATH_ARC) {
                    float sweepingAngle = -brushPath->fittingPara().sweepingAngle;
                    vec2 center = vec2(brushPath->fittingPara().A, brushPath->fittingPara().B);
                    float radius = brushPath->fittingPara().C;
                    assert(sweepingAngle < M_PI);
                    int numOvershootSegment = (int)overshoot;

                    vec2 v1 = brushPath->first()->position();
                    vec2 dir = v1 - center;
                    vec2 nDir, nPos, pPos;

                    float arcLength;
                    float length = fabs(sweepingAngle * radius);
                    float pen_width = brushPath->first()->penWidth();

                    for (int ido = numOvershootSegment; ido >= 1; ido--) {
                        nDir[0] = cos(ido * sweepingAngle) * dir[0] - sin(ido * sweepingAngle) * dir[1];
                        nDir[1] = sin(ido * sweepingAngle) * dir[0] + cos(ido * sweepingAngle) * dir[1];
                        nPos = nDir + center;

                        if (ido != numOvershootSegment) {
                            arcLength = - length * ido;
                            arc_length_offset += length;
                            num_samples_offset += length / 2.0;
                            vec4 s  = vec4(num_samples_offset,arc_length_offset,length / 2.0,length);

                            Segment seg;

                            if(k_taperStart>0){
                                seg.vertex_0 = vec4(pPos[0], pPos[1], brushPath->first()->alpha(), pen_width - pen_width * float(ido+1)/float(numOvershootSegment));
                                seg.vertex_1 = vec4(nPos[0], nPos[1], brushPath->first()->alpha(), pen_width - pen_width * float(ido)/float(numOvershootSegment));
                            }else{
                                seg.vertex_0 = vec4(pPos[0], pPos[1], brushPath->first()->alpha(), pen_width);
                                seg.vertex_1 = vec4(nPos[0], nPos[1], brushPath->first()->alpha(), pen_width);
                            }
                            seg.path_start_end = vec4(path_start_end_vec[0], path_start_end_vec[1], 0, brushPath->slope());
                            seg.offset = s;

                            vec4 param(brushPath->param(- length * (ido+1))*fact,brushPath->param(arcLength)*fact,level,level);
                            seg.param = param;

                            segments<<seg;
                            segment_counter++;
                        }
                        pPos = nPos;
                    }
                    nDir[0] = cos(sweepingAngle) * dir[0] - sin(sweepingAngle) * dir[1];
                    nDir[1] = sin(sweepingAngle) * dir[0] + cos(sweepingAngle) * dir[1];
                    pPos = nDir + center;

                    arc_length_offset += length;
                    num_samples_offset += length / 2.0;
                    vec4 s  = vec4(num_samples_offset,arc_length_offset,length / 2.0,length);
                    Segment seg;

                    if(k_taperStart>0){
                        seg.vertex_0 = vec4(pPos[0], pPos[1], brushPath->first()->alpha(), pen_width-pen_width/float(numOvershootSegment));
                        seg.vertex_1 = vec4(v1[0], v1[1], brushPath->first()->alpha(), pen_width);
                    }else{
                        seg.vertex_0 = vec4(pPos[0], pPos[1], brushPath->first()->alpha(), pen_width);
                        seg.vertex_1 = vec4(v1[0], v1[1], brushPath->first()->alpha(), pen_width);
                    }
                    seg.path_start_end = vec4(path_start_end_vec[0], path_start_end_vec[1], 0, brushPath->slope());
                    seg.offset = s;

                    vec4 param(brushPath->param(- length)*fact,brushPath->param(0)*fact,level,level);
                    seg.param = param;

                    segments<<seg;
                    segment_counter++;

                }
                else if (mode == AS_BRUSHPATH_LINE)
                {
                    vec2 v1 = brushPath->first()->position();
                    brushPath->computeTangent();
                    vec2 overshootDir = (brushPath->first()->position() - brushPath->last()->position());
                    normalize(overshootDir);
                    vec2 v0 = v1 + overshoot * overshootDir;

                    float arc_length = overshoot;
                    float num_samples = arc_length / 2.0;
                    arc_length_offset += arc_length;
                    num_samples_offset += num_samples;
                    vec4 s  = vec4(num_samples_offset,arc_length_offset,num_samples,arc_length);

                    Segment seg;
                    if(k_taperStart > 0.0f)
                        seg.vertex_0 = vec4(v0[0], v0[1], brushPath->first()->alpha(), 0);
                    else
                        seg.vertex_0 = vec4(v0[0], v0[1], brushPath->first()->alpha(), brushPath->first()->penWidth());
                    seg.vertex_1 = vec4(v1[0], v1[1], brushPath->first()->alpha(), brushPath->first()->penWidth());
                    seg.path_start_end = vec4(path_start_end_vec[0], path_start_end_vec[1], 0, brushPath->slope());
                    seg.offset = s;

                    vec4 param(brushPath->param(-overshoot)*fact,brushPath->param(0)*fact,level,level);
                    seg.param = param;

                    segments<<seg;
                    segment_counter++;
                }
            }

            for (int j = 0; j<brushPath->nbVertices()-1; j++){

                vec2 v0 = brushPath->at(j)->position();
                vec2 v1 = brushPath->at(j+1)->position();

                double arc_length = brushPath->at(j+1)->length();
                if(arc_length<=1e-6)
                    continue;
                double num_samples = arc_length / 2.0;
                arc_length_offset += arc_length;
                num_samples_offset += num_samples;

                Segment seg;
                seg.vertex_0 = vec4(v0[0], v0[1], brushPath->at(j)->alpha(), brushPath->at(j)->penWidth());
                seg.vertex_1 = vec4(v1[0], v1[1], brushPath->at(j+1)->alpha(), brushPath->at(j+1)->penWidth());
                seg.path_start_end = vec4(path_start_end_vec[0], path_start_end_vec[1], 0, brushPath->slope());
                seg.offset = vec4(num_samples_offset,arc_length_offset,num_samples,arc_length);
                seg.param = vec4(brushPath->param(j)*fact,brushPath->param(j+1)*fact,level,level);

                segments<<seg;

                segment_counter++;
            }

            if(brushPath->isClosed()){
                vec2 v0 = brushPath->last()->position();
                vec2 v1 = brushPath->first()->position();

                double arc_length = brushPath->last()->arcLength() + dist(v0,v1);
                if(arc_length<=1e-6)
                    continue;
                double num_samples = arc_length / 2.0;
                arc_length_offset += arc_length;
                num_samples_offset += num_samples;
                vec4 s  = vec4(num_samples_offset,arc_length_offset,num_samples,arc_length);

                Segment seg;
                seg.vertex_0 = vec4(v0[0], v0[1], brushPath->last()->alpha(), brushPath->last()->penWidth());
                seg.vertex_1 = vec4(v1[0], v1[1], brushPath->last()->alpha(), brushPath->last()->penWidth());
                seg.path_start_end = vec4(path_start_end_vec[0], path_start_end_vec[1], 0, brushPath->slope());
                seg.offset = s;

                vec4 param(brushPath->param(brushPath->last()->length())*fact,brushPath->param(arc_length)*fact,level,level);
                seg.param = param;

                segments<<seg;

                segment_counter++;
            }

            if(overshoot > 0.0)
            {
                if (mode == AS_BRUSHPATH_ARC) {
                    float sweepingAngle = brushPath->fittingPara().sweepingAngle;
                    vec2 center = vec2(brushPath->fittingPara().A, brushPath->fittingPara().B);
                    float radius = brushPath->fittingPara().C;
                    int numOvershootSegment = (int)overshoot;

                    vec2 v0 = brushPath->last()->position();
                    vec2 dir = v0 - center;
                    vec2 nDir, nPos, pPos;

                    float arcLength;
                    float length = fabs(sweepingAngle * radius);

                    nDir[0] = cos(sweepingAngle) * dir[0] - sin(sweepingAngle) * dir[1];
                    nDir[1] = sin(sweepingAngle) * dir[0] + cos(sweepingAngle) * dir[1];
                    nPos = nDir + center;

                    arc_length_offset += length;
                    num_samples_offset += length / 2.0;
                    vec4 s  = vec4(num_samples_offset,arc_length_offset,length / 2.0,length);
                    Segment seg;

                    float pen_width = brushPath->last()->penWidth();

                    if(k_taperEnd>0){
                        seg.vertex_0 = vec4(v0[0], v0[1], brushPath->last()->alpha(), pen_width);
                        seg.vertex_1 = vec4(nPos[0], nPos[1], brushPath->last()->alpha(), pen_width-pen_width/float(numOvershootSegment));
                    }else{
                        seg.vertex_0 = vec4(v0[0], v0[1], brushPath->last()->alpha(), pen_width);
                        seg.vertex_1 = vec4(nPos[0], nPos[1], brushPath->last()->alpha(), pen_width);
                    }

                    seg.path_start_end = vec4(path_start_end_vec[0], path_start_end_vec[1], 0, brushPath->slope());
                    seg.offset = s;

                    vec4 param(brushPath->param(brushPath->last()->length())*fact,brushPath->param(brushPath->last()->length() + length)*fact,level,level);
                    seg.param = param;

                    segments<<seg;
                    segment_counter++;

                    for (int ido = 1; ido <= numOvershootSegment; ido++)
                    {
                        nDir[0] = cos(ido * sweepingAngle) * dir[0] - sin(ido * sweepingAngle) * dir[1];
                        nDir[1] = sin(ido * sweepingAngle) * dir[0] + cos(ido * sweepingAngle) * dir[1];
                        nPos = nDir + center;

                        if (ido != 1)
                        {
                            arcLength = length * ido;
                            arc_length_offset += length;
                            num_samples_offset += length / 2.0;
                            vec4 s  = vec4(num_samples_offset,arc_length_offset,length / 2.0,length);

                            Segment seg;

                            if(k_taperEnd>0){
                                seg.vertex_0 = vec4(pPos[0], pPos[1], brushPath->last()->alpha(), pen_width-pen_width*float(ido-1)/float(numOvershootSegment));
                                seg.vertex_1 = vec4(nPos[0], nPos[1], brushPath->last()->alpha(), pen_width-pen_width*float(ido)/float(numOvershootSegment));
                            }else{
                                seg.vertex_0 = vec4(pPos[0], pPos[1], brushPath->last()->alpha(), pen_width);
                                seg.vertex_1 = vec4(nPos[0], nPos[1], brushPath->last()->alpha(), pen_width);
                            }

                            seg.path_start_end = vec4(path_start_end_vec[0], path_start_end_vec[1], 0, brushPath->slope());
                            seg.offset = s;

                            vec4 param(brushPath->param(brushPath->last()->length() + (ido-1) * length)*fact,brushPath->param(brushPath->last()->length() + arcLength)*fact,level,level);
                            seg.param = param;

                            segments<<seg;
                            segment_counter++;
                        }
                        pPos = nPos;
                    }

                }
                else if (mode == AS_BRUSHPATH_LINE)
                {
                    vec2 v0 = brushPath->last()->position();
                    vec2 overshootDir = (brushPath->last()->position() - brushPath->first()->position());
                    normalize(overshootDir);
                    vec2 v1 = v0 + overshoot * overshootDir;

                    float arc_length = overshoot;
                    float num_samples = arc_length / 2.0;
                    arc_length_offset += arc_length;
                    num_samples_offset += num_samples;
                    vec4 s  = vec4(num_samples_offset,arc_length_offset,num_samples,arc_length);

                    Segment seg;

                    seg.vertex_0 = vec4(v0[0], v0[1], brushPath->last()->alpha(), brushPath->last()->penWidth());
                    if(k_taperEnd>0)
                        seg.vertex_1 = vec4(v1[0], v1[1], brushPath->last()->alpha(), 0);
                    else
                        seg.vertex_1 = vec4(v1[0], v1[1], brushPath->last()->alpha(), brushPath->last()->penWidth());
                    seg.path_start_end = vec4(path_start_end_vec[0], path_start_end_vec[1], 0, brushPath->slope());
                    seg.offset = s;

                    vec4 param(brushPath->param(brushPath->last()->length())*fact,brushPath->param(brushPath->last()->length()+overshoot)*fact,level,level);
                    seg.param = param;

                    segments<<seg;

                    segment_counter++;
                }
            }

            if(path_end != segment_counter-1){
                path_end = segment_counter-1;

                for(int i=path_start; i<path_end; ++i){
                    segments[i].path_start_end[1]=path_end;
                }
            }
        }
    }

    // Count the total number of segments.
    _total_segments = segments.count();

    _clip_buf_width = GQFramebufferObject::maxFramebufferSize();
    int clip_buf_height = ceil(((double)_total_segments) / (double)_clip_buf_width);
    clip_buf_height = max(clip_buf_height,1);
    if (clip_buf_height > _clip_buf_width)
    {
        qCritical("Too many segments for clip buffer (%d, max is %d)\n",
                  _total_segments, _clip_buf_width*_clip_buf_width);
        return false;
    }
    int max_segments = _clip_buf_width*clip_buf_height;

    _path_verts_fbo.initGL(GL_TEXTURE_RECTANGLE_ARB, GL_RGBA_FLOAT32_ATI,
                           GQ_ATTACH_NONE, NUM_BRUSHPATH_BUFFERS,
                           _clip_buf_width, clip_buf_height );

    GQFloatImage vertex_0_img, vertex_1_img, path_start_end_img, offset_img, param_img;
    vertex_0_img.resize(_clip_buf_width, clip_buf_height, 4);
    vertex_1_img.resize(_clip_buf_width, clip_buf_height, 4);
    path_start_end_img.resize(_clip_buf_width, clip_buf_height, 4);
    offset_img.resize(_clip_buf_width, clip_buf_height, 4);
    param_img.resize(_clip_buf_width, clip_buf_height, 4);

    float* vertex_0_buf = vertex_0_img.raster();
    float* vertex_1_buf = vertex_1_img.raster();
    float* path_start_end_buf = path_start_end_img.raster();
    float* offset_buf = offset_img.raster();
    float* param_buf = param_img.raster();

    for (int i = 0; i < max_segments*4; i++){
        vertex_0_buf[i] = 0.0f;
        vertex_1_buf[i] = 0.0f;
        path_start_end_buf[i] = 0.0f;
        offset_buf[i] = 0.0f;
        param_buf[i] = 0.0f;
    }

    for (int i = 0; i < _total_segments; i++){
        Segment seg = segments.at(i);

        vertex_0_buf[i*4]   = seg.vertex_0[0];
        vertex_0_buf[i*4+1] = seg.vertex_0[1];
        vertex_0_buf[i*4+2] = seg.vertex_0[2];
        vertex_0_buf[i*4+3] = seg.vertex_0[3];

        vertex_1_buf[i*4]   = seg.vertex_1[0];
        vertex_1_buf[i*4+1] = seg.vertex_1[1];
        vertex_1_buf[i*4+2] = seg.vertex_1[2];
        vertex_1_buf[i*4+3] = seg.vertex_1[3];

        path_start_end_buf[i*4]   = seg.path_start_end[0];
        path_start_end_buf[i*4+1] = seg.path_start_end[1];
        path_start_end_buf[i*4+2] = seg.path_start_end[2];
        path_start_end_buf[i*4+3] = seg.path_start_end[3];

        offset_buf[i*4]   = seg.offset[0];
        offset_buf[i*4+1] = seg.offset[1];
        offset_buf[i*4+2] = seg.offset[2];
        offset_buf[i*4+3] = seg.offset[3];

        param_buf[i*4]   = seg.param[0];
        param_buf[i*4+1] = seg.param[1];
        param_buf[i*4+2] = seg.param[2];
        param_buf[i*4+3] = seg.param[3];
    }

    _path_verts_fbo.loadSubColorTexturef(BRUSHPATH_VERTEX_0_ID, vertex_0_img);
    _path_verts_fbo.loadSubColorTexturef(BRUSHPATH_VERTEX_1_ID, vertex_1_img);
    _path_verts_fbo.loadSubColorTexturef(BRUSHPATH_START_END_ID, path_start_end_img);
    _path_verts_fbo.loadSubColorTexturef(BRUSHPATH_OFFSET_ID, offset_img);
    _path_verts_fbo.loadSubColorTexturef(BRUSHPATH_PARAM_ID, param_img);
    _path_verts_fbo.setTextureFilter(GL_NEAREST, GL_NEAREST);
    _path_verts_fbo.setTextureWrap(GL_CLAMP, GL_CLAMP);

    return true;
}
