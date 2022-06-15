/*****************************************************************************\

ASSnakes.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "ASSnakes.h"
#include "ASEdgeContour.h"
#include "ASClipPath.h"
#include "GQGPUImageProcessing.h"
#include <QDebug>
#include <QTextStream>
#include <QDir>
#include <QFile>

#include "GQDraw.h"
#include "DialsAndKnobs.h"
#include "Stats.h"
#include <QSet>

#include <float.h>

//#define VERBOSE

static dkBool  k_enableRelaxation("Contours->Relaxation->Activate", true);
static dkInt   k_blurIter("Contours->Relaxation->Blur iterations", 2, 0, 100, 1);
static dkFloat k_samplingMax("Contours->Resampling->s max", 6.0f);
static dkFloat k_samplingMin("Contours->Resampling->s min", 4.0f);
       dkFloat k_coverRadius("Contours->Topology->Cover radius", 5.0f);
static dkFloat k_depthJump("Contours->Topology->Depth jump max", 50.0f);

extern dkBool  k_drawGrid;
static dkBool  k_sortsnakes("Contours->Topology->Sort", true);
static dkBool  k_useAdvection("Contours->Advection", true);
static dkBool  k_relAdvection("Contours->Adv. relative", true);

static dkInt   k_iterCoverage("Contours->Topology->Cover iter",10);
static dkFloat k_minLength("Contours->Topology->Min length", 2.0,0.0,10000.0,0.5);
static dkFloat k_minLengthHyst("Contours->Topology->Min length hyst", 0.5,0.0,1.0,0.1);
static dkBool  k_enableTopology("Contours->Topology->Activate", true);
static dkBool  k_enableSplit("Contours->Topology->Split", true);
static dkBool  k_enableSplitAtJunctions("Contours->Topology->Split junctions", false);
static dkBool  k_enableTrim("Contours->Topology->Trim", true);
static dkBool  k_enableMerge("Contours->Topology->Merge", true);
static dkBool  k_enableCoverage("Contours->Topology->Coverage", true);
static dkFloat k_dotProdT("Contours->Topology->Dot Prod.", 0.85, 0.0, 1.0, 0.05);
static dkFloat k_splitDotProd("Contours->Topology->Dot Prod. split", 0.5, 0.0, 1.0, 0.05);
       dkFloat k_dotProdCov("Contours->Topology->Dot Prod. cov.", 0.65, 0.0, 1.0, 0.05);
static dkFloat k_topoWeight("Contours->Topology->Weight", 0.8, 0.0, 1.0, 0.05);
static dkBool  k_useVisibility("Contours->Topology->Visibility", false);
extern dkFloat k_visibilityTh;
static dkBool  k_initBP("BrushPaths->Confidence->Init",false);
static dkBool  k_mergeBP("BrushPaths->Merge->Activate", true);

dkFloat k_trimRatio("Contours->Topology->Trim ratio", 1.5f,0.1f,100.f,1.f);

ASSnakes::ASSnakes()
{
    _refImg = NULL;
    _sMax = k_samplingMax.value();
    _sMin = k_samplingMin.value();
}

ASSnakes::~ASSnakes()
{
    clear();
}

void ASSnakes::clear()
{
    qDeleteAll(_contourList);
    _contourList.clear();
    _simpleGrid.clear();
}

void ASSnakes::init(ASClipPathSet& pathSet, bool noConnectivity)
{
    _width = pathSet.viewportWidth();

    _noConnectivity = noConnectivity;

    if(!_noConnectivity){
        for(int i=0; i<pathSet.size(); i++){
            ASClipPath* p = pathSet[i];
            ASContour* c = new ASContour(this, *p, k_visibilityTh);
            if(c->nbVertices()<=1)
                delete c;
            else
                _contourList.append(c);
        }
    }

    /****** Needs to be done before building the grid ! ********/
    connect(&k_samplingMin,SIGNAL(valueChanged(double)),this,SLOT(setSMin(double)));
    connect(&k_samplingMax,SIGNAL(valueChanged(double)),this,SLOT(setSMax(double)));
    connect(&k_coverRadius,SIGNAL(valueChanged(double)),this,SLOT(setCoverRadius(double)));

    setSMax(k_samplingMax);
    setSMin(k_samplingMin);
    setCoverRadius(k_coverRadius);
    /********************************************************/

    buildSimpleGrid(pathSet);
    addSnakesToGrid();

    if(_noConnectivity){
        //create connectivity using coverage
        coverage(pathSet);
    }
    minLengthCleaning();
    remove();
}

void ASSnakes::setSMin(double min) {
    _sMin=min;
}

void ASSnakes::setSMax(double max) {
    _sMax = max;
}

void ASSnakes::setCoverRadius(double radius) {
    _coverRadius = radius*radius;
    _simpleGrid.setCellSize(ceil(radius*2.0));
}

void ASSnakes::updateRefImage(GQTexture2D* refImg, GQFloatImage* geomFlow, GQFloatImage* denseFlow, ASClipPathSet& pathSet, bool useMotion)
{
    _refImg = refImg;
    _width = refImg->width();

    /******************* ADVECTION **********************/
    if(k_useAdvection && useMotion) {
        __TIME_CODE_BLOCK("Advection");
        advect(geomFlow,denseFlow);
    }

    GQFloatImage fext(_width,_refImg->height(),4);

    buildSimpleGrid(pathSet);

    /************ Attraction field computation **********/

    // Blur and gradient on the GPU
    GQGPUImageProcessing::blurAndGrad(k_blurIter, refImg, fext);

    /*************** RELAXATION ****************/
    {
        __TIME_CODE_BLOCK("Relaxation");
        // Deform the contours
        for(int i=0; i<_contourList.size(); i++){
            ASContour* c = _contourList[i];
            c->notNew();
            c->computeLength();
            c->checkClosed();

            if(k_enableRelaxation){
                _deformer.iterate(*c,fext);
            }else{
                while(c->resample()){};
            }
        }
    }

    /*************** CLEANING ****************/
    addSnakesToGrid();

    findClosestEdgeRef();

    if(k_enableTopology){

        __TIME_CODE_BLOCK("Topology");

         if(k_enableCoverage || k_enableSplit || k_enableMerge)
            remove(); // because "findClosestEdgeRef" decreases the confidence

        if(k_enableSplit){
            splitContoursTangent();
        }

        if(k_enableTrim){
            trim();
            remove(); // because "trim" decreases the confidence
        }

        if(k_enableCoverage)
            coverage(pathSet);

        if(k_enableSplitAtJunctions)
            splitAtJunctions();

        if(k_enableMerge)
            merge();

        if(k_minLength > 0.0){
            minLengthCleaning();
            remove(); // because "minLengthCleaning" decreases the confidence
        }
    }

    /*************** BRUSH PATHS ****************/
    fitBrushPath();
}


void ASSnakes::addSnakesToGrid()
{
    if(k_drawGrid){
        glPointSize(1.f);
        _simpleGrid.startDrawGrid(_simpleGrid.origin(),_simpleGrid.cellSize(),false);
        glColor3f(1.0f,0.0f,0.0f);
        glBegin(GL_POINTS);
    }

    //fill in the grid with the snakes
    for(int i=0; i<_contourList.size(); i++){
        _simpleGrid.addSnakeToGrid(_contourList[i]);
    }

    if(k_drawGrid){
        glEnd();
        _simpleGrid.stopDrawGrid();
    }
}

void ASSnakes::buildSimpleGrid(ASClipPathSet& pathSet)
{
    _simpleGrid.init(_width, pathSet);
}

void ASSnakes::trimEnd(ASVertexContour** endPt)
{
#ifdef VERBOSE
    qDebug()<<"### Trim: "<<(*endPt)->index()<<" of "<<_contourList.indexOf(endPtContour);
#endif

    (*endPt)->decrConfidence();
    (*endPt)->decrConfidence();
}

bool ASSnakes::trim()
{
    bool modified=false;

    QListIterator<ASContour*> itC = QListIterator<ASContour*>(_contourList);

    while(itC.hasNext()){
        ASContour* contour = itC.next();

        QList<ASVertexContour*> endPts, visitedVertices;
        endPts<<contour->first();
        endPts<<contour->last();
        visitedVertices.append(endPts);

        while(!endPts.isEmpty()){
            ASVertexContour* endPt = endPts.takeFirst();
            endPt->computeTangent();
            vec2 tangent = endPt->tangent();

            vec2i offsets;
            int key = _simpleGrid.posToKey(endPt->position(),offsets);
            bool found = false;
            ASCell* cell;

            // Find a snake locally parallel to this endpoint
            for (int i = 0 ; i < 2 && !found; ++i) {
                for (int j = 0 ; j < 2 && !found; ++j) {
                    int key2 =  key + i*offsets[1] + j*offsets[0]*_simpleGrid.nbCols();
                    if(_simpleGrid.contains(key2)){
                        cell = _simpleGrid[key2];

                        QListIterator<ASVertexContour*> itV = cell->contourVerticesIterator();
                        while(itV.hasNext() && !found){
                            ASVertexContour* v = itV.next();
                            if(v->contour()==contour || v->confidence()==0 || (v->isHidden() && !endPt->isHidden()))
                                continue;

                            float dist = dist2(endPt->position(),v->position());
                            if(dist<_coverRadius/(k_trimRatio*k_trimRatio)){
                                v->computeTangent();
                                float dotProd = (v->tangent() DOT tangent);
                                if(fabs(dotProd) >= k_dotProdT){
                                    found = true;
                                }
                            }
                        }
                    }
                }
            }

            if(!found)
                continue;

            bool notFound=false;

            // Ensure that the coverage is preserved if this vertex is removed
            for (int i = 0 ; i < 2 && !notFound; ++i) {
                for (int j = 0 ; j < 2 && !notFound; ++j) {
                    int key2 =  key + i*offsets[1] + j*offsets[0]*_simpleGrid.nbCols();
                    if(_simpleGrid.contains(key2)){
                        cell = _simpleGrid[key2];

                        QList<ASClipVertex*> clipVertices = cell->clipVertices();

                        // Check all the clip vertices in this cell that this vertex covers
                        for(int k=0; k < clipVertices.size() && !notFound; ++k){
                            bool foundCV=false;
                            ASClipVertex* cv = clipVertices.at(k);
                            if(dist2(cv->position2D(),endPt->position())>_coverRadius) // not covered by this vertex
                                continue;

                            vec2i offsetsCV;
                            int keyCV = _simpleGrid.posToKey(cv->position2D(),offsetsCV);

                            for (int ii=0 ; ii<2 && !foundCV; ++ii) {
                                for (int jj=0 ; jj<2 && !foundCV; ++jj) {
                                    int keyCV2 = keyCV + ii*offsetsCV[1] + jj*offsetsCV[0]*_simpleGrid.nbCols();
                                    if(_simpleGrid.contains(keyCV2)){
                                            ASCell* cell2 = _simpleGrid[keyCV2];

                                        QListIterator<ASVertexContour*> itV = cell2->contourVerticesIterator();

                                        while(itV.hasNext() && !foundCV){
                                            ASVertexContour* v = itV.next();
                                            if(v==endPt || v->confidence()==0)
                                                continue;

                                            float dist = dist2(cv->position2D(),v->position());
                                            if(dist<=_coverRadius){
                                                v->computeTangent();
                                                float dotProd = fabs(v->tangent() DOT cv->tangent());
                                                if(dotProd >= k_dotProdCov){
                                                    foundCV = true; // covered by another vertex
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            if(foundCV==false)
                                notFound = true;
                        }
                    }
                }
            }

            if(!notFound){
                modified = true;

                trimEnd(&endPt);

                if(endPt==NULL)
                    break;
                if(endPt->index() <= ((float)contour->nbVertices()-1.0)/2.0 && !visitedVertices.contains(endPt->following())){
                    endPts.push_back(endPt->following());
                    visitedVertices.push_back(endPt->following());
                }else if(endPt->index() > ((float)contour->nbVertices()-1.0)/2.0 && !visitedVertices.contains(endPt->previous())){
                    endPts.push_back(endPt->previous());
                    visitedVertices.push_back(endPt->previous());
                }
            }
        }
    }

    return modified;
}

void ASSnakes::splitAtIndices(ASContour* c, QList<int>& splitIndices)
{
    std::sort(splitIndices.begin(),splitIndices.end());
    QListIterator<int> it(splitIndices);
    it.toBack();
    while(it.hasPrevious()){
        int idx = it.previous();
        if(idx==0){
            int key = _simpleGrid.posToKey(c->first()->position());
            _simpleGrid[key]->removeEndPoint(c->first());
            _simpleGrid[key]->removeContourVertex(c->first());
            c->removeFirstVertex();
            float r,cc;
            _simpleGrid.posToCellCoord(c->first()->position(),r,cc);
            key = cc+_simpleGrid.nbCols()*r;
            if(!_simpleGrid.contains(key))
                _simpleGrid.insert(key,new ASCell(r,cc));
            _simpleGrid[key]->addEndPoint(c->first());
        }else if(idx==c->nbVertices()-1){
            int key = _simpleGrid.posToKey(c->last()->position());
            _simpleGrid[key]->removeEndPoint(c->last());
            _simpleGrid[key]->removeContourVertex(c->last());
            c->removeLastVertex();
            float r,cc;
            _simpleGrid.posToCellCoord(c->last()->position(),r,cc);
            key = cc+_simpleGrid.nbCols()*r;
            if(!_simpleGrid.contains(key))
                _simpleGrid.insert(key,new ASCell(r,cc));
            _simpleGrid[key]->addEndPoint(c->last());
        }else{
            ASContour* newContour = c->split(idx,idx);

            ASVertexContour* newEnd = c->last();
            float r,cc;
            _simpleGrid.posToCellCoord(newEnd->position(),r,cc);
            int key = cc+_simpleGrid.nbCols()*r;
            if(!_simpleGrid.contains(key))
                _simpleGrid.insert(key,new ASCell(r,cc));
            _simpleGrid[key]->addEndPoint(newEnd);

            if(newContour != NULL){
                _contourList<<newContour;
                ASVertexContour* newStart = newContour->first();
                float r,cc;
                _simpleGrid.posToCellCoord(newStart->position(),r,cc);
                key = cc+_simpleGrid.nbCols()*r;
                if(!_simpleGrid.contains(key))
                    _simpleGrid.insert(key,new ASCell(r,cc));
                _simpleGrid[key]->addEndPoint(newStart);
            }
        }
    }
    if(c->nbVertices()==1){
        removeContourFromGrid(c);
        _contourList.removeAll(c);
        delete c;
    }
}

void ASSnakes::splitDepthJump()
{
    QListIterator<ASContour*> itC = QListIterator<ASContour*>(_contourList);
    while(itC.hasNext()){
        ASContour* c = itC.next();

        ASVertexContour* v_prev = c->first();

        QList<int> splitIndices;

        for(int j=1; j<c->nbVertices(); ++j){
            ASVertexContour* v_cour = c->at(j);
            if(fabs(v_prev ->z()*float(_width) - v_cour->z()*float(_width)) > k_depthJump){
                if(!splitIndices.contains(j)){
#ifdef VERBOSE
                    qDebug()<<"### Split depth"<<_contourList.indexOf(c)<<": "<<j;
#endif
                    splitIndices<<j;
                }
            }
            v_prev = v_cour;
        }
        splitAtIndices(c,splitIndices);
    }
}

void ASSnakes::splitContoursTangent()
{
    QListIterator<ASContour*> itC = QListIterator<ASContour*>(_contourList);
    while(itC.hasNext()){
        ASContour* c = itC.next();

        if(c->nbVertices()<=5)
            continue;

        c->computeTangent();

        ASVertexContour* v_prev = c->first();

        QList<int> splitIndices;

        for(int j=1; j<c->nbVertices()-1; ++j){
            ASVertexContour* v_cour = c->at(j+1);
            float dotProd = (v_prev->tangent() DOT v_cour->tangent());
            if(dotProd <= -k_splitDotProd) { //record split
                if(!splitIndices.contains(j)){
#ifdef VERBOSE
                    qDebug()<<"### Split "<<_contourList.indexOf(c)<<": "<<j;
#endif
                    splitIndices << j;
                }
            }
            v_prev = c->at(j);
        }
        splitAtIndices(c,splitIndices);
    }
}

void ASSnakes::merge()
{
    QHashIterator<int,ASCell*> itCell(_simpleGrid.hash());
    while(itCell.hasNext()){
        itCell.next();
        ASCell* cell = itCell.value();
        int key = itCell.key();

        initIterator:

        QListIterator<ASVertexContour*> itEndPoints = cell->endPointsIterator();

        nextIterator:
        while(itEndPoints.hasNext()){

            ASVertexContour* endPt = itEndPoints.next();
            ASContour* endPtContour = endPt->contour();
            if(!_contourList.contains(endPtContour) || (endPt->closestClipVertex() && endPt->closestClipVertex()->visibility()<k_visibilityTh)) //sanity check
                continue;

            endPt->computeTangent();

            QList<QPair<float,ASVertexContour*> > neighbors;

            for(int i=-1; i<=1; ++i){
                for(int j=-1; j<=1; ++j){
                    int key2 = key+i+j*_simpleGrid.nbCols();
                    if(_simpleGrid.contains(key2)){
                        ASCell* c = _simpleGrid[key2];
                        for(int k=0; k<c->nbEndPoints(); ++k){
                            ASVertexContour* endPt2 = c->endPoint(k);
                            if(endPt==endPt2 || endPtContour == endPt2->contour() ||(endPt2->closestClipVertex() && endPt2->closestClipVertex()->visibility()<k_visibilityTh))
                                continue;

                            //Estimate distance
                            float D=dist2(endPt2->position(),endPt->position());
                            if(D<=4.0*_coverRadius){
                                //Estimate tangent
                                endPt2->computeTangent();
                                vec2 tangentEndPt2;
                                vec2 tangentJoin1;
                                vec2 tangentJoin2;
                                vec2 tangentEndPt;
                                if(endPt->isFirst() && endPt2->isFirst()){
                                    tangentJoin1 = endPt2->position() - endPt->following()->position();
                                    tangentJoin2 = endPt->position() - endPt2->following()->position();
                                    tangentEndPt2 = endPt2->tangent();
                                    tangentEndPt = endPt->tangent();
                                }else if(endPt->isLast() && endPt2->isFirst()){
                                    tangentJoin1 = endPt2->position() - endPt->previous()->position();
                                    tangentJoin2 = endPt->position() - endPt2->following()->position();
                                    tangentEndPt2 = endPt2->tangent();
                                    tangentEndPt = -endPt->tangent();
                                }else if(endPt->isFirst() && endPt2->isLast()){
                                    tangentJoin1 = endPt2->position() - endPt->following()->position();
                                    tangentJoin2 = endPt->position() - endPt2->previous()->position();
                                    tangentEndPt2 = -endPt2->tangent();
                                    tangentEndPt = endPt->tangent();
                                }else if(endPt->isLast()
                                    && endPt2->isLast()){
                                    tangentJoin1 = endPt2->position() - endPt->previous()->position();
                                    tangentJoin2 = endPt->position() - endPt2->previous()->position();
                                    tangentEndPt2 = -endPt2->tangent();
                                    tangentEndPt = -endPt->tangent();
                                }
                                normalize(tangentJoin1);
                                normalize(tangentJoin2);
                                float dotProd1 = (tangentJoin1 DOT tangentEndPt2);
                                float dotProd2 = (tangentJoin2 DOT tangentEndPt);
                                float dotProd = std::min(dotProd1,dotProd2);
                                if(dotProd >= k_dotProdT){
                                    float coef = k_topoWeight*((1.0-dotProd)/(1.0-k_dotProdT)) + (1.0-k_topoWeight)*D/(4.0*_coverRadius);
                                    neighbors << QPair<float,ASVertexContour*>(coef,endPt2);
                                }
                            }
                        }
                    }
                }
            }

            if(neighbors.isEmpty())
                continue;

            std::sort(neighbors.begin(),neighbors.end());

            bool found = false;
            ASVertexContour* closestEndPt = 0;
            ASContour* closestEndPtContour = 0;
            vec2 midPos, midTangent;
            float midZ, midDist;
            ASClipVertex *closestCV = 0;
            vec2i offsets;

            while(!found && !neighbors.isEmpty()){

                closestEndPt = neighbors.takeFirst().second;
                closestEndPtContour = closestEndPt->contour();

                midPos = 0.5f*(endPt->position()+closestEndPt->position());
                midTangent = (closestEndPt->position() - endPt->position());
                midDist = len2(midTangent);
                normalize(midTangent);

                closestCV = findClosestEdgeRef(midPos,midTangent,false,_coverRadius);
                if(closestCV!=NULL && closestCV->visibility()>=k_visibilityTh)
                    found=true;
            }
            if(!found)
                continue;

            // Cross another snake?
            for (int i = 0 ; i < 2 && !found; ++i) {
                for (int j = 0 ; j < 2 && !found; ++j) {
                    int key2 =  key + i*offsets[1] + j*offsets[0]*_simpleGrid.nbCols();
                    if(_simpleGrid.contains(key2)){
                        cell = _simpleGrid[key2];

                        for(int k=0; k<cell->nbContourVertices(); ++k){
                            ASVertexContour* v = cell->contourVertex(k);
                            if(v->contour() == closestEndPtContour || v->contour() == endPtContour)
                                continue;
                            float distance = dist2(v->position(),midPos);
                            if(distance <= midDist/4.0){
                                goto nextIterator;
                            }
                        }
                    }
                }
            }
            midZ = closestCV->position()[2];


            //update grid
            removeEndPointsFromGrid(endPtContour);
            removeEndPointsFromGrid(closestEndPtContour);

            endPtContour->computeLength();
            closestEndPtContour->computeLength();

            if(endPt->isFirst() && closestEndPt->isFirst()){

                if(endPtContour->length() > closestEndPtContour->length()){
                    ASContour* tmpContour = endPtContour;
                    endPtContour = closestEndPtContour;
                    closestEndPtContour = tmpContour;
                }

#ifdef VERBOSE
                qDebug()<<"### Merge 0: "<<_contourList.indexOf(endPtContour)<<" with "<<_contourList.indexOf(closestEndPtContour);
#endif
                _contourList.removeAll(closestEndPtContour);

                endPtContour->inverse();

                endPtContour->removeLastVertex();
                ASVertexContour* newV = new ASVertexContour(endPtContour,midPos,endPtContour->nbVertices(),midZ);
                newV->setInitialPosition(0.5f*(endPtContour->last()->initialPosition()+closestEndPtContour->first()->initialPosition()));
                newV->setFinalPosition(0.5f*(endPtContour->last()->finalPosition()+closestEndPtContour->first()->finalPosition()));
                newV->setClosestClipVertex(closestCV);
                if(!_simpleGrid.contains(key) || !_simpleGrid[key]){
                    float r,c;
                    _simpleGrid.posToCellCoord(midPos,r,c);
                    ASCell* cell = new ASCell(r,c);
                    _simpleGrid.insert(key,cell);
                    assert(_simpleGrid[key]);
                }
                assert(_simpleGrid[key]);
                _simpleGrid[key]->addContourVertex(newV);
                endPtContour->last()->setEdge(new ASEdgeContour(endPtContour->last(),newV));
                newV->setTangent(endPtContour->last()->edge()->direction());
                endPtContour->addVertexLast(newV);
                closestEndPtContour->removeFirstVertex();

                endPtContour->debugColor() = closestEndPtContour->debugColor();

                endPtContour->stitchAfter(closestEndPtContour);

                addEndPointsToGrid(endPtContour);

            }else if(endPt->isLast() && closestEndPt->isFirst()){

#ifdef VERBOSE
                qDebug()<<"### Merge 1: "<<_contourList.indexOf(endPtContour)<<" with "<<_contourList.indexOf(closestEndPtContour);
#endif
                _contourList.removeAll(closestEndPtContour);

                endPtContour->removeLastVertex();
                ASVertexContour* newV = new ASVertexContour(endPtContour,midPos,endPtContour->nbVertices(),midZ);
                newV->setInitialPosition(0.5f*(endPtContour->last()->initialPosition()+closestEndPtContour->first()->initialPosition()));
                newV->setFinalPosition(0.5f*(endPtContour->last()->finalPosition()+closestEndPtContour->first()->finalPosition()));
                newV->setClosestClipVertex(closestCV);
                if(!_simpleGrid.contains(key) || !_simpleGrid[key]){
                    float r,c;
                    _simpleGrid.posToCellCoord(midPos,r,c);
                    ASCell* cell = new ASCell(r,c);
                    _simpleGrid[key] = cell;
                }
                assert(_simpleGrid[key]);
                _simpleGrid[key]->addContourVertex(newV);
                endPtContour->last()->setEdge(new ASEdgeContour(endPtContour->last(),newV));
                newV->setTangent(endPtContour->last()->edge()->direction());
                endPtContour->addVertexLast(newV);
                closestEndPtContour->removeFirstVertex();

                if(endPtContour->length() < closestEndPtContour->length()){
                    endPtContour->debugColor() = closestEndPtContour->debugColor();
                }

                endPtContour->stitchAfter(closestEndPtContour);

                addEndPointsToGrid(endPtContour);

            }else if(endPt->isFirst() && closestEndPt->isLast()){

#ifdef VERBOSE
                qDebug()<<"### Merge 2: "<<_contourList.indexOf(closestEndPtContour)<<" with "<<_contourList.indexOf(endPtContour);
#endif
                _contourList.removeAll(endPtContour);

                closestEndPtContour->removeLastVertex();
                ASVertexContour* newV = new ASVertexContour(closestEndPtContour,midPos,closestEndPtContour->nbVertices(),midZ);
                newV->setInitialPosition(0.5f*(endPtContour->first()->initialPosition()+closestEndPtContour->last()->initialPosition()));
                newV->setFinalPosition(0.5f*(endPtContour->first()->finalPosition()+closestEndPtContour->last()->finalPosition()));
                newV->setClosestClipVertex(closestCV);
                if(!_simpleGrid.contains(key) || !_simpleGrid[key]){
                    float r,c;
                    _simpleGrid.posToCellCoord(midPos,r,c);
                    ASCell* cell = new ASCell(r,c);
                    _simpleGrid[key] = cell;
                }
                assert(_simpleGrid[key]);
                _simpleGrid[key]->addContourVertex(newV);
                closestEndPtContour->last()->setEdge(new ASEdgeContour(closestEndPtContour->last(),newV));
                newV->setTangent(closestEndPtContour->last()->edge()->direction());
                closestEndPtContour->addVertexLast(newV);
                endPtContour->removeFirstVertex();

                if(endPtContour->length() > closestEndPtContour->length()){
                    closestEndPtContour->debugColor() = endPtContour->debugColor();
                }

                closestEndPtContour->stitchAfter(endPtContour);

                addEndPointsToGrid(closestEndPtContour);

            }else if(endPt->isLast()
                && closestEndPt->isLast()){

                endPtContour->computeLength();
                closestEndPtContour->length();
                if(endPtContour->length() > closestEndPtContour->length()){
                    ASContour* tmpContour = endPtContour;
                    endPtContour = closestEndPtContour;
                    closestEndPtContour = tmpContour;
                }
#ifdef VERBOSE
                qDebug()<<"### Merge 3: "<<_contourList.indexOf(closestEndPtContour)<<" with "<<_contourList.indexOf(endPtContour);
#endif
                _contourList.removeAll(endPtContour);

                endPtContour->inverse();

                closestEndPtContour->removeLastVertex();
                ASVertexContour* newV = new ASVertexContour(closestEndPtContour,midPos,closestEndPtContour->nbVertices(),midZ);
                newV->setInitialPosition(0.5f*(endPtContour->first()->initialPosition()+closestEndPtContour->last()->initialPosition()));
                newV->setFinalPosition(0.5f*(endPtContour->first()->finalPosition()+closestEndPtContour->last()->finalPosition()));
                newV->setClosestClipVertex(closestCV);

                if(!_simpleGrid.contains(key) || !_simpleGrid[key]){
                    float r,c;
                    _simpleGrid.posToCellCoord(midPos,r,c);
                    ASCell* cell = new ASCell(r,c);
                    _simpleGrid[key] = cell;
                }
                assert(_simpleGrid[key]);
                _simpleGrid[key]->addContourVertex(newV);
                closestEndPtContour->last()->setEdge(new ASEdgeContour(closestEndPtContour->last(),newV));
                newV->setTangent(closestEndPtContour->last()->edge()->direction());
                closestEndPtContour->addVertexLast(newV);
                endPtContour->removeFirstVertex();

                closestEndPtContour->stitchAfter(endPtContour);

                addEndPointsToGrid(closestEndPtContour);
            }
            goto initIterator;
        }
    }
}

void ASSnakes::splitAtJunctions()
{
    QHashIterator<int,ASCell*> itCell(_simpleGrid.hash());
    //all the cells
    while(itCell.hasNext()){
        itCell.next();
        ASCell* cell = itCell.value();

        QListIterator<ASVertexContour*> itEndPoints = cell->endPointsIterator();

        // all the endpoints
        while(itEndPoints.hasNext()){
            ASVertexContour* endPt = itEndPoints.next();
            if(!endPt->closestClipVertex() || endPt->closestClipVertex()->visibility()<k_visibilityTh)
                continue;

            ASContour* endPtContour = endPt->contour();

            QList<QPair<float,ASVertexContour*> > neighbors;

            QListIterator<ASVertexContour*> itVertices = cell->contourVerticesIterator();
            while(itVertices.hasNext()){
                ASVertexContour* endPt2 = itVertices.next();

                if(endPt2->contour()==endPtContour || endPt2->isFirst() || endPt2->isLast() ||
                   !endPt2->closestClipVertex() || endPt2->closestClipVertex()->visibility()<k_visibilityTh)
                    continue;

                //Estimate distance
                float D=dist2(endPt2->position(),endPt->position());
                if(D<=4.0*_coverRadius){
                    //Estimate tangent
                    vec2 tangentJoin1;
                    vec2 tangentJoin2F = endPt->position() - endPt2->following()->position();
                    vec2 tangentJoin2P = endPt->position() - endPt2->previous()->position();
                    vec2 tangentEndPt;
                    vec2 tangentEndPt2F = endPt2->following()->position() - endPt2->position();
                    normalize(tangentEndPt2F);
                    vec2 tangentEndPt2P = endPt2->previous()->position() - endPt2->position();
                    normalize(tangentEndPt2P);
                    if(endPt->isFirst()){
                        tangentJoin1 = endPt2->position() - endPt->following()->position();
                        tangentEndPt = endPt->tangent();
                    }else if(endPt->isLast()){
                        tangentJoin1 = endPt2->position() - endPt->previous()->position();
                        tangentEndPt = -endPt->tangent();
                    }
                    normalize(tangentJoin1);
                    normalize(tangentJoin2F);
                    normalize(tangentJoin2P);
                    float dotProd1 = (tangentJoin1 DOT tangentEndPt2F);
                    float dotProd2 = (tangentJoin2F DOT tangentEndPt);
                    float dotProd = std::min(dotProd1,dotProd2);
                    if(dotProd >= k_dotProdT){
                        float coef = k_topoWeight*((1.0-dotProd)/(1.0-k_dotProdT)) + (1.0-k_topoWeight)*D/(4.0*_coverRadius);
                        neighbors << QPair<float,ASVertexContour*>(coef,endPt2);
                    }

                    dotProd1 = (tangentJoin1 DOT tangentEndPt2P);
                    dotProd2 = (tangentJoin2P DOT tangentEndPt);
                    dotProd = std::min(dotProd1,dotProd2);
                    if(dotProd >= k_dotProdT){
                        float coef = k_topoWeight*((1.0-dotProd)/(1.0-k_dotProdT)) + (1.0-k_topoWeight)*D/(4.0*_coverRadius);
                        neighbors << QPair<float,ASVertexContour*>(coef,endPt2);
                    }
                }
            }

            if(neighbors.isEmpty())
                continue;

            std::sort(neighbors.begin(),neighbors.end());

            bool found = false;
            ASVertexContour* closestEndPt;
            ASContour* closestEndPtContour;
            vec2 midPos;
            float closestCoef;

            while(!found && !neighbors.isEmpty()){

                QPair<float,ASVertexContour*> pair = neighbors.takeFirst();
                closestEndPt = pair.second;
                closestCoef = pair.first;
                closestEndPtContour = closestEndPt->contour();

                midPos = 0.5f*(endPt->position()+closestEndPt->position());

                vec2i offsets;
                int key = _simpleGrid.posToKey(midPos,offsets);


                for (int i = 0 ; i < 2 && !found; ++i) {
                    for (int j = 0 ; j < 2 && !found; ++j) {
                        int key2 =  key + i*offsets[1] + j*offsets[0]*_simpleGrid.nbCols();
                        if(_simpleGrid.contains(key2)){
                            cell = _simpleGrid[key2];

                            for(int k=0; k<cell->nbClipVertices() && !found; ++k){
                                ASClipVertex *vertex = cell->clipVertex(k);
                                vec2 sample = vertex->position2D();
                                float distance = dist2(midPos,sample);
                                if(distance<=_coverRadius){
                                    found=true;
                                }
                            }
                        }
                    }
                }
            }
            if(!found && neighbors.isEmpty())
                continue;

            vec2 tangentJoin = closestEndPt->previous()->position() - closestEndPt->following()->position();
            normalize(tangentJoin);
            vec2 tangentF = closestEndPt->following()->position() - closestEndPt->position();
            normalize(tangentF);
            vec2 tangentP = closestEndPt->previous()->position() - closestEndPt->position();
            normalize(tangentP);
            float dotProd1=(tangentJoin DOT tangentP);
            float dotProd2=(-tangentJoin DOT tangentF);
            float dotProd = std::min(dotProd1,dotProd2);
            if(dotProd >= k_dotProdT){

                float coef = k_topoWeight*(1.0-dotProd/k_dotProdT);
                if(coef <= closestCoef)
                    continue;
            }

            removeEndPointsFromGrid(closestEndPtContour);

            ASContour* newContour = closestEndPtContour->split(closestEndPt->index(),closestEndPt->index());
#ifdef VERBOSE
            qDebug()<<"### Split at junction";
#endif
            addEndPointsToGrid(closestEndPtContour);

            if(newContour != NULL){
                _contourList<<newContour;
                addEndPointsToGrid(newContour);
            }
        }
    }
}

void ASSnakes::removeContourFromGrid(ASContour* oldC)
{
    if(oldC->nbVertices()==0)
        return;

    int key = _simpleGrid.posToKey(oldC->first()->position());
    _simpleGrid[key]->removeEndPoint(oldC->first());
    key = _simpleGrid.posToKey(oldC->last()->position());
    _simpleGrid[key]->removeEndPoint(oldC->last());

    ASContour::ContourIterator itC = oldC->iterator();
    itC.toBack();

    while(itC.hasPrevious()){
        ASVertexContour* vertex = itC.previous();
        key = _simpleGrid.posToKey(vertex->position());
        _simpleGrid[key]->removeContourVertex(vertex);
    }
}

void ASSnakes::removeEndPointsFromGrid(ASContour* oldC)
{
    vec2 pos = oldC->last()->position();
    int key = _simpleGrid.posToKey(pos);
    if(_simpleGrid.contains(key)){
        _simpleGrid[key]->removeEndPoint(oldC->last());
        _simpleGrid[key]->removeContourVertex(oldC->last());
    }
    pos = oldC->first()->position();
    key = _simpleGrid.posToKey(pos);
    if(_simpleGrid.contains(key)){
        _simpleGrid[key]->removeEndPoint(oldC->first());
        _simpleGrid[key]->removeContourVertex(oldC->first());
    }
}

void ASSnakes::addEndPointsToGrid(ASContour* newC)
{
    vec2 pos = newC->last()->position();
    int key = _simpleGrid.posToKey(pos);
    if(_simpleGrid.contains(key)){
        _simpleGrid[key]->addEndPoint(newC->last());
        _simpleGrid[key]->addContourVertex(newC->last());
    }else{
        float r,c;
        _simpleGrid.posToCellCoord(pos,r,c);
        ASCell* cell = new ASCell(r,c);
        _simpleGrid[key] = cell;
        cell->addContourVertex(newC->last());
        cell->addEndPoint(newC->last());
    }
    pos = newC->first()->position();
    key = _simpleGrid.posToKey(pos);
    if(_simpleGrid.contains(key)){
        _simpleGrid[key]->addEndPoint(newC->first());
        _simpleGrid[key]->addContourVertex(newC->first());
    }else{
        float r,c;
        _simpleGrid.posToCellCoord(pos,r,c);
        ASCell* cell = new ASCell(r,c);
        _simpleGrid[key] = cell;
        cell->addContourVertex(newC->first());
        cell->addEndPoint(newC->first());
    }
}

void ASSnakes::addStartPoint(ASVertexContour *vertex, ASClipVertex* clipVertex, ASContour* contour)
{
    _simpleGrid[_simpleGrid.posToKey(vertex->position())]->removeEndPoint(vertex);
#ifdef VERBOSE
    qDebug()<<"### Extend start: "<<_contourList.indexOf(contour);
#endif
    vec3 clipPos = clipVertex->position();
    ASVertexContour *newVertex = new ASVertexContour(contour,vec2(clipPos[0],clipPos[1]),0,clipPos[2]);
    newVertex->setEdge(new ASEdgeContour(newVertex, contour->first()));
    newVertex->setConfidence(1.0);
    newVertex->setTangent(newVertex->edge()->direction());

    contour->addVertexFirst(newVertex);
    newVertex->setClosestClipVertex(clipVertex);

    contour->computeLength();

    vec2i offsets;
    ASCell* cell = _simpleGrid[_simpleGrid.posToKey(newVertex->position(),offsets)];
    cell->addEndPoint(newVertex);
    cell->addContourVertex(newVertex);

    for (int i = 0 ; i < 2 ; ++i) {
        for (int j = 0 ; j < 2; ++j) {
            int key2 = cell->column()+i*offsets[1]+(cell->row()+j*offsets[0])*_simpleGrid.nbCols();
            if(_simpleGrid.contains(key2)){
                ASCell* cell2=_simpleGrid[key2];
                QList<ASClipVertex*> clipVertices = cell2->clipVertices();
                for(int k=0; k<clipVertices.size(); ++k){
                    ASClipVertex* cv = clipVertices.at(k);
                    if (!cv->isUncovered())
                        continue;
                    float dotProd = fabs(newVertex->tangent() DOT cv->tangent());
                    if(dotProd >= k_dotProdT && dist2(newVertex->position(),cv->position2D())<=_coverRadius){
                        cv->setCovered();
                        _uncovered.removeAll(cv);
                    }
                }
            }
        }
    }
}

void ASSnakes::addEndPoint(ASVertexContour *vertex, ASClipVertex* clipVertex, ASContour* contour)
{
    _simpleGrid[_simpleGrid.posToKey(vertex->position())]->removeEndPoint(vertex);
#ifdef VERBOSE
    qDebug()<<"### Extend end: "<<_contourList.indexOf(contour);
#endif
    vec3 clipPos = clipVertex->position();
    ASVertexContour *newVertex = new ASVertexContour(contour,vec2(clipPos[0],clipPos[1]),contour->nbVertices(),clipPos[2]);
    newVertex->setConfidence(1.0);
    vertex->setEdge(new ASEdgeContour(vertex,newVertex));
    newVertex->setTangent(vertex->edge()->direction());

    contour->addVertexLast(newVertex);
    newVertex->setClosestClipVertex(clipVertex);

    contour->computeLength();

    vec2i offsets;
    ASCell* cell = _simpleGrid[_simpleGrid.posToKey(newVertex->position(),offsets)];
    cell->addEndPoint(newVertex);
    cell->addContourVertex(newVertex);

    for (int i = 0 ; i < 2 ; ++i) {
        for (int j = 0 ; j < 2; ++j) {
            int key2 = cell->column()+i*offsets[1]+(cell->row()+j*offsets[0])*_simpleGrid.nbCols();
            if(_simpleGrid.contains(key2)){
                ASCell* cell2=_simpleGrid[key2];
                QList<ASClipVertex*> clipVertices = cell2->clipVertices();
                for(int k=0; k<clipVertices.size(); ++k){
                    ASClipVertex* cv = clipVertices.at(k);
                    if (!cv->isUncovered())
                        continue;
                    float dotProd = fabs(newVertex->tangent() DOT cv->tangent());
                    if(dotProd >= k_dotProdT && dist2(newVertex->position(),cv->position2D())<=_coverRadius){
                        cv->setCovered();
                        _uncovered.removeAll(cv);
                    }
                }
            }
        }
    }
}

bool ASSnakes::extend() {
    bool modified=false;

    for(int idx=0; idx<_contourList.size(); idx++){
        ASContour* contour = _contourList[idx];

        if(contour->isClosed())
            continue;

        QList<ASVertexContour*> endPts;
        endPts<<contour->first();
        endPts<<contour->last();

        while(!endPts.isEmpty()){

            ASVertexContour* vertex = endPts.takeFirst();

            int key = _simpleGrid.posToKey(vertex->position());

            QList<QPair<float,ASClipVertex*> > neighborsCV;

            // Find uncovered clip vertices which are candidates for extension
            for(int i=-1; i<=1; ++i){
                for(int j=-1; j<=1; ++j){
                    if(_simpleGrid.contains(key+i+j*_simpleGrid.nbCols())){
                        ASCell* cell = _simpleGrid[key+i+j*_simpleGrid.nbCols()];

                        for(int k=0; k<cell->nbClipVertices(); ++k){
                            ASClipVertex *cv = cell->clipVertex(k);
                            if (!cv->isUncovered())
                                continue;
                            if(cv->visibility()>=k_visibilityTh && cv->strength()>0){
                                float dist=dist2(vertex->position(),cv->position2D());
                                if(dist<=4.0*_coverRadius) {
                                    vec2 tangent;
                                    if(vertex->isFirst())
                                        tangent = vertex->position() - cv->position2D();
                                    else
                                        tangent = cv->position2D() - vertex->position();
                                    normalize(tangent);
                                    float dotProd = (vertex->tangent() DOT tangent);
                                    float dotProdCV = fabs(cv->tangent() DOT tangent);
                                    if(dotProd>=k_dotProdT && dotProdCV >=k_dotProdT){
                                        float coef = k_topoWeight*((1.0-dotProd)/(1.0-k_dotProdT)) +
                                                (1.0-k_topoWeight)*((4.0*_coverRadius)-dist/(2.0*_coverRadius));
                                        neighborsCV << QPair<float,ASClipVertex*>(coef,cv);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if(!neighborsCV.empty()){

                std::sort(neighborsCV.begin(),neighborsCV.end());

                bool canExtend = false;
                ASClipVertex *cv;

                while(!canExtend && !neighborsCV.empty()){
                    bool found=false;

                    cv = neighborsCV.takeFirst().second;

                    vec2 midPos = 0.5f*(vertex->position()+cv->position2D());
                    vec2 midTangent = (cv->position2D() - vertex->position());
                    float midDist = len2(midTangent);

                    vec2i offsets;
                    key = _simpleGrid.posToKey(midPos,offsets);

                    // Cross another snake?
                    for (int i = 0 ; i < 2 && !found; ++i) {
                        for (int j = 0 ; j < 2 && !found; ++j) {
                            int key2 = key + i*offsets[1] + j*offsets[0]*_simpleGrid.nbCols();
                            if(_simpleGrid.contains(key2)){
                                ASCell* cell = _simpleGrid[key2];

                                // Cross another snake?
                                for(int k=0; k<cell->nbContourVertices() && !found; ++k){
                                    ASVertexContour* v = cell->contourVertex(k);
                                    if(v->contour() == vertex->contour()) continue;
                                    float distance = dist2(v->position(),midPos);
                                    if(distance <= midDist/4.0f){
                                        found=true;
                                    }
                                }
                            }
                        }
                    }
                    if(!found)
                        canExtend = true;
                }

                if(!canExtend)
                    continue;

                if(vertex->isFirst()){
                    addStartPoint(vertex, cv, contour);
                    vertex = contour->first();
                    endPts<<vertex;
                }else{
                    addEndPoint(vertex, cv, contour);
                    vertex = contour->last();
                    endPts<<vertex;
                }
                modified = true;
                vertex->computeTangent();

            }
        }
    }

    return modified;
}

void ASSnakes::remove()
{
    /**** delete marked snakes ****/

    QListIterator<ASContour*> itc(_contourList);

    while(itc.hasNext()){
        ASContour* contour = itc.next();

        QList<int> splitIndices;

        for(int i=0; i<contour->nbVertices(); ++i) {
            ASVertexContour* v = contour->at(i);

            if(v->confidence()==0.0) {
                if(!splitIndices.contains(i)){
                    splitIndices<<i;
                }
            }
        }
        if(splitIndices.size()>0){
            QListIterator<int> it(splitIndices);
            it.toBack();
            while(it.hasPrevious()){
                int idx = it.previous();
                int key;

#ifdef VERBOSE
                qDebug()<<"### Remove vertex "<<idx<<"/"<<contour->nbVertices()<<" of "<<_contourList.indexOf(contour);
#endif

                if(idx==0){ //First sample => trim
                    ASVertexContour* v = contour->first();
                    key = _simpleGrid.posToKey(v->position());
                    _simpleGrid[key]->removeContourVertex(v);
                    _simpleGrid[key]->removeEndPoint(v);

                    contour->removeFirstVertex();

                    if(contour->nbVertices()>1){
                        v = contour->first();
                        key = _simpleGrid.posToKey(v->position());
                        _simpleGrid[key]->addEndPoint(v);
                    }else if(contour->nbVertices()==1){
                        v = contour->last();
                        key = _simpleGrid.posToKey(v->position());
                        _simpleGrid[key]->removeContourVertex(v);
                        _simpleGrid[key]->removeEndPoint(v);
                    }
                }else if(idx==contour->nbVertices()-1){ //Last sample => trim
                    ASVertexContour* v = contour->last();
                    key = _simpleGrid.posToKey(v->position());
                    _simpleGrid[key]->removeContourVertex(v);
                    _simpleGrid[key]->removeEndPoint(v);

                    contour->removeLastVertex();

                    if(contour->nbVertices()>1){
                        v = contour->last();
                        float r,c;
                        _simpleGrid.posToCellCoord(v->position(),r,c);
                        key = c+_simpleGrid.nbCols()*r;
                        if(!_simpleGrid.contains(key))
                            _simpleGrid.insert(key,new ASCell(r,c));
                        _simpleGrid[key]->addEndPoint(v);
                    }else if(contour->nbVertices()==1){
                        v = contour->first();
                        key = _simpleGrid.posToKey(v->position());
                        _simpleGrid[key]->removeContourVertex(v);
                        _simpleGrid[key]->removeEndPoint(v);
                    }
                }else if(idx==contour->nbVertices()-2){ //Before the last => trim twice
                    ASVertexContour* v = contour->last();
                    key = _simpleGrid.posToKey(v->position());
                    _simpleGrid[key]->removeContourVertex(v);
                    _simpleGrid[key]->removeEndPoint(v);

                    contour->removeLastVertex();

                    v = contour->last();
                    key = _simpleGrid.posToKey(v->position());
                    _simpleGrid[key]->removeContourVertex(v);
                    contour->removeLastVertex();

                    if(contour->nbVertices()>1){
                        v = contour->last();
                        key = _simpleGrid.posToKey(v->position());
                        _simpleGrid[key]->addEndPoint(v);
                    }else if(contour->nbVertices()==1){
                        v = contour->first();
                        key = _simpleGrid.posToKey(v->position());
                        _simpleGrid[key]->removeContourVertex(v);
                        _simpleGrid[key]->removeEndPoint(v);
                    }
                }else if(idx==1){ //After the first sample => trim twice
                    ASVertexContour* v = contour->first();
                    key = _simpleGrid.posToKey(v->position());
                    _simpleGrid[key]->removeContourVertex(v);
                    _simpleGrid[key]->removeEndPoint(v);

                    contour->removeFirstVertex();

                    v = contour->first();
                    key = _simpleGrid.posToKey(v->position());
                    _simpleGrid[key]->removeContourVertex(v);

                    if(contour->nbVertices()>1){
                        v = contour->first();
                        key = _simpleGrid.posToKey(v->position());
                        _simpleGrid[key]->addEndPoint(v);
                    }else  if(contour->nbVertices()==1){
                        v = contour->last();
                        key = _simpleGrid.posToKey(v->position());
                        _simpleGrid[key]->removeContourVertex(v);
                        _simpleGrid[key]->removeEndPoint(v);
                    }
                    break;
                }else{//Middle sample => split
                    ASVertexContour* v = contour->at(idx);
                    key = _simpleGrid.posToKey(v->position());
                    _simpleGrid[key]->removeContourVertex(v);

                    ASContour* newC = contour->split(idx-1,idx+1);

                    if(contour->nbVertices()>1){
                        v = contour->last();
                        key = _simpleGrid.posToKey(v->position());
                        _simpleGrid[key]->addEndPoint(v);
                    }
                    if(newC!=NULL){
#ifdef VERBOSE
                        qDebug()<<"### Add new contour";
#endif
                        v = newC->first();
                        key = _simpleGrid.posToKey(v->position());
                        _simpleGrid[key]->addEndPoint(v);
                        _contourList<<newC;
                    }
                }

            }
        }
    }

    QListIterator<ASContour*> it(_contourList);
    it.toBack();
    while(it.hasPrevious()){
        ASContour* contour = it.previous();
        // Final cleaning
        if(contour->nbVertices()<2){
#ifdef VERBOSE
            qDebug()<<"### Remove contour "<<_contourList.indexOf(contour);
#endif
            //removeContourFromGrid(contour);
            _contourList.removeAll(contour);
            delete contour;
        }
    }
}

void ASSnakes::minLengthCleaning()
{
    /**** delete too small snakes ****/

    QListIterator<ASContour*> iterContour(_contourList);
    while(iterContour.hasNext()){
        ASContour *contour = iterContour.next();
        contour->computeLength();
        if(contour->isNew() && contour->length()<=k_minLength){
#ifdef VERBOSE
            qDebug()<<"### Delete (min length) "<<_contourList.indexOf(contour);
#endif
            for(int i=0; i<contour->nbVertices(); i++){
                contour->at(i)->setConfidence(0);
            }
        }else if(!contour->isNew() && contour->length()<=k_minLengthHyst*k_minLength){
#ifdef VERBOSE
            qDebug()<<"### Delete (min length) "<<_contourList.indexOf(contour);
#endif
            for(int i=0; i<contour->nbVertices(); i++){
                contour->at(i)->decrConfidence();
                contour->at(i)->decrConfidence();
            }
        }
    }
}

void ASSnakes::markCoverage()
{
    QHashIterator<int,ASCell*> itCell(_simpleGrid.hash());
    while(itCell.hasNext()){
        itCell.next();
        ASCell* cell = itCell.value();

        int key = itCell.key();
        ASVertexContour* v = NULL;

        const QList<ASClipVertex*>& clipVerticies = cell->clipVertices();

        for(int l=0; l < clipVerticies.size(); ++l) {
            ASClipVertex* cv = clipVerticies.at(l);

            if(cv->visibility()<k_visibilityTh)
                continue;

            vec2 tangentClipV = cv->tangent();

            vec2 posCV = cv->position2D();

            bool found = false;

            if(v!=NULL){
                vec2 pos = v->position();
                if(dist2(posCV,pos)<=_coverRadius){
                    vec2 tangent = v->tangent();
                    float dotProd = fabs(tangent DOT tangentClipV);
                    if(dotProd >= k_dotProdCov)
                        found = true;
                }
            }

            for(int i=-1; i<=1 && !found; ++i){
                for(int j=-1; j<=1 && !found; ++j){
                    if(_simpleGrid.contains(key+i+j*_simpleGrid.nbCols())){
                        ASCell* cell2 = _simpleGrid[key+i+j*_simpleGrid.nbCols()];
                        for(int k=0; k<cell2->nbContourVertices() && !found; ++k){
                            v = cell2->contourVertex(k);

                            if(v->confidence()<=0)
                                continue;

                            if(dist2(posCV,v->position())<=_coverRadius){
                                vec2 tangent = v->tangent();
                                float dotProd = fabs(tangent DOT tangentClipV);
                                if(dotProd >= k_dotProdCov)
                                    found = true;
                            }
                        }
                    }
                }
            }

            if(!found){
                v=NULL;
                if(!_uncovered.contains(cv) && cv->visibility()>=k_visibilityTh)
                    _uncovered<<cv;
                cell->addAndSetUncovered(cv);
            }else{
                cell->addAndSetCovered(cv);
            }
        }
    }
}

void ASSnakes::coverage(ASClipPathSet& pathSet)
{
    /****************************************************/
    /************* Mark uncovered vertices **************/
    /****************************************************/
    _uncovered.clear();
    markCoverage();

    /****************************************************/
    /************* Extend snakes ****************/
    /****************************************************/

    extend();

    if(_noConnectivity && k_enableMerge)
        merge();

    /****************** Update coverage *****************/
    int nbUncovered = _uncovered.size();

    /****************************************************/
    /**** Create new contours for uncovered geometry ****/
    /****************************************************/

    if(_noConnectivity){
        // No connectivity: stochastic candidates + extension
        int prevNbUncovered = nbUncovered+1;
        int iter =  k_iterCoverage;

        QList<ASClipVertex*> tested;

        while(nbUncovered>0 && iter>0){

            if(nbUncovered==prevNbUncovered)
                iter--;
            prevNbUncovered = nbUncovered;

            ASClipVertex* cv = NULL;

            while(cv==NULL && !_uncovered.isEmpty()){
                int randCV = floor(float(rand()%_uncovered.size()));
                cv = _uncovered.takeAt(randCV);
                if(tested.contains(cv)){
                    cv=NULL;
                    continue;
                }else{
                    tested<<cv;
                }
            }
            if(_uncovered.isEmpty() || cv==NULL)
                break;

            vec2i offsets;
            ASCell* cell = _simpleGrid[_simpleGrid.posToKey(cv->position2D(),offsets)];

            ASContour* c = new ASContour(this);
            ASVertexContour* v = new ASVertexContour(c,cv->position2D(),0,cv->position()[2]);
            v->setTangent(cv->tangent(),false);
            v->setConfidence(1.0);
            v->setClosestClipVertex(cv);
            c->addVertex(v);

            v = new ASVertexContour(c,cv->position2D()+cv->tangent(),1,cv->position()[2]);
            v->setTangent(cv->tangent(),false);
            v->setConfidence(1.0);
            v->setClosestClipVertex(cv);
            c->first()->setEdge(new ASEdgeContour(c->first(),v));
            c->addVertex(v);

            for (int i = 0 ; i < 2; ++i) {
                for (int j = 0 ; j < 2; ++j) {
                    int key = cell->column()+i*offsets[1]+(cell->row()+j*offsets[0])*_simpleGrid.nbCols();
                    if(_simpleGrid.contains(key)){
                        ASCell* cell2=_simpleGrid[key];
                        for(int k=0; k<cell2->nbClipVertices(); ++k){
                            ASClipVertex* cv2 = cell2->clipVertex(k);
                            if (!cv2->isUncovered())
                                continue;
                            float dotProd = fabs(v->tangent() DOT cv->tangent());
                            if(dotProd >= k_dotProdT && dist2(v->position(),cv2->position2D())<=_coverRadius){
                                cv2->setCovered();
                                _uncovered.removeAll(cv2);
                            }
                        }
                    }
                }
            }

            _contourList << c;
#ifdef VERBOSE
            qDebug() << "*** add contour "<< (_contourList.size()-1) << "("<<c->nbVertices() << " vertices)";
#endif
            _simpleGrid.addSnakeToGrid(_contourList.last());

            extend();

            removeContourFromGrid(c);
            while(c->resample(true)){};
            c->checkClosed();
            c->computeTangent();
            c->computeLength();

            _simpleGrid.addSnakeToGrid(_contourList.last());

            c->removeAllBrushPath();
            c->initParameterization();

            nbUncovered = _uncovered.size();
        }

    }else{
        // Usual pipeline with connectivity

        for(int i=0; i<pathSet.size(); ++i){
            ASClipPath* path = pathSet[i];

            int idx=0;

            ASContour* newContour = NULL;
            ASVertexContour *cour_v, *prev_v=NULL;

            for(int j=0; j<path->size(); ++j){
                ASClipVertex* cv = (*path)[j];

                if(cv->isCovered() || cv->visibility()<k_visibilityTh || (_noConnectivity && newContour!=NULL && newContour->nbVertices()==2)){
                    if(newContour != NULL){
                        newContour->computeLength();
                        if(newContour->nbVertices()>=2){ // long enough => add new contour
                            while(newContour->resample(true)){};
                            newContour->checkClosed();
                            newContour->computeTangent();
                            newContour->initParameterization();
                            _contourList.append(newContour);
                            _simpleGrid.addSnakeToGrid(_contourList.last());
#ifdef VERBOSE
                            qDebug()<<"### Add1 contour "<<_contourList.indexOf(newContour)<<" ("<<newContour->nbVertices()<<")";
#endif
                        }else{
                            delete newContour;
                        }
                    }
                    newContour = NULL;
                    prev_v = NULL;
                    idx=0;
                }else{
                    if(newContour == NULL)
                        newContour = new ASContour(this);

                    vec3 pos = cv->position();
                    cour_v = new ASVertexContour(newContour,vec2(pos[0],pos[1]),idx,pos[2]);
                    cour_v->setConfidence(1.0);
                    cour_v->setClosestClipVertex(cv);

                    //not first vertex? => add edge
                    if(prev_v != NULL)
                        prev_v->setEdge(new ASEdgeContour(prev_v,cour_v));

                    //add vertex to contour
                    newContour->addVertex(cour_v);

                    prev_v = cour_v;
                    idx++;
                }
            }

            if(newContour != NULL){
                newContour->computeLength();

                if(newContour->nbVertices()>=2){ // long enough => add new contour
                    while(newContour->resample(true)){};
                    newContour->checkClosed();
                    newContour->computeTangent();
                    newContour->initParameterization();
                    _contourList.append(newContour);
                    _simpleGrid.addSnakeToGrid(_contourList.last());
#ifdef VERBOSE
                    qDebug()<<"### Add2 contour "<<_contourList.indexOf(newContour)<<" ("<<newContour->nbVertices()<<")";
#endif
                }else{
                    delete newContour;
                }
            }
        }
    }
}

ASClipVertex* ASSnakes::findClosestEdgeRef(vec2 pos, vec2 tangent, bool useVisibility, float coverage)
{
    ASClipVertex *closestClipVertex = NULL;
    QList<QPair<float,ASClipVertex*> > neighbors;

    vec2i offsets;
    int key = _simpleGrid.posToKey(pos,offsets);

    for (int i = 0 ; i < 2; ++i) {
        for (int j = 0 ; j < 2; ++j) {
            int key2 =  key + i*offsets[1] + j*offsets[0]*_simpleGrid.nbCols();
            if(_simpleGrid.contains(key2)){
                ASCell* cell = _simpleGrid[key2];

                for(int k=0; k<cell->nbClipVertices(); ++k){
                    ASClipVertex *vertex = cell->clipVertex(k);

                    vec2 tangentClipV = vertex->tangent();

                    float dotProd = fabs(tangent DOT tangentClipV);

                    vec3 sample = vertex->position();
                    sample[2]*=float(_width);
                    float distance = dist2(vertex->position2D(),pos);

                    if(dotProd>=k_dotProdCov && distance<=coverage
                            && (!useVisibility || vertex->visibility() >= k_visibilityTh)){
                        float coef = distance;
                        neighbors << QPair<float,ASClipVertex*>(coef,vertex);
                    }
                }
            }
        }
    }
    if(!neighbors.isEmpty()){
        std::sort(neighbors.begin(), neighbors.end());
        closestClipVertex = neighbors.first().second;
    }

    return closestClipVertex;
}

void ASSnakes::findClosestEdgeRef()
{
    for(int l=0; l<_contourList.size(); l++){
        ASContour *contour = _contourList[l];
        contour->computeTangent();
        ASContour::ContourIterator it = contour->iterator();

        while(it.hasNext()){
            ASVertexContour* v = it.next();
            ASClipVertex* closestClipVertex = findClosestEdgeRef(v->position(),v->tangent(),k_useVisibility,_coverRadius);

            if(closestClipVertex==NULL){
                // closest clip vertex not found
                v->setConfidence(0.f);
                v->setClosestClipVertex(NULL);
            }else{
                if(closestClipVertex->visibility() >= k_visibilityTh && closestClipVertex->strength() > 0) {
                    // Visible
                    v->incrConfidence();
                    v->setHidden(false);
                }else{
                    // Invisible
                    v->decrConfidence();
                    v->setHidden(true);
                }
                v->setClosestClipVertex(closestClipVertex);
                v->setZ(closestClipVertex->position()[2]);
                v->setStrength(closestClipVertex->strength());
            }
        }
    }
}

void ASSnakes::advect(GQFloatImage* geomFlow, GQFloatImage* denseFlow)
{
    for(int i=0; i<_contourList.size(); i++){
        ASContour *contour = _contourList[i];
        ASContour::ContourIterator it = contour->iterator();

        while(it.hasNext()){
            ASVertexContour* v = it.next();
            vec2 newPos = vec2(0,0);
            float z = 0;

            vec2 pos = v->position();

            if(geomFlow!=NULL && v->closestClipVertex() != NULL){ // update position with geomFlow
                vec2 atlasCoord = v->atlasCoord();
                if(k_relAdvection){
                    vec3 cvPos = v->closestEdgePosition();
                    newPos[0] = pos[0] + geomFlow->pixel(atlasCoord[0],atlasCoord[1],0) - cvPos[0];
                    newPos[1] = pos[1] + geomFlow->pixel(atlasCoord[0],atlasCoord[1],1) - cvPos[1];
                    z = geomFlow->pixel(atlasCoord[0],atlasCoord[1],2); // absolute location in z
                }else{
                    newPos[0] = geomFlow->pixel(atlasCoord[0],atlasCoord[1],0);
                    newPos[1] = geomFlow->pixel(atlasCoord[0],atlasCoord[1],1);
                    z = geomFlow->pixel(atlasCoord[0],atlasCoord[1],2);
                }
            }else if(denseFlow != NULL){ // closestClipVertex == NULL : use dense flow
                vec2 motion(0.f,0.f);
                if(v->closestClipVertex() != NULL){
                    vec2 posCV = v->closestClipVertex()->position2D();

                    //3 x 3 lookup and take the max
                    for(int i=-2; i<=2 ; ++i){
                        for(int j=-2; j<=2 ; ++j){
                            if(posCV[0]+i<0 || posCV[0]+i>=denseFlow->width() || posCV[1]+j<0 || posCV[1]+j>=denseFlow->height())
                                continue;
                            vec2 m;
                            m[0] = denseFlow->pixel(posCV[0]+i,posCV[1]+j,0);
                            m[1] = denseFlow->pixel(posCV[0]+i,posCV[1]+j,1);

                            if(len2(m)>len2(motion))
                                motion=m;
                        }
                    }
                }else{
                    motion = vec2(denseFlow->pixel(pos[0],pos[1],0), denseFlow->pixel(pos[0],pos[1],1));
                }
                newPos = pos + motion;
                z = v->z();
            }else{ // To be deleted
                v->setConfidence(0);
            }

            // This is a hacked check to see if the value is valid.
            // It should be changed to actually inspect the result of clipping
            // somehow. -fcole nov 17 2010
            if (!isnan(newPos[0]) && !isnan(newPos[1]) && len(newPos) > 0) {
                v->setPosition(newPos);
                v->setZ(z);
            }else{
                v->setConfidence(0.0);
                qWarning("Advection NaN");
            }
        }
    }
}

int ASSnakes::nbBrushPaths() const
{
    int sum = 0;
    foreach(ASContour* c,_contourList)
        sum += c->nbBrushPaths();
    return sum;
}

ASBrushPath* ASSnakes::brushPath(int i)
{
    int sum = 0;
    foreach(ASContour* c,_contourList){
        sum += c->nbBrushPaths();
        if(i < sum)
            return c->brushPath(sum-i-1);
    }
    return NULL;
}

/********************************************************/
/*              Terminal print functions                */
/********************************************************/

void ASSnakes::printContours()
{
    qDebug();
    for(int i=0; i<_contourList.size(); i++){
        ASContour* contour = _contourList[i];
        qDebug()<<i<<" ("<<contour->nbVertices()<<"): ";
        contour->print();
        qDebug();
    }
}

void ASSnakes::printSimpleGrid()
{
    QHashIterator<int,ASCell*> itCell(_simpleGrid.hash());

    while(itCell.hasNext()){
        itCell.next();
        ASCell* cell = itCell.value();

        QListIterator<ASVertexContour*> itEndPoints = cell->endPointsIterator();

        if(itEndPoints.hasNext()){
            qDebug()<<cell->row()<<","<<cell->column()<<" : ";

            while(itEndPoints.hasNext()){
                ASVertexContour* endPt = itEndPoints.next();
                qDebug()<<"("<<_contourList.indexOf(endPt->contour())<<" - "<<endPt->index()<<"/"<<endPt->contour()->nbVertices()<<") ";
            }
            qDebug();
        }
    }
    qDebug();
}

void ASSnakes::fitBrushPath()
{
    __TIME_CODE_BLOCK("Brush paths processing");

    if(k_initBP && !k_initBP.changedLastFrame()) {
        for(int i=0; i<_contourList.size(); i++)
        {
            ASContour* c = at(i);
            c->removeAllBrushPath();
            c->initParameterization();
        }
        k_initBP.setValue(false);
    }

    //Brush paths processing
    for(int i=0; i<_contourList.size(); i++){
        ASContour* c = at(i);
        c->computeTangent();
        c->checkClosed();
        int iter = 0;
        while(c->updateOverdraw() && iter<10) iter++;

        int numBrushPath = at(i)->nbBrushPaths();
        for(int j=0; j<numBrushPath; j++){
            c->brushPath(j)->computeArcLength();
            QVector<ASBrushPath*> newBrushpathList = at(i)->brushPath(j)->fitting();
            if (newBrushpathList.size() > 0){
                for (int k=0; k<newBrushpathList.size(); k++)
                    c->addBrushPath(newBrushpathList[k]);
            }
        }
    }

    for(int i=0; i<_contourList.size(); i++){
        ASContour* c = at(i);
        int numBrushPath = c->nbBrushPaths();
        for(int j=0; j<numBrushPath; j++)
            c->brushPath(j)->computeTangent();
    }

    for(int i=0; i<_contourList.size(); i++){
        ASContour* c = at(i);
        c->fitLinearParam();

        if(k_mergeBP){
            for(int j=0; j<c->nbBrushPaths(); j++){
                ASBrushPath* bp = c->brushPath(j);
                bp->computeParam();
            }
            c->mergeBrushPaths();
        }
    }
}
