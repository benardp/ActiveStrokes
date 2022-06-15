/*****************************************************************************\

MainWindow.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "GQInclude.h"

#include <QtGui>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QRegularExpression>
#include <QSignalMapper>

#include "GLViewer.h"
#include "MainWindow.h"
#include "Scene.h"
#include "GQShaderManager.h"

#include "Stats.h"
#include "DialsAndKnobs.h"
#include "Session.h"
#ifndef LINUX
#include "Console.h"
#endif

#include <XForm.h>
#include <assert.h>

const int CURRENT_INTERFACE_VERSION = 1;

const int MAX_RECENT_SCENES = 4;

MainWindow::MainWindow( )
{
    _console = NULL;
    _scene = NULL;
    _dials_and_knobs = NULL;
    _current_session = NULL;
    _gl_viewer = new GLViewer;
}

MainWindow::~MainWindow()
{
}

void MainWindow::init( const QDir& working_dir, const QString& scene_name )
{
    _working_dir = working_dir;

    setCentralWidget(_gl_viewer);
    setupUi();

    // read back qsettings and update window state and recent file list
    QSettings settings("qviewer", "qviewer");
    settings.beginGroup("mainwindow");
    QByteArray windowstate = settings.value("windowstate").toByteArray();
    bool restored = restoreState(windowstate, CURRENT_INTERFACE_VERSION);
    if (!restored)
    {
        hideAllDockWidgets();
    }

    settings.endGroup();
    settings.beginGroup("recent_scenes");
    for (int i = 0; i < MAX_RECENT_SCENES; i++) {
        QString scene_name = settings.value(QString("scene_%1").arg(i)).toString();

        if (!scene_name.isEmpty()) {
            _recent_scenes.append(scene_name);
        }
    }
    updateRecentScenesMenu();

    _last_scene_dir = working_dir.canonicalPath();
    _last_export_dir = working_dir.canonicalPath();
    _last_camera_dir = working_dir.canonicalPath();

    resize(512,512);

    if (!scene_name.isEmpty()) {
        openScene(scene_name);
    }

    makeWindowTitle();

    _gl_viewer->finishInit();
}

void MainWindow::closeEvent( QCloseEvent* event )
{
#ifndef LINUX
    if (_console)
        _console->removeMsgHandler();
#endif

    QSettings settings("qviewer", "qviewer");
    settings.beginGroup("mainwindow");
    settings.setValue("windowstate", saveState(CURRENT_INTERFACE_VERSION));
    settings.endGroup();

    addCurrentSceneToRecentList();

    settings.beginGroup("recent_scenes");
    for (int i = 0; i < _recent_scenes.size(); i++)
    {
        if (!_recent_scenes[i].isEmpty())
        {
            settings.setValue(QString("scene_%1").arg(i), _recent_scenes[i]);
        }
    }

    settings.sync();

    if(_current_session)
        delete _current_session;

    setCentralWidget(nullptr);
    delete _scene;
    delete _gl_viewer;

    event->accept();
}

bool MainWindow::openScene( const QString& filename )
{
    // Try to do all the file loading before we change
    // the mainwindow state at all.

    QString absfilename = QDir::fromNativeSeparators(_working_dir.absoluteFilePath(filename));
    QFileInfo fileinfo(absfilename);
    
    if (!fileinfo.exists())
    {
        QMessageBox::critical(this, "File Not Found",
                              QString("\"%1\" does not exist.").arg(absfilename));
        return false;
    }

    Scene* new_scene = new Scene();
    
    if (!new_scene->load(absfilename))
    {
        QMessageBox::critical(this, "Open Failed",
                              QString("Failed to load \"%1\". Check console.").arg(absfilename));
        delete new_scene;
        return false;
    }

    // Success: file has been loaded, now change state

    if (_scene)
        delete _scene;

    _scene = new_scene;
    _scene_name = QDir::fromNativeSeparators(filename);
    connect(_scene->animationTimer(), SIGNAL(timeout()),
            _gl_viewer, SLOT(update()));

    Stats::instance().clear();
    _scene->recordStats(Stats::instance());
    _scene->setTexToonFilename(_last_scene_dir + "/../samples/textures/toon/toonBW.png");

    _gl_viewer->setScene(_scene);
    _dials_and_knobs->load(_scene->dialsAndKnobsState());

    _current_session = _scene->session();
    if (_current_session)
        changeSessionState(SESSION_LOADED);
    else
        changeSessionState(SESSION_NONE);

    addCurrentSceneToRecentList();

    makeWindowTitle();
    
    return true;
}

bool MainWindow::saveScene( const QString& filename )
{
    _scene->setSession(_current_session);
    return _scene->save(filename, _gl_viewer, _dials_and_knobs);
}

void MainWindow::on_actionOpen_ToonTexture_triggered()
{
    if(_scene){
        QString filename = myFileDialog(QFileDialog::AcceptOpen, "Open Toon Texture",
                                        "Toon tex (*.png *.jpg *.jpeg .bmp)",
                                        _last_scene_dir);
        if (!filename.isNull()) {
            _scene->loadToonTex(filename, GL_TEXTURE_2D);
            _gl_viewer->update();
        }
    }
}

void MainWindow::on_actionOpen_Recent_Scene_triggered(int which)
{
    if (_recent_scenes.size() > which &&
            !_recent_scenes[which].isEmpty())
    {
        QFileInfo fileinfo(_recent_scenes[which]);
        QString fullpath = fileinfo.absoluteFilePath();

        openScene( _recent_scenes[which] );
        _gl_viewer->update();
    }
}

void MainWindow::addCurrentSceneToRecentList()
{
    if (!_scene_name.isEmpty() && !_recent_scenes.contains(_scene_name))
    {
        _recent_scenes.push_front(_scene_name);
        if (_recent_scenes.size() > MAX_RECENT_SCENES)
        {
            _recent_scenes.pop_back();
        }

        updateRecentScenesMenu();
    }
}

void MainWindow::updateRecentScenesMenu()
{
    for (int i=0; i < MAX_RECENT_SCENES; i++)
    {
        if (_recent_scenes.size() > i && !_recent_scenes[i].isEmpty())
        {
            if (_recent_scenes_actions.size() > i)
            {
                _recent_scenes_actions[i]->setText(_recent_scenes[i]);
            }
            else
            {
                QAction* new_action = new QAction(_recent_scenes[i], this);
                connect(new_action, SIGNAL(triggered(bool)),
                        &_recent_scenes_mapper, SLOT(map()));
                _recent_scenes_mapper.setMapping(new_action, i);
                new_action->setShortcut(Qt::CTRL + Qt::Key_1 + i);
                _recent_scenes_actions.append(new_action);
                _recent_scenes_menu->addAction(new_action);
            }
        }
    }
    connect(&_recent_scenes_mapper, SIGNAL(mappedInt(int)),
            this, SLOT(on_actionOpen_Recent_Scene_triggered(int)));
}

void MainWindow::setupUi()
{
    setupFileMenu();
    setupSessionMenu();
    QMenu* windowMenu = menuBar()->addMenu(tr("&Window"));
    setupViewerResizeActions(windowMenu);
    windowMenu->addSeparator();
    setupDockWidgets(windowMenu);
}

void MainWindow::setupFileMenu()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* openSceneAction = new QAction(tr("&Open Scene"), this);
    openSceneAction->setShortcut(QKeySequence(tr("Ctrl+O")));
    connect(openSceneAction, SIGNAL(triggered()),
            this, SLOT(on_actionOpen_Scene_triggered()));
    fileMenu->addAction(openSceneAction);

    _recent_scenes_menu = fileMenu->addMenu(tr("&Recent Scenes"));

    fileMenu->addSeparator();

    QAction* saveSceneAction = new QAction(tr("&Save Scene..."), this);
    connect(saveSceneAction, SIGNAL(triggered()),
            this, SLOT(on_actionSave_Scene_triggered()));
    fileMenu->addAction(saveSceneAction);

    QAction* save_screenshot = new QAction(tr("Save S&creenshot..."), this);
    save_screenshot->setShortcut(QKeySequence(tr("Ctrl+S")));
    connect(save_screenshot, SIGNAL(triggered()),
            this, SLOT(on_actionSave_Screenshot_triggered()));
    fileMenu->addAction(save_screenshot);

    fileMenu->addSeparator();

    QAction* reloadShadersAction = new QAction(tr("&Reload Shaders"), this);
    reloadShadersAction->setShortcut(QKeySequence(tr("Ctrl+L")));
    connect(reloadShadersAction, SIGNAL(triggered()),
            this, SLOT(on_actionReload_Shaders_triggered()));
    fileMenu->addAction(reloadShadersAction);

    QAction* openToonTexture = new QAction(tr("&Open Toon Tex."), this);
    openToonTexture->setShortcut(QKeySequence(tr("Ctrl+T")));
    connect(openToonTexture, SIGNAL(triggered()),
            this, SLOT(on_actionOpen_ToonTexture_triggered()));
    fileMenu->addAction(openToonTexture);

    fileMenu->addSeparator();

    QAction* quit_action = new QAction(tr("&Quit"), this);
    quit_action->setShortcut(QKeySequence(tr("Ctrl+Q")));
    connect(quit_action, SIGNAL(triggered()),
            this, SLOT(close()));
    fileMenu->addAction(quit_action);
}

void MainWindow::setupSessionMenu()
{
    QMenu* sessionMenu = menuBar()->addMenu(tr("&Session"));

    actionStart_Recording = new QAction(tr("&Start Recording"), this);
    connect(actionStart_Recording, SIGNAL(triggered()), this, SLOT(on_actionStart_Recording_triggered()));
    sessionMenu->addAction(actionStart_Recording);

    actionStop_Recording = new QAction(tr("&Stop Recording"), this);
    connect(actionStop_Recording, SIGNAL(triggered()), this, SLOT(on_actionStop_Recording_triggered()));
    sessionMenu->addAction(actionStop_Recording);

    actionReplay_Session = new QAction(tr("&Replay"), this);
    connect(actionReplay_Session, SIGNAL(triggered()), this, SLOT(on_actionReplay_Session_triggered()));
    sessionMenu->addAction(actionReplay_Session);

    actionReplay_as_fast_as_possible = new QAction(tr("&Replay AFAP"), this);
    connect(actionReplay_as_fast_as_possible, SIGNAL(triggered()), this, SLOT(on_actionReplay_as_fast_as_possible_triggered()));
    sessionMenu->addAction(actionReplay_as_fast_as_possible);

    actionReplay_and_Save_Screenshots = new QAction(tr("&Replay and Save Screenshots"), this);
    connect(actionReplay_and_Save_Screenshots, SIGNAL(triggered()), this, SLOT(on_actionReplay_and_Save_Screenshots_triggered()));
    sessionMenu->addAction(actionReplay_and_Save_Screenshots);

    actionStop_Replaying = new QAction(tr("&Stop Replaying"), this);
    connect(actionStop_Replaying, SIGNAL(triggered()), this, SLOT(on_actionStop_Replaying_triggered()));
    sessionMenu->addAction(actionStop_Replaying);
}

void MainWindow::setupDockWidgets(QMenu* menu)
{
    QStringList separate_windows = QStringList() << "Image Lines" << "Contours"<<"BrushPaths" << "Fitting" << "Style";
    _dials_and_knobs = new DialsAndKnobs(this, menu, separate_windows);
    connect(_dials_and_knobs, SIGNAL(dataChanged()), _gl_viewer, SLOT(update()));

#ifndef LINUX
    _console = new Console(this, menu);
    _console->installMsgHandler();
    Session::setConsole(_console);
#endif

    _stats_widget = new StatsWidget(this, menu);
}

void MainWindow::makeWindowTitle()
{
    QFileInfo fileinfo(_scene_name);
    QString title = QString("qviewer - %1").arg( fileinfo.fileName() );
    setWindowTitle( title );
}

void MainWindow::hideAllDockWidgets()
{
    _dials_and_knobs->hide();
#ifndef LINUX
    _console->hide();
#endif
}

void MainWindow::on_actionOpen_Scene_triggered()
{
    QString filename =
            myFileDialog(QFileDialog::AcceptOpen, "Open Scene",
                         "Scenes (*." + Scene::fileExtension() + " *.off *.obj *.ply *.3ds *.seq)",
                         _last_scene_dir );

    if (!filename.isNull())
    {
        openScene( filename );
    }
}

void MainWindow::on_actionSave_Scene_triggered()
{
    QString filename = myFileDialog(QFileDialog::AcceptSave, "Save Scene",
                                    "Scenes (*." + Scene::fileExtension() + ")", _last_scene_dir );
    if (!filename.isNull()) {
        saveScene( filename );
    }
}

void MainWindow::on_actionSave_Screenshot_triggered()
{
    _gl_viewer->saveSnapshot( false );
}

void MainWindow::setFoV(float degrees)
{
    _gl_viewer->camera()->setFieldOfView( degrees * ( 3.1415926f / 180.0f ) );
    _gl_viewer->update();
}


void MainWindow::resizeToFitViewerSize( int x, int y )
{
    QSize currentsize = size();
    QSize viewersize = _gl_viewer->size();
    QSize newsize = currentsize - viewersize + QSize(x,y);
    resize( newsize );
}

void MainWindow::resizeToFitViewerSize(const QString& size)
{
    QStringList dims = size.split("x");
    resizeToFitViewerSize(dims[0].toInt(), dims[1].toInt());
}

void MainWindow::on_actionReload_Shaders_triggered()
{
    GQShaderManager::reload();
    _gl_viewer->update();
}

void MainWindow::setupViewerResizeActions(QMenu* menu)
{
    QStringList sizes = QStringList() << "512x512" <<
                                         "640x480" << "800x600" << "1024x768" << "1024x1024" << "1920x1080";

    QMenu* resize_menu = menu->addMenu("&Resize Viewer");

    for (int i = 0; i < sizes.size(); i++)
    {
        QAction* size_action = new QAction(sizes[i], this);
        _viewer_size_mapper.setMapping(size_action, sizes[i]);
        connect(size_action, SIGNAL(triggered()),
                &_viewer_size_mapper, SLOT(map()));
        resize_menu->addAction(size_action);
    }

    connect(&_viewer_size_mapper, SIGNAL(mappedString(const QString&)),
            this, SLOT(resizeToFitViewerSize(const QString&)));

    menu->addMenu(resize_menu);
}

QString MainWindow::myFileDialog( int mode, const QString& caption, 
                                  const QString& filter, QString& last_dir)
{
    QFileDialog dialog(this, caption, last_dir, filter);
    dialog.setAcceptMode((QFileDialog::AcceptMode)mode);

    QString filename;
    int ret = dialog.exec();
    if (ret == QDialog::Accepted)
    {
        last_dir = dialog.directory().path();
        if (dialog.selectedFiles().size() > 0)
        {
            filename = dialog.selectedFiles()[0];
            if (mode == QFileDialog::AcceptSave)
            {
                QStringList acceptable_extensions;
                int last_pos = filter.indexOf("*.", 0);
                while (last_pos > 0)
                {
                    int ext_end = filter.indexOf(QRegularExpression("[ ;)]"), last_pos);
                    acceptable_extensions << filter.mid(last_pos+1,
                                                        ext_end-last_pos-1);
                    last_pos = filter.indexOf("*.", last_pos+1);
                }
                if (acceptable_extensions.size() > 0)
                {
                    bool ext_ok = false;
                    for (int i = 0; i < acceptable_extensions.size(); i++)
                    {
                        if (filename.endsWith(acceptable_extensions[i]))
                        {
                            ext_ok = true;
                        }
                    }
                    if (!ext_ok)
                    {
                        filename = filename + acceptable_extensions[0];
                    }
                }
            }
        }
    }
    return filename;
}

void MainWindow::changeSessionState(int newstate)
{
    if (newstate == SESSION_NONE)
    {
        actionStart_Recording->setEnabled(true);
        actionStop_Recording->setEnabled(false);
        actionReplay_Session->setEnabled(false);
        actionReplay_as_fast_as_possible->setEnabled(false);
        actionReplay_and_Save_Screenshots->setEnabled(false);
        actionStop_Replaying->setEnabled(false);
    }
    else if (newstate == SESSION_LOADED)
    {
        actionStart_Recording->setEnabled(true);
        actionStop_Recording->setEnabled(false);
        actionReplay_Session->setEnabled(true);
        actionReplay_as_fast_as_possible->setEnabled(true);
        actionReplay_and_Save_Screenshots->setEnabled(true);
        actionStop_Replaying->setEnabled(false);

        disconnect( _current_session, SIGNAL(playbackFinished()), this, SLOT(onmy_replayFinished()) );
    }
    else if (newstate == SESSION_PLAYING)
    {
        actionStart_Recording->setEnabled(false);
        actionStop_Recording->setEnabled(false);
        actionReplay_Session->setEnabled(false);
        actionReplay_as_fast_as_possible->setEnabled(false);
        actionReplay_and_Save_Screenshots->setEnabled(false);
        actionStop_Replaying->setEnabled(true);

        connect( _current_session, SIGNAL(playbackFinished()), this, SLOT(onmy_replayFinished()) );
    }
    else if (newstate == SESSION_RECORDING)
    {
        actionStart_Recording->setEnabled(false);
        actionStop_Recording->setEnabled(true);
        actionReplay_Session->setEnabled(false);
        actionReplay_as_fast_as_possible->setEnabled(false);
        actionReplay_and_Save_Screenshots->setEnabled(false);
        actionStop_Replaying->setEnabled(false);
    }

    _session_state = newstate;
}

void MainWindow::onmy_replayFinished()
{
    changeSessionState( SESSION_LOADED );
}

void MainWindow::on_actionStart_Recording_triggered()
{
    if (_current_session)
        delete _current_session;

    _current_session = new Session();
    _current_session->startRecording( _gl_viewer, this, _dials_and_knobs );
    changeSessionState( SESSION_RECORDING );
}

void MainWindow::on_actionStop_Recording_triggered()
{
    changeSessionState( SESSION_LOADED );
    _current_session->stopRecording();
}

void MainWindow::on_actionReplay_Session_triggered()
{
    changeSessionState( SESSION_PLAYING );
    _current_session->startPlayback( _gl_viewer, this, _dials_and_knobs );
}

void MainWindow::on_actionReplay_as_fast_as_possible_triggered()
{
    changeSessionState( SESSION_PLAYING );
    _current_session->startPlayback( _gl_viewer, this, _dials_and_knobs,
                                     Session::PLAYBACK_AS_FAST_AS_POSSIBLE );
}

void MainWindow::on_actionReplay_and_Save_Screenshots_triggered()
{
    QString filename = myFileDialog(QFileDialog::AcceptSave, "Save Screenshots", "Image (*.png *.jpg)",_last_screenshot_dir);

    if (!filename.isNull())
    {
        changeSessionState( SESSION_PLAYING );
        _current_session->startPlayback( _gl_viewer, this, _dials_and_knobs,
                                         Session::PLAYBACK_SCREENSHOTS, filename );//_INTERPOLATED
    }
}

void MainWindow::on_actionStop_Replaying_triggered()
{
    _current_session->stopPlayback();
    changeSessionState( SESSION_LOADED );
}

void MainWindow::on_actionToggle_Capture_triggered()
{
    _capture_started = !_capture_started;
    if(_capture_started)
    {
        QString filename = myFileDialog(QFileDialog::AcceptSave, "Save Screenshots", "Image (*.png)",_last_screenshot_dir);

        if (!filename.isNull())
        {
            if(!filename.endsWith(".png",Qt::CaseInsensitive))
                filename.append(".png");
            _gl_viewer->setSnapshotFileName(filename);
            connect( _gl_viewer, SIGNAL( drawFinished(bool)), _gl_viewer, SLOT(saveSnapshot(bool)) );
        }
    }
    else
        _gl_viewer->disconnect(_gl_viewer,SLOT(saveSnapshot(bool)));
}
