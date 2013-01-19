/*****************************************************************************\

image_lines.frag
Authors: Forrester Cole (fcole@csail.mit.edu)
Copyright (c) 2012 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform float gain;
uniform float offset;
uniform sampler2DRect source;
uniform float use_color;

void main()
{
    vec4 color = texture2DRect(source, gl_FragCoord.xy);
    if (use_color==0)
        color.rgb = vec3(sqrt(dot(color.rgb, color.rgb)));
    gl_FragColor = gain * color + offset;
}
