CONFIG += debug_and_release

CONFIG(release, debug|release) {
        DBGNAME = release
}
else {
        DBGNAME = debug
}
DESTDIR = $${DBGNAME}

win32 {
        TEMPLATE = vclib
}
else {
	TEMPLATE = lib
    QMAKE_CXXFLAGS += -fopenmp
    macx {
        DEFINES += DARWIN
    }
    else {
        DEFINES += LINUX
    }
}

CONFIG += staticlib
QT += opengl xml

TARGET = as

DEPENDPATH += ../trimesh2/include
INCLUDEPATH += ../trimesh2/include

DEPENDPATH += ../qglviewer
INCLUDEPATH += ../qglviewer

DEPENDPATH += ../libgq/include
INCLUDEPATH += ../libgq/include

DEPENDPATH += ../libnpr/include
INCLUDEPATH += ../libnpr/include

DEPENDPATH += ../libas/include
INCLUDEPATH += ../libas/include

DEPENDPATH += ../demoutils/include
INCLUDEPATH += ../demoutils/include

INCLUDEPATH += ../eigen3

#Input
HEADERS += include/*.h
SOURCES += libsrc/*.cc
