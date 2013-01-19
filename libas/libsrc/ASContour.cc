/*****************************************************************************\

ASContour.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "ASContour.h"
#include "ASSnakes.h"
#include "ASDeform.h"
#include "ASBrushPath.h"

#include "DialsAndKnobs.h"

#include <QTextStream>
#include <QDebug>

#include <float.h>
#include <math.h>

#ifdef WIN32
#define fmax max
#define fmin min
#endif
#ifndef isnan
# define isnan(x) \
	(sizeof (x) == sizeof (long double) ? isnan_ld (x) \
	: sizeof (x) == sizeof (double) ? isnan_d (x) \
	: isnan_f (x))
static inline int isnan_f  (float       x) { return x != x; }
static inline int isnan_d  (double      x) { return x != x; }
static inline int isnan_ld (long double x) { return x != x; }
#endif

#ifndef isinf
# define isinf(x) \
	(sizeof (x) == sizeof (long double) ? isinf_ld (x) \
	: sizeof (x) == sizeof (double) ? isinf_d (x) \
	: isinf_f (x))
static inline int isinf_f  (float       x)
{ return !isnan (x) && isnan (x - x); }
static inline int isinf_d  (double      x)
{ return !isnan (x) && isnan (x - x); }
static inline int isinf_ld (long double x)
{ return !isnan (x) && isnan (x - x); }
#endif

#include<QSet>

       dkFloat k_penWidth("Style->Main->Pen width", 2.0);
static dkBool  k_taperBP("Style->Taper->BP length", false);
static dkFloat k_fade("Style->Taper->Fade", 0.0,0.0,1.0,0.05);
static dkBool  k_useTargetLength("BrushPaths->Target length->Activate", false);
       dkFloat k_targetLength("BrushPaths->Target length->Length", 200.0,1.0,10000.0,1.0);

static dkFloat k_targetOverdraw("Style->Overdraw->Target", 1.0,0.0,100.0,0.1);
static dkFloat k_gammaExt("Style->Overdraw->Extend fact.", 1.0,0.0,1.0,0.1);
static dkFloat k_gammaContract("Style->Overdraw->Contract fact.", 1.0,0.0,1.0,0.1);
       dkBool  k_randomOffsets("Style->Overdraw->Random offsets",false);

static dkFloat k_mergeSlopeTh("BrushPaths->Merge->Slope th.", 0.01,0.0,100.0,0.001);
static dkFloat k_mergePhaseTh("BrushPaths->Merge->Phase th.", 0.005,0.0,100.0,0.001);
static dkFloat k_healingSlopeTh("BrushPaths->Merge->Healing slope th.", 0.05,0.0,100.0,0.01);
static dkFloat k_healingPhaseTh("BrushPaths->Merge->Healing phase th.", 0.01,0.0,100.0,0.1);
static dkFloat k_mergeDist("BrushPaths->Merge->Dist. th.", 2.0);
static dkFloat k_mergeDotProd("BrushPaths->Merge->Dot prod. th.", 0.8);
static dkBool  k_checkParam("BrushPaths->Merge->Check Param.", false);
static dkBool  k_averageMerge("BrushPaths->Merge->Average", false);
       dkFloat k_fittingMaxDistance("Fitting->Segments->Max distance merge", 0.5);

static dkBool  k_confToThickness("BrushPaths->Confidence->To thickness",true);

dkFloat k_taperStart("Style->Taper->Start", 0.0,0.0,1.0,0.05);
dkFloat k_taperEnd("Style->Taper->End", 0.0,0.0,1.0,0.05);
dkFloat k_overshootMax("Style->Overshoot->Max", 0);
dkFloat k_overshootMin("Style->Overshoot->Min", 0);
dkFloat k_overshootTh("Style->Overshoot->Threshold", 10);
dkFloat k_taperMin("Style->Taper->T min", 0.01,0.0,1.0,0.05);
extern dkBool  k_fittingAveraging;

extern dkFloat k_lineSegmentCost;
extern dkFloat k_arcSegmentCost;
extern dkFloat k_arcTargetRadius;
extern dkFloat k_arcRadiusWeight;
extern dkStringList k_fittingMode;

ASContour::ASContour(ASSnakes* ac) :
        _ac(ac), _closed(false), _isNew(true), _length(0.0)
{
    assignDebugColor();
}

ASContour::ASContour(ASSnakes* ac, ASClipPath &p, float visTh) {

    _ac = ac;

    int idx = 0;

    ASClipVertex* cv = p[0];

    int j=1;
    while(cv->visibility()<visTh){
        if(j>=p.size())
            return;
        cv = p[j];
        j++;
    }
    vec pos = cv->position();

    ASVertexContour* v1 = new ASVertexContour(this,vec2(pos[0],pos[1]),idx, pos[2]);
    v1->setClosestClipVertex(cv);
    addVertex(v1);

    for(int i=j; i<p.size(); i++){
        cv = p[i];
        idx++;
        pos = cv->position();
        ASVertexContour* v2 = new ASVertexContour(this,vec2(pos[0],pos[1]),idx, pos[2]);
        v2->setClosestClipVertex(cv);
        if(cv->visibility()<visTh)
            v2->setConfidence(0);
        addVertex(v2);
        ASEdgeContour* e = new ASEdgeContour(v1,v2);
        v1->setEdge(e);
        v1 = v2;
    }

    if(nbVertices()<=1){
        first()->setTangent(first()->closestClipVertex()->tangent(),false);
    }else{
        while(resample()!=0){};

        checkClosed(); //compute length

        computeLength();
        computeTangent();
        initParameterization();
    }

    _isNew = true;

    assignDebugColor();
}

void ASContour::clear() {
    qDeleteAll(_vertexList);
    _vertexList.clear();
    qDeleteAll(_brushPaths);
    _brushPaths.clear();
}

ASContour::~ASContour() {
    clear();
}

static int counter=0;

void ASContour::assignDebugColor() {
    int r = counter++ % ncolors;
    _debug_color = QColor(color_list[r]);
}

void ASContour::cleanBrushPath() {
    QMutableListIterator<ASBrushPath*> itB(_brushPaths);
    while(itB.hasNext()){
        ASBrushPath* b = itB.next();
        if(this->nbVertices()>=2 && b->nbVertices()<2){
            itB.remove();
            delete b;
        }
    }
}

/****************************/
/*  Properties Computation  */
/****************************/

void ASContour::initParameterization() {

    for(int i=0; i<ceil(k_targetOverdraw); ++i){
        if(_length==0)
            computeLength();
        ASBrushPath* b;
        //Initialize arc-length parametrization in [0,1/viewport_width]
        b = new ASBrushPath(this,0,nbVertices()-1,1.0/float(_ac->width()),0.0);
        _brushPaths << b;
    }
}

void ASContour::computeTangent() {
    ContourIterator it = iterator();
    while(it.hasNext()){
        ASVertexContour* v = it.next();
        v->computeTangent();
    }
}

void ASContour::computeCurvature() {
    ContourIterator it = iterator();
    while(it.hasNext()){
        ASVertexContour* v = it.next();
        v->computeCurvature();
    }
}

void ASContour::computeLength() {
    _length = 0.0;
    at(0)->setArcLength(_length);
    for(int i=0; i<nbVertices()-1; ++i){
        ASVertexContour* v = at(i);
        ASEdgeContour*   e = v->edge();
        Q_ASSERT(e != NULL);
        e->computeDirection();
        _length += e->length();
        at(i+1)->setArcLength(_length);
    }
}

void ASContour::computeEdgeDirection() {
    ContourIterator it = iterator();
    while(it.hasNext()){
        ASVertexContour* v = it.next();
        ASEdgeContour*   e = v->edge();
        if(e != NULL)
            e->computeDirection();
    }
}

ASVertexContour* ASContour::at(int index) {
    int idx = index;
    if(index >= nbVertices() || index < 0){
        if(_closed && index >= nbVertices())
            idx = (index % nbVertices());
        else if (_closed && index < 0)
            idx = index + nbVertices();
        else
            return NULL;
    }
    return _vertexList.at(idx);
}

const ASVertexContour* ASContour::at(int index) const {
    return at(index);
}

void ASContour::checkClosed(){
    computeLength();

    if(length() > 4.0*_ac->sMax()
        && dist2(first()->position(),last()->position())<=2.5*_ac->coverRadius()
        && (first()->tangent() DOT last()->tangent()) >= 0.9){
        _closed=true;

    }else{
        _closed = false;
    }
}


/************************/
/*  Drawing functions   */
/************************/

void ASContour::draw(const vec3 color, const bool drawParam) const {
    ContourIterator it = iterator();
    while(it.hasNext()){
        ASVertexContour* vertex = it.next();
        vec2 v = vertex->position();
        if(drawParam)
            glColor3f(vertex->alpha(),vertex->alpha(),vertex->alpha());
        else
            glColor3fv(color);
        glVertex2fv(v);
    }
    if(_closed){
        glColor3fv(color);
        glVertex2fv(first()->position());
    }
}

void ASContour::draw3D(const GLdouble *model, const GLdouble *proj, const GLint *view, const vec3 color, const bool drawParam) const {

    ContourIterator it = iterator();
    while(it.hasNext()){
        ASVertexContour* vertex = it.next();
        vec2 v = vertex->position();
        float depth = vertex->z();
        if(drawParam)
            glColor3f(vertex->alpha(),vertex->alpha(),vertex->alpha());
        else
            glColor3fv(color);

        GLdouble objX, objY, objZ;
        gluUnProject(v[0], v[1], depth, model, proj, view, &objX, &objY, &objZ);
        glVertex3d(objX,objY,objZ);
    }
    if(_closed){
        vec2 v = first()->position();
        float depth = first()->z();
        GLdouble objX, objY, objZ;
        gluUnProject(v[0], v[1], depth, model, proj, view, &objX, &objY, &objZ);

        glColor3fv(color);
        glVertex3d(objX,objY,objZ);
    }
}

float ASContour::calculate3Dpositions(const GLdouble *model, const GLdouble *proj, const GLint *view) const {
    float min = 0;
    ContourIterator it = iterator();
    float depth;
    while(it.hasNext()){
        ASVertexContour* vertex = it.next();
        vec2 v = vertex->position();
        if (vertex->closestClipVertex())
            depth = vertex->closestClipVertex()->position()[2];
        else
            depth = vertex->z();
        GLdouble objX, objY, objZ;

        gluUnProject(v[0], view[1]-v[1], depth, model, proj, view, &objX, &objY, &objZ);
        vertex->set3Dposition(objX, objY, objZ);
        if (objZ < min)
            min = objZ;
    }

    return min;
}

void ASContour::drawClosestEdge3D(const GLdouble *model, const GLdouble *proj, const GLint *view) const {
    ContourIterator it = iterator();
    GLdouble objX, objY, objZ;
    while(it.hasNext()){
        ASVertexContour* v = it.next();
        vec2 pos = v->position();
        float depth = v->z();
        gluUnProject(pos[0], pos[1], depth, model, proj, view, &objX, &objY, &objZ);
        glVertex3d(objX,objY,objZ);

        vec3 posCV = v->closestEdgePosition();
        gluUnProject(posCV[0], posCV[1], posCV[2], model, proj, view, &objX, &objY, &objZ);
        glVertex3d(objX,objY,objZ);
    }
}

void ASContour::drawClosestEdge() const {
    ContourIterator it = iterator();
    while(it.hasNext()){
        ASVertexContour* v = it.next();
        vec2 pos = v->position();
        glVertex2fv(pos);
        vec3 posCV = v->closestEdgePosition();
        glVertex2f(posCV[0],posCV[1]);
    }
}

void ASContour::drawBrushPaths(bool black) const
{
    foreach(ASBrushPath* bp, _brushPaths)
        bp->draw(black);
}

void ASContour::rasterizeOffsets(GQShaderRef& shader) const
{
    foreach(ASBrushPath* bp, _brushPaths)
        bp->rasterizeOffsets(shader);
}


/*********************/
/*     Processing    */
/*********************/

int ASContour::resample(bool findClosest)
{
    int nbVert=nbVertices();

    computeLength();
    QList<ASVertexContour*> newVertexList; // new list = result of the resampling
    ASVertexContour* newVertex;
    vec2 newPosition;
    float previousLength = 0.0;

    QList<ASVertexContour*> paramToFix;

    int idx=0; //current index in the new list of verticies

    for(int i=0; i<nbVert-1; ++i){
        ASVertexContour* v = at(i);
        ASEdgeContour*   e = v->edge();

        Q_ASSERT(e != NULL);

        float l = e->length();

        if(l>_ac->sMax() && l>2.0*_ac->sMin()){ // add vertex between v and v->following
            int prev_idx = v->index();
            newPosition = 0.5f * (v->position() + v->following()->position());
            newVertex = new ASVertexContour(this, newPosition, idx+1, 0.5f*(v->z()+v->following()->z()) );
            newVertex->setConfidence(0.5f*(v->confidence()+v->following()->confidence()));
            v->setEdge(new ASEdgeContour(v,newVertex));

            // For artificial Snakes
            newVertex->setFinalPosition(0.5f*(v->finalPosition()+v->following()->finalPosition()));
            newVertex->setInitialPosition(0.5f*(v->initialPosition()+v->following()->initialPosition()));

            v->setIndex(idx); // keep v and update its index in the new list
            idx++;
            newVertexList<<v;
            newVertex->setEdge(new ASEdgeContour(newVertex,this->at(prev_idx+1)));
            newVertexList<<newVertex;
            paramToFix<<newVertex;

            delete e;
            // Update previous edge length
            previousLength = newVertex->edge()->length();

        }else if(idx>0 && l<_ac->sMin() && l+previousLength<_ac->sMax()){ // remove vertex v
            newVertex = newVertexList.at(idx-1);
            delete newVertex->edge();
            newVertex->setEdge(new ASEdgeContour(newVertex,this->at(v->index()+1)));
            paramToFix.removeAll(v);
            delete v;
            idx--;
            // Update previous edge length
            previousLength = newVertex->edge()->length();

        }else{ // Keep vertex v
            v->setIndex(idx);
            newVertexList<<v;
            previousLength = l;
        }
        idx++;
    }
    // Keep last vertex
    last()->setIndex(idx);
    newVertexList<<last();

    _vertexList.clear();
    _vertexList.append(newVertexList);

    // Fix brush paths parameterization
    if(paramToFix.size()>0){
        computeTangent();

        foreach(ASVertexContour* v, paramToFix){
            v->setParamFromNeighbors();
            if(findClosest){
                v->setClosestClipVertex(_ac->findClosestEdgeRef(v->position(),v->tangent(),true,_ac->coverRadius()));
            }
        }
    }
    cleanBrushPath();

    return nbVert-nbVertices();
}


ASContour* ASContour::split(int idx1, int idx2) {
    cleanBrushPath();

    ASVertexContour* v = _vertexList.at(idx1);
    v->setEdge(NULL);

    // New contour with the remaining split part
    ASContour* newContour = NULL;
    ASVertexContour* newVertex  = NULL;
    if(nbVertices()-2 >= idx2){
        newContour = new ASContour(_ac);
        newContour->_isNew=false;
        int idx=0;
        if(idx1 == idx2){ //new vertex
            newVertex = new ASVertexContour(newContour,v->position(),idx,v->z());
            newVertex->setFinalPosition(v->finalPosition());
            newVertex->setInitialPosition(v->initialPosition());
            newVertex->setClosestClipVertex(v->closestClipVertex());
            newContour->addVertex(newVertex);
            newVertex->setEdge(new ASEdgeContour(newVertex, _vertexList.at(idx2+1)));
            newVertex->computeTangent();
            idx++;
        }
        // Process brush paths
        QListIterator<ASBrushPath*> it(_brushPaths);
        while(it.hasNext()){
            ASBrushPath* b = it.next();

            if(b->last()->sample()->index()<=idx1){
                continue;
            }
            if(b->first()->sample()->index()>=idx2){
                _brushPaths.removeAll(b);
                if(b->first()->sample()->index()==idx1){
                    b->first()->sample()->removeBrushVertex(b->first());
                    b->first()->setSample(newVertex);
                }
                newContour->_brushPaths<<b;
                continue;
            }
            if(b->first()->sample()->index()>=idx1 && b->last()->sample()->index()<=idx2){
                _brushPaths.removeAll(b);
                delete b;
                continue;
            }

            //Brush path crossing the split point
            ASBrushPath* newBrushPath = b->split(idx1,idx2,newVertex);

            if(newContour!=NULL && newBrushPath!=NULL){
                newContour->_brushPaths<<newBrushPath;
            }

            if(b->nbVertices()<2){
                _brushPaths.removeAll(b);
                delete b;
            }
        }

        if(idx1 == idx2)
            idx2++;

        for(int i=idx2; i<_vertexList.size(); ++i){
            v = _vertexList.at(i);
            v->setIndex(idx);
            idx++;
            v->setContour(newContour);
            newContour->addVertex(v);
        }
    }

    //Clean old snake
    for(int i=_vertexList.size()-1; i>idx1; --i){
        v = _vertexList.takeAt(i);
        if(i<idx2)
            delete v;
    }

    computeLength();
    checkClosed();

    if(newContour){
        newContour->computeLength();
        newContour->checkClosed();

        if(newContour->length()>_length){
            QColor newColor = newContour->debugColor();
            newContour->debugColor() = _debug_color;
            _debug_color = newColor;
        }
    }

    return newContour;
}

void ASContour::inverse() {
    ASVertexContour *v_prev;
    QList<ASVertexContour*> inversedContour;

    int idx=0;
    for(int k=nbVertices()-1; k>=0; --k){ //inverse indexes and edges
        ASVertexContour *v_cc = _vertexList.at(k);
        v_cc->setIndex(idx);
        idx++;
        if(k==0){
            v_cc->setEdge(NULL);
        }
        if(k<nbVertices()-1){
            delete v_prev->edge();
            v_prev->setEdge(new ASEdgeContour(v_prev,v_cc));
        }
        v_prev = v_cc;
        inversedContour<<v_cc;
    }
    _vertexList.clear();
    _vertexList.append(inversedContour);

    foreach(ASBrushPath*b,_brushPaths)
        b->inverse();
}

void ASContour::stitchAfter(ASContour* c) {

    if(!isNew() && !c->isNew()) {
        //Extend brush paths of c
        for(int i=0; i<c->first()->nbBrushVertices(); ++i){
            ASBrushVertex* vb = c->first()->brushVertex(i);
            ASBrushPath* b = vb->path();
            ASBrushVertex* newV = new ASBrushVertex(vb->path(),this->last(),vb->offset(),0.0,vb->timestamp());
            newV->setPenWidth(vb->penWidth());
            newV->setParam(b->first()->param() - b->slope()*dist(b->first()->position(),newV->position()));
            b->insertFirst(newV);
        }
    }

    int idx=nbVertices();
    ASVertexContour* v = _vertexList.at(idx-1);

    if(v->edge())
        delete v->edge();

    v->setEdge(new ASEdgeContour(v,c->at(0)));
    for(int i=0; i<c->nbVertices(); ++i){
        v = c->at(i);
        v->setIndex(idx);
        v->setContour(this);
        addVertex(v);
        idx++;
    }
    c->_vertexList.clear();

    if(!isNew() && !c->isNew()) {
        //Append brush paths of the second contour
        _brushPaths.append(c->_brushPaths);
    }else if(isNew() && !c->isNew()){
        qDeleteAll(_brushPaths);
        _brushPaths.clear();
        _brushPaths.append(c->_brushPaths);
        while(updateOverdraw()){};
        _isNew=false;
    }else if(c->isNew() && !isNew()){
        qDeleteAll(c->_brushPaths);
        c->_brushPaths.clear();
        while(updateOverdraw()){};
    }else{
        qDeleteAll(c->_brushPaths);
        qDeleteAll(_brushPaths);
        _brushPaths.clear();
        c->_brushPaths.clear();
        initParameterization();
    }
}

void ASContour::addVertexFirst(ASVertexContour* v) {
    //Extend brush paths
    for(int i=0; i<first()->nbBrushVertices(); i++){
        ASBrushVertex* vb = first()->brushVertex(i);
        ASBrushPath* b = vb->path();
        ASBrushVertex* newV = new ASBrushVertex(vb->path(),v,vb->offset());
        newV->setParam(vb->param() - b->slope()*dist(vb->position(),newV->position()));
        b->insertFirst(newV);
    }

    _vertexList.push_front(v);
    for(int i=0; i<nbVertices(); ++i){
        _vertexList.at(i)->setIndex(i);
    }
}

void ASContour::addVertexLast(ASVertexContour* v) {
    //Extend brush paths
    for(int i=0; i<last()->nbBrushVertices(); i++){
        ASBrushVertex* vb = last()->brushVertex(i);
        ASBrushPath* b = vb->path();
        ASBrushVertex* newV = new ASBrushVertex(b,v,vb->offset());
        newV->setParam(vb->param() + b->slope()*dist(vb->position(),newV->position()));
        b->insertLast(newV);
    }
    _vertexList << v;
}

ASContour* ASContour::removeLastVertex() {
    delete _vertexList.last();
    _vertexList.removeLast();
    cleanBrushPath();
    if(_vertexList.size()>0){
        _vertexList.last()->setEdge(NULL);
       checkClosed();
    }
    return NULL;
}

ASContour* ASContour::removeFirstVertex() {
    delete _vertexList.first();
    _vertexList.removeFirst();
    cleanBrushPath();
    for(int i=0; i<nbVertices(); ++i){
        _vertexList.at(i)->setIndex(i);
    }
    if(_vertexList.size()>0)
        checkClosed();
    return NULL;
}

void ASContour::print(){
    const ASVertexContour *v = at(0);
    QString output;
    QTextStream outputStream;
    outputStream.setString(&output);
    while(v!=NULL){
        outputStream<<v->index()<<" ("<<v->nbBrushVertices()<<": ";
        for(int i=0; i<v->nbBrushVertices();++i){
            outputStream<<v->brushVertex(i)->path();
            if(i<v->nbBrushVertices()-1)
                outputStream<<" , ";
        }
        outputStream<<") / ";//v->edge()<<") / ";
        v = v->following();
        if(_closed && v==first())
            break;
    }
    if(_closed)
        outputStream<<" closed ";
    if(_isNew)
        outputStream<<" new ";
    qDebug()<<output;
}


/****************************/
/*******   Fitting   ********/
/****************************/

void ASContour::fitLinearParam() {
    cleanBrushPath();

    // Per brush path fitting
    foreach(ASBrushPath* b, _brushPaths){

        int nbSamples = b->nbVertices();

        b->computeArcLength();

        vec2 *points=new vec2[nbSamples];
        for (int i=0;i<nbSamples;i++) {
            points[i]=vec2(b->at(i)->arcLength(),b->at(i)->param());
        }

        float slope, phase;
        heightLineFit(nbSamples,points,slope,phase);

        b->setSlope(slope);
        b->setPhase(phase);

        delete[] points;
    }
}

void ASContour::mergeBrushPaths() {
    // consider all pairs
    QList< QPair<ASBrushVertex*,ASBrushVertex*> > pairs;
    brushPathPairs(pairs);

    while(!pairs.empty()){
        QPair<ASBrushVertex*,ASBrushVertex*> p = pairs.takeFirst();
        ASBrushVertex* bv1 = p.first;
        ASBrushVertex* bv2 = p.second;

        // Too long to be merged
        if(k_useTargetLength &&
           (bv1->path()->last()->arcLength() + bv2->path()->last()->arcLength()) > k_targetLength
                && bv1->path()->last()->arcLength() > k_targetLength/4.0f
                && bv2->path()->last()->arcLength() > k_targetLength/4.0f ) {
            continue;
        }

        // Check if the merged BP wouldn't be split by abstraction

        ASBrushPathFitting fit = ASBrushPathFitting::instance();
        ACBrushpathStyleMode mode = (ACBrushpathStyleMode) k_fittingMode.index();;

        if (mode == AS_BRUSHPATH_LINE){
            ASBrushPath* mergeBP = new ASBrushPath(*bv2->path(),*bv1->path());

            float lineSegmentCost = k_lineSegmentCost * 0.75f; //0.75 for hysteresis
            fit.findBreakingPositionForLine(lineSegmentCost, mergeBP);
            if(fit.getSegmentInfoSet().size()>1)
                continue;

        }else if (mode == AS_BRUSHPATH_ARC){
            ASBrushPath* mergeBP = new ASBrushPath(*bv2->path(),*bv1->path());

            float arcSegmentCost = k_arcSegmentCost * 0.75f; //0.75 for hysteresis
            float arcTargetRadius = k_arcTargetRadius;
            float arcRadiusWeight = k_arcRadiusWeight;
            fit.findBreakingPositionForArc(arcSegmentCost, arcTargetRadius, arcRadiusWeight, mergeBP);
            if(fit.getSegmentInfoSet().size()>1)
                continue;
        }

        /*************** Parameterization *****************/

        if(k_checkParam){

            float slope1 = bv1->path()->slope();
            float slope2 = bv2->path()->slope();
            if(fabs(slope1)!=slope1 && fabs(slope2)==slope2 || fabs(slope1)==slope1 && fabs(slope2)!=slope2)
                continue;

            float phase1 = bv1->path()->phase();

            float intpart1=0, intpart2=0;
            float param1 = modff(bv1->param(),&intpart1);
            float param2 = modff(bv2->param(),&intpart2);
            float lastParam=0, firstParam=0, bv1ArcLength=0, bv2ArcLength=0;

            float maxIntPart = max(intpart1,intpart2);

            if((param2-param1) > 0.5){
                param1 += 1.0;
                lastParam += 1.0;
            }

            if((param1-param2) > 0.5){
                param2 += 1.0;
                firstParam += 1.0;
            }

            param1 += maxIntPart;
            param2 += maxIntPart;

            ASBrushVertex* bvLast = bv1->path()->last();
            ASBrushVertex* bvFirst = bv2->path()->first();

            firstParam += (bvFirst->param() - intpart2 + maxIntPart);
            lastParam  += (bvLast->param()  - intpart1 + maxIntPart);
            bv1ArcLength = bvLast->arcLength();
            bv2ArcLength = bv2->arcLength();

            float meanSlope = (lastParam - firstParam)/(bv2ArcLength + bv1ArcLength);
            float meanPhase = meanSlope * bv2ArcLength + firstParam;

            float diffphase = fmax(fmax(fabs(param1 - param2), fabs(param1 - meanPhase)), fabs(meanPhase - param2));

            // Merge the 2 brush paths if their parameterization match
            if(diffphase > k_mergePhaseTh || fabs(slope1 - slope2) > k_mergeSlopeTh )
            {
                if(diffphase < k_healingPhaseTh && fabs(slope1 - slope2) < k_healingSlopeTh) {
                    // "healing"
                    float w = 1.0 - fabs(meanPhase-param2)/k_healingPhaseTh;
                    bv2->path()->setSlope((param2 + w * (meanPhase-param2) - firstParam)/bv2ArcLength);

                    w = 1.0 - fabs(meanPhase-param1)/k_healingPhaseTh;
                    bv1->path()->setPhase(phase1 + w*(meanPhase-param1));
                    bv1->path()->setSlope((lastParam - bv1->path()->phase())/bv1ArcLength);

                    qDebug()<<"*** Healing";

                    bv1->path()->computeParam();
                    bv2->path()->computeParam();
                }
                continue;
            }
        }

        // random offsets, converge to the mid-offset
        if(k_randomOffsets){
            vec2 offset1 = bv1->path()->offset();
            vec2 offset2 = bv2->path()->offset();
            vec2 offsetAvg = 0.5f * (offset1 + offset2);
            if(dist2(offset1,offsetAvg) >= 1 || dist2(offset2,offsetAvg) >= 1) {
                qDebug()<<"*** Offsets healing";
                bv1->path()->setOffset(0.9f*offset1 + 0.1f*offsetAvg);
                bv2->path()->setOffset(0.9f*offset2 + 0.1f*offsetAvg);
                continue;
            }
        }

        // For line segments, averaging
        if (mode == AS_BRUSHPATH_LINE && k_fittingAveraging && k_averageMerge){

            float A = fit.getSegmentInfoSet()[0].A;
            float B = fit.getSegmentInfoSet()[0].B;
            float C = fit.getSegmentInfoSet()[0].C;
            vec2 start1, end1, start2, end2, pos1, pos2;
            fit.calculateEndPointsPosition(bv1->path(), A, B, C, pos1, pos2);
            vec2 similar1 = fit.averageLineEndPoint(bv1->path(), pos1, pos2, start1, end1);
            fit.calculateEndPointsPosition(bv2->path(), A, B, C, pos1, pos2);
            vec2 similar2 = fit.averageLineEndPoint(bv2->path(), pos1, pos2, start2, end2);
            float mergeThreshold = k_fittingMaxDistance;

            if (similar1[0] > mergeThreshold || similar1[1] > mergeThreshold
                    || similar2[0] > mergeThreshold || similar2[1] > mergeThreshold) //do not merge the two brushpaths yet they would move too much
            {
                fit.interpolateEndPoint(bv1->path(), start1, end1);
                fit.interpolateEndPoint(bv2->path(), start2, end2);
                continue;
            }
        }

        // Merge the two BP

        // Remove one of the two paths from the contour's BPs
        _brushPaths.removeAll(bv1->path());

        if(bv1->sample() == bv2->sample()){
            // Remove shared brush vertex
            bv1->sample()->removeBrushVertex(bv1);
            bv1 = bv1->removeFromBrushPath();
        }
        bv2->path()->merge(bv1->path());

        bv2->path()->computeParam();

        if (mode == AS_BRUSHPATH_LINE){
            bv2->path()->setFittingPara(fit.getSegmentInfoSet()[0].A, fit.getSegmentInfoSet()[0].B, fit.getSegmentInfoSet()[0].C, fit.getSegmentInfoSet()[0].cost);
            bv2->path()->setNewFitting(true);
            fit.calculateLinePosition(bv2->path());
            bv2->path()->setNewFitting(false);
        }
        else if (mode == AS_BRUSHPATH_ARC){
            bv2->path()->setFittingPara(fit.getSegmentInfoSet()[0].A, fit.getSegmentInfoSet()[0].B, fit.getSegmentInfoSet()[0].C, fit.getSegmentInfoSet()[0].cost);
            fit.calculateArcPosition(bv2->path());
        }

        pairs.clear();
        cleanBrushPath();
        brushPathPairs(pairs);
    }
}

void ASContour::heightLineFit(int numPoints, const vec2 *points, float &m, float &b) {
    // compute sums for linear system
    float fSumX = 0.0;
    float fSumY = 0.0;
    float fSumXX = 0.0;
    float fSumXY = 0.0;

    float n=numPoints;

    for (int i = 0; i < numPoints; i++)
    {
        fSumX += points[i][0];
        fSumY += points[i][1];
        fSumXX += points[i][0]*points[i][0];
        fSumXY += points[i][0]*points[i][1];
    }

    if(fSumX==0 && fSumXX==0){
        m=FLT_MAX;
        b=FLT_MAX;
    }else{
        m=(n*fSumXY-fSumX*fSumY)/(n*fSumXX-fSumX*fSumX);
        b=(fSumY*fSumXX-fSumX*fSumXY)/(n*fSumXX-fSumX*fSumX);
    }

    if(isnan(m) || isnan(b) || isinf(m) || isinf(b))
    {
        qCritical("NAN :-(");
        m = 100000;
    }
}

void ASContour::brushPathPairs(QList< QPair<ASBrushVertex*,ASBrushVertex*> > &pairs) {
    // consider all pairs
    for(int i=0; i<nbBrushPaths(); ++i) {
        ASBrushPath* bp1 = brushPath(i);
        if (bp1->nbVertices() == 0) // Sanity check
            continue;
        for(int j=i+1; j<nbBrushPaths(); ++j) {
            ASBrushPath* bp2 = brushPath(j);
            if (bp2->nbVertices() == 0) // Sanity check
                continue;

            ASBrushVertex *bv1, *bv2;

            // NB: all the brush paths are oriented coherently on a contour => only 2 cases
            if(bp1->first()->sample()==bp2->last()->sample() ||
               (!bp1->first()->sample()->isFirst() && bp1->first()->sample()->previous() == bp2->last()->sample())){
                bv1 = bp1->first();
                bv2 = bp2->last();
            }else if(bp1->last()->sample()==bp2->first()->sample() ||
                     (!bp1->last()->sample()->isLast() && bp1->last()->sample()->following()==bp2->first()->sample())){
                bv1 = bp2->first();
                bv2 = bp1->last();
            }else{
                continue;
            }
            // Too far away
            if(dist2(bv1->position(),bv2->position())>k_mergeDist*k_mergeDist)
                continue;

            vec2 t1 = bv1->tangent();
            vec2 t2 = bv2->tangent();
            // Tangents don't line up
            if(fabs(t1 DOT t2) < k_mergeDotProd)
                continue;

            if(abs(bv1->sample()->index()-bv2->sample()->index())<2)
                pairs<<QPair<ASBrushVertex*,ASBrushVertex*>(bv1,bv2);
        }
    }

    std::random_shuffle( pairs.begin(), pairs.end() );
}

/********************************/
/***** Brush paths overdraw *****/
/********************************/

void ASContour::recursiveSplit(ASBrushPath* b) {
    if(b->last()->arcLength() > k_targetLength && b->nbVertices()>5){
        int splitIdx = rand()%(b->nbVertices()/2)+b->nbVertices()/4;
        ASBrushPath* newB = b->split(splitIdx);
        recursiveSplit(b);
        newB->computeArcLength();
        recursiveSplit(newB);
    }else{
        _newBrushPaths<<b;
    }
}

bool ASContour::updateOverdraw() {
    cleanBrushPath();

    // If the target length is relevant
    if(k_useTargetLength){
        //Split too long brush paths
        _newBrushPaths.clear();
        foreach(ASBrushPath* b, _brushPaths){
            b->computeArcLength();
            recursiveSplit(b);
        }
        _brushPaths.clear();
        _brushPaths << _newBrushPaths;
    }

    // Local brush path extension / contraction at end points

    bool modified = false;

    foreach(ASBrushPath* b, _brushPaths){

        if(b->nbVertices()<2){
            continue;
        }

        ASBrushVertex* first = b->first();
        ASVertexContour* prev = first->sample()->previous();
        ASVertexContour* next = first->sample()->following();
        if(next != NULL && next->index() > first->sample()->index()) { //next sample exist and does't cross loop
            float firstOverdraw = first->sample()->overdraw();
            if(firstOverdraw > k_targetOverdraw){
                // could contract
                float alea = float(rand())/RAND_MAX;
                float proba = k_gammaContract*(1.0-k_targetOverdraw/firstOverdraw);
                if(alea <= proba){
                    first->removeFromBrushPath();
                    first->sample()->removeBrushVertex(first);
                    delete first;
                    b->computeArcLength();
#ifdef VERBOSE
                    qDebug()<<"*** Remove first brush vertex "<<b;
#endif
                    modified=true;
                    if(b->nbVertices()<2){
                        continue;
                    }
                    first = b->first();
                }
            }
        }
        if(prev != NULL && prev->index() < first->sample()->index()) { //previous sample exist and does't cross loop
            float prevOverdraw = prev->overdraw();
            if(prevOverdraw < k_targetOverdraw || prevOverdraw == k_targetOverdraw) {
                // could extend
                float alea = float(rand())/RAND_MAX;
                float proba = k_gammaExt*(1.0-prevOverdraw/k_targetOverdraw);
                if(alea <= proba){
                    ASBrushPath* bp = first->path();
                    ASBrushVertex* newV = new ASBrushVertex(bp,prev,bp->first()->offset());
                    newV->setParam(bp->first()->param() - bp->slope()*dist(newV->position(),bp->first()->position()));
                    bp->insertFirst(newV);
                    prev->addBrushVertex(newV);
                    bp->computeArcLength();
#ifdef VERBOSE
                    qDebug()<<"*** Insert first brush vertex "<<bp;
#endif
                    modified=true;
                    first = b->first();
                }
            }
        }
        ASBrushVertex* last = b->last();
        prev = last->sample()->previous();
        next = last->sample()->following();
        if(prev != NULL && prev->index() < last->sample()->index()) {
            float lastOverdraw = prev->overdraw();
            if(lastOverdraw > k_targetOverdraw){
                // could contract
                float alea = float(rand())/RAND_MAX;
                float proba = k_gammaContract*(1.0-k_targetOverdraw/lastOverdraw);
                if(alea <= proba){
                    last->removeFromBrushPath();
                    last->sample()->removeBrushVertex(last);
                    delete last;
#ifdef VERBOSE
                    qDebug()<<"*** Remove last brush vertex "<<b;
#endif
                    modified=true;
                    if(b->nbVertices()<2){
                        continue;
                    }
                    last = b->last();
                }
            }
        }
        if(next!=NULL && next->index() > last->sample()->index()){
            float follOverdraw = last->sample()->overdraw();
            if(follOverdraw < k_targetOverdraw || follOverdraw == k_targetOverdraw) {
                // could extend
                float alea = float(rand())/RAND_MAX;
                float proba = k_gammaExt*(1.0-follOverdraw/k_targetOverdraw);
                if(alea <= proba){
                    ASBrushPath* bp = last->path();
                    ASBrushVertex* newV = new ASBrushVertex(bp,next,bp->last()->offset());
                    newV->setParam(bp->last()->param() + b->slope()*dist(newV->position(),bp->last()->position()));
                    bp->insertLast(newV);
                    next->addBrushVertex(newV);
                    bp->computeArcLength();
#ifdef VERBOSE
                    qDebug()<<"*** Insert last brush vertex "<<bp;
#endif
                    modified=true;
                    last = b->last();
                }
            }
        }
    }

    cleanBrushPath();

    return modified;
}

void ASContour::updateTaper()
{
    foreach(ASBrushPath* b, _brushPaths) {

        float arclength;

        if(k_taperBP){
            arclength = b->last()->arcLength();
        }else{
            computeLength();
            arclength = _length;
        }

        float overshoot = 0.0;
        if(arclength >k_overshootTh){
            if(arclength > k_targetLength){
                overshoot = k_overshootMax;
            }else{
                overshoot = (k_overshootMax - k_overshootMin)*(arclength - k_overshootMin)/(k_targetLength - k_overshootMin) + k_overshootMin;
            }
        }

        arclength += overshoot*2.0;

        float taper_len = k_taperStart * arclength;
        float fade_len = k_fade * arclength;

        for(int i=0; i<b->nbVertices(); ++i){
            b->at(i)->setAlpha(1.0);
        }

        float dl;

        for(int i=0; i<b->nbVertices(); ++i) {
            ASBrushVertex* v = b->at(i);

            if(k_taperBP)
                dl = v->arcLength() + overshoot;
            else
                dl = v->sample()->arcLength()  + overshoot;

            float f=1.0f, a=1.0f;
            if (dl < taper_len)
                f = sqrt(1.0f - pow((taper_len-dl)/(taper_len),2.0f));

            if (dl < fade_len)
               a = pow((1.0f-pow((fade_len-dl)/(fade_len),2.0f)),1.5f);

            v->setPenWidth(v->penWidth() * f + k_taperMin/k_penWidth);
            v->setAlpha(v->alpha() * a);

        }

        taper_len = k_taperEnd * arclength;

        for(int i=b->nbVertices()-1; i>=0; --i) {
            ASBrushVertex* v = b->at(i);

            if(k_taperBP)
                dl = arclength - v->arcLength() + overshoot;
            else
                dl =arclength - v->sample()->arcLength()  + overshoot;

            float f=1.0, a=1.0;
            if (dl < taper_len)
                f = sqrt(1 - pow((taper_len-dl)/(taper_len),2));

            if (dl < fade_len)
               a = pow((1.0f-pow((fade_len-dl)/(fade_len),2.0f)),1.5f);

            v->setPenWidth(v->penWidth() * f + k_taperMin/k_penWidth);
            v->setAlpha(v->alpha() * a);
        }

        for(int i=0; i<b->nbVertices(); ++i) {
            ASBrushVertex* v = b->at(i);
            if(k_confToThickness)
                v->setPenWidth(v->penWidth()*v->sample()->alpha());
            v->updatePenWidth();
        }
    }
}

void ASContour::removeAllBrushPath(){
    qDeleteAll(_brushPaths);
    _brushPaths.clear();
}
