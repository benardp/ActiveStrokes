/*****************************************************************************\

ASEdgeContour.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "ASEdgeContour.h"

ASEdgeContour::ASEdgeContour(ASVertexContour *firstVertex, ASVertexContour *secondVertex) {
    _vertex[0] = firstVertex;
    _vertex[1] = secondVertex;
    _length = dist(firstVertex->position(), secondVertex->position());
    _restLength = _length;
    _direction = secondVertex->position() - firstVertex->position();
    normalize(_direction);
}

ASEdgeContour::~ASEdgeContour() {}

void ASEdgeContour::computeDirection() {
    _restLength = _length;
    _length = dist(firstVertex()->position(), secondVertex()->position());
    if(restLength()==0 && _length!=0)
        _restLength = _length;
    _direction = secondVertex()->position() - firstVertex()->position();
    normalize(_direction);
}

void ASEdgeContour::inverse() {
    ASVertexContour* v = _vertex[0];
    _vertex[0] = _vertex[1];
    _vertex[1] = v;
}
