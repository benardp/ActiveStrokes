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
        DEFINES += HAVE_VA_COPY

        macx {
                DEFINES += DARWIN
        }
        else {
            QMAKE_CXXFLAGS += -fopenmp
            DEFINES += LINUX
        }
}

CONFIG += staticlib
QT += opengl xml

TARGET = npr

DEPENDPATH += include
INCLUDEPATH += include
DEPENDPATH += ../libgq/include
INCLUDEPATH += ../libgq/include
DEPENDPATH += ../demoutils/include
INCLUDEPATH += ../demoutils/include
DEPENDPATH += ../trimesh2/include
INCLUDEPATH += ../trimesh2/include

#Input
HEADERS += include/*.h
SOURCES += libsrc/*.cc

OTHER_FILES += \
    shaders/programs.xml \
    shaders/*.vert \
    shaders/*.frag \
    shaders/*.glsl \
    shaders/*.geom
