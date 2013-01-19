/*****************************************************************************\

toon.frag
Authors: Pierre Benard (pierre.benard@laposte.net)
Copyright (c) 2012 Benard Pierre

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform vec3 light_dir_world;
uniform sampler2D textureGrayScale;
uniform int toon_factor;
uniform int tex_height;

varying vec4 vert_color;
varying vec4 vert_pos_view;
varying vec3 vert_normal_view;

vec3 cellShade(float dotNL, vec3 color){
    float  toon_smooth = float(toon_factor)/tex_height;
    float  U = texture2D(textureGrayScale,vec2(dotNL,toon_smooth)).r;
    return U*color;
}

void main(void)
{
    vec3 N = normalize(vert_normal_view);
    float dotNL = dot(light_dir_world,N);
    gl_FragColor = vec4(cellShade(dotNL,vert_color.rgb), 1.0);
}
