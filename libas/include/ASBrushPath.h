/*****************************************************************************\

ASBrushPath.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/


#ifndef BRUSHPATH_H
#define BRUSHPATH_H

#include "ASBrushVertex.h"
#include "ASBrushPathFitting.h"
#include "GQShaderManager.h"

class ASBrushPathFitting;
struct fittingSegment;

class ASContour;

struct FittingPara{
    float A;
    float B;
    float C;
    float cost;
    float arclength;
    vec2 midPoint;

    float sweepingAngle;
    //int    direction; //positive : turning left, negative : turning right  -2 : haven't assigned a turning direction

    FittingPara(){A=0; B=0; C=0; cost=0; sweepingAngle = 0; arclength = 0;}
    FittingPara(float a, float b, float c, float fittingcost)
    {A = a; B = b; C = c; cost = fittingcost;}
    FittingPara(float a, float b, float c, float fittingcost, float s)
    {A = a; B = b; C = c; cost = fittingcost; sweepingAngle = s; }
};

typedef enum
{
    AS_BRUSHPATH_NOFITTING = 0,
    AS_BRUSHPATH_LINE,
    AS_BRUSHPATH_ARC
} ACBrushpathStyleMode;

class ASBrushPath : public QObject {
    Q_OBJECT
public:
    ASBrushPath(ASContour*c, int start, int end, float slope, float intercept);
    ASBrushPath(float slope, vec2 offset);
    ASBrushPath(ASBrushPath &bp1, ASBrushPath &bp2);

    ~ASBrushPath();

    int indexOf(ASBrushVertex* v) { return _vertices.indexOf(v); }

    float slope() const;
    float phase() const { return _phase; }
    float level() const { return _level; }
    float fact() const { return _fact; }

    void setSlope(float s) { _slope = s; }
    void setPhase(float p) { _phase = p; }
    void setLevel(float l) { _level = l; }
    void setFact(float f) { _fact = f; }

    float param(double arcLength) const;
    float param(int i) const;

    vec2 offset() const;
    void setOffset(vec2 offset);

    bool isReversed() const { return _reversed; }
    void setReversed(bool b) { _reversed = b; }

    int nbVertices() const;
    inline ASBrushVertex* at(int i) { return _vertices.at(i); }
    inline const ASBrushVertex* at(int i) const  { return _vertices.at(i); }
    ASBrushVertex* first() { return _vertices.first(); }
    const ASBrushVertex* first() const { return _vertices.first(); }
    ASBrushVertex* last() { return _vertices.last(); }
    const ASBrushVertex* last() const { return _vertices.last(); }
    const QList<ASBrushVertex*>& vertices() const { return _vertices; }

    void  computeArcLength();
    float computeOriginalLength();
    void  computeParam();
    void  computeParamNewFrame();

    void remove(ASBrushVertex* v);
    void insertAfter (ASBrushVertex* prevVertex, ASBrushVertex* newVertex);
    void insertBefore(ASBrushVertex* nextVertex, ASBrushVertex* newVertex);
    void insertFirst (ASBrushVertex* newVertex);
    void insertLast  (ASBrushVertex* newVertex);

    void merge(ASBrushPath* bp);

    ASBrushPath* split(int idx1, int idx2, ASVertexContour* newV);
    ASBrushPath* split(int idx);
    QVector<ASBrushPath*> splitToMultSegments(const QVector<fittingSegment>& segmentInfoSet);
    void trimLast()  { _vertices.pop_back();  }
    void trimFirst() { _vertices.pop_front(); }

    QVector<ASBrushPath*> fitting();

    void inverse();

    void draw(bool black);
    void rasterizeOffsets(GQShaderRef &shader) const;
    void print() const;

    void setNewSpline(bool n){_newSpline = n;}
    bool isNewSpline(){return _newSpline;}

    void setNewFitting(bool n){ _newFitting = n; }
    bool isNewFitting(){ return _newFitting; }

    bool isClosed() const{ return _closed; }
    void computeTangent();
    void setNewBrushPath();

    void assignDebugColor();
    QColor& debugColor() { return _debug_color; }

    // Fitting
    void setFittingPara(float A, float B, float C, float cost)
    { _fittingPara = FittingPara(A, B, C, cost); }
    void setFittingPara(float A, float B, float C, float cost, float sweepingAngle)
    { _fittingPara = FittingPara(A, B, C, cost, sweepingAngle); }
    void setSweepingAngle(float a){ _fittingPara.sweepingAngle = a; }
    FittingPara& fittingPara(){return _fittingPara;}
    void setFittingArclength(float l){ _fittingPara.arclength = l; }
    float fittingArclength() const{ return _fittingPara.arclength; }
    void setMidPoint(vec2 m) { _fittingPara.midPoint = m; }
    vec2 midPoint() const { return _fittingPara.midPoint; }
    static int turn(vec2 A, vec2 B, vec2 C, float threshold = 0.001);

public slots:
    void randomizeOffset(bool b);

private:

    // Parameterization
    float _slope;
    float _phase;

    float _level;
    float _fact;

    bool _newSpline;
    bool _closed;
    bool _reversed;
    bool _newFitting;

    //List of vertices
    QList<ASBrushVertex*> _vertices;

    FittingPara _fittingPara;

    // Brush path offset in the normal and tangent direction
    vec2 _offset;

    QColor _debug_color;
};

const int ncolors = 12;
const QColor color_list[ncolors] = {QColor("#CC0000"), QColor("#73D216"), QColor("#75507B"), QColor("#AD7FA8"),
                                    QColor("#FCAF3E"), QColor("#C17D11"), QColor("#F57900"), QColor("#FCE94F"),
                                    QColor("#C4A000"), QColor("#E9B96E"), QColor("#A40000"), QColor("#3465A4")};

const QColor color_list_BP[ncolors] = {QColor("#3366AA"), QColor("#FF2A98"), QColor("#AD6CFF"), QColor("#4D53FF"),
                                    QColor("#2A84FF"), QColor("#722A24"), QColor("#9C9658"), QColor("#624740"),
                                    QColor("#5B24B4"), QColor("#536073"), QColor("#FF356C"), QColor("#D1D67C")};

#endif // BRUSHPATH_H
