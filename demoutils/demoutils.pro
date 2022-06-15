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
	macx {
		DEFINES += DARWIN
	}
	else {
                QMAKE_CXXFLAGS += -fopenmp
		DEFINES += LINUX
	}
}

CONFIG += staticlib
QT += opengl xml qml

equals (QT_MAJOR_VERSION, 6) {
	QT += gui widgets
}

TARGET = demoutils

DEPENDPATH += include
INCLUDEPATH += include

#Input
HEADERS += include/*.h
SOURCES += libsrc/*.cc

# Trimesh2
INCLUDEPATH += ../trimesh2/include
