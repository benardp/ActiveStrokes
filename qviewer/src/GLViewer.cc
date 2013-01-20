/*****************************************************************************\

GLViewer.cc
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu
Copyright (c) 2009 Forrester Cole

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "GLViewer.h"
#include <XForm.h>
#include <assert.h>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QDebug>

#include "GQDraw.h"
#include "NPRGLDraw.h"
#include "GQGPUImageProcessing.h"
#include "Scene.h"
#include "Stats.h"

extern dkStringList k_shading;
static dkBool k_drawShading("Draw->Shading", false);

const QStringList modelChoices = QStringList() << "cube" << "sphere" << "mesh";
dkStringList k_model("Current->Model",QStringList(modelChoices));

static dkBool k_useSnakes("Current->Activate snakes", true);
       dkBool k_snapshot("Current->Activate snapshot",false);
static QStringList k_ref_list = QStringList() << "none" << "line" << "blur" << "grad" << "color" << "motion";
static dkStringList k_drawRefImg("Draw->Ref. Img.", k_ref_list);
static QStringList k_draw_list = QStringList() << "strokes" << "brushPath" << "snakes" << "samples" << "none";
static dkStringList k_drawContour("Draw->Contour", k_draw_list);
static dkBool k_drawClosest("Contours->Draw->Closest sample", false);
static dkBool k_initSnakes("Contours->Init",false);

GLViewer::GLViewer(QWidget* parent) : QGLViewer( parent )
{ 
    _inited = false;
    _visible = false;
    _display_timers = false;
    _new_scene = false;
    _scene = NULL;
    _ac_initialized = false;
    _prev_frame_number = -1;
    _snapshotPath = "";

    camera()->frame()->setWheelSensitivity(-1.0);

    QGLFormat format;
    format.setAlpha(true);
    format.setSampleBuffers(true);
    format.setSamples(8);
    format.setDoubleBuffer(true);
    setFormat(format);
    makeCurrent();
}

void GLViewer::resetView()
{
    vec center;
    float radius;

    if (_scene)
    {
        _scene->boundingSphere(center, radius,(ModelType) k_model.index());

        setSceneRadius( radius );
        setSceneCenter( qglviewer::Vec( center[0], center[1], center[2] ) );
        camera()->setFieldOfView(3.1415926f / 6.0f);
        camera()->setZNearCoefficient(0.01f);
        camera()->setOrientation(0,0);
        xform cam_xf = xform(camera()->frame()->matrix());
        _scene->setCameraTransform(cam_xf);
        showEntireScene();
    }
}

void GLViewer::setScene( Scene* scene )
{ 
    bool was_inited = _inited;

    // Temporarily clear initialized flag to stop drawing during initFromDOMElement.
    _inited = false;
    _scene = scene;
    _new_scene = true;

    if (!_scene->viewerState().isNull())
        initFromDOMElement(_scene->viewerState());

    _inited = was_inited;
}

void GLViewer::finishInit()
{
    _inited = true;
}

void GLViewer::resizeGL( int width, int height )
{
    if (width < 0 || height < 0) {
        _visible = false;
        return;
    }

    _visible = (width * height != 0);

    QGLViewer::resizeGL(width,height);
}

static bool in_draw_function = false;

void GLViewer::draw()
{  
    if (in_draw_function)
        return; // recursive draw

    if (!(_visible && _inited && _scene))
        return;

    in_draw_function = true;

    if (GQShaderManager::status() == GQ_SHADERS_NOT_LOADED)
    {
        GQShaderManager::initialize();
    }
    if (GQShaderManager::status() != GQ_SHADERS_OK)
        return;

    if(_new_scene)
    {
        glClearColor(1,1,1,1);
        _scene->init();
        _new_scene =false;
        resetView();
    }

    DialsAndKnobs::incrementFrameCounter();
    QString idx;
    idx.setNum(DialsAndKnobs::frameCounter());

    if(k_model.changedLastFrame()){
        resetView();
        _ac_initialized = false;
        in_draw_function = false;
        return;
    }

    Stats& perf = Stats::instance();
    if (_display_timers)
    {
        perf.reset();
    }

    if(k_snapshot && _snapshotPath == ""){
        _snapshotPath = QFileDialog::getExistingDirectory(this,"Snapshot directory",QDir::currentPath(),
                                                          QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);
    }

    xform modelView_xf;
    camera()->getModelViewMatrix(modelView_xf);
    _scene->setModelViewMatix(modelView_xf);
    xform proj_xf;
    camera()->getProjectionMatrix(proj_xf);
    _scene->setProjectionMatrix(proj_xf);
    qglviewer::Vec cameraPos = camera()->position();
    _scene->setCameraPosition(vec4f(cameraPos[0],cameraPos[1],cameraPos[2],cameraPos[3]));

    _imgLines.drawScene(*_scene,!k_useSnakes && (k_drawRefImg != k_ref_list[4]));

    if(k_useSnakes){
        if(_prev_frame_number != _scene->currentFrameNumber()){
            _imgLines.nextGeomFlowBuffer();
        }
        _imgLines.readbackSamples(proj_xf,modelView_xf,_scene->isAnimated(),true);
        int _clipPathSize = _imgLines.clipPathSet()->size();

        if(_clipPathSize > 0){
            GQDraw::startScreenCoordinatesSystem(true,width(),height());
            GQDraw::clearGLScreen(vec(1,1,1),1.f);

            if(!_ac_initialized || (k_initSnakes && !k_initSnakes.changedLastFrame())){
                k_initSnakes.setValue(false);
                _imgLines.initGeomFlowBuffer();
                _snakes.clear();
                _snakes.init(*_imgLines.clipPathSet(),true);
                _snakesRenderer.init(&_snakes);
                _ac_initialized = true;
            }else{
                _refImg = _imgLines.offscreenTexture();
                if(_scene->isAnimated()){
                    _snakes.updateRefImage(_refImg,
                                           _imgLines.geometricFlowBuffer(),
                                           NULL,
                                           *_imgLines.clipPathSet(),
                                           _prev_frame_number != _scene->currentFrameNumber());
                    if(_prev_frame_number != _scene->currentFrameNumber())
                        _prev_frame_number = _scene->currentFrameNumber();
                }else{
                    //_scene->computeAdvection();
                    _snakes.updateRefImage(_refImg,
                                           _imgLines.geometricFlowBuffer(),
                                           NULL,//_scene->denseFlowBuffer()
                                           *_imgLines.clipPathSet(),
                                           true);
                }
            }

            if(k_drawRefImg == k_ref_list[1]) {
                GQDraw::visualizeTexture(_refImg);
            }else if(k_drawRefImg == k_ref_list[2]) {
                GQDraw::visualizeTexture(GQGPUImageProcessing::blurredImage());
            }else if(k_drawRefImg == k_ref_list[3]){
                GQDraw::visualizeTexture(GQGPUImageProcessing::gradientImage());
            }else if(k_drawRefImg == k_ref_list[4]){
                GQDraw::visualizeTexture(_imgLines.colorTexture());
            }else if(k_drawRefImg == k_ref_list[5]){
                GQDraw::visualizeTexture(_scene->denseFlowTexture());
            }

            if(k_snapshot){
                _snapshotBuffer.initFullScreen(1);
                glClearColor(1,1,1,0);
                _snapshotBuffer.bind(GQ_CLEAR_BUFFER);
            }
            if(k_drawShading){
                GQDraw::clearGLScreen(vec(1,1,1),1);
                _scene->drawScene(k_shading,(ModelType) k_model.index());
            }

            if(k_drawContour == k_draw_list[2]){
                _snakesRenderer.draw();
                if(k_drawClosest)
                    _snakesRenderer.drawClosestEdge();
            }else if(k_drawContour == k_draw_list[1]){
                _snakesRenderer.drawBrushPaths(false);
            }else if(k_drawContour == k_draw_list[3]){
                _imgLines.clipPathSet()->draw(true,width(),height());
            }else if(k_drawContour == k_draw_list[0]){
                GQDraw::stopScreenCoordinatesSystem();
                _snakesRenderer.renderStrokes();
            }

            if(k_snapshot){
                _snapshotBuffer.unbind();
                _snapshotBuffer.saveColorTextureToFile(0,QString(_snapshotPath + "/ActiveStrokes.%1.png").arg(idx,4,QLatin1Char('0')));
                GQDraw::visualizeTexture(_snapshotBuffer.colorTexture(0));
            }

            if(k_drawContour != k_draw_list[0])
                GQDraw::stopScreenCoordinatesSystem();
        }
    }else if(k_drawRefImg == k_ref_list[4]){
        GQDraw::clearGLScreen(vec(1,1,1),1);
        _scene->drawScene(k_shading,(ModelType) k_model.index());
    }

    if (_display_timers)
    {
        perf.updateView();
    }

    in_draw_function = false;
}
