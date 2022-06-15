/*****************************************************************************\

ASBrushPath.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "ASBrushPath.h"
#include "ASContour.h"
#include "DialsAndKnobs.h"
#include "GQVertexBufferSet.h"
#include "GQDraw.h"

#include <QDebug>


static dkBool  k_drawNormalDirection("BrushPaths->Draw->Normal", false);
static dkFloat k_bpThickness("BrushPaths->Draw->Thickness", 3.0,0.1,100.0,1.0);
static dkInt   k_offsetStrengthN("Style->Overdraw->Offset strength N", 10);
static dkInt   k_offsetStrengthT("Style->Overdraw->Offset strength T", 1);
static dkBool  k_randomize("Style->Overdraw->Randomize", false);
static dkBool  k_fixOffset("Style->Overdraw->Fix offsets", false);
extern dkBool  k_randomOffsets;

const QStringList fittingModes = QStringList() << "none" << "segments" << "arcs" ;
       dkStringList k_fittingMode("Fitting-> Mode",fittingModes);
static dkBool  k_drawCorrespondence("Fitting->Draw->Correspondences", false);
static dkBool  k_drawContour("Fitting->Draw->Original Contour", true);
       dkBool  k_fittingAveraging("Fitting->Segments->Averaging", false);
       dkFloat k_lineSegmentCost("Fitting->Segments->Cost",10.0);
       dkFloat k_arcSegmentCost("Fitting->Arcs->Cost",400.0);
       dkFloat k_arcTargetRadius("Fitting->Arcs->Target radius",5.0);
       dkFloat k_arcRadiusWeight("Fitting->Arcs->Radius weight",1.0);

ASBrushPath::ASBrushPath(ASContour*c, int start, int end, float slope, float intercept) :
        _slope(slope), _phase(intercept), _reversed(false) {

    vec2 offset(0.f,0.f);

    for(int i=start; i<=end; ++i){
        ASBrushVertex* v = new ASBrushVertex(this, c->at(i),offset);
        _vertices<<v;
    }
    computeArcLength();
    for(int i=0; i<nbVertices(); ++i)
        at(i)->setParam(param(i));

    _newSpline = true;
    _newFitting = true;
    _closed = c->isClosed();
    _level = -1;
    _fact = -1;

    if(k_randomOffsets) {
        randomizeOffset(true);
    } else {
        _offset = vec2(0,0);
    }

    assignDebugColor();

    connect(&k_randomize,SIGNAL(valueChanged(bool)),this,SLOT(randomizeOffset(bool)));
}

ASBrushPath::ASBrushPath(float slope, vec2 offset) :
        _slope(slope), _closed(false), _reversed(false)
{
    assignDebugColor();
    _newFitting = true;
    _newSpline = true;
    _level = -1;
    _fact = -1;
    _offset = offset;
    connect(&k_randomize,SIGNAL(valueChanged(bool)),this,SLOT(randomizeOffset(bool)));
}

ASBrushPath::ASBrushPath(ASBrushPath &bp1, ASBrushPath &bp2) {
    _vertices.append(bp1._vertices);
    _vertices.append(bp2._vertices);
    computeArcLength();
    _newFitting = true;
    _newSpline = true;
    _fittingPara = bp1.fittingPara(); //testing
}

ASBrushPath::~ASBrushPath() {
    foreach(ASBrushVertex* v,_vertices)
        v->sample()->removeBrushVertex(v);
    qDeleteAll(_vertices);
}

void ASBrushPath::randomizeOffset(bool b) {
    if(b){
        if(k_fixOffset){
            _offset = vec2(k_offsetStrengthT,k_offsetStrengthN);
        }else{
            _offset[0] = k_offsetStrengthT <= 0 ? 0 : rand()%k_offsetStrengthT-float(k_offsetStrengthT)/2.0f;
            _offset[1] = k_offsetStrengthN <= 0 ? 0 : rand()%k_offsetStrengthN-float(k_offsetStrengthN)/2.0f;
        }
        k_randomize.setValue(false);
    }
}

vec2 ASBrushPath::offset() const {
    if(_reversed)
        return -_offset;

    return _offset;
}

void ASBrushPath::setOffset(vec2 offset) {
    _offset = offset;
}

float ASBrushPath::slope() const {
    return _slope;
}

void ASBrushPath::assignDebugColor() {
    static int counter = 0;
    int r = counter++ % ncolors;
    _debug_color = QColor(color_list_BP[r]);
}

int ASBrushPath::nbVertices() const {
    return _vertices.size();
}

void ASBrushPath::computeArcLength() {
    float length = 0.0;
    at(0)->setLength(length);
    at(0)->setArcLength(length);

    for(int i=1; i<nbVertices(); ++i){
        float l = dist(at(i-1)->position(),at(i)->position());
        at(i)->setLength(l);
        length += l;
        at(i)->setArcLength(length);
    }
}

float ASBrushPath::param(double arcLength) const {
    return _slope * arcLength + _phase;
}

float ASBrushPath::param(int i) const {
    return _slope * at(i)->arcLength() + _phase;
}

void ASBrushPath::computeParam() {
    for(int i=0; i<nbVertices(); ++i){
        at(i)->setParam(param(i));
     }
}

void ASBrushPath::computeParamNewFrame() {
    for(int i=0; i<nbVertices(); ++i){
        at(i)->setParam(param(i));
        at(i)->incrTimestamp();
        at(i)->saveTangent();
    }
}

void ASBrushPath::insertAfter(ASBrushVertex* prevVertex, ASBrushVertex* newVertex) {
    int idx = _vertices.indexOf(prevVertex)+1;
    if(idx==nbVertices())
        _vertices.push_back(newVertex);
    else
        _vertices.insert(idx,newVertex);
}

void ASBrushPath::insertBefore(ASBrushVertex* nextVertex, ASBrushVertex* newVertex){
    int idx = _vertices.indexOf(nextVertex);
    if(idx == -1)
        _vertices.push_front(newVertex);
    else
        _vertices.insert(idx,newVertex);
}

void ASBrushPath::insertFirst(ASBrushVertex* newVertex) {
    _vertices.push_front(newVertex);
}

void ASBrushPath::insertLast(ASBrushVertex* newVertex) {
    _vertices.push_back(newVertex);
}

void ASBrushPath::remove(ASBrushVertex* v) {
    if(_vertices.first()==v && nbVertices()>1)
        _phase = param(1);
    _vertices.removeAll(v);
}

ASBrushPath* ASBrushPath::split(int idx) {
    ASBrushPath* newBrushPath = new ASBrushPath(_slope,_offset);
    newBrushPath->setReversed(_reversed);

    ASBrushVertex* v;

    int lastIdx = nbVertices()-1;
    for(int i=lastIdx; i>=0; --i){
        if(i==idx){
            v = _vertices.at(i);
            ASBrushVertex* bv = new ASBrushVertex(newBrushPath,v->sample(),v->offset(),v->param());
//            bv->setAbsolutePosition(v->absolutePosition());
            newBrushPath->_vertices.push_front(bv);
            newBrushPath->setPhase(param(i));
        }else if(i>idx){
            v = _vertices.takeAt(i);
            v->setPath(newBrushPath);
            newBrushPath->_vertices.push_front(v);
        }
    }

    _closed = false;

    if(newBrushPath->nbVertices()>nbVertices()){
        QColor color = debugColor();
        debugColor() = newBrushPath->debugColor();
        newBrushPath->debugColor() = color;
    }

    return newBrushPath;
}

ASBrushPath* ASBrushPath::split(int idx1, int idx2, ASVertexContour* newV) {
    ASBrushPath* newBrushPath = new ASBrushPath(_slope,_offset);
    newBrushPath->setReversed(_reversed);

    QList<ASBrushVertex*> kept;
    QList<ASBrushVertex*> toDelete;

    computeArcLength();

    for(int i=0; i<nbVertices(); ++i){
        ASBrushVertex* v = at(i);
        const ASVertexContour* vc = v->sample();
        if(vc->index()<=idx1){
            kept<<v;
            if(vc->index()==idx2){
                ASBrushVertex* bv = new ASBrushVertex(newBrushPath,newV,v->offset(),v->param());
//                bv->setAbsolutePosition(v->absolutePosition());
                newBrushPath->_vertices<<bv;
                newBrushPath->setPhase(v->param());
            }
        }else if(vc->index()>=idx2){
            newBrushPath->_vertices<<v;
            v->setPath(newBrushPath);
            if(vc->index()==idx2)
                newBrushPath->setPhase(v->param());
        }else{
            v->sample()->removeBrushVertex(v);
            toDelete<<v;
        }
    }

    //Update current brush path
    _vertices.clear();
    _vertices.append(kept);

    // Check brush path length
    if(newBrushPath->nbVertices()<=1){
        delete newBrushPath;
        newBrushPath = NULL;
    }else{
        if(newBrushPath->nbVertices()>nbVertices()){
            QColor color = debugColor();
            debugColor() = newBrushPath->debugColor();
            newBrushPath->debugColor() = color;
        }
    }

    //Clear uneeded part
    qDeleteAll(toDelete);

    _closed = false;

    return newBrushPath;
}

void ASBrushPath::merge(ASBrushPath* bp) {
    int insertIdx = nbVertices();
    for(int i=bp->nbVertices()-1; i>=0; --i) {
        ASBrushVertex* bv = bp->at(i);
        bv->path()->remove(bv);
        bv->setPath(this);
       _vertices.insert(insertIdx,bv);
    }
    _offset = 0.5f*(bp->_offset + _offset);
    computeArcLength();
    computeParam();

    if(bp->nbVertices()>nbVertices()){
        debugColor() = bp->debugColor();
    }
}

void ASBrushPath::inverse() {
    computeArcLength();
    _phase = param(nbVertices()-1);
    _slope = -_slope;

    QListIterator<ASBrushVertex*> it(_vertices);
    it.toBack();
    QList<ASBrushVertex*> invOrder;
    while(it.hasPrevious())
        invOrder<<it.previous();

    _vertices.clear();
    _vertices.append(invOrder);

    _reversed = !_reversed;
}

void ASBrushPath::draw(bool black)
{
    if (k_drawContour && !black)
    {
        glLineWidth(k_bpThickness/2.0);
        glBegin(GL_LINE_STRIP);

        glColor3f(0.8,0.8,0.8);
        for(int i=0; i<_vertices.size(); i++){
            glVertex2fv(at(i)->sample()->position());
        }
        glEnd();
    }

    glLineWidth(k_bpThickness);
    if(!black){
        vec3 color;
        QColor qline_col = debugColor();
        color = vec3(qline_col.red(), qline_col.green(), qline_col.blue());
        color /= 255.0;
        glColor3fv(color);
    }else{
        glColor3f(0.f,0.f,0.f);
    }
    if (k_drawNormalDirection && !black)
    {
        //draw the normal direction
        glLineWidth(1);
        glBegin(GL_LINES);

        for(int i=0; i<nbVertices(); i++){
            glVertex2fv(at(i)->position());
            glVertex2fv(at(i)->normal() * 10.0f + at(i)->position());
        }
        glEnd();
    }

    //draw the input position
    glBegin(GL_LINE_STRIP);
    for(int i=0; i<_vertices.size(); i++){
        glVertex2fv(at(i)->position());
    }
    glEnd();

    if (k_drawCorrespondence && !black)
    {
        //Draw the correspondance between the updated sample and the input sample
        glLineWidth(1);
        glColor3f(0, 0, 0);
        glBegin(GL_LINES);
        for(int i=0; i<nbVertices(); i++){
            glVertex2fv(at(i)->position());
            glVertex2fv(at(i)->sample()->position());
        }
        glEnd();
    }
}

void ASBrushPath::rasterizeOffsets(GQShaderRef &shader) const
{
    std::vector<int> indices;
    std::vector<vec2> vertices;
    std::vector<vec2> offsets;
    for(int i=0; i<_vertices.size(); i++){
        const ASBrushVertex* v = at(i);
        vec2 pos = v->sample()->position();
        vec2 targetPos = v->position();
        vec2 offset = pos - targetPos;

        indices.push_back(i);
        vertices.push_back(targetPos);
        offsets.push_back(offset);
    }

    GQVertexBufferSet vertex_buffer_set;
    vertex_buffer_set.add(GQ_VERTEX, vertices);
    vertex_buffer_set.add(GQ_INDEX, 1, indices);
    vertex_buffer_set.add("offset", offsets);
    vertex_buffer_set.copyToVBOs();

    vertex_buffer_set.bind(shader);
    GQDraw::drawElements(vertex_buffer_set, GL_LINE_STRIP, 0, indices.size());
    vertex_buffer_set.unbind();
}

void ASBrushPath::print() const
{
    qDebug()<<" # "<<this<<" ("<<nbVertices()<<") slope: "<<_slope<<" / intercept: "<<_phase;
    for(int i=0; i<nbVertices(); i++){
        ASBrushVertex* v = _vertices.at(i);
        qDebug("%d",v->sample()->index());
        if(i<nbVertices()-1)
            qDebug(" / ");
        if(i!=0 && v->sample()->index()!=_vertices.at(i-1)->sample()->index()+1){
            qFatal("BrushPath error: %p",v->sample()->contour()->at(v->sample()->index()-1));
            if(!v->sample()->containsBrushVertex(v))
                qFatal("BrushVertex error");
        }
    }
    qDebug();
}

QVector<ASBrushPath*> ASBrushPath::fitting()
{
    QVector<ASBrushPath*> newBrushpathList;

    if(!k_randomOffsets)
        _offset = vec2(0,0);

    ASBrushPathFitting fit = ASBrushPathFitting::instance();

    ACBrushpathStyleMode mode = (ACBrushpathStyleMode) k_fittingMode.index();

    switch(mode){
    case AS_BRUSHPATH_LINE:
    {
        if(nbVertices()<=2)
        {
            vec2 posAvg1 = first()->position();
            vec2 posAvg2 = last()->position();

            if (!isNewFitting() && k_fittingAveraging){
                fit.averageLineEndPoint(this,first()->position(),last()->position(),posAvg1, posAvg2);
            }
            fit.interpolateEndPoint(this,posAvg1,posAvg2);
        }else{
            float _lineSegmentCost = k_lineSegmentCost;
            fit.findBreakingPositionForLine(_lineSegmentCost, this);
            newBrushpathList = splitToMultSegments(fit.getSegmentInfoSet());
            newBrushpathList.push_back(this);
            foreach(ASBrushPath* bp, newBrushpathList)
                fit.calculateLinePosition(bp);
            newBrushpathList.pop_back();
        }
        _newFitting = false;
        break;
    }
    case AS_BRUSHPATH_ARC:
    {
        //if the number of vertices is small, cannot do real circle fitting
        if (_vertices.size() < 3){
            return newBrushpathList;
        }
        fit.findBreakingPositionForArc(k_arcSegmentCost, k_arcTargetRadius, k_arcRadiusWeight, this);
        newBrushpathList = splitToMultSegments(fit.getSegmentInfoSet()); //the brushpath fitting parameter is set within this
        newBrushpathList.push_back(this);
        foreach(ASBrushPath* bp, newBrushpathList)
            fit.calculateArcPosition(bp);
        newBrushpathList.pop_back();

        _newFitting = false;
        break;
    }
    default:
        foreach(ASBrushVertex* v, _vertices)
            v->setOffset(offset());
    }
    return newBrushpathList;
}

QVector<ASBrushPath*> ASBrushPath::splitToMultSegments(const QVector<fittingSegment>& segmentInfoSet)
{
    QVector<ASBrushPath*> newBrushpathList;
    int id;
    for (int i=segmentInfoSet.size()-1; i>0; i--)
    {
        id = segmentInfoSet[i].start;
        ASBrushPath *p = split(id);
        p->setFittingPara(segmentInfoSet[i].A, segmentInfoSet[i].B, segmentInfoSet[i].C, segmentInfoSet[i].cost);
        p->setNewFitting(false);
        newBrushpathList.push_back(p);

    }

    setFittingPara(segmentInfoSet[0].A, segmentInfoSet[0].B, segmentInfoSet[0].C, segmentInfoSet[0].cost);

    return newBrushpathList;
}

float ASBrushPath::computeOriginalLength()
{
    float length = 0.0;
    for(int i=1; i<nbVertices(); ++i){
        float l = dist(at(i-1)->sample()->position(),at(i)->sample()->position());
        printf("l : %f\n", l);
        length += l;
    }
    return length;
}

int ASBrushPath::turn(vec2 A, vec2 B, vec2 C, float threshold)
{
    float det = A[0] * (B[1] - C[1]) - B[0] * (A[1] - C[1]) + C[0] * (A[1] - B[1]);
    if (det > threshold)
        return 1; //left
    else if (det < -threshold)
        return -1; //right
    else
        return 0;
}

// Compute the tangent information after fitting the brushpath
// Should be called after fitting brushpath
void ASBrushPath::computeTangent()
{
    vec2 fol_pos;
    vec2 prev_pos = at(0)->position();

    vec2 tangent, normal;
    for (int i=0; i<nbVertices()-1; i++)
    {
        fol_pos = at(i+1)->position();
        tangent = normalized(fol_pos - prev_pos);
        if(_reversed)
            tangent *= -1;
        normal = vec2(-tangent[1],tangent[0]);

        if(_reversed){
            if (turn(fol_pos, at(i)->position(), at(i)->position() + normal) >= 0)
                normal = -normal;
        }else{
            if (turn(prev_pos, at(i)->position(), at(i)->position() + normal) >= 0)
                normal = -normal;
        }

        at(i)->setTangent(tangent);
        at(i)->setNormal(normal);
        prev_pos = at(i)->position();
    }

    if (_closed)
    {
        fol_pos = at(0)->position();
        tangent = normalized(fol_pos - prev_pos);
    }else{
        fol_pos = at(nbVertices()-1)->position();
        tangent = normalized(fol_pos - prev_pos);
    }
    if(_reversed)
        tangent *= -1;
    normal = vec2(-tangent[1],tangent[0]);

    if(_reversed){
        if (turn(fol_pos, at(nbVertices()-1)->position(), at(nbVertices()-1)->position() + normal) >= 0)
            normal = -normal;
    }else{
        if (turn(prev_pos, at(nbVertices()-1)->position(), at(nbVertices()-1)->position() + normal) >= 0)
            normal = -normal;
    }
    at(nbVertices()-1)->setTangent(tangent);
    at(nbVertices()-1)->setNormal(normal);

}
