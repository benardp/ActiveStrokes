/*****************************************************************************\

NPRSettings.h
Authors: Forrester Cole (fcole@cs.princeton.edu)
         R. Keith Morley 
Copyright (c) 2009 Forrester Cole

Style storage. Contains pen, paper, background, etc.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _NPR_STYLE_H_
#define _NPR_STYLE_H_

#include <stdio.h>
#include "GQTexture.h"
#include "Vec.h"
#include <QString>
#include <QtXml/QDomElement>
#include <QDir>
#include <vector>
using std::vector;

class NPRTransfer 
{
public:
    NPRTransfer() : v1(0), v2(0), vnear(0), vfar(0) {}

    void load( const QDomElement& element );
    void save( QDomDocument& doc, QDomElement& element );

    void set( float v1, float v2, float vnear, float vfar );
    void print() const;

    float compute( float f ) const;

    void toArray( float ar[4]) const;
    void toArray( float ar[4], float v1_multiple, float v2_multiple) const;

public:
    float v1, v2, vnear, vfar;
};

class NPRPenStyle
{
public:
    NPRPenStyle();
    ~NPRPenStyle() { clear(); }

    void clear();
    void copyFrom( const NPRPenStyle& style );
    void load( const QDir& style_dir, const QDomElement& element );
    void save( const QDir& style_dir, QDomDocument& doc, QDomElement& element );

    const QString&      name() const                 { return _name; }
    const vec&          color() const                { return _color; }
    const QString&      textureFile() const          { return _texture_file; }
    const QString&      offsetTextureFile() const    { return _offsetTexture_file;}
    const QString&      motionTextureFile() const    { return _motionTexture_file;}
    const QString&      interiorTextureFile() const  { return _interiorTexture_file;}
    const GQTexture*    texture() const              { return _texture; }
    GQTexture*          texture()                    { return _texture; }
    const GQTexture*    interiorTexture() const      { return _interiorTexture; }
    GQTexture*          interiorTexture()            { return _interiorTexture; }
    const GQTexture3D*    tangentTexture() const        { return _tangentTexture; }
    GQTexture3D*          tangentTexture()              { return _tangentTexture; }
    const GQTexture3D*    normalTexture() const        { return _normalTexture; }
    GQTexture3D*          normalTexture()              { return _normalTexture; }
    const GQTexture*    motionTexture() const        { return _motionTexture;}
    GQTexture*          motionTexture()              { return _motionTexture;}

    float               opacity() const              { return _opacity; }
    float               stripWidth() const           { return _strip_width; }
    float               elisionWidth() const         { return _elision_width; }
    float               lengthScale() const          { return _length_scale; }
    float               offsetScale() const          { return _offset_scale; }

    void setName( const QString& name ) { _name = name; }
    void setColor( const vec& color ) { _color = color; }
    bool setTexture( const QString& filename );
    bool setInteriorTexture( const QString & filename);
    bool setOffsetTexture( const QString& filename );
    bool setMotionTexture( const QString& filename );
    float userOffsetScale() const {return _userOffset_Scale;}
    void setUserOffsetScale(double s){_userOffset_Scale = s;}

    void setOpacity( float val ) { _opacity = val; }
    void setStripWidth( float width ) { _strip_width = width; }
    void setElisionWidth( float width ) { _elision_width = width; }
    void setLengthScale( float scale ) { _length_scale = scale; }

protected:
    QString     _name;
    vec         _color;
    QString     _texture_file;
    GQTexture*  _texture;
    QString     _offsetTexture_file;
    GQTexture3D*  _tangentTexture;
    GQTexture3D*  _normalTexture;
    QString     _interiorTexture_file;
    GQTexture*  _interiorTexture;

    QString     _motionTexture_file;
    GQTexture*  _motionTexture;


    float       _opacity;
    float       _strip_width;
    float       _elision_width;
    float       _length_scale;  // Texture scale along the line's length.
    float       _offset_scale;
    float       _userOffset_Scale;
};


class NPRStyle
{
public:
    NPRStyle();
    ~NPRStyle() { clear(); }

    void clear();
    bool load( const QString& filename );
    bool load( const QDir& style_dir, const QDomElement& root );
    bool save( const QString& filename );
    bool save( const QDir& style_dir, QDomDocument& doc, QDomElement& root );

    void loadDefaults();

    bool loadPaperTexture(const QString& filename);
    GQTexture* paperTexture() { return _paper_texture; }
    const GQTexture* paperTexture() const { return _paper_texture; }

    bool loadBackgroundTexture(const QString& filename);
    GQTexture* backgroundTexture() { return _background_texture; }
    const GQTexture* backgroundTexture() const { return _background_texture; }


    typedef enum 
    {
        FOCUS_TRANSFER,

        LINE_COLOR,
        LINE_OPACITY,
        LINE_TEXTURE,
        LINE_ELISION,

        FILL_FADE,
        FILL_DESAT,

        PAPER_PARAMS,

        SC_THRESHOLD,
        RV_THRESHOLD,

        NUM_TRANSFER_FUNCTIONS
    } TransferFunc;

    inline const NPRTransfer& transfer( TransferFunc func ) const { return _transfers[func]; }
    NPRTransfer& transferRef( TransferFunc func );
    NPRTransfer* transferByName( const QString& name );

    inline const vec&   backgroundColor() const      { return _background_color; }
    inline bool  drawInvisibleLines() const { return _draw_invisible_lines; }
    inline bool  enableLineElision() const { return _enable_line_elision; }

    inline float scThreshold() const     { return _sc_threshold; }
    inline float rvThreshold() const     { return _rv_threshold; }
    inline float appRidgeThreshold() const     { return _app_ridge_threshold; }

    inline void  setSCThreshold( float threshold ) { _sc_threshold = threshold; }
    inline void  setRVThreshold( float threshold ) { _rv_threshold = threshold; }
    inline void  setAppRidgeThreshold( float threshold ) { _app_ridge_threshold = threshold; }

    void         setBackgroundColor( const vec& color ) { _background_color = color; }
    void         setDrawInvisibleLines( bool set ) { _draw_invisible_lines = set; }
    void         setEnableLineElision( bool set ) { _enable_line_elision = set; }

    bool         isPathStyleDirty() { return _path_style_dirty; }
    void         setPathStyleClean() { _path_style_dirty = false; }

    int                 numPenStyles() const { return (int)(_pen_styles.size()); }
    const NPRPenStyle*  penStyle(int which) const { return _pen_styles[which]; }
    NPRPenStyle*        penStyle(int which) { return _pen_styles[which]; }
    const NPRPenStyle*  penStyle (const QString& name) const;
    NPRPenStyle*        penStyle(const QString& name);
    bool                hasPenStyle(const QString& name) const;

    void                addPenStyle(NPRPenStyle* style);
    bool                deletePenStyle( int which );
    bool                deletePenStyle( const QString& name );

protected:
    bool loadTexture(QString& output_name, GQTexture*& output_ptr, 
                     const QString& field_name, const QString& filename);
protected:
    NPRTransfer _transfers[NUM_TRANSFER_FUNCTIONS];
    vector<NPRPenStyle*> _pen_styles;

    vec         _background_color;       
    bool        _draw_invisible_lines;
    bool        _enable_line_elision;

    QString     _paper_file;
    GQTexture*  _paper_texture;

    QString     _background_file;
    GQTexture*  _background_texture;

    float _sc_threshold;
    float _rv_threshold;
    float _app_ridge_threshold;

    bool _path_style_dirty;

};


#endif // _NPR_STYLE_H_
