/*****************************************************************************\

ASSimpleGrid.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "ASSimpleGrid.h"
#include "GQDraw.h"

dkBool k_drawGrid("Contours->Draw->Grid", false);

static int seed = 0;

ASSimpleGrid::ASSimpleGrid()
{
}

void ASSimpleGrid::clear()
{
    qDeleteAll(_hashTable);
    _hashTable.clear();
}

void ASSimpleGrid::init(int width, ASClipPathSet& pathSet)
{
    clear();
    _nbCols = float(width)/_cellSize;

    //random origin
    //srand ( time(NULL) );
    srand (seed);
    seed++;
    _oGrid = ivec2((rand() % _cellSize),(rand() % _cellSize));//ivec2(0,0);//

    if(k_drawGrid){
        startDrawGrid(_oGrid,_cellSize);
        glBegin(GL_POINTS);
    }

    //fill in the grid with the clipped paths
    for(int i=0; i<pathSet.size(); ++i){
        ASClipPath *path = pathSet[i];

        ASCell* cell;

        for(int j=0; j<path->size(); ++j){
            ASClipVertex *cv = (*path)[j];
            vec2 pos = cv->position2D();
            float r,c;
            posToCellCoord(pos,r,c);

            if(contains(c+_nbCols*r)){
                cell = _hashTable[c+_nbCols*r];
            }else{
                if(k_drawGrid)
                    glVertex2f(r*_cellSize + _oGrid[0],c*_cellSize + _oGrid[1]);
                cell = new ASCell(r,c);
                _hashTable[c+_nbCols*r] = cell;
            }

            cell->addClipVertex(cv);
        }
    }
    if(k_drawGrid){
        glEnd();
        stopDrawGrid();
    }

}

void ASSimpleGrid::addSnakeToGrid(ASContour* contour)
{
    ASContour::ContourIterator it = contour->iterator();

    ASCell* cell;
    int idx=0;

    while(it.hasNext()){
        ASVertexContour* v = it.next();

        float r,c;
        posToCellCoord(v->position(),r,c);

        int key = c+_nbCols*r;
        if(contains(key)){
            cell = _hashTable[key];
        }else{
            cell = new ASCell(r,c);
            _hashTable[key] = cell;
        }
        if(k_drawGrid)
            glVertex2f((r+0.5)*_cellSize + _oGrid[0],(c+0.5)*_cellSize + _oGrid[1]);
        cell->addContourVertex(v,false);

        if((idx==0) || (idx == contour->nbVertices()-1) ){
            //first or last vertex of the contour
            cell->addEndPoint(v,false);
        }

        idx++;
    }
}

void ASSimpleGrid::startDrawGrid(ivec2 o_grid, int step, bool drawLines) {

    glPushAttrib(GL_ENABLE_BIT);

    GLint viewport[4];
    glGetIntegerv (GL_VIEWPORT, viewport);

    glViewport(0,0,viewport[2], viewport[3]);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, viewport[2], 0, viewport[3], 0.0, -1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    if(drawLines){
        glLineWidth(1);
        glColor4f(0.5, 0.5, 0.5, 1.0);

        for(int i=0; i<viewport[2]-step+1; i+=step){
            glBegin(GL_LINES);
            glVertex2f(i+o_grid[0],0);
            glVertex2f(i+o_grid[0],viewport[3]);
            glEnd();
        }
        for(int j=0; j<viewport[3]-step+1; j+=step){
            glBegin(GL_LINES);
            glVertex2f(0,j+o_grid[1]);
            glVertex2f(viewport[2],j+o_grid[1]);
            glEnd();
        }

        glPointSize(2);
        glBegin(GL_POINTS);
        glVertex2f(o_grid[0],o_grid[1]);
        glEnd();
    }

    glColor4f(0.0, 0.8, 0.0, 1.0);
}

void ASSimpleGrid::stopDrawGrid(){

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();

    reportGLError();
}

