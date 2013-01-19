/*****************************************************************************\

Quad.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
Copyright (c) 2012 Pierre Benard

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "Quad.h"

#include "GQDraw.h"
using namespace GQDraw;

Quad::Quad(void)
{
	QVector<float> vertex = QVector<float>() << -1.f << -1.f << 1.f << -1.f << 1.f << 1.f << -1.f << 1.f;
	_buffer.add(GQ_VERTEX,2,vertex);
	QVector<float> color = QVector<float>() << 0.5f << 0.5f << 0.5f << 0.5f << 0.5f << 0.5f << 0.5f << 0.5f << 0.5f << 0.5f << 0.5f << 0.5f;
	_buffer.add(GQ_COLOR,3,color);
	QVector<float> texcoord = QVector<float>() << 0.f << 0.f << 1.f << 0.f << 1.f << 1.f << 0.f << 1.f;
	_buffer.add(GQ_TEXCOORD,2,texcoord);
	QVector<int> index = QVector<int>() << 0 << 1 << 2 << 3;
	_buffer.add(GQ_INDEX,1,index);

	_buffer.copyToVBOs();
}

void Quad::draw(GQShaderRef& shader)
{
	_buffer.bind(shader);

	drawElements(_buffer, GL_QUADS, 0, 4);

	_buffer.unbind();
}
