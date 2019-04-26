/*****************************************************************************\

GQFramebufferObject.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "GQFramebufferObject.h"

#include <QtGlobal>
#include <QFile>
#include <assert.h>

int GQFramebufferObject::_bound_guid = 0;
int GQFramebufferObject::_last_used_guid = 1;

GQFramebufferObject::GQFramebufferObject()
{
    _fbo = -1;
    _gl_target = 0;
    _gl_format = 0;
    _format = -1;
    _coordinates = -1;
    _attachments = GQ_ATTACH_NONE;
    _num_color_attachments = 0;
    _color_attachments = NULL;
    _width = -1;
    _height = -1;
    _guid = _last_used_guid++;
    if (_guid == 0) _guid = _last_used_guid++;
}

GQFramebufferObject::~GQFramebufferObject()
{
    // Dirty but segfault due to OpenGL context already destroyed
    // clear();
}

void GQFramebufferObject::clear()
{
    QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());
    if (_fbo >= 0)
    {
        for (int i = 0; i < _num_color_attachments; i++) {
            delete _color_attachments[i];
        }

        delete _color_attachments;
        
        if (_depth_attachment >= 0)
        {
            glFuncs.glDeleteRenderbuffers(1, (GLuint*)(&_depth_attachment));
        }

        glFuncs.glDeleteFramebuffers(1, (GLuint*)(&_fbo));

        _fbo = -1;
    }
    _gl_target = 0;
    _gl_format = 0;
    _format = -1;
    _coordinates = -1;
    _attachments = GQ_ATTACH_NONE;
    _num_color_attachments = 0;
    _color_attachments = NULL;
    _width = -1;
    _height = -1;
    _guid = _last_used_guid++;
    if (_guid == 0) _guid = _last_used_guid++;
}

bool GQFramebufferObject::init(int width, int height, 
                               int num_color_attachments,
                               uint32 extra_attachments, 
                               uint32 coordinates, uint32 format)
{
    int gl_format = GL_RGBA32F_ARB;
    if (format == GQ_FORMAT_RGBA_BYTE)
        gl_format = GL_RGBA;
    else if (format == GQ_FORMAT_RGBA_HALF)
        gl_format = GL_RGBA16F_ARB;

    int gl_target = GL_TEXTURE_RECTANGLE_ARB;
    if (coordinates == GQ_COORDS_NORMALIZED)
        gl_target = GL_TEXTURE_2D;

    _format = format;
    _coordinates = coordinates;

    return initGL(gl_target, gl_format, extra_attachments, 
            num_color_attachments, width, height);
}
    
bool GQFramebufferObject::initFullScreen(int num_color_attachments,
                               uint32 extra_attachments, 
                               uint32 coordinates, uint32 format)
{
    GLint viewport[4];
    glGetIntegerv (GL_VIEWPORT, viewport);

    return init(viewport[2], viewport[3], num_color_attachments,
                extra_attachments, coordinates, format);
}


bool GQFramebufferObject::initGL(int target, int format, 
                               uint32 attachments, 
                               int num_color_attachments, 
                               int width, int height)
{
    if (_fbo < 0 ||
        _gl_target != target ||
        _gl_format != format ||
        _attachments != attachments ||
        _num_color_attachments != num_color_attachments ||
        _width != width ||
        _height != height)
    {
         assert(width > 0 && height > 0);
         int max_size = maxFramebufferSize();
         if (width > max_size || height > max_size)
         {
             qCritical("Requested FBO size %d x %d, which exceeds max size %d x %d",
                 width, height, max_size, max_size);
             return false;
         }

         int max_attachments = maxColorAttachments();
         if (num_color_attachments > max_attachments)
         {
             qCritical("Requested FBO with %d color attachments, but only %d are supported",
                 num_color_attachments, max_attachments);
             return false;
         }

        clear();

        _gl_target = target;
        _gl_format = format;
        _width = width;
        _height = height;
        _attachments = attachments;

        // create the fbo
        QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());
        glFuncs.glGenFramebuffers(1, (GLuint*)(&_fbo));
        glFuncs.glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

        // attach the color buffers
        _num_color_attachments = num_color_attachments;
        _color_attachments = new GQTexture2D*[_num_color_attachments];
        for (int i = 0; i < _num_color_attachments; i++)
        {
            _color_attachments[i] = new GQTexture2D;
            _color_attachments[i]->create(_width, _height, _gl_format, 
                    GL_RGBA, GL_UNSIGNED_BYTE, NULL, _gl_target);

            glFuncs.glFramebufferTexture2D(GL_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0 + i, _gl_target,
                    _color_attachments[i]->id(), 0);
        }
        glFuncs.glBindTexture( _gl_target, 0 );
        
        if ( _attachments & GQ_ATTACH_DEPTH)
        {
            // attach a depth buffer
            glFuncs.glGenRenderbuffers(1, (GLuint*)(&_depth_attachment));
            glFuncs.glBindRenderbuffer(GL_RENDERBUFFER, _depth_attachment);
            glFuncs.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
                    _width, _height);
            glFuncs.glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                    GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                    _depth_attachment);
        }
        else if ( _attachments & GQ_ATTACH_DEPTH_TEXTURE)
        {
			// attach a depth buffer texture
			// Création d'une texture pour stocker la profondeur
                        glFuncs.glGenTextures(1, (GLuint*)(&_depthMapTex));
                        glFuncs.glBindTexture(GL_TEXTURE_2D, _depthMapTex);
                        glFuncs.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glFuncs.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        glFuncs.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                        glFuncs.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        glFuncs.glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _width, _height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
                        glFuncs.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthMapTex, 0);
        }
		else
		{
            _depth_attachment = -1;
        }
        glFuncs.glBindRenderbuffer(GL_RENDERBUFFER, 0);

        GLenum status = glFuncs.glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            QString die_msg;
            die_msg.sprintf("Could not set up framebuffer object. Tried %dx%d, %d attachments, %d format, %d depth attachment",
                _width, _height, _num_color_attachments, 
                _gl_format, _attachments & GQ_ATTACH_DEPTH);
            qCritical("%s", qPrintable(die_msg));
            glFuncs.glBindFramebuffer(GL_FRAMEBUFFER, 0);
            clear();
            return false;
        }

        glFuncs.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    return true;
}

void GQFramebufferObject::bind(uint32 clear_behavior) const
{
    assert(_fbo >= 0);
    assert(_bound_guid == 0);
    QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());
    glFuncs.glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glPushAttrib(GL_VIEWPORT_BIT | GL_COLOR_BUFFER_BIT);
    
    if (clear_behavior == GQ_CLEAR_BUFFER)
    {
        drawToAllBuffers();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
    _bound_guid = _guid;
}

void GQFramebufferObject::unbind() const
{
    assert(_fbo >= 0);
    assert(_bound_guid == _guid);
    glPopAttrib();
    QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());
    glFuncs.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    _bound_guid = 0;
}

void GQFramebufferObject::drawBuffer( int which ) const
{
    assert( which >= 0 && which < _num_color_attachments);
    glDrawBuffer( GL_COLOR_ATTACHMENT0 + which );
}

void GQFramebufferObject::drawToAllBuffers() const
{
    GLenum render_targets[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
                                GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5,
                                GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7,
                                GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9,
                                GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11,
                                GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13,
                                GL_COLOR_ATTACHMENT14, GL_COLOR_ATTACHMENT15 };
    assert(_num_color_attachments > 0 && _num_color_attachments < 16);
    QOpenGLExtraFunctions glFuncs(QOpenGLContext::currentContext());
    glFuncs.glDrawBuffers(_num_color_attachments, render_targets);
}

GQTexture2D* GQFramebufferObject::colorTexture( int which )
{
    assert( which >= 0 && which < _num_color_attachments );
    return _color_attachments[which];
}

const GQTexture2D* GQFramebufferObject::colorTexture( int which ) const
{
    assert( which >= 0 && which < _num_color_attachments );
    return _color_attachments[which];
}

void GQFramebufferObject::setTexParameteri(unsigned int param, int value)
{
    for (int i = 0; i < _num_color_attachments; i++) {
        _color_attachments[i]->bind();
        glTexParameteri(_gl_target, param, value);
        _color_attachments[i]->unbind();
    }
}

void GQFramebufferObject::setTexParameteriv(unsigned int param, int* value)
{
    for (int i = 0; i < _num_color_attachments; i++) {
        _color_attachments[i]->bind();
        glTexParameteriv(_gl_target, param, (GLint*)value);
        _color_attachments[i]->unbind();
    }
}

void GQFramebufferObject::setTexParameterf(unsigned int param, float value)
{
    for (int i = 0; i < _num_color_attachments; i++) {
        _color_attachments[i]->bind();
        glTexParameterf(_gl_target, param, value);
        _color_attachments[i]->unbind();
    }
}
void GQFramebufferObject::setTexParameterfv(unsigned int param, float* value)
{
    for (int i = 0; i < _num_color_attachments; i++) 
    {
        _color_attachments[i]->bind();
        glTexParameterfv(_gl_target, param, value);
        _color_attachments[i]->unbind();
    }
}

void GQFramebufferObject::setTextureFilter(int filter_min, int filter_mag)
{
    for (int i = 0; i < _num_color_attachments; i++)
    {
        _color_attachments[i]->bind();
        setTexParameteri(GL_TEXTURE_MAG_FILTER, filter_mag);
        setTexParameteri(GL_TEXTURE_MIN_FILTER, filter_min);
        _color_attachments[i]->unbind();
    }
}

void GQFramebufferObject::setTextureWrap(int wrap_s, int wrap_t)
{
    for (int i = 0; i < _num_color_attachments; i++)
    {
        _color_attachments[i]->bind();
        setTexParameteri(GL_TEXTURE_WRAP_S, wrap_s);
        setTexParameteri(GL_TEXTURE_WRAP_T, wrap_t);
        _color_attachments[i]->unbind();
    }
}

void GQFramebufferObject::readColorTexturei( int which, GQImage& image, int num_channels ) const 
{
    assert( which >= 0 && which < _num_color_attachments );
    assert(num_channels == 3 || num_channels == 4);
    int format = GL_RGBA;
    if (num_channels == 3)
        format = GL_RGB;
    image.resize( width(), height(), num_channels );

    _color_attachments[which]->bind();
    glGetTexImage( _gl_target, 0, format, GL_UNSIGNED_BYTE, 
                   image.raster());
    _color_attachments[which]->unbind();
}

void GQFramebufferObject::readColorTexturef( int which, GQFloatImage& image, int num_channels ) const 
{
    assert( which >= 0 && which < _num_color_attachments );
    assert(num_channels == 3 || num_channels == 4);
    int format = GL_RGBA;
    if (num_channels == 3)
        format = GL_RGB;
    image.resize( width(), height(), num_channels );

    _color_attachments[which]->bind();
    glGetTexImage( _gl_target, 0, format, GL_FLOAT, 
                   image.raster());
    _color_attachments[which]->unbind();
}   

void GQFramebufferObject::loadColorTexturei( int which, const GQImage& image )
{
    assert( which >= 0 && which < _num_color_attachments );
    _color_attachments[which]->bind();
    int format = GL_RGBA;
    if (image.chan() == 3)
        format = GL_RGB;
    glTexSubImage2D( _gl_target, 0, 0,0,_width,_height, format, GL_UNSIGNED_BYTE, 
                   image.raster());
    _color_attachments[which]->unbind();
}


void GQFramebufferObject::loadColorTexturef( int which, const GQFloatImage& image )
{
    assert( which >= 0 && which < _num_color_attachments );
    _color_attachments[which]->bind();
    int format = GL_RGBA;
    if (image.chan() == 3)
        format = GL_RGB;
    glTexSubImage2D( _gl_target, 0, 0,0,_width,_height, format, GL_FLOAT, 
                   image.raster());
    _color_attachments[which]->unbind();
}

void GQFramebufferObject::loadSubColorTexturei( int which, const GQImage& image, int x, int y )
{
    assert( which >= 0 && which < _num_color_attachments );
    assert(x+image.width() <= _width && y+image.height() <= _height);
    _color_attachments[which]->bind();
    int format = GL_RGBA;
    if (image.chan() == 3)
        format = GL_RGB;
    glTexSubImage2D( _gl_target, 0, x,y,image.width(),image.height(), format, GL_UNSIGNED_BYTE,
                   image.raster());
    _color_attachments[which]->unbind();
}


void GQFramebufferObject::loadSubColorTexturef( int which, const GQFloatImage& image, int x, int y )
{
    assert( which >= 0 && which < _num_color_attachments );
    assert(x+image.width() <= _width && y+image.height() <= _height);
    _color_attachments[which]->bind();
    int format = GL_RGBA;
    if (image.chan() == 3)
        format = GL_RGB;
    glTexSubImage2D( _gl_target, 0, x,y,image.width(),image.height(), format, GL_FLOAT,
                   image.raster());
    _color_attachments[which]->unbind();
}

void GQFramebufferObject::readSubColorTexturei( int which, int x, int y, int width, int height,
                                                GQImage& image, int num_channels ) const 
{
    // N.B.: on several cards (8800GTS, 7800GT) this call returns bogus results
    // if the FBO width > 4k
    assert( which >= 0 && which < _num_color_attachments );
    assert(num_channels == 3 || num_channels == 4);
    assert(_bound_guid == _guid);
    int format = GL_RGBA;
    if (num_channels == 3)
        format = GL_RGB;
    image.resize( width, height, num_channels );

    GLint last_buffer;
    glGetIntegerv( GL_READ_BUFFER, &last_buffer );
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer( GL_COLOR_ATTACHMENT0 + which );
    glReadPixels( x, y, width, height, format, GL_UNSIGNED_BYTE, image.raster() );
    glReadBuffer( last_buffer );

    reportGLError();
}

void GQFramebufferObject::readSubColorTexturef( int which, int x, int y, int width, int height,
                                                GQFloatImage& image, int num_channels ) const 
{
    assert( which >= 0 && which < _num_color_attachments );
    assert(num_channels == 3 || num_channels == 4);
    assert(_bound_guid == _guid);
    int format = GL_RGBA;
    if (num_channels == 3)
        format = GL_RGB;
    image.resize( width, height, num_channels );

    GLint last_buffer;
    glGetIntegerv( GL_READ_BUFFER, &last_buffer );
    glReadBuffer( GL_COLOR_ATTACHMENT0 + which );
    glReadPixels( x, y, width, height, format, GL_FLOAT, image.raster() );
    glReadBuffer( last_buffer );
}

void GQFramebufferObject::readDepthBuffer( GQFloatImage& image ) const 
{
    readSubDepthBuffer( 0, 0, _width, _height, image );
}

void GQFramebufferObject::readSubDepthBuffer( int x, int y, int width, int height, GQFloatImage& image ) const 
{
    assert(_attachments & GQ_ATTACH_DEPTH);
    image.resize( width, height, 1 );

    glReadPixels( x, y, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, image.raster() );
}

void GQFramebufferObject::saveColorTextureToFile( int which, const QString& filename ) const 
{
    if (filename.endsWith("float") || filename.endsWith("pfm"))
    {
        GQFloatImage img;
        readColorTexturef( which, img );
        img.save(filename);
    }
    else 
    {
        GQImage img;
        readColorTexturei( which, img );
        img.save(filename);
    }
}

void GQFramebufferObject::saveDepthTextureToFile( const QString& filename ) const
{
        GQFloatImage img;

        int format = GL_DEPTH_COMPONENT;
        img.resize( width(), height(), 1);

        glBindTexture(GL_TEXTURE_2D, _depthMapTex);
        glGetTexImage(GL_TEXTURE_2D, 0, format, GL_FLOAT, img.raster());
        glBindTexture(GL_TEXTURE_2D, 0);

        img.scaleValues(255.f);
        img.save(filename);
}

void GQFramebufferObject::saveDepthBufferToFile( const QString& filename ) const 
{
    GQFloatImage img;
    readDepthBuffer( img );
    img.save(filename);
}

int GQFramebufferObject::maxFramebufferSize()
{
    GLint size;
    glGetIntegerv( GL_MAX_TEXTURE_SIZE, &size );
    return size;
}

int GQFramebufferObject::maxColorAttachments()
{
    GLint num;
    glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &num);
    return num;
}


// GQSingleFBO

bool GQSingleFBO::init( int target, int format, uint32 attachments, 
                                   int num_color_attachments, int width, int height )
{
    if (_fbo < 0 ||
        _gl_target != target ||
        _gl_format != format ||
        _attachments != attachments ||
        _num_color_attachments != num_color_attachments ||
        _width != width ||
        _height != height)
    {
         assert(width > 0 && height > 0);
         int max_size = maxFramebufferSize();
         if (width > max_size || height > max_size)
         {
             qCritical("Requested FBO size %d x %d, which exceeds max size %d x %d",
                 width, height, max_size, max_size);
             return false;
         }

        clear();

        _gl_target = target;
        _gl_format = format;
        _width = width;
        _height = height;
        _attachments = attachments;

        // create the fbo
        QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());
        glFuncs.glGenFramebuffers(1, (GLuint*)(&_fbo));
        glFuncs.glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

        // create the color buffers, but only bind the first
        _num_color_attachments = num_color_attachments;
        _color_attachments = new GQTexture2D*[_num_color_attachments];
        for (int i = 0; i < _num_color_attachments; i++)
        {
            _color_attachments[i] = new GQTexture2D;
            _color_attachments[i]->create(_width, _height, _gl_format, GL_RGBA, GL_UNSIGNED_BYTE, NULL, _gl_target);
        }

        glFuncs.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _gl_target, _color_attachments[0]->id(), 0);
        _current_color_attachment = 0;

        glBindTexture( _gl_target, 0 );
        
        if ( _attachments & GQ_ATTACH_DEPTH)
        {
            // attach a depth buffer
            glFuncs.glGenRenderbuffers(1, (GLuint*)(&_depth_attachment));
            glFuncs.glBindRenderbuffer(GL_RENDERBUFFER, _depth_attachment);
            glFuncs.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _width, _height);
            glFuncs.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth_attachment);
        }
        else
        {
            _depth_attachment = -1;
        }
        glFuncs.glBindRenderbuffer(GL_RENDERBUFFER, 0);

        reportGLError();

        GLenum status = glFuncs.glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            qCritical("Could not set up frame buffer object.");
            glFuncs.glBindFramebuffer(GL_FRAMEBUFFER, 0);
            clear();
            return false;
        }

        glFuncs.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    return true;
}


void GQSingleFBO::drawBuffer( int which )
{
    assert(which >= 0 && which < _num_color_attachments );
    assert(_bound_guid == _guid);

    _current_color_attachment = which;
    QOpenGLExtraFunctions glFuncs(QOpenGLContext::currentContext());
    glFuncs.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _gl_target, _color_attachments[which]->id(), 0);
    GLenum render_targets[] = { GL_COLOR_ATTACHMENT0 };
    glFuncs.glDrawBuffers(1, render_targets);
}

void GQSingleFBO::drawToAllBuffers()
{
    // multiple render targets is not supported by this class
    assert(0);
}

