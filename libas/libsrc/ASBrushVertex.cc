/*****************************************************************************\

ASBrushVertex.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "ASBrushVertex.h"
#include "ASBrushPath.h"
#include "DialsAndKnobs.h"

extern dkBool  k_randomOffsets;
static dkBool  k_confToAlpha("BrushPaths->Confidence->To alpha",false);
extern dkFloat k_perturbation_scale;

ASBrushVertex::ASBrushVertex(ASBrushPath* path, ASVertexContour* v, vec2 offset, float param, int timestamp) :
        _path(path), _sample(v), _offsets(offset), _length(0.0), _arcLength(0.0),
        _param(param), _timestamp(timestamp), _alpha(1.0)
{
    v->addBrushVertex(this);
    _segmentIndex = -1;
    _normal = vec2(0, 0);
    _tangent = vec2(0, 0);
    _offsetScale = 1;
    _penWidth = 0;
    _prevPenWidth = 0;
    _initialOffset = vec2(0, 0);
}

void ASBrushVertex::setSample(ASVertexContour* s) {
    _sample = s;
    _sample->addBrushVertex(this);
}

ASBrushVertex* ASBrushVertex::removeFromBrushPath() {
    ASBrushVertex* newV = NULL;
    if(isFirst() && _path->nbVertices()>1){
        newV = _path->at(1);
    }else if(isLast() && _path->nbVertices()>1){
        newV = _path->at(_path->nbVertices()-2);
    }
    _path->remove(this);
    return newV;
}

vec2 ASBrushVertex::initialOffsetedPosition() const {
    vec2 samplePos = _sample->position();
    vec2 tangent = _sample->tangent();
    vec2 normal = _sample->normal();
    vec2 pos;
    if(k_randomOffsets){
        vec2 random_offset = _path->offset();
        pos = samplePos + tangent * ((_offsets[0] + random_offset[0]) * _offsetScale) + normal * float((_offsets[1] + random_offset[1]) * _offsetScale);
    }else{
        pos = samplePos + tangent * (_initialOffset[0] * _offsetScale) + normal * (_initialOffset[1] * _offsetScale);
    }
    return pos;
}

vec2 ASBrushVertex::position(vec2 offset) const {
    vec2 samplePos = _sample->position();
    vec2 tangent = _sample->tangent();
    vec2 normal = _sample->normal();
    vec2 pos = samplePos + tangent * (offset[0] * _offsetScale) + normal * (offset[1] * _offsetScale);
    return pos;
}

vec2 ASBrushVertex::position() const {
    vec2 samplePos = _sample->position();
    vec2 tangent = _sample->tangent();
    vec2 normal = _sample->normal();
    vec2 pos = samplePos + tangent * (_offsets[0] * _offsetScale) + normal * (_offsets[1] * _offsetScale);
    return pos;
}

void ASBrushVertex::setPosition(vec2 pos) {
    vec2 samplePos = _sample->position();
    vec2 relPos = pos - samplePos;
    vec2 tangent = _sample->tangent();
    vec2 normal = _sample->normal();
    if(k_randomOffsets){
        vec2 random_offset = _path->offset();
        _offsets = vec2( ((relPos DOT tangent) - random_offset[0])/_offsetScale, ((relPos DOT normal) - random_offset[1])/_offsetScale);
    }else{
        _offsets = vec2( (relPos DOT tangent)/_offsetScale, (relPos DOT normal)/_offsetScale);
    }
}

vec2 ASBrushVertex::offset() const {
    return _offsetScale*_offsets;
}

void ASBrushVertex::setOffset(vec2 o) {
    _offsets = o;
}

bool ASBrushVertex::isEndPoint() const {
    return (isFirst() || isLast());
}

bool ASBrushVertex::isFirst() const {
    return _path->first()==this;
}

bool ASBrushVertex::isLast() const {
    return _path->last()==this;
}

void ASBrushVertex::setAlpha(float a) {
    _alpha = a;
}

float ASBrushVertex::alpha() {
    if(k_confToAlpha)
        return _sample->alpha() * _alpha;
    return _alpha;
}

void ASBrushVertex::incrTimestamp() {
    _timestamp++;
}

vec2 ASBrushVertex::prevTangent() const {
    if(_prevTangent[0]==0.0 && _prevTangent[1]==0.0)
        return _tangent;
    return _prevTangent;
}

float ASBrushVertex::slope() {
    int idx = _path->indexOf(this);
    if(_path->nbVertices()==1){
        return _path->slope();
    }
    if(idx == _path->nbVertices()-1){
        return (this->param() - _path->at(idx-1)->param()) / this->length();
    }

    return (_path->at(idx+1)->param() - this->param()) / _path->at(idx+1)->length();
}

void ASBrushVertex::updatePenWidth() {
    _penWidth = std::min(1.0f,0.1f*_penWidth + 0.9f*_prevPenWidth);
}
