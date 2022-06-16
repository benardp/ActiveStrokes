/************************************************************************
NPRGeometry.h
Copyright (c) 2009 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef LN_GEOMETRY_H_
#define LN_GEOMETRY_H_

#include <QVector>
#include <QHash>
#include <QList>
#include "GQInclude.h"
#include "GQVertexBufferSet.h"
#include "TriMesh.h"

// NPRPrimitive

enum NPRPrimitiveType { NPR_TRIANGLES, NPR_TRIANGLE_STRIP, NPR_LINES, 
                        NPR_LINE_STRIP, NPR_PROFILES, 
                        NPR_NUM_PRIMITIVES };

class NPRPrimitive : public QVector<int>
{
    public:
        NPRPrimitive( NPRPrimitiveType type ) : _type(type), _material_symbol() {}

        NPRPrimitiveType type() const { return _type; }
        const QString& materialSymbol() const { return _material_symbol; }
        void setMaterialSymbol(const QString& m) { _material_symbol = m; }

    protected:
        NPRPrimitiveType _type;
        QString          _material_symbol;
};
typedef QList<NPRPrimitive*> NPRPrimPointerList;

// NPRDataSource

class NPRDataSource : public QVector<float>
{
public:
    NPRDataSource( int width ) : _width(width) {} 

    int width() const { return _width; }
    int length() const { return size() / _width; }

    float* entry(int i) { return &(data()[i * _width]); }
    vec3f* asVec3() { return (vec3f*)data(); }
    vec2f* asVec2() { return (vec2f*)data(); }

    const float* entry(int i) const { return &constData()[i * _width]; }
    const vec3f* asVec3() const { return (vec3f*)data(); }
    const vec2f* asVec2() const { return (vec2f*)data(); }

    void appendFloat( const float& f ) { *this << f; }
    void appendVec2( const vec2f& v ) { *this << v[0] << v[1]; }
    void appendVec3( const vec3f& v ) { *this << v[0] << v[1] << v[2]; }

protected:
    int _width;
};
typedef QList<NPRDataSource> NPRDataSourceList;
typedef QList<NPRDataSource*> NPRDataSourcePointerList;
typedef QHash<QString, NPRDataSource*> NPRDataSourcePointerHash;

// NPRGeometry

class CdaGeometry;
class GQShaderRef;

using trimesh::TriMesh;

class NPRGeometry
{
    public:
        NPRGeometry();
        NPRGeometry( const CdaGeometry* cda_geometry );
        NPRGeometry( const TriMesh* trimesh );

        void clear(); // clears the geometry to an uninitialized state
        void reset(); // removes the data, but leaves the semantic structure intact

        NPRDataSource*                data( GQVertexBufferType semantic )          { return _semantic_list[semantic]; }
        const NPRDataSource*          data( GQVertexBufferType semantic ) const    { return _semantic_list[semantic]; }
        NPRDataSource*                data( const QString& name )                { return _source_hash.value(name); }
        const NPRDataSource*          data( const QString& name ) const          { return _source_hash.value(name); }
        bool                          hasData( const QString& name ) const          { return _source_hash.contains(name); }
        const NPRPrimPointerList& primList( NPRPrimitiveType type ) const     { return _primitives[type]; }

        int  numVertices() const { return _semantic_list[GQ_VERTEX]->length(); }

        const vec& bsphereCenter() const { return _bsphere_center; }
        float bsphereRadius() const { return _bsphere_radius; }

        void bind(GQShaderRef* shader = 0) const;
        void unbind() const;

        void copyToVBO(); 
        void deleteVBO(); 
        const GQVertexBufferSet* vertexBufferSet() const { return &_vertex_buffer_set; }
        GQVertexBufferSet* vertexBufferSet() { return &_vertex_buffer_set; }

        void addData( GQVertexBufferType semantic, int width ); // adds a source of appropriate length filled with zeros
        void addData( GQVertexBufferType semantic, NPRDataSource* source );
        void addData( const QString& name, int width );
        void addData( const QString& name, NPRDataSource* source );

        void debugDump( const QString& filename );

        int& material_index() { return _material_index; }

        static void convertFromCdaGeometry( NPRGeometry* geom, const CdaGeometry* cda_geometry );
        static void convertFromTrimesh( NPRGeometry* geom, const TriMesh* trimesh );

        static const NPRGeometry* currentlyBoundGeom() { return _currently_bound_geom; }

    protected:
        void findBoundingSphere();

    protected:
        // pointer list to allow storing descendants of NPRPrimitive (e.g. LnPath)
        NPRPrimPointerList       _primitives[NPR_NUM_PRIMITIVES];
        NPRDataSource*           _semantic_list[GQ_NUM_VERTEX_BUFFER_TYPES];
        NPRDataSourcePointerList _sources;
        NPRDataSourcePointerHash _source_hash;

        GQVertexBufferSet _vertex_buffer_set;

        vec               _bsphere_center;
        float             _bsphere_radius;

        int               _material_index;

        static const NPRGeometry*  _currently_bound_geom;
};

#endif


