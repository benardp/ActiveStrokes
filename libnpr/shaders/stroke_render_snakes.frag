/*****************************************************************************\

stroke_render_snakes.frag
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu
Copyright (c) 2009 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform sampler3D vis_focus_texture;
uniform vec4 vis_focus_color;

varying float visibility;
varying vec2 relative_position;

void main()
{
    float elision = 1.0f;
    float focus = 1.0;

    float vis_focus_pen = texture3D(vis_focus_texture, gl_TexCoord[0].xyz).r;

    vec4 pen_color = vec4(vis_focus_color.rgb, vis_focus_pen * vis_focus_color.a);
    
    gl_FragColor = pen_color * visibility;
}

