/*****************************************************************************\

Scene.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef SCENE_H_
#define SCENE_H_

#include <QDomElement>
#include <QDir>
#include <QTimer>

#include "GQVertexBufferSet.h"
/*****************************************************************************\

Scene.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu
Copyright (c) 2009 Forrester Cole

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "GQShaderManager.h"
#include "Stats.h"
#include "XForm.h"
#include "TriMesh.h"

#include "Sphere.h"
#include "Cube.h"
#include "Quad.h"

#include "DialsAndKnobs.h"

enum ModelType
{
	CUBE,
	SPHERE,
    MESH,
	NUM_MODELS
};

struct Mesh {
    Mesh(TriMesh* m, const QString& f) : trimesh(m), filename(f) {}
    ~Mesh() { delete trimesh; }
    TriMesh*             trimesh;
    QString              filename;
    GQVertexBufferSet   vertex_buffer_set;
    QVector<int>		 tristrips;
};

class GLViewer;
class Session;

class Scene : public QObject
{
    Q_OBJECT

public:
	Scene();
	~Scene();

	void init();
	void clear();

	bool load( const QString& filename );
	bool load( const QDomElement& root, const QDir& path );
    bool loadTrimesh( const QString& filename );
	bool save( const QString& filename, const GLViewer* viewer,
		const DialsAndKnobs* dials_and_knobs );
	bool save( QDomDocument& doc, QDomElement& root, const QDir& path );

    bool loadToonTex( const QString &filename, int target );

	void recordStats(Stats& stats);

    void drawScene(GQShaderRef &shader, const ModelType type);
    void drawScene(const QString &programName, const ModelType type);
    void drawToon(bool saveBuffer, QString &filename);

    void computeAdvection();
    GQTexture2D*  denseFlowTexture() { return _geomflow_buffer.colorTexture(0); }
    GQFloatImage* denseFlowBuffer()  { return &_denseFlow; }

	void boundingSphere(vec& center, float& radius, const ModelType type);

    void setCameraTransform( const xform& xf ) { _camera_transform = xf; }
	void setCameraPosition( const vec4f pos ) { _camera_position = pos; }
    void setModelViewMatix( const xform& xf )  { _prevModelView_matrix = _modelView_matrix; _modelView_matrix = xf; }
    void setProjectionMatrix( const xform& xf ){ _projection_matrix = xf; }

	const QDomElement& viewerState() { return _viewer_state; }
	const QDomElement& dialsAndKnobsState() { return _dials_and_knobs_state; }

	static QString fileExtension() { return QString("qvs"); }

    bool isAnimated() const { return _meshes.size() > 1; }
    QTimer* animationTimer() { return &_animation_timer; }
    int currentFrameNumber() const { return _current_frame; }
    Mesh* currentMesh() { return _meshes[currentFrameNumber()]; }
    const Mesh* currentMesh() const { return _meshes.at(currentFrameNumber()); }

    Session* session() { return _session; }
    void setSession( Session* session ) { _session = session; }

public slots:
    void toggleAnimation(bool play);
    void advanceAnimation();
    void computeGeometricFlow();

protected:
    void setupMesh(TriMesh *trimesh);
    void setupVertexBufferSet(Mesh *mesh);
    void setupTextures(GQShaderRef& shader);

    void setupLighting(GQShaderRef& shader);
    void drawMesh(GQShaderRef& shader);

    // Triangle meshes
    QList<Mesh*>     _meshes;

    // Procedural objects
	Sphere*				_sphere;
	Cube*				_cube;
	Quad*				_quad;

    // Toon
    GQTexture2D*        _tex_toon;
    QString             _tex_toon_filename;

    // FrameBuffer Objects
    GQFramebufferObject _fbo;
    GQFramebufferObject _fbo_toon;
    GQFramebufferObject _warpFbo;
    GQFramebufferObject _finalFbo;

    GQFramebufferObject _geomflow_buffer;
    GQFloatImage        _denseFlow;

    // Matrices
    xform               _camera_transform;
    xform               _modelView_matrix;
    xform               _prevModelView_matrix;
    xform               _projection_matrix;
	vec                 _light_direction;
	vec4f               _camera_position;

	QDomElement         _viewer_state;
	QDomElement         _dials_and_knobs_state;

    // Animation
    int _current_frame;
    QTimer _animation_timer;

    Session* _session;
};

#endif // SCENE_H_
