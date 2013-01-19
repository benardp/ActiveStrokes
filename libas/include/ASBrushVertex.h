/*****************************************************************************\

ASBrushVertex.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef BRUSHVERTEX_H
#define BRUSHVERTEX_H

#include "Vec.h"
#include "ASVertexContour.h"

#include "DialsAndKnobs.h"

class ASBrushPath;

class ASBrushVertex {
public:
    ASBrushVertex(ASBrushPath* path, ASVertexContour* v, vec2 offset, float param=0.0, int timestamp=0);

    ~ASBrushVertex(){}

    vec2 position(vec2 offset) const;
    vec2 position() const;
    void setPosition(vec2 pos);
    vec2 initialOffsetedPosition() const;
    vec2 offset() const;
    void setOffset(vec2 o);
    vec2 initialOffset() const{ return _initialOffset; }
    void setInitialOffset(vec2 o){ _initialOffset = o; }

    float length() const { return _length; }
    void  setLength(float l) { _length = l; }

    float arcLength() const { return _arcLength; }
    void  setArcLength(float l) { _arcLength = l; }

    const ASVertexContour* sample() const { return _sample; }
    ASVertexContour* sample() { return _sample; }
    void setSample(ASVertexContour* s);

    ASBrushPath* path() const { return _path; }
    void setPath(ASBrushPath* p) { _path = p; }
    ASBrushVertex* removeFromBrushPath();

    float param() const { return _param; }
    void  setParam(float p) { _param = p; }
    float slope();

    void incrTimestamp();
    void resetTimestamp() { _timestamp=0; }
    int  timestamp() const { return _timestamp; }

    int segmentIndex(){return _segmentIndex;}
    void setSegmentIndex(int i){_segmentIndex = i;}

    bool isEndPoint() const;
    bool isFirst() const;
    bool isLast() const;

    float alpha();
    void  setAlpha(float a);

    void setTangent(vec2 t){_tangent = t;}
    vec2 tangent() const {return _tangent;}
    void setPrevTangent(vec2 t){_prevTangent = t;}
    vec2 prevTangent() const;
    void saveTangent() { _prevTangent = _tangent; }

    void setNormal(vec2 n){_normal = n;}
    vec2 normal(){return _normal;}

    void  setOffsetScale(float s){_offsetScale = s;}
    float offsetScale(){return _offsetScale;}

    void  setPenWidth(float width){_penWidth = width;}
    float penWidth(){return _penWidth;}
    void  savePenWidth(){ _prevPenWidth = _penWidth;}
    void  updatePenWidth();

    void  setPreNextVertex();

private:
    ASBrushPath* _path;

    // Associated sample of the snake
    ASVertexContour* _sample;

    // _offsets[0] => tangent offset
    // _offsets[1] => normal offset
    vec2 _offsets;
    vec2 _initialOffset;

    float _length;
    float _arcLength;

    // Previous frame parameterization
    float _param;

    int _segmentIndex;

    int _timestamp;

    //Style attributes
    float _alpha;

    vec2 _tangent;
    vec2 _prevTangent;
    vec2 _normal;

    float _offsetScale;
    float _penWidth;
    float _prevPenWidth;
};

#endif // BRUSHVERTEX_H
