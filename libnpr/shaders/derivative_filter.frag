/*****************************************************************************\

derivative_filter.frag
Authors: Forrester Cole (fcole@csail.mit.edu)
Copyright (c) 2012 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform sampler2DRect colors;
uniform vec4 dx_read_mask;
uniform vec4 dy_read_mask;

void main()
{
    vec4 accum;

    float x = gl_FragCoord.x;
    float y = gl_FragCoord.y;

    vec4 left = texture2DRect(colors, vec2(x-1,y));
    vec4 right = texture2DRect(colors, vec2(x+1,y));
    vec4 up = texture2DRect(colors, vec2(x,y+1));
    vec4 down = texture2DRect(colors, vec2(x,y-1));

    accum += (right - left) * dx_read_mask;
    accum += (up - down) * dy_read_mask;

    accum *= 0.5;

    gl_FragColor = accum;
}
