/*****************************************************************************\

derivative_filter_rgb.frag
Authors: Forrester Cole (fcole@csail.mit.edu)
Copyright (c) 2012 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform sampler2DRect colors;

void main()
{
    const vec4 dx_write_mask = vec4(1,1,0,0);
    const vec4 dy_write_mask = vec4(0,0,1,1);
    
    vec4 accum_r, accum_g, accum_b;

    float x = gl_FragCoord.x;
    float y = gl_FragCoord.y;

    vec4 left = texture2DRect(colors, vec2(x-1,y));
    vec4 right = texture2DRect(colors, vec2(x+1,y));
    vec4 up = texture2DRect(colors, vec2(x,y+1));
    vec4 down = texture2DRect(colors, vec2(x,y-1));

    accum_r += (right.r - left.r) * dx_write_mask;
    accum_r += (up.r - down.r) * dy_write_mask;
    accum_g += (right.g - left.g) * dx_write_mask;
    accum_g += (up.g - down.g) * dy_write_mask;
    accum_b += (right.b - left.b) * dx_write_mask;
    accum_b += (up.b - down.b) * dy_write_mask;

    accum_r *= 0.5;
    accum_g *= 0.5;
    accum_b *= 0.5;

    gl_FragData[0] = accum_r;
    gl_FragData[1] = accum_g;
    gl_FragData[2] = accum_b;		
}
