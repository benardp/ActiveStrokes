/*****************************************************************************\

ASSnakes.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef ACTIVECONTOURS_H_
#define ACTIVECONTOURS_H_

#include "ASContour.h"
#include "ASSimpleGrid.h"
#include "ASDeform.h"

#include "GQImage.h"
#include "GQFramebufferObject.h"

#include <QHash>
#include <QList>

class ASSnakes: QObject {

    Q_OBJECT

public:
    ASSnakes();
    ~ASSnakes();

    void init(ASClipPathSet& pathSet, bool noConnectivity);
    void clear();

    void updateRefImage(GQTexture2D* refImg, GQFloatImage* geomFlow, GQFloatImage* denseFlow, ASClipPathSet& pathSet, bool useMotion=true);

    int nbContours() const { return _contourList.size(); }
    ASContour* at(int i) const { return _contourList.at(i); }

    int nbBrushPaths() const;
    ASBrushPath* brushPath(int i);

    float coverRadius() const { return _coverRadius; }
    float sMin() const { return _sMin; }
    float sMax() const { return _sMax; }
    float width() const { return _width; }

    void setNbFrames(int f) {_nbFrames =f; }
    bool animEnds() { return _currentFrame>=_nbFrames; }
    void resetCurrentFrame() { _currentFrame=-1; }

    void setNoConnect(bool connect){ _noConnectivity = connect; }

    ASClipVertex* findClosestEdgeRef(vec2 pos, vec2 tangent, bool useVisibility, float coverage);

public slots:
    void setSMin(double min);
    void setSMax(double max);
    void setCoverRadius(double radius);

protected:
    void buildSimpleGrid(ASClipPathSet& pathSet);
    void addSnakesToGrid();
    void removeContourFromGrid(ASContour* oldC);
    void removeEndPointsFromGrid(ASContour* oldC);
    void addEndPointsToGrid(ASContour* newC);
    void posToCellCoord(vec2 pos, float& r, float& c);
    int  posToKey(vec2 pos);
    int  posToKey(vec2 pos, vec2i& offsets);
    void coverage(ASClipPathSet& pathSet);
    void minLengthCleaning();
    void merge();
    bool trim();
    void trimEnd(ASVertexContour** endPt);
    void remove();
    bool extend();
    void addStartPoint(ASVertexContour *vertex, ASClipVertex* clipVertex, ASContour* contour);
    void addEndPoint(ASVertexContour *vertex, ASClipVertex* clipVertex, ASContour* contour);
    void splitAtIndices(ASContour* c, QList<int>& splitIndices);
    void splitDepthJump();
    void splitAtJunctions();
    void splitContoursTangent();
    void markCoverage();
    void findClosestEdgeRef();
    void advect(GQFloatImage* geomFlow, GQFloatImage* denseFlow);

    void printContours();
    void printSimpleGrid();

    void clearNearDeleteState();

    void fitBrushPath(); //fit line, arc, or spline to the brushpath

private:

    QList<ASContour*> _contourList;

    ASSimpleGrid _simpleGrid;

    ASDeform _deformer;

    GQTexture2D* _refImg;

    bool _noConnectivity;

    float _coverRadius;
    float _sMax;
    float _sMin;

    int _nbFrames;
    int _currentFrame;

    int _width;

    QList<ASClipVertex*> _uncovered;

    GQFramebufferObject off;
};

#endif /* ACTIVECONTOURS_H_ */
