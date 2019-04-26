CONFIG += debug_and_release

CONFIG(release, debug|release) {
	DBGNAME = release
}
else {
	DBGNAME = debug
}
DESTDIR = $${DBGNAME}

win32 {
    TEMPLATE = vcapp
    UNAME = Win32
}
else {
    TEMPLATE = app
    TRIMESH = trimesh
    macx {
        DEFINES += DARWIN
        UNAME = Darwin
        CONFIG -= app_bundle
        LIBS += -framework CoreFoundation -framework OpenGL
    }
    else {
        QMAKE_CXXFLAGS += -fopenmp
        QMAKE_LFLAGS += -fopenmp
        DEFINES += LINUX
        UNAME = Linux
        LIBS += -lGLU
    }
}

TRIMESH = trimesh

QT += opengl xml script openglextensions
TARGET = qviewer

PRE_TARGETDEPS += ../libgq/$${DBGNAME}/libgq.a
DEPENDPATH += ../libgq/include
INCLUDEPATH += ../libgq/include
LIBS += -L../libgq/$${DBGNAME} -lgq

PRE_TARGETDEPS += ../demoutils/$${DBGNAME}/libdemoutils.a
DEPENDPATH += ../demoutils/include
INCLUDEPATH += ../demoutils/include
LIBS += -L../demoutils/$${DBGNAME} -ldemoutils

PRE_TARGETDEPS += ../trimesh2/$${DBGNAME}/libtrimesh.a
DEPENDPATH += ../trimesh2/include
INCLUDEPATH += ../trimesh2/include
LIBS += -L../trimesh2/$${DBGNAME} -l$${TRIMESH}

PRE_TARGETDEPS += ../qglviewer/$${DBGNAME}/libqglviewer.a
DEPENDPATH += ../qglviewer
INCLUDEPATH += ../qglviewer 
LIBS += -L../qglviewer/$${DBGNAME} -lqglviewer
DEFINES += QGLVIEWER_STATIC

PRE_TARGETDEPS += ../libas/$${DBGNAME}/libas.a
DEPENDPATH += ../libas/include
INCLUDEPATH += ../libas/include
LIBS += -L../libas/$${DBGNAME} -las

PRE_TARGETDEPS += ../libnpr/$${DBGNAME}/libnpr.a
DEPENDPATH += ../libnpr/include
INCLUDEPATH += ../libnpr/include
LIBS += -L../libnpr/$${DBGNAME} -lnpr

INCLUDEPATH += ../eigen3

# Input
HEADERS += src/*.h
SOURCES += src/*.cc
FORMS   += src/*.ui
RESOURCES = src/icons.qrc
