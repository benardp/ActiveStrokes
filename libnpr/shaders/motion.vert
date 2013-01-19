/*****************************************************************************\

motion.vert
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform mat4 model_view_matrix;
uniform mat4 prevModel_view_matrix;
uniform mat4 projection_matrix;

attribute vec4 vertex;

varying vec4 courPos, prevPos;

void main()
{
        courPos = projection_matrix * model_view_matrix * vertex;
        prevPos = projection_matrix * prevModel_view_matrix * vertex;
	
        gl_Position = prevPos;
}
