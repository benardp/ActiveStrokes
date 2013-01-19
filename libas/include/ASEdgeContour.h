/*****************************************************************************\

ASEdgeContour.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libas is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef EDGECONTOUR_H_
#define EDGECONTOUR_H_

#include "ASVertexContour.h"
#include "Vec.h"

class ASEdgeContour {
public:
    ASEdgeContour(ASVertexContour *firstVertex, ASVertexContour *secondVertex=0);
    virtual ~ASEdgeContour();

    ASVertexContour* firstVertex()  const { return _vertex[0]; }
    ASVertexContour* secondVertex() const { return _vertex[1]; }

        float 		length()    const { return _length; }
	const vec2   direction() const { return _direction; }

        float restLength() const { return _restLength; }
        void   setRestLength(const float rl) { _restLength = rl; }

	void computeDirection();

	void inverse();

protected:
    ASVertexContour* _vertex[2];

        float _length;
	vec2   _direction;

        float _restLength;
};

#endif /* EDGECONTOUR_H_ */
