/*****************************************************************************\

image_lines.frag
Authors: Forrester Cole (fcole@csail.mit.edu)
Copyright (c) 2012 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform vec4 viewport;

uniform sampler2DRect camera_pos;
uniform sampler2DRect geom_flow;

uniform sampler2DRect energy;
uniform float threshold;
uniform float thresh_range;
uniform mat4 projection;

void main()
{
    vec4 my_texel = texture2DRect(energy, gl_FragCoord.xy);
    float my_value = my_texel.x;

    float stepsize = 1.0;
    vec2 g = my_texel.yz;

    vec2 forward = my_texel.yz * stepsize + gl_FragCoord.xy;
    vec2 back = my_texel.yz * -stepsize + gl_FragCoord.xy;

    vec4 forward_texel = texture2DRect(energy, forward);
    vec4 back_texel = texture2DRect(energy, back);

    bool maxima = (my_value > forward_texel.x) &&
                  (my_value > back_texel.x) &&
                  (my_value > threshold);
    if (!maxima)
        discard;

    vec4 color;
    float strength = (my_value - threshold) / thresh_range;

    color = vec4(strength, -g.y, g.x, 1.0);

    vec2 search_f = (g * stepsize) + gl_FragCoord.xy;
    vec2 search_b = (g * -stepsize) + gl_FragCoord.xy;

    vec2 my_screen = gl_FragCoord.xy;
    vec4 my_pos = texture2DRect(camera_pos, my_screen);
    vec4 forward_pos = texture2DRect(camera_pos, search_f);
    vec4 back_pos = texture2DRect(camera_pos, search_b);

    if (back_pos.a > 0.5 && back_pos.z > my_pos.z) {
        my_pos = back_pos;
        my_screen = search_b;
    }
    if (forward_pos.a > 0.5 && forward_pos.z > my_pos.z) {
        my_pos = forward_pos;
        my_screen = search_f;
    }

    if (my_pos.a < 0.5) {
        if (back_pos.a > 0.5) {
            my_pos = back_pos;
            my_screen = search_b;
        }
        if (forward_pos.a > 0.5) {
            my_pos = forward_pos;
            my_screen = search_f;
        }
    }

    vec4 proj_my_pos = projection * my_pos;
    color.a = proj_my_pos.z / proj_my_pos.w;
    gl_FragData[0] = color;

    vec4 my_geom_flow = texture2DRect(geom_flow, my_screen);
    vec4 proj_flow = projection * vec4(my_geom_flow.xyz,1);
    vec4 proj_flow_NDC = proj_flow / proj_flow.w;
    vec3 proj_flow_screen;
    proj_flow_screen.x = 0.5*(proj_flow_NDC.x + 1.0)*viewport[2];
    proj_flow_screen.y = 0.5*(proj_flow_NDC.y + 1.0)*viewport[3];
    proj_flow_screen.z = 0.5*(gl_DepthRange.far - gl_DepthRange.near) * proj_flow_NDC.z + 0.5*(gl_DepthRange.far + gl_DepthRange.near);

    gl_FragData[1] = vec4(proj_flow_screen,1.0);
}
