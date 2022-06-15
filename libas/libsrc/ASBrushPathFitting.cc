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

#include "DialsAndKnobs.h"
#include "ASBrushPathFitting.h"

#include <fstream>
#include <limits.h>
#include <QDebug>
#include "assert.h"

#ifdef WIN32
#define _USE_MATH_DEFINES
#include <math.h>
#endif

ASBrushPathFitting ASBrushPathFitting::_instance;

#define MIN_DISTANCE_FROM_ZERO 0.001
#define CURV_FACTOR sqrt(M_PI/2.0)

extern dkBool  k_fittingAveraging;
extern dkFloat k_fittingMaxDistance;

int clipToRange(int index, int range)
{
    if (index < 0) return 0;
    else if (index >= range) return range-1;
    return index;
}

ASBrushPathFitting::~ASBrushPathFitting()
{
    _segmentInfoSet.clear();
    _samplePointSet.clear();
}

float ASBrushPathFitting::totalLeastSquareFit(int numPoints, const vec2 *points, float &A, float &B, float &C) const
{
    float sumX = 0, sumY = 0, sumXY = 0;
    for (int i=0; i<numPoints; i++)
    {
        sumX += points[i][0];
        sumY += points[i][1];
        sumXY += points[i][0] * points[i][1];
    }
    float meanX = sumX / numPoints;
    float meanY = sumY / numPoints;
    float meanXY = sumXY / numPoints;

    float varianceX = 0;
    float varianceY = 0;
    for (int i=0; i<numPoints; i++)
    {
        varianceX += (points[i][0]-meanX) * (points[i][0]-meanX);
        varianceY += (points[i][1]-meanY) * (points[i][1]-meanY);
    }
    varianceX /= numPoints;
    varianceY /= numPoints;
    float varianceXY = meanXY - meanX * meanY;

    float theta = M_PI_2;
    if(varianceXY!=0)
        theta = atan((varianceY - varianceX - sqrt((varianceY - varianceX) * (varianceY - varianceX) + 4 * varianceXY * varianceXY)) / (2 * varianceXY));
    float n = meanX * cos(theta) + meanY * sin(theta);
    float error = varianceX * cos(theta) * cos(theta) + varianceXY * sin(2 * theta) + varianceY * sin(theta) * sin(theta);

    A = cos(theta);

    if(isnan(A))
    {
        qCritical("NaN offset");
    }

    B = sin(theta);
    C = n;

    return error;

}

float ASBrushPathFitting::circularArcFit(int n, const vec2 *points, float &centerX, float &centerY, float &radius) const
{
    double A, B, C, D, E, aM, bM, rM=0;


    //fit the circular Arc using Modified Least-Squares Methods in paper "A Few Methods for Fitting Circles to Data"

    double x=0, x2=0, xy=0, y=0, y2=0, xy2=0, x3=0, yx2=0, y3=0;
    double xs, ys;
    for (int i=0; i<n; i++)
    {
        x += points[i][0];
        y += points[i][1];
        xs = points[i][0] * points[i][0];
        ys = points[i][1] * points[i][1];
        x2 += xs;
        y2 += ys;
        xy += points[i][1] * points[i][0];
        xy2 += ys * points[i][0];
        yx2 += xs * points[i][1];
        x3 += xs * points[i][0];
        y3 += ys * points[i][1];
    }

    A = n * x2 - x * x;
    B = n * xy - x * y;
    C = n * y2 - y * y;
    D = 0.5 * (n * xy2 - x * y2 + n * x3 - x * x2);
    E = 0.5 * (n * yx2 - y * x2 + n * y3 - y * y2);

    double divide;
    if (fabs(A*C - B*B) < 0.00000001)
        divide = 0.00000001;
    else
        divide = A * C - B * B;
    aM = (D*C - B*E) / divide;
    bM = (A*E - B*D) / divide;

    for (int i=0; i<n; i++)
    {
        rM += sqrt((points[i][0] - aM) * (points[i][0] - aM) + (points[i][1] - bM) * (points[i][1] - bM));
    }
    rM /= n;

    centerX = aM;
    centerY = bM;
    radius = rM;

    //calculate the fitting error
    double error = 0;
    double dist;
    for (int i=0; i<n; i++)
    {
        dist = radius - sqrt((points[i][0] - centerX)*(points[i][0] - centerX) + (points[i][1] - centerY)*(points[i][1] - centerY));
        error += dist * dist;
    }
    return error;
}

float ASBrushPathFitting::testLine(int numPoints, const vec2 *points, float &A, float &B, float &C) const
{
    float fittingError = totalLeastSquareFit(numPoints, points, A, B, C);

    //when the line fitting error is very small, it means, the three points are almost colinear
    //then instead of doing the real circle fitting, fit a fixed size circle to the three points
    if (fittingError < 1)
    {
        float error1 = 0, error2 = 0;
        vec2 center = vec2(0, 0), center1, center2;
        for (int i=0; i<numPoints; i++)
            center += points[i];
        center /= float(numPoints);

        vec2 normalDir1 = vec2(A, B);
        normalize(normalDir1);
        vec2 normalDir2 = -normalDir1;
        C = 1000.0f;
        center1 = center + normalDir1 * C;
        center2 = center + normalDir2 * C;

        float dist1, dist2;
        for (int i=0; i<numPoints; i++)
        {
            dist1 = C - sqrt((points[i][0] - center1[0])*(points[i][0] - center1[0]) + (points[i][1] - center1[1])*(points[i][1] - center1[1]));
            error1 += dist1 * dist1;

            dist2 = C - sqrt((points[i][0] - center2[0])*(points[i][0] - center2[0]) + (points[i][1] - center2[1])*(points[i][1] - center2[1]));
            error2 += dist2 * dist2;
        }

        if (error1 < error2)
        {
            A = center1[0];
            B = center1[1];
            return error1;
        }
        else
        {
            A = center2[0];
            B = center2[1];
            return error2;
        }
    }

    return -1;
}

float ASBrushPathFitting::LineFit(int numPoints, const vec2 *points, float &A, float &B, float &C) const
{
    return totalLeastSquareFit(numPoints, points, A, B, C);
}

void ASBrushPathFitting::widthLineFit(int numPoints, const vec2 *points, float &m, float &b) const
{
    // compute sums for linear system
    float fSumX = 0.0;
    float fSumY = 0.0;
    float fSumXX = 0.0;
    float fSumXY = 0.0;

    float n=numPoints;

    for (int i = 0; i < numPoints; i++)
    {
        fSumX += points[i][1];
        fSumY += points[i][0];
        fSumXX += points[i][1]*points[i][1];
        fSumXY += points[i][0]*points[i][1];
    }
    m=(n*fSumXY-fSumX*fSumY)/(n*fSumXX-fSumX*fSumX); //strictly vertical, slope infinity

    if (isnan(m))
    {
        m = INT_MAX;
    }

    b=(fSumY-m*fSumX)/n;


    if (isnan(n)) qDebug("n is nan\n");
}

void ASBrushPathFitting::heightLineFit(int numPoints, const vec2 *points, float &m, float &b) const
{

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
    m=(n*fSumXY-fSumX*fSumY)/(n*fSumXX-fSumX*fSumX); //strictly vertical, slope infinity

    if (isnan(m))
    {
        m = INT_MAX;
    }

    b=(fSumY-m*fSumX)/n;


    if (isnan(n)) qDebug("n is nan\n");

}

void ASBrushPathFitting::recurseThroughWalkMatrix(QVector <QVector <int> > walkMatrix, int begin, int end, bool *segmentEnd) const
{

    if (begin+1>=end) {
        segmentEnd[begin]=true;
        segmentEnd[end]=true;
    }

    if (walkMatrix[begin][end]==-1) {
        segmentEnd[begin]=true;
        segmentEnd[end]=true;
    }
    else {
        recurseThroughWalkMatrix(walkMatrix, begin, walkMatrix[begin][end], segmentEnd);
        recurseThroughWalkMatrix(walkMatrix, walkMatrix[begin][end], end, segmentEnd);
    }

}

void ASBrushPathFitting::updateOffset(ASBrushPath* bp) const
{
    int numSamples = bp->nbVertices();

    vec2 pos;
    vec2 tangent, normal;
    vec2 offset;
    for (int i=0; i<numSamples; i++)
    {
        ASVertexContour* sample =  bp->at(i)->sample();
        pos = sample->position();
        vec2 newPos(_samplePointSet[i][0], _samplePointSet[i][1]);
        pos = newPos - pos;
        tangent = sample->tangent();
        normal  = sample->normal();

        offset[0] = pos DOT tangent;
        offset[1] = pos DOT normal;
        bp->at(i)->setOffset(offset);
    }
}

void ASBrushPathFitting::calculateArcPosition(ASBrushPath * bp)
{
    vec2 pos, nPos, offset, tangent, normal, center;
    vec2 pos1, dir1, nDir, nPos1;
    int numSamples;

    vec2 random_offset = bp->offset();

    float centerX = bp->fittingPara().A;
    float centerY = bp->fittingPara().B;
    float radius = bp->fittingPara().C;

    center = vec2(centerX, centerY) + random_offset;
    numSamples = bp->nbVertices();
    if (numSamples <= 3)
    {
        bp->setSweepingAngle(0);
        float arclength = 0;
        vec2 midPoint = vec2(0,0);
        for (int i=0; i<numSamples; i++)
        {
            if (i!= 0)
                arclength += dist(bp->at(i)->sample()->position(), bp->at(i-1)->sample()->position());
            midPoint += bp->at(i)->sample()->position();
        }
        midPoint /= numSamples;
        bp->setMidPoint(midPoint);
        bp->setFittingArclength(arclength);
        return;
    }

    pos1 = bp->at(0)->sample()->position();
    dir1 = normalized(pos1 - center);
    nPos1 = center + dir1 * float(radius); //the first point on the arc

    double finalAngle = determineSweepingAngle(bp, center, radius);

    double sweepingAngle;
    for (int j = 0; j<numSamples; j++)
    {
        sweepingAngle = j * finalAngle;

        pos = bp->at(j)->sample()->position(); //the original position
        nDir[0] = cos(sweepingAngle) * dir1[0] - sin(sweepingAngle) * dir1[1];
        nDir[1] = sin(sweepingAngle) * dir1[0] + cos(sweepingAngle) * dir1[1];
        nPos = nDir * float(radius) + center;
        pos = nPos + - pos;

        tangent = bp->at(j)->sample()->tangent();
        normal = bp->at(j)->sample()->normal();

        offset[0] = pos DOT tangent;
        offset[1] = pos DOT normal;

        float t = std::min(1.0, k_fittingMaxDistance / dist(offset, bp->at(j)->offset()));
        if (k_fittingAveraging && !bp->isNewFitting())
            offset = bp->at(j)->offset() * (1 - t) + t * offset;

        bp->at(j)->setOffset(offset);
    }
}

void ASBrushPathFitting::calculateEndPointsPosition(ASBrushPath *bp, float A, float B, float C, vec2& pos1, vec2& pos2)
{
    float denom = (A * A + B * B);

    vec2 pos, nPos;
    vec2 random_offset = bp->offset();

    ASVertexContour* sample = bp->first()->sample();

    pos = sample->position();
    nPos[0] = (B * B * pos[0] - A * B * pos[1] + A * C) / denom;
    nPos[1] = (B * C - A * B * pos[0] + A * A * pos[1]) / denom;
    pos1 = nPos  + random_offset[0]*sample->tangent() + random_offset[1]*sample->normal();

    sample = bp->last()->sample();

    pos = sample->position();
    nPos[0] = (B * B * pos[0] - A * B * pos[1] + A * C) / denom;
    nPos[1] = (B * C - A * B * pos[0] + A * A * pos[1]) / denom;
    pos2 = nPos  + random_offset[0]*sample->tangent() + random_offset[1]*sample->normal();
}

vec2 ASBrushPathFitting::averageLineEndPoint(ASBrushPath *bp, vec2 pos1, vec2 pos2, vec2 &posAvg1, vec2 &posAvg2)
{

    float dist1 = dist(pos1, bp->first()->position());
    float dist2 = dist(pos2, bp->last()->position());

    //average the end point offset with the previous end point offsets
    float minT = 0.125f;
    float maxDist = 10.f;
    float t1 = dist1 > maxDist ? minT : (1.f-minT)*1.f/sqrt(1.f+dist1)+minT;
    float t2 = dist2 > maxDist ? minT : (1.f-minT)*1.f/sqrt(1.f+dist2)+minT;

    posAvg1 = bp->first()->position() * (1.f - t1) + t1 * pos1;
    posAvg2 = bp->last()->position() * (1.f - t2) + t2 * pos2;

    return vec2(dist1,dist2);
}

void ASBrushPathFitting::interpolateEndPoint(ASBrushPath *bp, vec2 start, vec2 end)
{
    float t;
    vec2 offset, pos;
    //interpolate between the two new endpoints to get the rest of the line
    for (int j=0; j<bp->nbVertices(); j++)
    {
        ASVertexContour* sample = bp->at(j)->sample();
        t = float(j) / (bp->nbVertices()-1);
        pos = (1-t) * start + t * end;
        pos = pos - sample->position();

        offset[0] = pos DOT sample->tangent();
        offset[1] = pos DOT sample->normal();
        bp->at(j)->setOffset(offset);
    }
}

void ASBrushPathFitting::calculateLinePosition(ASBrushPath *bp)
{
    float A = bp->fittingPara().A;
    float B = bp->fittingPara().B;
    float C = bp->fittingPara().C;

    vec2 pos1, pos2;

    calculateEndPointsPosition(bp,A,B,C, pos1, pos2);

    if (!bp->isNewFitting() && k_fittingAveraging){
        vec2 posAvg1, posAvg2;
        averageLineEndPoint(bp, pos1, pos2, posAvg1, posAvg2);
        pos1=posAvg1;
        pos2=posAvg2;
    }

    interpolateEndPoint(bp, pos1, pos2);
}

float ASBrushPathFitting::calculateDifference(vec2 input, vec2 fitted) const
{
    return len2(input - fitted);
}


void ASBrushPathFitting::findBreakingPositionForLine(float eachSegmentCost, const ASBrushPath* bp)
{
    _segmentInfoSet.clear();

    int numSamples = bp->nbVertices();

    _samplePointSet.clear();

    //using all the input points for fitting lines
    for (int i=0; i<numSamples; i++)
    {
        _samplePointSet.push_back(bp->at(i)->sample()->position());
    }

    QVector <QVector <float> > errorMatrix(numSamples, QVector<float>(numSamples,0));
    QVector <QVector <int> >   walkMatrix(numSamples, QVector<int>(numSamples,-1));
    QVector <QVector <float> > AMatrix(numSamples, QVector<float>(numSamples,0));
    QVector <QVector <float> > BMatrix(numSamples, QVector<float>(numSamples,0));
    QVector <QVector <float> > CMatrix(numSamples, QVector<float>(numSamples,0));

    //populate first diagonal (cost of line segment between neighbouring points)
    for (int i=0; i<numSamples-1; i++) {
        errorMatrix[i][i+1]=eachSegmentCost;
        vec2 points[2];
        points[0] = _samplePointSet[i];
        points[1] = _samplePointSet[i+1];
        LineFit(2, points, AMatrix[i][i+1], BMatrix[i][i+1], CMatrix[i][i+1]);
    }

    float minError, fitError;
    int minIndex;
    //iterate through remaining diagonals
    for (int j=2; j<numSamples; j++) { //the length of segment
        for (int i=0; i+j<numSamples; i++) {

            //do linear regression on segments between i and i+j inclusive
            //if that error + penalty for segment is less than
            //sum of errors [i][i+j-1] and [i+1][i+j], then use that
            //otherwise, use sum of [i][i+j-1] and [i+1][i+j]

            vec2 *points=new vec2[j+1];
            for (int each=i;each<=i+j;each++) {
                points[each-i]=_samplePointSet[each];
            }

            fitError = LineFit(j+1, points, AMatrix[i][i+j], BMatrix[i][i+j], CMatrix[i][i+j]);

            minError=fitError+eachSegmentCost;
            minIndex=-1;

            for (int each=i+1;each<i+j;each++) { //check each partitioning at this level
                if (errorMatrix[i][each]+errorMatrix[each][i+j]<minError) {
                    minIndex=each;
                    minError=errorMatrix[i][each]+errorMatrix[each][i+j];
                }
            }

            walkMatrix[i][i+j]=minIndex;
            errorMatrix[i][i+j]=minError;

            delete[] points;
        }
    }

    //use walk matrix and determine partitions
    bool *segmentEnd = new bool[numSamples];
    for (int i=0;i<numSamples;i++) {
        segmentEnd[i]=false;
    }
    recurseThroughWalkMatrix(walkMatrix, 0, numSamples-1, segmentEnd);

    //intersect linear regressed lines
    int endIndex = -1;
    int endNextIndex = -1;

    for (int j=1;j<numSamples;j++) {
        if (segmentEnd[j]) {
            endIndex=j;
            break;
        }
    }

    _segmentInfoSet.push_back(fittingSegment(0, endIndex, AMatrix[0][endIndex], BMatrix[0][endIndex], CMatrix[0][endIndex], errorMatrix[0][endIndex]));
    for (int i=0;i<numSamples-1;i++) {
        if (segmentEnd[i]) {
            for (int j=i+1;j<numSamples;j++) {
                if (segmentEnd[j]) {
                    endIndex=j;
                    break;
                }
            }

            endNextIndex=endIndex;
            for (int j=endIndex+1;j<numSamples;j++) {
                if (segmentEnd[j]) {
                    endNextIndex=j;
                    break;
                }
            }

            if (endNextIndex>endIndex) { //this is an intersection between two lines

                float A2 = AMatrix[endIndex][endNextIndex];
                float B2 = BMatrix[endIndex][endNextIndex];
                float C2 = CMatrix[endIndex][endNextIndex];
                float error = errorMatrix[endIndex][endNextIndex];

                _segmentInfoSet.push_back(fittingSegment(endIndex, endNextIndex, A2, B2, C2, error));
            }

        }
    }

    delete[] segmentEnd;

    errorMatrix.clear();
    walkMatrix.clear();
    AMatrix.clear();
    BMatrix.clear();
    CMatrix.clear();
}

double ASBrushPathFitting::determineSweepingAngle(ASBrushPath *bp, vec2 center, double radius)
{
    vec2 pos1, dir1, nPos1;
    vec2 pos3, dir3, nPos3;
    int numSamples = bp->nbVertices();

    pos1 = bp->at(0)->sample()->position();
    dir1 = normalized(pos1 - center);
    nPos1 = center + dir1 * float(radius); //the first point on the arc

    pos3 = bp->at(numSamples-1)->sample()->position();
    dir3 = normalized(pos3 - center);
    nPos3 = center + dir3 * float(radius); //the other end point on the arc

    double totalAngle = acos(dir1 DOT dir3);
    double angle[4];
    angle[0] = totalAngle / (numSamples - 1);
    angle[1] = -angle[0];
    angle[2] = (2 * M_PI - totalAngle) / (numSamples - 1);
    angle[3] = -angle[2];

    double sweepingAngle;
    vec2 pos, nDir, nPos;
    //find the one way of sweeping with the smallest fitting error over the whole brushpath
    double minError = 100000000;
    int minIndex = -1;
    for (int i=0; i<4; i++)
    {
        double curError = 0;
        for (int j = 0; j<numSamples; j++)
        {
            sweepingAngle = j * angle[i];

            pos = bp->at(j)->sample()->position(); //the original position
            nDir[0] = cos(sweepingAngle) * dir1[0] - sin(sweepingAngle) * dir1[1];
            nDir[1] = sin(sweepingAngle) * dir1[0] + cos(sweepingAngle) * dir1[1];
            nPos = nDir * float(radius) + center;
            pos = nPos  - pos;
            curError += len2(pos);
        }
        if (curError < minError){
           minError = curError;
           minIndex = i;
        }
    }

    sweepingAngle = angle[minIndex];
    vec2 midPoint;
    double halfAngle = sweepingAngle * (numSamples - 1) * 0.5;
    midPoint[0] = cos(halfAngle) * dir1[0] - sin(halfAngle) * dir1[1];
    midPoint[1] = sin(halfAngle) * dir1[0] + cos(halfAngle) * dir1[1];
    midPoint = midPoint * (float)radius + center;

    bp->setSweepingAngle(angle[minIndex]);
    bp->setFittingArclength(fabs(sweepingAngle * (numSamples - 1) * radius));
    bp->setMidPoint(midPoint);

    return sweepingAngle;
}

int ASBrushPathFitting::findBreakingPositionForArc(float segmentCost, float targetRadius, float radiusWeight, const ASBrushPath* bp)
{
    float eachSegmentCost = segmentCost;

    _segmentInfoSet.clear();

    int numSamples = bp->nbVertices();

    _samplePointSet.clear();

    //using all the input points for fitting lines
    for (int i=0; i<numSamples; i++) {
        _samplePointSet.push_back(bp->at(i)->sample()->position());
    }

    QVector <QVector <float> > errorMatrix(numSamples, QVector<float>(numSamples,0));
    QVector <QVector <int> >   walkMatrix(numSamples, QVector<int>(numSamples,0));
    QVector <QVector <float> > AMatrix(numSamples, QVector<float>(numSamples,0)); //x of circle center
    QVector <QVector <float> > BMatrix(numSamples, QVector<float>(numSamples,0)); //y of circle center
    QVector <QVector <float> > CMatrix(numSamples, QVector<float>(numSamples,0)); //radius or circle

    for (int i=0;i<numSamples;i++) {
        for (int j=0;j<numSamples;j++) {
            errorMatrix[i][j] = 0.0;
            walkMatrix[i][j] = -1;
        }
    }

    //populate first diagonal (cost of circle segment fitting three points)
    for (int i=0;i+2<numSamples;i++) {
        errorMatrix[i][i+2] = eachSegmentCost;
        vec2 points[3];
        points[0] = _samplePointSet[i];
        points[1] = _samplePointSet[i+1];
        points[2] = _samplePointSet[i+2];

        if (testLine(3, points, AMatrix[i][i+2], BMatrix[i][i+2], CMatrix[i][i+2]) < 0)
            circularArcFit(3, points, AMatrix[i][i+2], BMatrix[i][i+2], CMatrix[i][i+2]);

        float radius = CMatrix[i][i+2] - targetRadius;
        errorMatrix[i][i+2] += radius * radius * radiusWeight * 10;
    }

    float minError, fitError, differFromTargetRadiusError;
    int minIndex;
    //iterate through remaining diagonals
    for (int j=3;j<numSamples;j++) { //the length of segment
        for (int i=0;i+j<numSamples;i++) {

            // do linear regression on segments between i and i+j inclusive
            // if that error + penalty for segment is less than
            //sum of errors [i][i+j-1] and [i+1][i+j], then use that
            // otherwise, use sum of [i][i+j-1] and [i+1][i+j]

            vec2 *points=new vec2[j+1];
            for (int each=i;each<=i+j;each++) {
                points[each-i] = _samplePointSet[each];
            }

            int colinear = testLine(j+1, points, AMatrix[i][i+j], BMatrix[i][i+j], CMatrix[i][i+j]);
            if (colinear<0)
                fitError = circularArcFit(j+1, points, AMatrix[i][i+j], BMatrix[i][i+j], CMatrix[i][i+j]);
            else
                fitError = colinear;

            float radius = CMatrix[i][i+j] - targetRadius;
            differFromTargetRadiusError = radius * radius * radiusWeight * 10;

            minError = fitError + eachSegmentCost + differFromTargetRadiusError;
            minIndex = -1;

            for (int each=i+2;each<i+j-1;each++) { //check each partitioning at this level
                if (errorMatrix[i][each]+errorMatrix[each][i+j]<minError) {
                    minIndex = each;
                    minError = errorMatrix[i][each]+errorMatrix[each][i+j];
                }
            }

            walkMatrix[i][i+j] = minIndex;
            errorMatrix[i][i+j] = minError;

            delete[] points;
        }
    }

    //use walk matrix and determine partitions
    bool *segmentEnd=new bool[numSamples];
    for (int i=0;i<numSamples;i++) {
        segmentEnd[i]=false;
    }
    recurseThroughWalkMatrix(walkMatrix, 0, numSamples-1, segmentEnd);

    //intersect linear regressed lines
    int endIndex = -1;
    int endNextIndex = -1;

    for (int j=1;j<numSamples;j++) {
        if (segmentEnd[j]) {
            endIndex=j;
            break;
        }
    }

    _segmentInfoSet.push_back(fittingSegment(0, endIndex, AMatrix[0][endIndex], BMatrix[0][endIndex], CMatrix[0][endIndex], errorMatrix[0][endIndex]));
    for (int i=0;i<numSamples-1;i++) {
        if (segmentEnd[i]) {
            for (int j=i+1;j<numSamples;j++) {
                if (segmentEnd[j]) {
                    endIndex=j;
                    break;
                }
            }

            endNextIndex=endIndex;
            for (int j=endIndex+1;j<numSamples;j++) {
                if (segmentEnd[j]) {
                    endNextIndex=j;
                    break;
                }
            }

            if (endNextIndex>endIndex) { //this is an intersection between two lines

                float A2=AMatrix[endIndex][endNextIndex];
                float B2=BMatrix[endIndex][endNextIndex];
                float C2=CMatrix[endIndex][endNextIndex];
                float error = errorMatrix[endIndex][endNextIndex];

                _segmentInfoSet.push_back(fittingSegment(endIndex, endNextIndex, A2, B2, C2, error));
            }

        }
    }

    delete[] segmentEnd;

    errorMatrix.clear();
    walkMatrix.clear();
    AMatrix.clear();
    BMatrix.clear();
    CMatrix.clear();

    return _segmentInfoSet.size();
}
