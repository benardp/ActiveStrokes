/*****************************************************************************\

ASClipPath.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "ASClipPath.h"

#include "GQDraw.h"
#include "Stats.h"
#include "GQImage.h"
#include "DialsAndKnobs.h"

static dkFloat k_tangent_length("Image Lines->Viz.->Tangent Length", 0, 0, 1000, 0.1);
       dkFloat k_visibilityTh("Contours->Topology->Visibility th.", 0.75);
extern dkBool  k_recordStats;

void ASClipVertex::computeLambda() {
    int s = 10;
    float mx=0.f, my=0.f, mx2=0.f, my2=0.f, mxy=0.f;

    int nbSamplesForward, nbSamplesBackward;
    if(_index<s)
        nbSamplesBackward=_index;
    else
        nbSamplesBackward=s;
    if(_clipPath->size()-_index < s+1)
        nbSamplesForward=_clipPath->size()-_index-1;
    else
        nbSamplesForward=s;

    int nbSamples = nbSamplesBackward + nbSamplesForward + 1;

    for(int i=-nbSamplesBackward; i<=nbSamplesForward; ++i){
        vec2 pos = (*_clipPath)[_index+i]->position2D();
        mx  += pos[0];
        my  += pos[1];
        mx2 += pos[0]*pos[0];
        my2 += pos[1]*pos[1];
        mxy += pos[0]*pos[1];
    }
    mx  /= float(nbSamples);
    my  /= float(nbSamples);
    mx2 /= float(nbSamples);
    my2 /= float(nbSamples);
    mxy /= float(nbSamples);

    float m11 = mx2 - mx*mx;
    float m22 = my2 - my*my;
    float m12 = mxy - mx*my;
    _lambda = (m11 + m22 - sqrt((m11 - m22)*(m11 - m22) + 4*m12*m12))/2.0;
}

void ASClipPath::computeLambda() {
    foreach(ASClipVertex *v,_clipVertices){
        v->computeLambda();
    }
}

ASClipPath::ASClipPath() {}

ASClipPath::~ASClipPath() {
    qDeleteAll(_clipVertices);
    _clipVertices.clear();
    _endPointsIdx.clear();
}

inline vec2 indexToCoordinate( float index, float buf_size )
{
    return vec2(fmod(index,buf_size), floor(index / buf_size) );
}

inline float clamp(float value, float valMin, float valMax)
{
    return std::min(std::max(valMin,value),valMax);
}

void ASClipPath::addSegment(const GQFloatImage &samples, const GQFloatImage &visibility,
                             float offset, float numSamples, int atlasWrapWidth)
{
    vec2 coord = indexToCoordinate(offset, atlasWrapWidth);
    for(int i=0; i<numSamples; ++i)
    {
        float vis = visibility.pixel(coord[0],coord[1],3);

        vec3 pos;
        pos[0] = samples.pixel(coord[0],coord[1],0);
        pos[1] = samples.pixel(coord[0],coord[1],1);
        pos[2] = samples.pixel(coord[0],coord[1],2);
        float strength = samples.pixel(coord[0],coord[1],3);

        ASClipVertex *clipVertex = new ASClipVertex(pos,coord,_clipVertices.size(),this,vis,strength);
        _clipVertices<<clipVertex;

        coord[0] += 1;
    }
}

float ASClipPath::length() {
    float length = 0.0;
    for(int i=1; i<_clipVertices.size(); ++i){
        length += len(_clipVertices.at(i)->position2D() - _clipVertices.at(i-1)->position2D());
    }
    return length;
}

void ASClipPath::draw(const GLdouble *mv, const GLdouble *proj, const GLint *vp, bool drawTangent) {

    foreach(ASClipVertex *v,_clipVertices){

        if(v->visibility()>k_visibilityTh){
            if(v->isUncovered()){
                glColor3f(1.0,0.0,0.0);
            }else{
                glColor3f(0.0,0.0,0.0);
            }
        }else{
            glColor3f(0.8,0.8,0.8);
        }

        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);

        const vec& p = v->position();
        GLdouble pw[3];
        gluUnProject(p[0], p[1], p[2], mv, proj, vp, &pw[0], &pw[1], &pw[2]);

        glBegin(GL_POINTS);
        glVertex3dv(pw);
        glEnd();

        if(drawTangent && k_tangent_length>0.0){

            glColor3f(52.0/255.0,101.0/255.0,164.0/255.0);
            glBegin(GL_LINES);
            GLdouble s[3], f[3], b[3];
            s[0]=p[0];
            s[1]=p[1];
            s[2]=p[2];

            vec2 st = v->tangent();
            st *= k_tangent_length;
            //gluProject(wp[0], wp[1], wp[2], mv, proj, vp, &s[0], &s[1], &s[2]);

            gluUnProject(s[0]+st[0], s[1]+st[1], s[2], mv, proj, vp, &f[0], &f[1], &f[2]);
            gluUnProject(s[0]-st[0], s[1]-st[1], s[2], mv, proj, vp, &b[0], &b[1], &b[2]);

            glVertex3dv(f);
            glVertex3dv(b);
            glEnd();
        }

        glEnable(GL_DEPTH_TEST);
    }
}

void ASClipPath::draw(bool drawTangent) {
    foreach(ASClipVertex *v,_clipVertices){
        if(v->isUncovered()){
            if(v->visibility()>k_visibilityTh)
                glColor3f(1.0,0.0,0.0);
            else
                glColor3f(0.95,0.95,0.95);
        }else{
            if(v->visibility()>k_visibilityTh)
                glColor3f(0.0,0.0,0.0);
            else
                continue;
        }

        vec2 pos = v->position2D();
        glBegin(GL_POINTS);
        glVertex2fv(pos);
        glEnd();

        if(drawTangent && k_tangent_length>0.0){

            glColor3f(0,1,0);
            glBegin(GL_LINES);

            vec2 st = v->tangent();
            st *= k_tangent_length;

            glVertex2fv(pos+st);
            glVertex2fv(pos-st);
            glEnd();
        }
    }
}

ASClipPath::ASClipPath(ASClipPath &p, int idx) {
    for(int i=p.size()-1; i>idx; --i){
        ASClipVertex* v = p._clipVertices.at(i);
        v->setClipPath(this);
        v->setIndex(i-idx-1);
        _clipVertices<<v;
        p._clipVertices.removeAt(i);
    }
}

ASClipPathSet::~ASClipPathSet() {
    qDeleteAll(_paths);
    _paths.clear();
    _paths_map.clear();
}

vec ASClipPathSet::clipToViewport(const vec4& v, const GLint viewport[4], const GLdouble depthRange[2])
{
    vec v_out;
    v_out[0] = (v[0] / v[3] + 1.0) * 0.5 * viewport[2];
    v_out[1] = (v[1] / v[3] + 1.0) * 0.5 * viewport[3];
    v_out[2] = (depthRange[1] - depthRange[0])/2.0 * v[2] / v[3] + (depthRange[1] + depthRange[0])/2.0;
    return v_out;
}

vec ASClipPathSet::clipToViewport(const vec4& v) const
{
    return clipToViewport(v,_viewport,_depthRange);
}

void ASClipPathSet::addPath( ASClipPath* path ) {
    _paths.append(path);
}

void ASClipPathSet::draw(const GLdouble *model, const GLdouble *proj, const GLint *view, bool drawTangent) const {

    glLineWidth(2);
    glPointSize(1);

    for(int i=0; i<_paths.size(); i++){
        ASClipPath* p = _paths.at(i);
        p->draw(model,proj,view,drawTangent);
    }
}

void ASClipPathSet::draw(bool drawTangent, int width, int height) const {
    glLineWidth(1);
    glPointSize(1);

    GQDraw::startScreenCoordinatesSystem(true, width, height);
    for(int i=0; i<_paths.size(); i++){
        ASClipPath* p = _paths.at(i);
        p->draw(drawTangent);
    }
    GQDraw::stopScreenCoordinatesSystem();
}

void ASClipPathSet::initFromPoints(const QVector<vec>& sample_positions2D,
                                   const QVector<vec>& sample_positions,
                                   const QVector<vec2>& sample_tangents,
                                   const QVector<float>& sample_strengths)
{
    glGetDoublev(GL_DEPTH_RANGE, _depthRange);
    glGetIntegerv (GL_VIEWPORT, _viewport);

    qDeleteAll(_paths);
    _paths.clear();

    for(int i=0; i < sample_positions.size(); ++i){
        ASClipPath* p = new ASClipPath();
        vec3 projPos = sample_positions2D.at(i);
        vec3 clipPos = clipToViewport(vec4(projPos[0],projPos[1],projPos[2],1.f));
        ASClipVertex* cv = new ASClipVertex(clipPos, sample_positions.at(i),
                                              vec2(i,0), 0, p, 1.0, sample_strengths.at(i));
        cv->setTangent(sample_tangents.at(i));
        p->addVertex(cv);
        _paths << p;
    }
}
