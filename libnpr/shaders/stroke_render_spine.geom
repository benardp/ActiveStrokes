/*****************************************************************************\

stroke_render_spine.geom
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu
Copyright (c) 2009 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform vec4 viewport;
uniform float pen_width;
uniform float texture_length;
uniform float length_scale;
uniform float overshoot_scale;

varying out vec4 spine_position;
varying out vec2 tex_coord;

vec2 textureOffsets(float segment_length)
{
    float texture_scale = (segment_length / texture_length) / length_scale;
    return vec2(-texture_scale * 0.5 + 0.5, texture_scale * 0.5 + 0.5);
}

void main(void)
{
    vec4 clip_p = gl_ModelViewProjectionMatrix * gl_PositionIn[0];
    vec4 clip_q = gl_ModelViewProjectionMatrix * gl_PositionIn[1];

    vec3 window_p = clipToWindow(clip_p, viewport);
    vec3 window_q = clipToWindow(clip_q, viewport);

    vec3 tangent = window_q - window_p;
    float segment_length = length(tangent.xy);
    vec2 texture_offsets = textureOffsets(segment_length);

    vec2 perp = normalize(vec2(-tangent.y, tangent.x));
    vec2 window_offset = perp*pen_width;


    float clip_p_w = clip_p.w;
    float clip_q_w = clip_q.w;
    vec4 offset_p = windowToClipVector(window_offset, viewport, clip_p_w);
    vec4 offset_q = windowToClipVector(window_offset, viewport, clip_q_w);

    spine_position = clip_p;
    gl_Position = clip_p + offset_p;
    tex_coord = vec2(texture_offsets.x, 0.0);
    EmitVertex();
    gl_Position = clip_p - offset_p;
    tex_coord = vec2(texture_offsets.x, 1.0);
    EmitVertex();

    spine_position = clip_q;
    gl_Position = clip_q + offset_q;
    tex_coord = vec2(texture_offsets.y, 0.0);
    EmitVertex();
    gl_Position = clip_q - offset_q;
    tex_coord = vec2(texture_offsets.y, 1.0);
    EmitVertex();

    EndPrimitive();
}
