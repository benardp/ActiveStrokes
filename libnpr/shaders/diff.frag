/*****************************************************************************\

diff.frag
Authors: Pierre Benard (pierre.benard@laposte.net)
Copyright (c) 2012 Benard Pierre

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform sampler2D image1;
uniform sampler2D image2;
uniform vec2 image_size;

void main()
{
	float pix1 = texture2D(image1,vec2(gl_FragCoord.x/image_size.x,gl_FragCoord.y/image_size.y)).r;
	float pix2 = texture2D(image2,vec2(gl_FragCoord.x/image_size.x,gl_FragCoord.y/image_size.y)).r;
	float diff = pix1-pix2;
	
	gl_FragColor = vec4(diff,0,-diff,1.0);
}
