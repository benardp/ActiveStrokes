/*****************************************************************************\

ASCell.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef CELL_H_
#define CELL_H_

#include "Vec.h"
#include "ASVertexContour.h"
#include "ASClipPath.h"

class ASCell {
public:
    ASCell(const int r, const int c);
    virtual ~ASCell();

    int row() const { return _row; }
    int column() const { return _col; }

    int nbContourVertices() const { return _contourVertices.size(); }
    ASVertexContour* contourVertex(uint i) { return _contourVertices.at(i); }
    void removeContourVertex(ASVertexContour* v) { _contourVertices.removeAll(v); }
    void removeContourVertices(ASContour* c);
    QListIterator<ASVertexContour*> contourVerticesIterator() { return QListIterator<ASVertexContour*>(_contourVertices); }
    void addContourVertex(ASVertexContour* v, bool checkPresence=true);
    void addContourVertices(ASContour* c, ivec2 oGrid, int cellSize, bool checkPresence=true);

    int nbEndPoints() const { return _endPoints.size(); }
    ASVertexContour* endPoint(uint i) { return _endPoints.at(i); }
    void removeEndPoint(ASVertexContour* v) { _endPoints.removeAll(v); }
    QListIterator<ASVertexContour*> endPointsIterator() { return QListIterator<ASVertexContour*>(_endPoints); }
    bool containsEndPoint(ASVertexContour *v) { return _endPoints.contains(v); }
    void addEndPoint(ASVertexContour* v, bool checkPresence=true);

    inline int nbClipVertices() const { return _clipVertices.size(); }
    inline ASClipVertex* clipVertex(int i) const { return _clipVertices.at(i); }

    void addAndSetCovered(ASClipVertex *v);
    void addAndSetUncovered(ASClipVertex *v);
    void addClipVertex(ASClipVertex *v);

    const QList<ASClipVertex*>& clipVertices() { return _clipVertices; }

protected:

    QList<ASVertexContour*> _contourVertices;
    QList<ASVertexContour*> _endPoints;
    QList<ASClipVertex*> _clipVertices;

    const int _row;
    const int _col;
};

#endif /* CELL_H_ */
