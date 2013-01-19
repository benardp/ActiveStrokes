/************************************************************************
NPRGeometry.cc
Copyright (c) 2009 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "NPRGeometry.h"
#include "GQInclude.h"
#include <assert.h>

#include <QFile>
#include <QTextStream>
#include <QDebug>

const NPRGeometry*  NPRGeometry::_currently_bound_geom = 0;

NPRGeometry::NPRGeometry()
{
    clear();
}

NPRGeometry::NPRGeometry( const CdaGeometry* cda_geometry )
{
    clear();
    convertFromCdaGeometry( this, cda_geometry );
}

NPRGeometry::NPRGeometry( const TriMesh* trimesh )
{
    clear();
    convertFromTrimesh( this, trimesh );
}

void NPRGeometry::clear()
{
    while (!_sources.isEmpty())
        delete _sources.takeFirst();

    for (int i = 0; i < NPR_NUM_PRIMITIVES; i++)
    {
        while (!_primitives[i].isEmpty())
            delete _primitives[i].takeFirst();
    }

    for (int i = 0; i < GQ_NUM_VERTEX_BUFFER_TYPES; i++)
        _semantic_list[i] = 0;

    _source_hash.clear();

    _bsphere_center = vec(0,0,0);
    _bsphere_radius = -1;

    _vertex_buffer_set.clear();

    _material_index = -1;
}

void NPRGeometry::reset()
{
    for (int i = 0; i < _sources.size(); i++)
        _sources[i]->clear();

    for (int i = 0; i < NPR_NUM_PRIMITIVES; i++)
    {
        while (!_primitives[i].isEmpty())
            delete _primitives[i].takeFirst();
    }
    _vertex_buffer_set.deleteVBOs();
}

void NPRGeometry::addData( const QString& name, int width )
{
    int current_length = 0;
    if (_sources.size() > 0)
        current_length = _sources[0]->length();

    NPRDataSource* newsource = new NPRDataSource(width);
    newsource->resize(width*current_length);
    _sources.push_back(newsource);
    _source_hash[name] = newsource;
    for (int i = 0; i < GQ_NUM_VERTEX_BUFFER_TYPES; i++)
    {
        if (name == GQVertexBufferNames[i])
        {
            _semantic_list[i] = newsource;
            break;
        }
    }

    _vertex_buffer_set.add(name, newsource->width(), *newsource);
}

void NPRGeometry::addData( const QString& name, NPRDataSource* source )
{
    int current_length = 0;
    if (_sources.size() > 0)
        current_length = _sources[0]->length();

    assert(_sources.size() == 0 || source->length() == current_length);

    _sources.push_back(source);
    _source_hash[name] = source;
    for (int i = 0; i < GQ_NUM_VERTEX_BUFFER_TYPES; i++)
    {
        if (name == GQVertexBufferNames[i])
        {
            _semantic_list[i] = source;
            break;
        }
    }
    _vertex_buffer_set.add(name, source->width(), *source);
}

void NPRGeometry::addData( GQVertexBufferType semantic, int width )
{
    addData( GQVertexBufferNames[semantic], width );
}

void NPRGeometry::addData( GQVertexBufferType semantic, NPRDataSource* source )
{
    addData( GQVertexBufferNames[semantic], source );
}

void NPRGeometry::bind(GQShaderRef* shader) const
{ 
    assert(_currently_bound_geom == 0);
    if (shader)
        _vertex_buffer_set.bind(*shader);
    else
        _vertex_buffer_set.bind();
    _currently_bound_geom = this;
}

void NPRGeometry::unbind() const 
{ 
    assert(_currently_bound_geom == this);
    _vertex_buffer_set.unbind();
    _currently_bound_geom = 0;
}


void NPRGeometry::copyToVBO()
{
    _vertex_buffer_set.copyToVBOs();
}

void NPRGeometry::deleteVBO()
{
    _vertex_buffer_set.deleteVBOs();
}

void NPRGeometry::debugDump(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);

    QHashIterator<QString, NPRDataSource*> iter(_source_hash);
    while (iter.hasNext()) {
        iter.next();
        out << iter.key() << "\n";
        NPRDataSource* source = iter.value();
        for (int i = 0; i < source->length(); i++)
        {
            float* entry = source->entry(i);
            for (int j = 0; j < source->width(); j++)
            {
                out << entry[j] << " ";
            }
            out << "\n";
        }
        out << "\n";
    } 

    for (int i = 0; i < NPR_NUM_PRIMITIVES; i++)
    {
        out << "prim type " << i << "\n";
        for (int j = 0; j < _primitives[i].size(); j++)
        {
            NPRPrimitive* prim = _primitives[i][j];
            out << "prim " << j << "\n";
            for (int k = 0; k < prim->size(); k++)
            {
                out << (*prim)[k] << " ";
                if ( ((NPRPrimitiveType)i == NPR_TRIANGLES && k % 3 == 2) ||
                     ((NPRPrimitiveType)i == NPR_LINES && k % 2 == 1) )
                    out << "\n";
            }
            out << "\n";
        }
        out << "\n";
    }

    file.close();
}

