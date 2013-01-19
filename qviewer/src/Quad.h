/*****************************************************************************\

Quad.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
Copyright (c) 2012 Pierre Benard

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _QUAD_H
#define _QUAD_H

#include "GQInclude.h"
#include "GQVertexBufferSet.h"

class Quad
{
public:
	Quad();

	void draw(GQShaderRef& shader);

private:
	GQVertexBufferSet _buffer;
};

#endif
