/*****************************************************************************\

ASBrushPathFitting.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef BRUSHPATHFITTING_H
#define BRUSHPATHFITTING_H

#ifdef WIN32
#include <math.h>
#endif

#include <QVector>
#include "Vec.h"
#include "ASBrushPath.h"

//for fitting a number of lines
struct fittingSegment
{
    int start;
    int end;
    float A;
    float B;
    float C;
    float cost;

    fittingSegment(){}

    fittingSegment(int st, int ed, float a, float b, float c, float fittingcost)
    {
        start = st;
        end = ed;
        A = a;
        B = b;
        C = c;
        cost = fittingcost;
    }
};

struct controlInfo
{
    vec2 c0;
    vec2 c1;
    vec2 c2;
    vec2 c3;
};

class ASBrushPathFitting
{
public:

    ~ASBrushPathFitting();
    static ASBrushPathFitting& instance() { return _instance; }
    const QVector<fittingSegment>& getSegmentInfoSet() const { return _segmentInfoSet;}

    //fitting multiple line segments
    void findBreakingPositionForLine(float eachSegmentCost, const ASBrushPath* bp);
    void calculateLinePosition(ASBrushPath* bp);
    void calculateEndPointsPosition(ASBrushPath *bp, float A, float B, float C, vec2& pos1, vec2& pos2);
    void interpolateEndPoint(ASBrushPath *bp, vec2 start, vec2 end);
    vec2 averageLineEndPoint(ASBrushPath *bp, vec2 pos1, vec2 pos2, vec2 &posAvg1, vec2 &posAvg2);

    //fitting multiple circular arcs
    int    findBreakingPositionForArc(float segmentCost, float targetRadius, float radiusWeight, const ASBrushPath* bp);
    void   calculateArcPosition(ASBrushPath * bp);
    double determineSweepingAngle(ASBrushPath *bp, vec2 center, double radius);

private:

    //for all
    QVector <vec2> _samplePointSet; //store the sample points (May not be all the input points from the tracking snake )

    void  updateOffset(ASBrushPath* bp) const;
    float calculateDifference(vec2 input, vec2 fitted) const;

    //for fitting both lines and clothoids
    void  heightLineFit(int numPoints, const vec2 *points, float &m, float &b) const;
    void  widthLineFit(int numPoints, const vec2 *points, float &m, float &b) const;
    float LineFit(int numPoints, const vec2 *points, float &A, float &B, float &C) const;
    float circularArcFit(int n, const vec2 *points, float &centerX, float &centerY, float &radius) const;
    void  recurseThroughWalkMatrix(QVector <QVector <int> > walkMatrix, int begin, int end, bool *segmentEnd) const;
    float totalLeastSquareFit(int numPoints, const vec2 *points, float &A, float &B, float &C) const;
    float testLine(int numPoints, const vec2 *points, float &A, float &B, float &C) const;

    //for fitting multiple lines
    QVector<fittingSegment> _segmentInfoSet;

    static ASBrushPathFitting _instance;
};


#endif // BRUSHPATHFITTING_H
