/*****************************************************************************\

deferredShafing.vert
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;
uniform mat3 normal_matrix;
uniform int type;

attribute vec4 color;
attribute vec4 vertex;
attribute vec3 normal;
attribute vec3 geom_flow;

varying vec4 vert_color;
varying vec3 vert_pos_camera;
varying vec3 vert_normal_camera;
varying vec3 vert_geom_flow;
varying vec4 vert_pos_clip;

void main()
{
  gl_Position = projection_matrix * model_view_matrix * vertex;
  vert_pos_clip = gl_Position;

  vert_color = color;

  vert_pos_camera  = (model_view_matrix * vertex).xyz;
  vert_normal_camera = normalize( normal_matrix * normal);
  vert_geom_flow = vec4(model_view_matrix * vec4(vertex.xyz + geom_flow,1)).xyz;
}
