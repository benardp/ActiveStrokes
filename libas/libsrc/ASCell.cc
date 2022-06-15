/*****************************************************************************\

ASCell.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "ASCell.h"
#include "ASContour.h"

ASCell::ASCell(const int r, const int c) : _row(r), _col(c) {}

ASCell::~ASCell() {}

void ASCell::addContourVertex(ASVertexContour* v, bool checkPresence) {
    if(checkPresence){
        if(!_contourVertices.contains(v))
            _contourVertices << v;
    }else{
        _contourVertices << v;
    }
}

void ASCell::addContourVertices(ASContour* c, vec2i oGrid, int cellSize, bool checkPresence) {
    ASContour::ContourIterator it = c->iterator();
    while(it.hasNext()){
        ASVertexContour *v = it.next();
        vec2 pos = v->position();
        int r = int(pos[0] - oGrid[0])/cellSize;
        int c = int(pos[1] - oGrid[1])/cellSize;
        if(_row==r && _col==c)
            addContourVertex(v,checkPresence);
    }

}

void ASCell::removeContourVertices(ASContour* c) {
    QListIterator<ASVertexContour*> it = contourVerticesIterator();
    while(it.hasNext()){
        ASVertexContour* v = it.next();
        if(v->contour()==c)
            removeContourVertex(v);
    }
}

void ASCell::addEndPoint(ASVertexContour* v, bool checkPresence) {
    if(checkPresence){
        if(!_endPoints.contains(v))
            _endPoints << v;
    }else{
        _endPoints << v;
    }
}

void ASCell::addClipVertex(ASClipVertex *v) {
    _clipVertices << v;
}

void ASCell::addAndSetCovered(ASClipVertex *v) {
    v->setCovered();
    if (!_clipVertices.contains(v))
        addClipVertex(v);
}

void ASCell::addAndSetUncovered(ASClipVertex* v) {
    v->setUncovered();
    if (!_clipVertices.contains(v))
        addClipVertex(v);
}
