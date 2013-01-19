/*****************************************************************************\

gradient.frag
Authors: Pierre Benard (pierre.benard@laposte.net)
Copyright (c) 2012 Benard Pierre

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform sampler2DRect image;
uniform float weight;

void main()
{
    float incrX = 1;
    float incrY = 1;

    float sample0 = texture2DRect(image, (gl_FragCoord.xy + vec2(-incrX,incrY))).x;
    float sample1 = texture2DRect(image, (gl_FragCoord.xy + vec2( 0.0  ,incrY))).x;
    float sample2 = texture2DRect(image, (gl_FragCoord.xy + vec2( incrX,incrY))).x;
    float sample3 = texture2DRect(image, (gl_FragCoord.xy + vec2(-incrX,0.0))).x;
    float sample4 = texture2DRect(image, (gl_FragCoord.xy + vec2( 0.0,0.0))).x;
    float sample5 = texture2DRect(image, (gl_FragCoord.xy + vec2( incrX,incrY))).x;
    float sample6 = texture2DRect(image, (gl_FragCoord.xy + vec2(-incrX,-incrY))).x;
    float sample7 = texture2DRect(image, (gl_FragCoord.xy + vec2( 0.0,-incrY))).x;
    float sample8 = texture2DRect(image, (gl_FragCoord.xy + vec2( incrX,-incrY))).x;
    
    // Sobel filter
    //    -1 -2 -1       1 0 -1 
    // H = 0  0  0   V = 2 0 -2
    //     1  2  1       1 0 -1

    vec2 grad;
    grad.x = sample2 + (2.0*sample5) + sample8 -
                     (sample0 + (2.0*sample3) + sample6);

    grad.y = sample0 + (2.0*sample1) + sample2 -
                    (sample6 + (2.0*sample7) + sample8);

    grad *= weight;
  
    gl_FragColor = vec4(grad, 0.0, 0.0);
}
