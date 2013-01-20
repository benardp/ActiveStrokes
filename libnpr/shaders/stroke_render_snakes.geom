/*****************************************************************************\

stroke_render_snakes.geom
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu
Copyright (c) 2009 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/


uniform vec4 viewport;
uniform sampler2DRect path_start_end_ptrs;
uniform sampler2DRect clip_vert_0_buffer;
uniform sampler2DRect clip_vert_1_buffer;
uniform sampler2DRect param_buffer;
uniform sampler3D tangent_texture;
uniform sampler3D normal_texture;

uniform float pen_width;
uniform float length_scale;
uniform float row;
uniform float clip_buffer_width;
uniform int use_artmaps;
uniform float offset_scale;
varying out float visibility;

vec2 getBrushpathOffset(vec2 tex_offset, float scale)
{
    vec3 tex_coord1 = vec3(tex_offset.x, 0.5, tex_offset.y);
    float tangentOffset = (texture3D(tangent_texture, tex_coord1).r *2.0 -1.0) * scale;
    vec3 tex_coord2 = vec3(tex_offset.x, 0.5, tex_offset.y);
    float normalOffset = (texture3D(normal_texture, tex_coord2).r *2.0 -1.0) * scale;
    return vec2(tangentOffset,normalOffset);
}

vec4 getTextureOffsets(float segment_index)
{
    vec2 this_coord = indexToCoordinate(segment_index, clip_buffer_width);
    vec4 parameters = texture2DRect(param_buffer,this_coord);
    vec2 param = parameters.xy;
    float level_p = parameters.z;
    float level_q = parameters.w;
    if(use_artmaps == 1){
        return vec4(param, level_p, level_q);
    }else{
        return vec4(param / length_scale,0,0);
    }
}

vec2 getVertexOffset(vec2 tangent, float scale)
{
    return normalize(vec2(-tangent.y, tangent.x)) * (pen_width * scale);
}

void emitVertexPair(vec2 v, vec2 vertex_offset, vec2 tex_offset, vec2 segment_coord, float pen, float alpha, float idx)
{
    float slope = (1.0+sign(texture2DRect(path_start_end_ptrs, segment_coord).w))/2.0;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(v - vertex_offset, 0.0, 1.0);
    gl_TexCoord[0] = vec4(tex_offset.x, slope, tex_offset.y, 0.0);
    if((pen*pen_width)<1.0)
        visibility = alpha*pen*pen_width-0.1;
    else
        visibility = alpha;
    EmitVertex();
    
    gl_Position = gl_ModelViewProjectionMatrix * vec4(v + vertex_offset, 0.0, 1.0);
    gl_TexCoord[0] = vec4(tex_offset.x, 1.0-slope, tex_offset.y, 0.0);
    if((pen*pen_width)<1.0)
        visibility = pen*pen_width-0.1;
    else
        visibility = alpha;
    EmitVertex();
}

void main()
{
    vec2 segment_coord = vec2(gl_PositionIn[0].x, row);
    float segment_index = coordinateToIndex(segment_coord, clip_buffer_width);
    vec4 path_texel = texture2DRect(path_start_end_ptrs, segment_coord);
    float path_start = unpackPathStart(path_texel);
    float path_end = unpackPathEnd(path_texel);
    float scale = offset_scale;
    
    //the position of the starting and the end point
    vec4 clip_p = texture2DRect(clip_vert_0_buffer, segment_coord);
    vec4 clip_q = texture2DRect(clip_vert_1_buffer, segment_coord);
    vec2 p = clip_p.xy;
    vec2 q = clip_q.xy;
    
    //the texture coordinate of the end points
    vec4 tex_offsets = getTextureOffsets(segment_index);
    
    //interpolating the texture coordinates
    float numSubSamples = 4;
    float spacing = 1.0 / (numSubSamples + 1);
    float i1 = spacing * 1.0, i2 = spacing * 2.0, i3 = spacing * 3.0, i4 = spacing * 4.0;
    vec2 tex1 = (1.0 - i1) * tex_offsets.xz + i1 * tex_offsets.yw;
    vec2 tex2 = (1.0 - i2) * tex_offsets.xz + i2 * tex_offsets.yw;
    vec2 tex3 = (1.0 - i3) * tex_offsets.xz + i3 * tex_offsets.yw;
    vec2 tex4 = (1.0 - i4) * tex_offsets.xz + i4 * tex_offsets.yw;
    
    //access the brushpath texture
    vec2 brushpath_offset1 = getBrushpathOffset(tex_offsets.xz, scale);
    vec2 off1 = getBrushpathOffset(tex1, scale);
    vec2 off2 = getBrushpathOffset(tex2, scale);
    vec2 off3 = getBrushpathOffset(tex3, scale);
    vec2 off4 = getBrushpathOffset(tex4, scale);
    vec2 brushpath_offset2 = getBrushpathOffset(tex_offsets.yw, scale);
    
    //interpolate the spine position
    vec4 p1 = (1.0 - i1) * clip_p + i1 * clip_q;
    vec4 p2 = (1.0 - i2) * clip_p + i2 * clip_q;
    vec4 p3 = (1.0 - i3) * clip_p + i3 * clip_q;
    vec4 p4 = (1.0 - i4) * clip_p + i4 * clip_q;
    
    //compute the tangent of each subsegment
    float tsign = sign(texture2DRect(path_start_end_ptrs, segment_coord).w);
    vec2 t1 = tsign * normalize(q - p);
    vec2 t2 = t1;
    vec2 t3 = t1;
    vec2 t4 = t1;
    vec2 t5 = t1;
    vec2 t0 = t1, t6 = t5;
    vec2 up0, up1, up2, up3, up4, up5, up6, up7;
    // Mitering
    if (segment_index > path_start)
    {
        vec2 prev_coord = indexToCoordinate(segment_index-1.0, clip_buffer_width);
        vec4 prev_clip = texture2DRect(clip_vert_0_buffer, prev_coord);
        vec2 p_prev = prev_clip.xy;
        vec2 pos1 = (1.0 - i3) * p_prev + i3 * p;
        vec2 pos2 = (1.0 - i4) * p_prev + i4 * p;
        t0 = tsign * (normalize(pos2 - pos1) + normalize(p - pos2)) * 0.5;
        vec4 tex_offsets_prev = getTextureOffsets(segment_index-1.0);
        vec2 tex = (1.0 - i4) * tex_offsets_prev.xz + i4 * tex_offsets_prev.yw;
        vec2 brushpath_offset_prev = getBrushpathOffset(tex, scale);
        up0 = pos2 + brushpath_offset_prev.x*t0 + brushpath_offset_prev.y*vec2(-t0.y,t0.x);
        t0 = tsign * normalize(p - pos2);
    }

    if (segment_index < path_end)
    {
        vec2 next_coord = indexToCoordinate(segment_index+1.0, clip_buffer_width);
        vec4 next_clip = texture2DRect(clip_vert_1_buffer, next_coord);
        vec2 q_next = next_clip.xy;
        vec2 pos1 = (1.0 - i1) * q + i1 * q_next;
        vec2 pos2 = (1.0 - i2) * q + i2 * q_next;
        t6 = tsign * (normalize(pos2 - pos1) + normalize(pos1 - q)) * 0.5;
        vec4 tex_offsets_next = getTextureOffsets(segment_index+1.0);
        vec2 tex = (1.0 - i1) * tex_offsets_next.xz + i1 * tex_offsets_next.yw;
        vec2 brushpath_offset_next = getBrushpathOffset(tex, scale);
        up7 = pos1 + brushpath_offset_next.x*t6 + brushpath_offset_next.y*vec2(-t6.y,t6.x);
        t6 = tsign * normalize(pos1 - q);
    }
    
    //calculate the tangent as the weighted average
    vec2 ut1 = (normalize(t0) + normalize(t1)) * 0.5;
    vec2 ut2 = (normalize(t1) + normalize(t2)) * 0.5;
    vec2 ut3 = (normalize(t2) + normalize(t3)) * 0.5;
    vec2 ut4 = (normalize(t3) + normalize(t4)) * 0.5;
    vec2 ut5 = (normalize(t4) + normalize(t5)) * 0.5;
    vec2 ut6 = (normalize(t5) + normalize(t6)) * 0.5;
    
    //apply the offset to the spine position
    up1 = clip_p.xy + brushpath_offset1.x*ut1 + brushpath_offset1.y*vec2(-ut1.y, ut1.x);
    up2 = p1.xy + off1.x * ut2 + off1.y * vec2(-ut2.y, ut2.x);
    up3 = p2.xy + off2.x * ut3 + off2.y * vec2(-ut3.y, ut3.x);
    up4 = p3.xy + off3.x * ut4 + off3.y * vec2(-ut4.y, ut4.x);
    up5 = p4.xy + off4.x * ut5 + off4.y * vec2(-ut5.y, ut5.x);
    up6 = clip_q.xy + brushpath_offset2.x*ut6 + brushpath_offset2.y*vec2(-ut6.y, ut6.x);
    
    //update the tangent after applying the offset
    if (segment_index > path_start) {
        ut1 = ( normalize(up1  - up0) + normalize(up2 - up1) ) * 0.5;
    }else{
        ut1 = normalize(up2 - up1);
    }

    ut2 = ( normalize(up2  - up1) + normalize(up3 - up2) ) * 0.5;
    ut3 = ( normalize(up3  - up2) + normalize(up4 - up3) ) * 0.5;
    ut4 = ( normalize(up4  - up3) + normalize(up5 - up4) ) * 0.5;
    ut5 = ( normalize(up5  - up4) + normalize(up6 - up5) ) * 0.5;
    
    if (segment_index < path_end){
        ut6 = ( normalize(up6  - up5) + normalize(up7 - up6) ) * 0.5;
    }else{
        ut6 = normalize(up6  - up5);
    }

    vec2 vertex_offset;
    float segment_length = abs(up1.x - up6.x);
    
    // Two vertices at the p end of the segment.
    vertex_offset = getVertexOffset(ut1, clip_p.w);
    emitVertexPair(up1, vertex_offset, tex_offsets.xz, segment_coord, clip_p.w,clip_p.z,0);
    
    // generate the vertices in the middle
    vertex_offset = getVertexOffset(ut2, p1.w);
    emitVertexPair(up2, vertex_offset, tex1, segment_coord, p1.w,p1.z,spacing);
    
    vertex_offset = getVertexOffset(ut3, p2.w);
    emitVertexPair(up3, vertex_offset, tex2, segment_coord, p2.w,p2.z,2*spacing);
    
    vertex_offset = getVertexOffset(ut4, p3.w);
    emitVertexPair(up4, vertex_offset, tex3, segment_coord, p3.w,p3.z,3*spacing);
    
    vertex_offset = getVertexOffset(ut5, p4.w);
    emitVertexPair(up5, vertex_offset, tex4, segment_coord, p4.w,p4.z,4*spacing);
    
    // Two vertices at the q end of the segment.
    vertex_offset = getVertexOffset(ut6, clip_q.w);
    emitVertexPair(up6, vertex_offset, tex_offsets.yw, segment_coord, clip_q.w,clip_q.z,1);
     
    EndPrimitive();
}
