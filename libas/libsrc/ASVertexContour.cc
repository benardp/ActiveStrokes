/*****************************************************************************\

ASVertexContour.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "ASVertexContour.h"
#include "ASEdgeContour.h"
#include "ASContour.h"
#include "ASBrushPath.h"

#include <QDebug>

#include "DialsAndKnobs.h"

#include <QSet>

       dkFloat k_maxConf("BrushPaths->Confidence->Max", 1.0,1.0,100.0,1.0);
static dkBool  k_useStrength("BrushPaths->Confidence->Use strength", false);

ASVertexContour::ASVertexContour(ASContour* c, const vec2 pos, int i, float z) {
    _contour = c;
    _position = pos;
    _prePosition = pos;
    _closestEdgePosition = vec3(pos[0],pos[1],z);
    _index = i;
    _edge = NULL;
    _z = z;
    _hidden = false;
    _weight = 1.0;
    _closestClipVertex = NULL;
    _initialPosition = vec3(pos[0],pos[1],z);
    _confidence = k_maxConf;
    _curvature = 0;
}

ASVertexContour::~ASVertexContour() {
    if(_edge != NULL)
        delete _edge;
    for(int i=0; i<_brushVertices.size(); i++)
        brushVertex(i)->removeFromBrushPath();
    qDeleteAll(_brushVertices);
}

const ASVertexContour* ASVertexContour::following() const {
    return _contour->at(_index+1);
}

ASVertexContour* ASVertexContour::following() {
    return _contour->at(_index+1);
}

const ASVertexContour* ASVertexContour::previous() const {
    return _contour->at(_index-1);
}

ASVertexContour* ASVertexContour::previous() {
    return _contour->at(_index-1);
}

bool ASVertexContour::isLast() const {
    return _index == _contour->nbVertices()-1;
}

void ASVertexContour::computeTangent() {

    // Delingette and Montagnat tangent formula

    const ASVertexContour* v_prev = previous();
    vec2 prev_pos;
    if(v_prev == NULL){
        if(_contour->isClosed())
            prev_pos = _contour->last()->position();
        else
            prev_pos = position();
    }else{
        prev_pos = v_prev->position();
    }
    const ASVertexContour* v_fol = following();
    vec2 fol_pos;
    if(v_fol == NULL){
        if(_contour->isClosed())
            fol_pos = _contour->first()->position();
        else
            fol_pos = position();
    }else{
        fol_pos = v_fol->position();
    }

    _tangent = (fol_pos - prev_pos);
    _r = sqrt(_tangent[0]*_tangent[0] + _tangent[1]*_tangent[1]);
    normalize(_tangent);

    _normal = vec2(-_tangent[1],_tangent[0]);
    if (ASBrushPath::turn(prev_pos, position(), position() + _normal) >= 0)
        _normal *= -1.f;
}

void ASVertexContour::setTangent(vec2 t, bool calcNormal){
    _tangent = t;
    _normal = vec2(-t[1],t[0]);
    if(calcNormal){
        vec2 prev_pos;
        if(isFirst()){
            if(_contour->isClosed())
                prev_pos = _contour->last()->position();
            else
                prev_pos = 2.0f * position() - following()->position();
        }else{
            prev_pos = previous()->position();
        }
        if (ASBrushPath::turn(prev_pos, position(), position() + _normal) >= 0)
            _normal *= -1.f;
    }
}

void ASVertexContour::computeCurvature() {
    const ASVertexContour* v_prev = previous();
    const ASVertexContour* v_fol = following();
    if(v_prev == NULL)
        if(_contour->isClosed())
            v_prev = _contour->last();
    else
        v_prev = v_fol;
    if(v_fol == NULL)
        if(_contour->isClosed())
            v_fol = _contour->first();
    else
        v_fol = v_prev;

    vec2 seg_prev = v_prev->position() - position();
    vec2 seg_fol = v_fol->position() - position();
    vec2 sum = seg_prev+seg_fol;
    float norm_prev = len(seg_prev);
    normalize(seg_prev);
    float norm_fol = len(seg_fol);
    normalize(seg_fol);

    //Sketching Piecewise Clothoid Curves, J. McCrae & K. Singh
    float dotProd=(seg_prev DOT seg_fol);
    float theta;
    if (dotProd>1.0)
        theta = 0.0;
    else if (dotProd<-1.0)
        theta = M_PI;
    else
        theta = acos(dotProd);

    float det = seg_prev[0]*seg_fol[1] - seg_prev[1]*seg_fol[0];

    if(det>0.0f)
        _curvature = -2.0*sin(theta/2.0)/sqrt(norm_prev*norm_fol);
    else
        _curvature = 2.0*sin(theta/2.0)/sqrt(norm_prev*norm_fol);

    //schneider/kobbelt
    //	float det = seg_prev[0]*seg_fol[1] - seg_prev[1]*seg_fol[0];
    //	_curvature = 2.0*det/(norm_prev*norm_fol*len(sum));

}

void ASVertexContour::computeMetricParameters() {
    const ASVertexContour* v_prev = previous();
    if(v_prev == NULL)
        if(_contour->isClosed())
            v_prev = _contour->last();
    else
        v_prev = this;
    const ASVertexContour* v_fol = following();
    if(v_fol == NULL)
        if(_contour->isClosed())
            v_fol = _contour->first();
    else
        v_fol = this;

    vec2 cour_next = position() - v_fol->position();

    vec2 fol_prev = v_prev->position() - v_fol->position();

    float r = sqrt(fol_prev[0]*fol_prev[0] + fol_prev[1]*fol_prev[1]);
    normalize(fol_prev);
    float f = fabs(cour_next[0]*fol_prev[0] + cour_next[1]*fol_prev[1]);

    _metricParameters[0] = f / r;
    _metricParameters[1] = 1.0 - _metricParameters[0];
}

void ASVertexContour::updatePosition(vec2 pos) {
#if 1
    if (isnan(pos[0]) || isnan(pos[1]) ||
        pos[0] < -1000 || pos[0] > 10000 ||
        pos[1] < -1000 || pos[1] > 10000) {
        qWarning() << "Setting bogus position";
        pos[0] = 0;
        pos[1] = 0;
    }
#endif
    _position = pos;
}

void ASVertexContour::setPosition(vec2 pos) {
    updatePosition(pos);
}

ASContour* ASVertexContour::remove() {
    if(_index == _contour->nbVertices()-1)
        return _contour->removeLastVertex();
    if(_index == _contour->nbVertices()-2){
        _contour->removeLastVertex();
        return _contour->removeLastVertex();
    }
    if(_index == 0)
        return _contour->removeFirstVertex();
    if(_index == 1){
        _contour->removeFirstVertex();
        return _contour->removeFirstVertex();
    }

    return _contour->split(_index-1,_index+1);
}

void ASVertexContour::setClosestClipVertex(ASClipVertex *v) {
    _closestClipVertex = v;

    if(v != NULL){
        _atlasCoord = _closestClipVertex->atlasCoord();
        setClosestEdgePosition(_closestClipVertex->position());
    }else{
        _atlasCoord = vec2(-1,-1);
        setClosestEdgePosition(vec3(0,0,0));
    }
}


ASClipPath* ASVertexContour::coherentPath() {
    int nbSamplesForward, nbSamplesBackward;
    if(_index==0){ //first vertex
        nbSamplesBackward=0;
        if(_contour->nbVertices()<6 && !_contour->isClosed())
            nbSamplesForward=_contour->nbVertices()-1;
        else
            nbSamplesForward=5;
    }else if(_edge==NULL){//last vertex
        nbSamplesForward=0;
        if(_contour->nbVertices()<6 && !_contour->isClosed())
            nbSamplesBackward=_contour->nbVertices()-1;
        else
            nbSamplesBackward=5;
    }else{//vertex in the middle of the contour
        if(_index<2 && !_contour->isClosed())
            nbSamplesBackward=_index;
        else
            nbSamplesBackward=2;
        if(_contour->nbVertices()-_index < 3 && !_contour->isClosed())
            nbSamplesForward=_contour->nbVertices()-_index-1;
        else
            nbSamplesForward=2;
    }

    QHash<ASClipPath*,int> map;

    for(int i=0; i<nbSamplesBackward; i++){
        ASClipPath* p = _contour->at(_index-i)->closestClipVertex()->clipPath();
        if(map.contains(p))
            map[p]+=1;
        else
            map.insert(p,1);
    }

    for(int i=0; i<nbSamplesForward; i++){
        ASClipPath* p = _contour->at(_index+i)->closestClipVertex()->clipPath();
        if(map.contains(p))
            map[p]+=1;
        else
            map.insert(p,1);
    }
    QList<int> l = map.values();
    qSort(l);
    if(l.last()>=3)
        return map.key(l.last());
    return NULL;
}


/****************************/
/***** Parameterization *****/
/****************************/

void ASVertexContour::setParamFromNeighbors() {
    ASVertexContour* next = following();
    ASVertexContour* prev = previous();

    if(prev==NULL && next!=NULL){
        for(int i=0; i<next->nbBrushVertices(); ++i){
            ASBrushVertex* v = next->brushVertex(i);
            ASBrushPath*   b = v->path();
            ASBrushVertex* newV = new ASBrushVertex(v->path(),this,v->offset(),0.0,v->timestamp());
            newV->setParam(b->first()->param() - b->slope()*dist(newV->position(),b->first()->position()));
            b->insertBefore(v,newV);
        }
    }else if(next==NULL && prev!=NULL){
        for(int i=0; i<prev->nbBrushVertices(); ++i){
            ASBrushVertex* v = prev->brushVertex(i);
            ASBrushPath*   b = v->path();
            ASBrushVertex* newV = new ASBrushVertex(v->path(),this,v->offset(),0.0,v->timestamp());
            newV->setParam(v->param()+b->slope()*dist(v->position(),newV->position()));
            b->insertAfter(v,newV);
        }
    }else if(prev!=NULL && next!=NULL){
        QSet<ASBrushPath*> visitedPaths;
        for(int i=0; i<next->nbBrushVertices(); ++i){
            ASBrushVertex* v = next->brushVertex(i);
            visitedPaths<<v->path();
        }
        for(int i=0; i<prev->nbBrushVertices(); ++i){
            ASBrushVertex* v = prev->brushVertex(i);
            if(visitedPaths.contains(v->path())){
                ASBrushVertex* newV = new ASBrushVertex(v->path(),this,v->offset(),0.0,v->timestamp());
                newV->setPenWidth(v->penWidth());
                ASBrushPath*   b = v->path();
                newV->setParam(v->param()+b->slope()*dist(v->position(),newV->position()));
                b->insertAfter(v,newV);
            }
        }
    }
}

void ASVertexContour::addBrushVertex(ASBrushVertex* b) {
    if(!_brushVertices.contains(b))
        _brushVertices << b;
}

void ASVertexContour::removeBrushVertex(ASBrushVertex* b) {
    _brushVertices.removeAll(b);
}

float ASVertexContour::overdraw() const {
    float overdraw = 0.0;
    QSet<ASBrushPath*> brushPathMap1;
    foreach(ASBrushVertex* bv,_brushVertices){
        brushPathMap1<<bv->path();
    }
    QSet<ASBrushPath*> brushPathMap2;
    for(int i=0; i<following()->nbBrushVertices(); ++i){
        brushPathMap2<<following()->brushVertex(i)->path();
    }

    QSet<ASBrushPath*> intersection = brushPathMap1.intersect(brushPathMap2);

    overdraw = intersection.size();
    return overdraw;
}

void ASVertexContour::incrConfidence() {
    if(_confidence+1.0>k_maxConf)
        _confidence = k_maxConf;
    else
        _confidence++;
}

void ASVertexContour::decrConfidence() {
    if(_confidence>=1.0)
        _confidence--;
    else
        _confidence=0;
}

void ASVertexContour::setStrength(float s) {
    if(k_useStrength)
        _confidence = min(_confidence,float(s*k_maxConf));
}

float ASVertexContour::alpha() {
    return _confidence / k_maxConf;
}
