/*****************************************************************************\

ASClipPath.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef ClipPath_H_
#define ClipPath_H_

#include <QPair>
#include <QMap>
#include <QList>

#include "GQInclude.h"
#include "GQImage.h"

class ASClipPath;

class ASClipVertex {
public:
    ASClipVertex(vec3 v, vec2 coord, int idx, ASClipPath *p, float vis, float s) :
            _index(idx), _clipPath(p), _v(v), _atlasCoord(coord), _covered(false), _visibility(vis), _strength(s) {}
    ASClipVertex(vec3 v, vec3 v3D, vec2 coord, int idx, ASClipPath *p, float vis, float s) :
            _index(idx), _clipPath(p), _v(v), _v3D(v3D), _atlasCoord(coord), _covered(false), _visibility(vis), _strength(s) {}
    ~ASClipVertex() {}

    int index() const { return _index; }

    ASClipPath* clipPath() const { return _clipPath; }

    vec3 position() const { return _v; }
    vec2 position2D() const { return vec2(_v[0],_v[1]); }
    vec3 position3D() const { return _v3D; }
    vec2 atlasCoord() const { return _atlasCoord; }

    bool isCovered() const { return _covered; }
    bool isUncovered() const { return !_covered; }
    void setCovered() { _covered = true; }
    void setUncovered() { _covered = false; }

    float visibility() const { return _visibility; }

    void setClipPath(ASClipPath* p) { _clipPath = p; }
    void setIndex(int idx) { _index = idx; }

    void  computeLambda();
    float lambda() { return _lambda; }

    vec2 tangent() { return _tangent; }
    void setTangent(vec2 t) { _tangent=t; }

    float strength() const { return _strength;}
    void setStrength(float s) { _strength = s; }

private:
    int _index;
    ASClipPath* _clipPath;

    vec3 _v;
    vec3 _v3D;
    vec2 _atlasCoord;

    bool _covered;

    float _visibility;

    float _lambda;

    vec2 _tangent;

    float _strength;
};

class ASClipPath {
public:
    ASClipPath();
    ASClipPath(ASClipPath &p, int idx);
    ~ASClipPath();

    void addSegment(const GQFloatImage &samples, const GQFloatImage &visibility, float offset, float numSamples, int atlasWrapWidth);
    void addVertex(ASClipVertex* v) { _clipVertices<<v; }

    ASClipVertex* operator[]( int i ) { return _clipVertices[i]; }
    const ASClipVertex* operator[]( int i ) const { return _clipVertices[i]; }

    int size() const { return _clipVertices.size(); }

    float length();

    QListIterator<int> endPointsIdxIterator() { return QListIterator<int>(_endPointsIdx); }

    void draw(const GLdouble *mv, const GLdouble *proj, const GLint *vp, bool drawTangent);
    void draw(bool drawTangent);

    void computeLambda();

protected:

    QList<ASClipVertex*> _clipVertices;
    QList<int> _endPointsIdx;
};

class ASClipPathSet
{
public:
    ASClipPathSet() {}
    ~ASClipPathSet();

    void initFromPoints(const QVector<vec> &sample_positions2D,
                        const QVector<vec> &sample_positions,
                        const QVector<vec2> &sample_tangents,
                        const QVector<float> &sample_strengths);

    ASClipPath* operator[]( int i ) { return _paths[i]; }
    const ASClipPath* operator[]( int i ) const { return _paths[i]; }

    void addPath( ASClipPath* path );

    int size() const { return _paths.size(); }

    void draw(const GLdouble *model, const GLdouble *proj, const GLint *view, bool drawTangent) const;
    void draw(bool drawTangent, int width, int height) const;

    int viewportWidth() const { return _viewport[2]; }
    static vec clipToViewport(const vec4& v, const GLint viewport[], const GLdouble depthRange[]);
    vec clipToViewport(const vec4& v) const;

protected:    

    QList<ASClipPath*>    _paths;
    QMap<int,ASClipPath*> _paths_map;

    GLdouble _depthRange[2];
    GLint   _viewport[4];
};

#endif /* ClipPath_H_ */
