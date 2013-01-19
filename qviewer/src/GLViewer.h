/*****************************************************************************\

GLViewer.h
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu
Copyright (c) 2009 Forrester Cole

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef GLVIEWER_H_
#define GLVIEWER_H_

#include "GQTexture.h"
#include "GQInclude.h"

#include "DialsAndKnobs.h"
#include "ImageSpaceLines.h"

#include "ASRenderer.h"

#include <QGLViewer/qglviewer.h>

class Scene;

class GLViewer : public QGLViewer
{
    Q_OBJECT

public:
    GLViewer( QWidget* parent = 0 );

    void setScene( Scene* npr_scene );
    void finishInit();
    void resetView();

    void setDisplayTimers(bool display) { _display_timers = display; }

protected:
    virtual void draw();
    virtual void resizeGL( int width, int height );

private:
    bool _inited;
    bool _visible;
    bool _display_timers;
    bool _new_scene;
    bool _ac_initialized;

    int _prev_frame_number;

    Scene* _scene;
    ImageSpaceLines _imgLines;
    GQTexture2D* _refImg;
    ASSnakes _snakes;
    ASRenderer _snakesRenderer;

    QString _snapshotPath;
    GQFramebufferObject _snapshotBuffer;
};

#endif /*GLVIEWER_H_*/
