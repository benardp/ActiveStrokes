DEFINES += NO_VECTORIAL_RENDER
DEFINES += QGLVIEWER_STATIC

CONFIG *= qt opengl warn_on shared thread create_prl rtti no_keywords

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
		DEFINES += LINUX
                QMAKE_CXXFLAGS += -fopenmp
	}
}

CONFIG += staticlib

equals (QT_MAJOR_VERSION, 5) {
	QT *= gui widgets
}
equals (QT_MAJOR_VERSION, 6) {
	QT *= gui widgets openglwidgets
}

TRANSLATIONS = qglviewer_fr.ts

QT += opengl xml

TARGET = qglviewer

DEPENDPATH += .
INCLUDEPATH += .

#Input
HEADERS += *.h
SOURCES += *.cpp
FORMS *= ImageInterface.ui
