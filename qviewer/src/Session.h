/*****************************************************************************\

Scene.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Class for recording and playing back an editing session.

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef SESSION_H_
#define SESSION_H_

#include "GQInclude.h"
#include "timestamp.h"
#include "DialsAndKnobs.h"
#include "ui_Player.h"
#include <vector>

#include <QDomElement>
#include <QString>
#include <QDialog>

using std::vector;

class QGLViewer;
class Console;
class PlayerDialog;
class MainWindow;

class SessionFrame
{
public:
    ~SessionFrame();
    void load( const QDomElement& element );
    void save( QDomDocument& doc, QDomElement& element );

public:
    float	_time;
    xform	_camera_mat;
    bool	_anim_playing;
    DialsAndKnobsValues _changed_values;
};

class Session : public QObject
{
    Q_OBJECT

public:
    typedef enum {
        PLAYBACK_NORMAL,
        PLAYBACK_AS_FAST_AS_POSSIBLE,
        PLAYBACK_SCREENSHOTS,
        NUM_PLAYBACK_MODES
    } PlaybackMode;

    typedef enum {
        STATE_NO_DATA,
        STATE_LOADED,
        STATE_PLAYING,
        STATE_RECORDING,
        STATE_PAUSED,
        NUM_STATES
    } State;

public:
    Session();
    ~Session();

    bool load( const QString& filename );
    bool load( const QDomElement& root );
    bool save( const QString& filename );
    bool save( QDomDocument& doc, QDomElement& root );

    void startRecording( QGLViewer* viewer, MainWindow *ui, DialsAndKnobs* dk );
    void stopRecording();

    void startPlayback( QGLViewer* viewer, MainWindow *ui, DialsAndKnobs* dk,
                        PlaybackMode mode = PLAYBACK_NORMAL,
                        const QString& filename = QString() );
    void stopPlayback();

    void restartPlayback();
    void pausePlayback();

    State getState() const { return _state; }

    float length() const;
    int	numFrames() const { return _frames.size(); }
    const SessionFrame& frame( int which ) const;

    void recordFrame( const SessionFrame& frame );

    static void execSettingsDialog();
    static void setConsole( Console* console ) { _console = console; }

    void setIncr(int i) { _incr = i; }
    void resetCurrentFrame() { _current_frame = -1; }

public slots:
    void gatherAndRecordFrame();
    void advancePlaybackFrame();
    void dumpScreenshot();

signals:
    void redrawNeeded();
    void recordingStarted();
    void recordingStopped();
    void playbackStarted();
    void playbackFinished();
    void playbackStopped();

protected:
    void cleanUpPlayback();

protected:
    State                _state;
    PlaybackMode         _playback_mode;
    QGLViewer*           _viewer;
    MainWindow*          _ui_mainwindow;
    PlayerDialog*        _playerDialog;
    DialsAndKnobs*       _dials_and_knobs;

    vector<SessionFrame> _frames;
    int                  _current_frame;
    int			 _incr;
    bool                 _has_dumped_current_frame;

    trimesh::timestamp            _session_timer;

    QString              _final_filename;
    QString              _screenshot_file_pattern;
    vector<QString>      _screenshot_filenames;

    static Console*      _console;
};

class PlayerDialog : public QDialog
{
    Q_OBJECT

public:
    PlayerDialog(Session* s);

    void setNumFrames(int f);
    void setValue(int f);
    void reset();

public slots:
    void on_backwardButton_clicked();
    void on_forwardButton_clicked();
    void on_playButton_clicked();
    void on_stopButton_clicked();

protected:
    Ui::PlayerDialog _ui;
    Session* _session;
};

#endif // SESSION_H_
