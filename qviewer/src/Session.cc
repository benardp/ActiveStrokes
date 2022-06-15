/*****************************************************************************\

Session.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "Session.h"
#include "Console.h"

#include <QScriptEngine>
#include <GQInclude.h>
#include <QFile>
#include <QDomDocument>
#include <QDomText>
#include <QString>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QFileDialog>
#include <assert.h>
#include <timestamp.h>
#include <QDebug>
#include <QProgressDialog>

#include "Stats.h"
#include "GLViewer.h"

using namespace trimesh;

Console* Session::_console = 0;

SessionFrame::~SessionFrame()
{
}

void SessionFrame::load( const QDomElement& element )
{
    QDomElement t = element.firstChildElement("time");
    assert(!t.isNull());
    _time = t.text().toFloat();

    QDomElement cam = element.firstChildElement("camera_mat");
    assert(!cam.isNull());

    QString es = cam.text();
    QTextStream stream(&es);

    stream	>> _camera_mat[0] >> _camera_mat[4] >> _camera_mat[8] >> _camera_mat[12]
            >> _camera_mat[1] >> _camera_mat[5] >> _camera_mat[9] >> _camera_mat[13]
            >> _camera_mat[2] >> _camera_mat[6] >> _camera_mat[10] >> _camera_mat[14]
            >> _camera_mat[3] >> _camera_mat[7] >> _camera_mat[11] >> _camera_mat[15];

    QDomElement dk_values = element.firstChildElement("dials_and_knobs_values");
    if (!dk_values.isNull())
        _changed_values.setFromDomElement(dk_values);
}

void SessionFrame::save( QDomDocument& doc, QDomElement& element )
{
    QDomElement t = doc.createElement("time");
    QDomText ttext = doc.createTextNode(QString("%1").arg(_time) );

    element.appendChild(t);
    t.appendChild(ttext);

    QDomElement cam = doc.createElement("camera_mat");
    QString cam_string;
    QTextStream stream(&cam_string);

    stream	<< "\n" << _camera_mat[0] << " " << _camera_mat[4] << " " << _camera_mat[8] << " " << _camera_mat[12]
            << "\n" << _camera_mat[1] << " " << _camera_mat[5] << " " << _camera_mat[9] << " " << _camera_mat[13]
            << "\n" << _camera_mat[2] << " " << _camera_mat[6] << " " << _camera_mat[10] << " " << _camera_mat[14]
            << "\n" << _camera_mat[3] << " " << _camera_mat[7] << " " << _camera_mat[11] << " " << _camera_mat[15]
            << "\n";

    QDomText camtext = doc.createTextNode(cam_string);
    element.appendChild(cam);
    cam.appendChild(camtext);

    QDomElement dk_values = _changed_values.domElement("dials_and_knobs_values", doc);
    element.appendChild(dk_values);
}

Session::Session()
{
    _viewer = 0;
    _state = STATE_NO_DATA;
    _playback_mode = PLAYBACK_NORMAL;
    _playerDialog = new PlayerDialog(this);
    _incr = 1;
}

Session::~Session()
{
    _playerDialog->hide();
    delete _playerDialog;
}

void Session::recordFrame( const SessionFrame& frame )
{
    int i = _frames.size();
    _frames.push_back(frame);
    if (i > 0)
    {
        _frames[i]._time = now() - _session_timer;
    }
    else
    {
        _frames[i]._time = 0;
    }

    QString msg;
    QTextStream(&msg) << "Recording frame " << i << " at time " << _frames[i]._time << "\n";
    _console->print( msg );
}

const SessionFrame& Session::frame( int which ) const
{
    assert( which >= 0 && which < (int)(_frames.size()) );
    return _frames[which];
}

float Session::length() const
{
    if (_frames.size() == 0)
        return 0;
    else
        return _frames[_frames.size()-1]._time;
}

const int CURRENT_VERSION = 1;

bool Session::load( const QString& filename )
{
    QDomDocument doc("session");
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning("Could not open %s", qPrintable(filename));
        return false;
    }

    QString parse_errors;
    if (!doc.setContent(&file, &parse_errors))
    {
        qWarning("Parse errors: %s", qPrintable(parse_errors));
        return false;
    }

    file.close();

    QDomElement root = doc.documentElement();

    return load(root);
}

bool Session::load( const QDomElement& root )
{
    assert(_state == STATE_NO_DATA || _state == STATE_LOADED );

    QDomElement header = root.firstChildElement("header");
    assert(!header.isNull());
    QDomElement version = header.firstChildElement("version");
    assert(!version.isNull());

    int version_num = version.text().toInt();
    if (version_num != CURRENT_VERSION)
    {
        qWarning("Obsolete file version %d (current is %d)", version_num, CURRENT_VERSION );
        return false;
    }

    _frames.clear();
    QDomElement frames = root.firstChildElement("frames");
    QDomElement frame = frames.firstChildElement();
    while (!frame.isNull())
    {
        SessionFrame sf;
        sf.load( frame );
        _frames.push_back( sf );
        frame = frame.nextSiblingElement();
    }

    _state = STATE_LOADED;

    return true;
}

bool Session::save( const QString& filename )
{
    QDomDocument doc("session");
    QDomElement root = doc.createElement("session");
    doc.appendChild(root);

    bool ret = save(doc, root);
    if (!ret)
        return false;

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning("Could not open %s", qPrintable(filename));
        return false;
    }

    file.write(doc.toByteArray());

    file.close();

    return true;
}

bool Session::save( QDomDocument& doc, QDomElement& root )
{
    assert( _state == STATE_LOADED );

    QDomElement header = doc.createElement("header");
    root.appendChild(header);

    QDomElement version = doc.createElement("version");
    QDomText versiontext = doc.createTextNode( QString("%1").arg(CURRENT_VERSION) );
    header.appendChild(version);
    version.appendChild(versiontext);

    QDomElement frames = doc.createElement("frames");
    root.appendChild(frames);
    for (int i = 0; i < (int)(_frames.size()); i++)
    {
        QDomElement frame = doc.createElement("frame");
        _frames[i].save(doc, frame);
        frames.appendChild(frame);
    }

    return true;
}

void Session::startRecording( QGLViewer* viewer, MainWindow *ui, DialsAndKnobs* dk )
{
    assert(_state == STATE_NO_DATA || _state == STATE_LOADED );

    _frames.clear();
    _viewer = viewer;
    _ui_mainwindow = ui;
    _dials_and_knobs = dk;
    _state = STATE_RECORDING;
    _session_timer = now();

    connect( _viewer, SIGNAL( drawFinished(bool)), this, SLOT(gatherAndRecordFrame()) );
    emit recordingStarted();
}

void Session::stopRecording()
{
    assert( _state == STATE_RECORDING );

    disconnect( _viewer, SIGNAL( drawFinished(bool)), this, SLOT(gatherAndRecordFrame()) );
    _state = STATE_LOADED;
    _viewer = 0;

    emit recordingStopped();
}

void Session::startPlayback( QGLViewer* viewer, MainWindow *ui, DialsAndKnobs* dk,
                             PlaybackMode mode, const QString& filename )
{
    assert( _state == STATE_LOADED );

    _playerDialog->show();

    _viewer = viewer;
    _ui_mainwindow = ui;
    _dials_and_knobs = dk;
    _state = STATE_PLAYING;
    _playback_mode = mode;
    _current_frame = -1;
    _has_dumped_current_frame = false;
    _screenshot_file_pattern = "";
    _screenshot_filenames.clear();

    connect( this, SIGNAL( redrawNeeded() ), _viewer, SLOT( update() ) );
    if (_playback_mode == PLAYBACK_SCREENSHOTS)
    {
        int extindex = filename.lastIndexOf('.');
        QTextStream(&_screenshot_file_pattern) 
                << filename.left(extindex) << "%04d" << filename.right(filename.length() - extindex);
        connect( _viewer, SIGNAL( drawFinished(bool)), this, SLOT(dumpScreenshot()) );
    }

    QString msg;
    QTextStream(&msg) << "Replaying session (" << length() << " s. / " 
            << numFrames() << " frames)...\n";
    _playerDialog->setNumFrames(numFrames());


    _console->print( msg );

    emit playbackStarted();

    _session_timer = now();
    
    advancePlaybackFrame();
}

void Session::cleanUpPlayback()
{
    assert( _state == STATE_PLAYING || _state == STATE_PAUSED);

    disconnect( this, SIGNAL( redrawNeeded() ), _viewer, SLOT( update() ) );
    disconnect( _viewer, SIGNAL( drawFinished(bool)), this, SLOT(dumpScreenshot()) );

    _state = STATE_LOADED;
}

void Session::stopPlayback()
{
    cleanUpPlayback(); 

    _console->print("Replay stopped by user.\n");

    _playerDialog->hide();

    _playerDialog->hide();

    emit playbackStopped();
}

void Session::restartPlayback()
{
    //assert( _state == STATE_PAUSED || _state == STATE_PLAYING);

    connect( this, SIGNAL( redrawNeeded() ), _viewer, SLOT( update() ) );
    if (_playback_mode == PLAYBACK_SCREENSHOTS)
    {
        connect( _viewer, SIGNAL( drawFinished(bool)), this, SLOT(dumpScreenshot()) );
    }

    _state = STATE_PLAYING;

    advancePlaybackFrame();
}

void Session::pausePlayback()
{
    if( _state != STATE_PAUSED && _state != STATE_PLAYING)
        return;

    disconnect( this, SIGNAL( redrawNeeded() ), _viewer, SLOT( update() ) );
    disconnect( _viewer, SIGNAL( drawFinished(bool)), this, SLOT(dumpScreenshot()) );

    _state = STATE_PAUSED;
}


void Session::gatherAndRecordFrame()
{
    SessionFrame frame;
    GLdouble mv[16];
    _viewer->camera()->getModelViewMatrix(mv);
    frame._camera_mat = xform( mv );
    if (_frames.size() == 0) {
        frame._changed_values = _dials_and_knobs->values();
    } else {
        frame._changed_values = _dials_and_knobs->changedValues();
    }

    recordFrame( frame );
}

void Session::advancePlaybackFrame()
{
    if (_state != STATE_PLAYING && _state != STATE_PAUSED)
        return;

    _current_frame += _incr;
    if(_current_frame<0)
    	_current_frame=0;
    else if(_current_frame >= numFrames())
    	_current_frame = numFrames()-1;

    int next_frame = _current_frame + 1;
    if (next_frame >= numFrames())
        next_frame = numFrames()-1;

    _playerDialog->setValue(_current_frame+1);
    _has_dumped_current_frame = false;

    const SessionFrame& current_frame = frame(_current_frame);
    _viewer->camera()->setFromModelViewMatrix( current_frame._camera_mat );
    _viewer->camera()->loadModelViewMatrix();
    _dials_and_knobs->applyValues(current_frame._changed_values);

    emit redrawNeeded();

    float elapsed_time = now() - _session_timer;

    if(_state != STATE_PAUSED){
    	if (_current_frame < numFrames() - 1)
    	{
            float time_until_next = frame(_current_frame+1)._time - elapsed_time;
            if (time_until_next < 0)
                time_until_next = 0;

            if (_playback_mode == PLAYBACK_NORMAL)
            {
                // realtime playback
                QTimer::singleShot( 1000 * time_until_next, this, SLOT(advancePlaybackFrame()));
            }
            else
            {
                // playback as fast as possible
                QTimer::singleShot( 0, this, SLOT(advancePlaybackFrame()));
            }
    	}
    	else
    	{
            cleanUpPlayback();

            _console->print(QString("Replay finished. fps: %1\n").arg((float)numFrames() / elapsed_time));

            _playerDialog->reset();

            emit playbackFinished();
    	}
    }else{
        _viewer->update();
    }
}

void Session::dumpScreenshot()
{
    if (!_has_dumped_current_frame)
    {
        QString filename;
        filename.asprintf(qPrintable(_screenshot_file_pattern), _current_frame);
        _screenshot_filenames.push_back(filename);
        
        _viewer->saveSnapshot( filename, true );

        _console->print( QString("Saved %1\n").arg(filename) );

        _has_dumped_current_frame = true;
    }
}



PlayerDialog::PlayerDialog(Session* s) : _session(s)
{
    _ui.setupUi(this);
}

void PlayerDialog::setNumFrames(int f)
{
    _ui.frameProgressBar->setMaximum(f);
}

void PlayerDialog::setValue(int f)
{
    _ui.frameProgressBar->setValue(f);
}

void PlayerDialog::on_backwardButton_clicked()
{
    _session->setIncr(-_ui.incrSpinBox->value());
    _session->advancePlaybackFrame();
}

void PlayerDialog::on_forwardButton_clicked()
{
    _session->setIncr(_ui.incrSpinBox->value());
    _session->advancePlaybackFrame();
}

void PlayerDialog::on_playButton_clicked()
{
    QIcon icon;
    if(_session->getState() == Session::STATE_PAUSED){
        _session->setIncr(1);
        _ui.incrSpinBox->setValue(1);
        _session->restartPlayback();

        icon.addFile(QString::fromUtf8(":/icons/icons/pause.xpm"), QSize(), QIcon::Normal, QIcon::Off);
        _ui.playButton->setIcon(icon);
    }else if(_session->getState() == Session::STATE_PLAYING){
        _session->pausePlayback();
        icon.addFile(QString::fromUtf8(":/icons/icons/play.xpm"), QSize(), QIcon::Normal, QIcon::Off);
        _ui.playButton->setIcon(icon);
    }else if(_session->getState() == Session::STATE_LOADED){
        _session->resetCurrentFrame();
        _session->restartPlayback();
        icon.addFile(QString::fromUtf8(":/icons/icons/pause.xpm"), QSize(), QIcon::Normal, QIcon::Off);
        _ui.playButton->setIcon(icon);
    }
}

void PlayerDialog::on_stopButton_clicked()
{
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/icons/icons/play.xpm"), QSize(), QIcon::Normal, QIcon::Off);
    _ui.playButton->setIcon(icon);

    _session->pausePlayback();
    _session->resetCurrentFrame();
    _ui.incrSpinBox->setValue(1);
    _session->setIncr(1);
    _session->advancePlaybackFrame();
}

void PlayerDialog::reset() {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/icons/icons/play.xpm"), QSize(), QIcon::Normal, QIcon::Off);
    _ui.playButton->setIcon(icon);
    _ui.incrSpinBox->setValue(1);
    _session->setIncr(1);
    _ui.frameProgressBar->setValue(0);
}
