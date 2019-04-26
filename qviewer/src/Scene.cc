/*****************************************************************************\

Scene.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu
Copyright (c) 2009 Forrester Cole

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "Scene.h"
#include "Session.h"
#include "GLViewer.h"

#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QStringList>

#include "GQDraw.h"
using namespace GQDraw;

#include <assert.h>

const int CURRENT_VERSION = 1;

static dkSlider k_toon_slider("Light->Toon factor",1);

static dkFloat k_animation_frame_time("Mesh animation->Frame time", 0.1, 0.01, 10, 0.1);
static dkBool  k_play_animation("Mesh animation->Play", false);

extern dkStringList k_model;

Scene::Scene()
{
    _current_frame = 0;
    _session = NULL;
    _tex_toon_filename = "";
    _tex_toon = NULL;
    connect(&k_play_animation, SIGNAL(valueChanged(bool)), this, SLOT(toggleAnimation(bool)));
    connect(&_animation_timer, SIGNAL(timeout()), this, SLOT(advanceAnimation()));
}

Scene::~Scene()
{
    clear();
    delete _tex_toon;
    delete _cube;
    delete _sphere;
    delete _quad;
}

void Scene::init()
{
    _tex_toon = new GQTexture2D();

    if (!loadToonTex(_tex_toon_filename, GL_TEXTURE_2D))
	{
		QMessageBox::critical(NULL, "Open Failed", "Failed to load texture");
	}

	_sphere = new Sphere();
	_cube = new Cube();
	_quad = new Quad();
}

void Scene::clear()
{
    qDeleteAll(_meshes);
	_viewer_state.clear();
	_dials_and_knobs_state.clear();
}

bool Scene::loadTrimesh( const QString& filename )
{
    TriMesh* trimesh = TriMesh::read(qPrintable(filename));
    if (!trimesh)
    {
        clear();
        return false;
    }
    _meshes << new Mesh(trimesh,filename);
    _viewer_state.clear();
    _dials_and_knobs_state.clear();
    setupMesh(trimesh);
    return true;
}

bool Scene::load( const QString& filename )
{
    if (filename.endsWith(fileExtension()))
	{
		QFile file(filename);
		if (!file.open(QIODevice::ReadOnly))
		{
			qWarning("Could not open %s", qPrintable(filename));
			return false;
		}

		QDomDocument doc("scene");
		QString parse_errors;
		if (!doc.setContent(&file, &parse_errors))
		{
			qWarning("Parse errors: %s", qPrintable(parse_errors));
			return false;
		}

		file.close();

		QDomElement root = doc.documentElement();
		QDir path = QFileInfo(filename).absoluteDir();

		return load(root, path); 
	}
    else if (filename.endsWith("seq")) { // sequence of meshes
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning("Could not open %s", qPrintable(filename));
            return false;
        }
        QTextStream ts(&file);
        QDir filedir = QFileInfo(filename).dir();
        QString line = ts.readLine();
        while (!line.isNull()) {
            QString framename = filedir.filePath(line);
            loadTrimesh(framename);
            line = ts.readLine();
        }
        computeGeometricFlow();
        _session = NULL;
    }else{
        loadTrimesh(filename);
        computeGeometricFlow();
        _session = NULL;
	}

    return true;
}

bool Scene::load( const QDomElement& root, const QDir& path )
{
	int version = root.attribute("version").toInt();
	if (version != CURRENT_VERSION)
	{
		qWarning("Scene::load: file version out of date (%d, current is %d)", 
			version, CURRENT_VERSION);
		return false;
	}

	QDomElement model = root.firstChildElement("model");
	if (model.isNull())
	{
		qWarning("Scene::load: no model node found.\n");
		return false;
	}

	QString relative_filename = model.attribute("filename");
	QString abs_filename = path.absoluteFilePath(relative_filename);

    TriMesh* trimesh = TriMesh::read(qPrintable(abs_filename));
    if (!trimesh)
	{
		qWarning("Scene::load: could not load %s\n", 
			qPrintable(abs_filename));
		return false;
	}
    _meshes << new Mesh(trimesh,abs_filename);

    QDomElement toon = root.firstChildElement("toon");
    if (!toon.isNull())
    {
        QString relative_filename = toon.attribute("filename");
        QString abs_filename = path.absoluteFilePath(relative_filename);
        loadToonTex(abs_filename, GL_TEXTURE_2D);
    }

	_viewer_state = root.firstChildElement("viewerstate");
	if (_viewer_state.isNull())
	{
		qWarning("Scene::load: no viewerstate node found.\n");
		clear();
		return false;
	}

	_dials_and_knobs_state = root.firstChildElement("dials_and_knobs");
	if (_dials_and_knobs_state.isNull())
	{
		qWarning("Scene::load: no dials_and_knobs node found.\n");
		return false;
	}

    QDomElement session = root.firstChildElement("session");
    if (!session.isNull())
    {
        _session = new Session();
        bool ret = _session->load(session);
        if (!ret)
        {
            qWarning("Scene::load: incorrect session node found.\n");
            clear();
            return false;
        }
    }

    setupMesh(trimesh);
    computeGeometricFlow();

	return true;
}

bool Scene::save(const QString& filename, const GLViewer* viewer,
				 const DialsAndKnobs* dials_and_knobs)
{
	QDomDocument doc("scene");
	QDomElement root = doc.createElement("scene");
	doc.appendChild(root);

	_viewer_state = viewer->domElement("viewerstate", doc);
	_dials_and_knobs_state = 
		dials_and_knobs->domElement("dials_and_knobs", doc);

	QDir path = QFileInfo(filename).absoluteDir();

	bool ret = save(doc, root, path);
	if (!ret)
		return false;

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly))
	{
		qWarning("Scene::save - Could not save %s", qPrintable(filename));
		return false;
	}

	file.write(doc.toByteArray());

	file.close();

	return true;
}

bool Scene::save( QDomDocument& doc, QDomElement& root, const QDir& path )
{
	root.setAttribute("version", CURRENT_VERSION);

	QDomElement model = doc.createElement("model");
    model.setAttribute("filename", path.relativeFilePath(_meshes.last()->filename));
	root.appendChild(model);

    QDomElement toon = doc.createElement("toon");
    toon.setAttribute("filename", path.relativeFilePath(_tex_toon_filename));
    root.appendChild(toon);

	root.appendChild(_viewer_state);
	root.appendChild(_dials_and_knobs_state);

    if (_session)
    {
        QDomElement session = doc.createElement("session");
        _session->save(doc, session);
        root.appendChild(session);
    }

	return true;
}

void Scene::advanceAnimation()
{
     _current_frame = (_current_frame+1)%_meshes.size();
}

void Scene::toggleAnimation(bool play)
{
    if (play) {
        _animation_timer.start(k_animation_frame_time*1000);
    } else {
        _animation_timer.stop();
    }
}

bool Scene::loadToonTex( const QString &filename, int target )
{
    if(_tex_toon)
        delete _tex_toon;
    GQImage _toon_img;
    _toon_img.load(filename);
    _tex_toon = new GQTexture2D();
    _tex_toon_filename = filename;

    bool success = _tex_toon->create(_toon_img, target);
    k_toon_slider.setLowerLimit(0);
    k_toon_slider.setUpperLimit(_tex_toon->height());
    k_toon_slider.setStepSize(0);
    _tex_toon->setWrapMode(GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE);

    return success;
}

void Scene::boundingSphere(vec& center, float& radius, const ModelType type)
{
	center = vec();
	switch(type)
	{
	case CUBE:
		radius = _cube->radius();
		break;
	case SPHERE:
		radius = _sphere->radius();
		break;
    case MESH:
        center = currentMesh()->trimesh->bsphere.center;
        radius = currentMesh()->trimesh->bsphere.r;
		break;
	default:
		break;
	}
}

void Scene::setupTextures(GQShaderRef& shader)
{
    if(shader.uniformLocation("textureGrayScale")>=0){
        shader.bindNamedTexture("textureGrayScale",_tex_toon);
        shader.setUniform1i("toon_factor",k_toon_slider.value());
        shader.setUniform1i("tex_height",_tex_toon->height());
    }
}

void Scene::drawScene(GQShaderRef& shader, const ModelType type)
{
    shader.setUniformxform("projection_matrix",_projection_matrix);
    shader.setUniformxform("model_view_matrix", _modelView_matrix);
    if(shader.uniformLocation("normal_matrix")>=0)
    {
        xform normal_matrix;
        normal_matrix = inv(_modelView_matrix);
        normal_matrix = transpose(normal_matrix);
        shader.setUniformMatrixUpper3x3("normal_matrix", normal_matrix);
    }
    if(shader.uniformLocation("model_view_matrix_inverse")>=0){
        xform model_view_matrix_inverse = inv(_modelView_matrix);
        shader.setUniformxform("model_view_matrix_inverse", model_view_matrix_inverse);
    }

    glEnable(GL_MULTISAMPLE);

    setupLighting(shader);

    setupTextures(shader);

    switch(type)
    {
    case CUBE:
        _cube->draw(shader);
        break;
    case SPHERE:
        _sphere->draw(shader);
        break;
    case MESH:
        drawMesh(shader);
        break;
    default:
        break;
    }

    glDisable(GL_MULTISAMPLE);
}

void Scene::drawScene(const QString &programName, const ModelType type)
{
    if (GQShaderManager::status() != GQ_SHADERS_OK)
        return;

    GQShaderRef shader = GQShaderManager::bindProgram(programName);

    setupTextures(shader);

    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glEnable(GL_MULTISAMPLE);
    drawScene(shader,type);
    glDisable(GL_MULTISAMPLE);

    shader.unbind();
}

void Scene::setupMesh(TriMesh* trimesh)
{
    trimesh->need_bsphere();
    trimesh->need_bbox();
    trimesh->need_normals();
    trimesh->need_tstrips();
    trimesh->need_faces();
    trimesh->need_uv_dirs();
}

void Scene::setupVertexBufferSet(Mesh* mesh)
{
	// Trimesh stores triangle strips as length followed by indices.
	// Grab them and store the offsets and lengths.
    mesh->tristrips.clear();
    const int *t = &mesh->trimesh->tstrips[0];
    const int *end = t + mesh->trimesh->tstrips.size();
    while (t < end) {
		int striplen = *t++;
        mesh->tristrips.push_back(striplen);
		t += striplen;
	}

    mesh->vertex_buffer_set.add(GQ_VERTEX, mesh->trimesh->vertices);
    mesh->vertex_buffer_set.add(GQ_NORMAL, mesh->trimesh->normals);
    if(mesh->trimesh->colors.empty()){
        // generate mid-gray color
        mesh->trimesh->colors.resize(mesh->trimesh->vertices.size(),Color(0.6,0.6,0.6));
    }
    mesh->vertex_buffer_set.add(GQ_COLOR,  mesh->trimesh->colors);
    if(!mesh->trimesh->texcoords.empty())
        mesh->vertex_buffer_set.add(GQ_TEXCOORD, mesh->trimesh->texcoords);
    mesh->vertex_buffer_set.add(GQ_INDEX, 1, mesh->trimesh->tstrips);

    mesh->vertex_buffer_set.copyToVBOs();
}	

void Scene::computeGeometricFlow()
{
    int nvertices = _meshes[0]->trimesh->vertices.size();
    int nframes = _meshes.size();
    for (int f = 0; f < nframes; f++) {
        std::vector<point> pf;
        for (int g = 0; g < nvertices; g++) {
            point p = _meshes[f]->trimesh->vertices[g];
            point q = _meshes[(f+1)%nframes]->trimesh->vertices[g];
            pf.push_back(q-p);
        }
        _meshes[f]->vertex_buffer_set.add("geom_flow",pf);
    }
}

void Scene::drawMesh(GQShaderRef& shader)
{
    Mesh* mesh = currentMesh();
    if (mesh->vertex_buffer_set.numBuffers() == 1)
        setupVertexBufferSet(mesh);

    assert(mesh->trimesh->tstrips.size() > 0);

    mesh->vertex_buffer_set.bind(shader);

	int offset = 1;
    for (int i = 0; i < mesh->tristrips.size(); i++) {
        drawElements(mesh->vertex_buffer_set, GL_TRIANGLE_STRIP, offset, mesh->tristrips[i]);
        offset += mesh->tristrips[i] + 1; // +1 to skip the length stored in the tristrip array.
	}

    mesh->vertex_buffer_set.unbind();
}

void Scene::drawToon(bool saveBuffer, QString &filename)
{
    GQShaderRef shader = GQShaderManager::bindProgram("05_toon");

    glClearColor(1, 1, 1, 0.0);

    _fbo_toon.initFullScreen(2,GQ_ATTACH_DEPTH,GQ_COORDS_NORMALIZED);
    _fbo_toon.bind(GQ_CLEAR_BUFFER);
    _fbo_toon.drawBuffer(0);

    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    drawScene(shader,(ModelType)k_model.index());

    _fbo_toon.unbind();
    shader.unbind();

    if(saveBuffer)
        _fbo_toon.saveColorTextureToFile(0,filename);
}

void Scene::computeAdvection()
{
    GLfloat viewport[4];
    glGetFloatv(GL_VIEWPORT, viewport);

    GQDraw::clearGLState();
    GQShaderRef shader = GQShaderManager::bindProgram("motion");

    _geomflow_buffer.initFullScreen(1,GQ_ATTACH_DEPTH);
    _geomflow_buffer.bind();

    GQDraw::clearGLScreen(vec(0,0,0),1.f);
    glEnable(GL_DEPTH_TEST);

    shader.setUniform4fv("viewport",viewport);
    shader.setUniformxform("prevModel_view_matrix",_prevModelView_matrix);

    drawScene(shader,(ModelType)k_model.index());

    _geomflow_buffer.unbind();
    shader.unbind();

    _geomflow_buffer.readColorTexturef(0,_denseFlow);
}

static QStringList lightPresetNames = (QStringList() << "Headlight" <<
									   "North" << "North-Northeast" << "Northeast" << "East-Northeast" <<
									   "East" << "East-Southeast" << "Southeast" << "South-Southeast" <<
									   "South" << "South-Southwest" << "Southwest" << "West-Southwest" <<
									   "West" << "West-Northwest" << "Northwest" << "North-Northwest" );
static dkStringList light_preset("Light->Direction", lightPresetNames);
static dkFloat light_depth("Light->Depth", 1.0f, 0.0f, 2.0f, 0.1f);

void Scene::setupLighting(GQShaderRef& shader)
{
	const QString& preset = light_preset.value();
	vec camera_light;

	if (preset == "Headlight")
		camera_light = vec(0.0f, 0.0f, 1.0f);
	else if (preset == "North")
		camera_light = vec(0.0f, 1.0f, light_depth);
	else if (preset == "North-Northeast")
		camera_light = vec(0.374f, 1.0f, light_depth);
	else if (preset == "Northeast")
		camera_light = vec(1.0f, 1.0f, light_depth);
	else if (preset == "East-Northeast")
		camera_light = vec(1.0f, 0.374f, light_depth);
	else if (preset == "East")
		camera_light = vec(1.0f, 0.0f, light_depth);
	else if (preset == "East-Southeast")
		camera_light = vec(1.0f, -0.374f, light_depth);
	else if (preset == "Southeast")
		camera_light = vec(1.0f, -1.0f, light_depth);
	else if (preset == "South-Southeast")
		camera_light = vec(0.374f, -1.0f, light_depth);
	else if (preset == "South")
		camera_light = vec(0.0f, -1.0f, light_depth);
	else if (preset == "South-Southwest")
		camera_light = vec(-0.374f, -1.0f, light_depth);
	else if (preset == "Southwest")
		camera_light = vec(-1.0f, -1.0f, light_depth);
	else if (preset == "West-Southwest")
		camera_light = vec(-1.0f, -0.374f, light_depth);
	else if (preset == "West")
		camera_light = vec(-1.0f, 0.0f, light_depth);
	else if (preset == "West-Northwest")
		camera_light = vec(-1.0f, 0.374f, light_depth);
	else if (preset == "Northwest")
		camera_light = vec(-1.0f, 1.0f, light_depth);
	else if (preset == "North-Northwest")
		camera_light = vec(-0.374f, 1.0f, light_depth);

	normalize(camera_light);
    xform mv_xf = rot_only( _camera_transform );
	_light_direction = mv_xf * camera_light;

	if(shader.uniformLocation("light_dir_world")>=0)
		shader.setUniform3fv("light_dir_world", _light_direction);
}

void Scene::recordStats(Stats& stats)
{
	stats.beginConstantGroup("Mesh");
    stats.setConstant("Num Vertices", currentMesh()->trimesh->vertices.size());
    stats.setConstant("Num Faces", currentMesh()->trimesh->faces.size());
	stats.endConstantGroup();
}

