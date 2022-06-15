/*****************************************************************************\

GQInclude.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Common include headers for libgq and dependent libraries. The main purpose
is to make sure the OpenGL headers get included properly.

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _GQ_INCLUDE_H_
#define _GQ_INCLUDE_H_

#include <QOpenGLExtraFunctions>

#ifdef DARWIN
#    include <OpenGl/glu.h>
#endif
#ifdef LINUX
#    include <QVariant>
#    include <QTextStream>
#    include <QDir>
#    include <QEvent>
#    include <QComboBox>
#    include <QMessageBox>
#    include <GL/glu.h>
#    include <QtGlobal>
#    include <QDir>
#    include <QVariant>
#    include <QTextStream>
#    include <QMessageBox>
#    include <QEvent>
#    include <QComboBox>
#endif
#ifdef WIN32
#    define NOMINMAX
#	 include <windows.h>
#    include <GL/glu.h>
#endif

#include <QtGlobal>

#include <Vec.h>
#include <XForm.h>
#include <Color.h>

typedef int             int32;
typedef char            int8;
typedef unsigned int    uint32; // This could break on 64 bit, I guess.
typedef unsigned char   uint8;
typedef unsigned int    uint;
typedef trimesh::Vec<4,int>	     vec4i;
typedef trimesh::Vec<3,int>	     vec3i;
typedef trimesh::Vec<2,int>	     vec2i;
typedef trimesh::Vec<4,float>    vec4f;
typedef trimesh::Vec<3,float>    vec3f;
typedef trimesh::Vec<2,float>    vec2f;
typedef trimesh::Vec<3,float>    vec;
typedef trimesh::Vec<2,float>    vec2;
typedef trimesh::Vec<3,float>    vec3;
typedef trimesh::Vec<4,float>    vec4;
typedef trimesh::Color Color;

#ifndef likely
#  if !defined(__GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
#    define likely(x) (x)
#    define unlikely(x) (x)
#  else
#    define likely(x)   (__builtin_expect((x), 1))
#    define unlikely(x) (__builtin_expect((x), 0))
#  endif
#endif

typedef trimesh::XForm<double>  xform;
typedef trimesh::XForm<double> dxform;
typedef trimesh::XForm<float>  fxform;


inline void reportGLError()
{
    GLint error = glGetError();
    if (error != 0)
    {
        qCritical("GL Error: %s\n", gluErrorString(error));
    }
}

#ifdef WIN32
#define _USE_MATH_DEFINES
#include <cmath>
#include <float.h>
#define isnan _isnan
#define isinf(x) (!_finite(x))
#else
#include <cmath>
using std::isnan;
#endif

#endif // _GQ_INCLUDE_H_
