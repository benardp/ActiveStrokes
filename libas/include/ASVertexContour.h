/*****************************************************************************\

ASVertexContour.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef VERTEXCONTOUR_H_
#define VERTEXCONTOUR_H_

#include "Vec.h"

#include "ASClipPath.h"

class ASEdgeContour;
class ASContour;
class ASBrushVertex;

class ASVertexContour {

public:
    ASVertexContour(ASContour* c, const vec2 pos=vec2(0.f,0.f), int i=0, float z=0.5f);
    virtual ~ASVertexContour();

    int index() const { return _index; }
    void setIndex(int i) { _index = i; }

    ASContour* contour() const { return _contour; }
    ASEdgeContour*   edge()    const { return _edge; }

    void setEdge(ASEdgeContour* edge) {_edge = edge; }
    void setContour(ASContour* contour) {_contour = contour; }

    const ASVertexContour* following() const;
    ASVertexContour* following();
    const ASVertexContour* previous() const;
    ASVertexContour* previous();

    bool isLast() const;
    inline bool isFirst() const { return _index == 0; }

    vec2 position() const { return _position; }
    void set3Dposition(float x, float y, float z){_3Dposition = vec3(x, y, z);}
    void set3Ddepth(float d){_3Dposition[2] = d;}
    float get3Ddepth(){return _3Dposition[2];}
    vec3 get3Dposition(){return _3Dposition;}
    vec2 tangent()  const { return _tangent; }
    vec2 normal ()  const { return _normal; }
    void setTangent(vec2 t, bool calcNormal=true);
    void setPosition(vec2 pos);
    vec2 motion() const { return _motion; }
    void setMotion(vec2 motion){_motion = motion;}

    void setArcLength(float l) { _arcLength = l; }
    float arcLength() const { return _arcLength; }

    vec3 finalPosition() const { return _finalPosition; }
    void setFinalPosition(vec3 pos) { _finalPosition = pos; }
    vec3 initialPosition() const { return _initialPosition; }
    void setInitialPosition(vec3 pos) { _initialPosition = pos; }

    vec3 closestEdgePosition() const { return _closestEdgePosition; }
    void setClosestEdgePosition(vec3 closestEdgePos) { _closestEdgePosition = closestEdgePos; }

    ASClipVertex* closestClipVertex() const { return _closestClipVertex; }
    void setClosestClipVertex(ASClipVertex *v);

    vec2 atlasCoord() const { return _atlasCoord; }
    void setAtlasCoord(vec2 coord) { _atlasCoord = coord; }

    float weight() const { return _weight; }
    void  setWeight(float weight) { _weight = weight; }
    float sigma() const { return _sigma; }
    void  setSigma(float sigma) { _sigma = sigma; }

    float curvature() const { return _curvature; }
    float r() const { return _r; }

    void computeTangent();
    void computeCurvature();
    void computeMetricParameters();

    void updatePosition(vec2 pos);

    float metricParameter() const { return _metricParameters[0]; }

    float z() const { return _z; }
    void   setZ(float z) { _z = z; }

    ASContour* remove();

    ASClipPath* coherentPath();

    void setParamFromNeighbors();

    void addBrushVertex(ASBrushVertex* b);
    void removeBrushVertex(ASBrushVertex* b);
    int  nbBrushVertices() const { return _brushVertices.size(); }
    ASBrushVertex* brushVertex(int i) const { return _brushVertices.at(i); }
    bool containsBrushVertex(ASBrushVertex* v) { return _brushVertices.contains(v); }
    float overdraw() const;

    float confidence() const { return _confidence; }
    void  setConfidence( float conf) { _confidence=conf; }
    void  incrConfidence();
    void  decrConfidence();
    void  setStrength(float s);
    float alpha();

    bool isHidden() const  { return _hidden; }
    void setHidden(bool h) { _hidden=h; }

    void setPrePos(vec2 pos){ _prePosition = pos; }
    vec2 prePos(){ return _prePosition; }

protected:
    int _index;

    ASEdgeContour* _edge;
    ASContour*     _contour;

    vec2   _prePosition;
    vec2   _position;
    vec3   _3Dposition;
    vec2   _tangent;
    vec2   _normal;
    vec2   _motion;

    vec3    _finalPosition;
    vec3    _initialPosition;

    vec3   _closestEdgePosition;
    vec2   _atlasCoord;
    ASClipVertex* _closestClipVertex;

    float _weight;
    float _sigma;

    float _curvature;
    float _r;
    float _metricParameters[2];
    float _arcLength;

    float _z;

    float _confidence;

    QList<ASBrushVertex*> _brushVertices;

    bool _hidden;
};

#endif /* VERTEXCONTOUR_H_ */
