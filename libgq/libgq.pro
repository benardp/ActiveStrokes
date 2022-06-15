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

equals (QT_MAJOR_VERSION, 6) {
	QT += gui widgets
}

TARGET = gq

DEPENDPATH += include
INCLUDEPATH += include

#Input
HEADERS += include/GQ*.h
SOURCES += libsrc/GQ*.cc

# Trimesh2
INCLUDEPATH += ../trimesh2/include
