/*****************************************************************************\

ASSimpleGrid.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef SIMPLEGRID_H
#define SIMPLEGRID_H

#include "ASCell.h"
#include "ASContour.h"
#include "ASClipPath.h"

class ASSimpleGrid
{
public:
    ASSimpleGrid();

    void init(int width, ASClipPathSet& pathSet);

    void clear();

    int  cellSize() const { return _cellSize; }
    void setCellSize(int size) { _cellSize = size; }

    int  nbCols() const { return _nbCols; }

    ivec2 origin() const { return _oGrid; }

    inline void posToCellCoord(vec2 pos, float& r, float& c);
    inline int posToKey(vec2 pos);
    inline int posToKey(vec2 pos, ivec2& offsets);

    inline bool contains(int key) { return _hashTable.contains(key); }
    inline void insert(int key, ASCell* cell) { _hashTable.insert(key,cell); }

    inline ASCell*& operator[](int key) { return _hashTable[key]; }

    const QHash<int,ASCell*>& hash() { return _hashTable; }

    void addSnakeToGrid(ASContour* contour);

    void startDrawGrid(ivec2 o_grid, int step, bool drawLines=true);
    void stopDrawGrid();

private:
    QHash<int,ASCell*> _hashTable;
    int   _nbCols;
    ivec2 _oGrid;
    int _cellSize;
};

inline void ASSimpleGrid::posToCellCoord(vec2 pos, float& r, float& c) {
    float i = (pos[0] - _oGrid[0])/float(_cellSize);
    float j = (pos[1] - _oGrid[1])/float(_cellSize);
    r = floor(i);
    c = floor(j);
}

inline int ASSimpleGrid::posToKey(vec2 pos) {
    float r,c;
    posToCellCoord(pos,r,c);
    return c+_nbCols*r;
}

inline int ASSimpleGrid::posToKey(vec2 pos, ivec2& offsets) {
    float i = (pos[0] - _oGrid[0])/float(_cellSize);
    float j = (pos[1] - _oGrid[1])/float(_cellSize);
    float r = floor(i);
    float c = floor(j);

    offsets[0] = ((i - r) > 0.5) ? 1 : -1;
    offsets[1] = ((j - c) > 0.5) ? 1 : -1;

    return c+_nbCols*r;
}

#endif // SIMPLEGRID_H
