/*****************************************************************************\

ASContour.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef CONTOURLINE_H_
#define CONTOURLINE_H_

#include "GQInclude.h"

#include "ASVertexContour.h"
#include "ASEdgeContour.h"
#include "ASBrushPath.h"

#include <QList>
#include <QColor>

class ASClipPath;
class ASSnakes;

class ASContour {
public:
    typedef QListIterator<ASVertexContour*> ContourIterator;

    ASContour(ASSnakes* ac);
    ASContour(ASSnakes* ac, ASClipPath &p, float visTh);
    virtual ~ASContour();

    void clear();

    int nbVertices() const { return _vertexList.size(); }

    void initParameterization();

    void addVertex(ASVertexContour* v) { _vertexList << v; }
    void addVertexFirst(ASVertexContour* v);
    void addVertexLast(ASVertexContour* v);
    ASContour* removeLastVertex();
    ASContour* removeFirstVertex();

    QList<ASVertexContour*>& vertices() { return _vertexList; }

    ASVertexContour* first() const { return _vertexList.first(); }
    ASVertexContour* last()  const { return _vertexList.last(); }

    ContourIterator iterator() const { return ContourIterator(_vertexList); }

    void computeTangent();
    void computeCurvature();
    void computeLength();
    void computeEdgeDirection();
    float calculate3Dpositions(const GLdouble *model, const GLdouble *proj, const GLint *view) const;

    int resample(bool findClosest=false);
    ASContour* split(int idx1, int idx2);

    float length() const { return _length; }

    ASVertexContour* at(int index);
    const ASVertexContour* at(int index) const;

    void draw(const vec3 color=vec3(1.0,1.0,1.0), const bool drawParam = false) const;
    void draw3D(const GLdouble *model , const GLdouble *proj , const GLint *view, const vec3 color=vec3(1.0,1.0,1.0), const bool drawParam = false) const;
    void drawClosestEdge3D(const GLdouble *model, const GLdouble *proj, const GLint *view) const;
    void drawClosestEdge() const;
    void drawBrushPaths(bool black) const;
    void rasterizeOffsets(GQShaderRef &shader) const;

    void inverse();

    void stitchAfter(ASContour* c);

    void print();

    bool isClosed() const { return _closed; }
    void close() { _closed = true; }
    void open()  { _closed = false; }
    void checkClosed();

    bool isNew() const { return _isNew; }
    void notNew() { _isNew = false; }

    // Brush Paths

    int nbBrushPaths() const { return _brushPaths.size(); }
    ASBrushPath* brushPath(int i) const { return _brushPaths.at(i); }

    void cleanBrushPath();
    void addBrushPath(ASBrushPath* b){_brushPaths.push_back(b);}
    void removeBrushPath(ASBrushPath* b){_brushPaths.removeAll(b);}
    void removeAllBrushPath();

    void fitLinearParam();

    void mergeBrushPaths();

    bool updateOverdraw();

    void updateTaper();

    void assignDebugColor();
    QColor& debugColor() { return _debug_color; }

protected:
    void heightLineFit(int numPoints, const vec2 *points, float &m, float &b);
    void recursiveSplit(ASBrushPath* b);

    void brushPathPairs(QList< QPair<ASBrushVertex*,ASBrushVertex*> > &pairs);

private:
    ASSnakes* _ac;

    QList<ASVertexContour*> _vertexList;

    bool _closed;
    bool _isNew;
    float _length;

    QVector<vec2> _segmentPointSet;
    QList<ASBrushPath*> _brushPaths;
    QList<ASBrushPath*> _newBrushPaths;

    QColor _debug_color;
};

#endif /* CONTOUR_H_ */
