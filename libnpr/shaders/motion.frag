/*****************************************************************************\

motion.frag
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform vec4 viewport;

varying vec4 courPos, prevPos;

void main()
{
    vec3 courPosNDC = courPos.xyz/courPos.w;
    vec3 prevPosNDC = prevPos.xyz/prevPos.w;
    vec2 courPosW, prevPosW;
    courPosW.x = viewport.z/2.0 * courPosNDC.x + (viewport.x + viewport.z/2.0);
    courPosW.y = viewport.w/2.0 * courPosNDC.y + (viewport.y + viewport.w/2.0);
    prevPosW.x = viewport.z/2.0 * prevPosNDC.x + (viewport.x + viewport.z/2.0);
    prevPosW.y = viewport.w/2.0 * prevPosNDC.y + (viewport.y + viewport.w/2.0);

    vec2 forward = courPosW - prevPosW;

    gl_FragColor=vec4(forward,0.0,1.0);
}
