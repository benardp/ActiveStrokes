/*****************************************************************************\

vblur.frag
Authors: Pierre Benard (pierre.benard@laposte.net)
Copyright (c) 2012 Benard Pierre

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform sampler2DRect src_tex;

void main()
{
  vec4 FragmentColor = texture2DRect( src_tex, gl_FragCoord.xy ) * 0.2270270270;

  FragmentColor += texture2DRect( src_tex, gl_FragCoord.xy + vec2(0.0, 1.3846153846) ) * 0.3162162162;
  FragmentColor += texture2DRect( src_tex, gl_FragCoord.xy - vec2(0.0, 1.3846153846) ) * 0.3162162162;
  FragmentColor += texture2DRect( src_tex, gl_FragCoord.xy + vec2(0.0, 3.2307692308) ) * 0.0702702703;
  FragmentColor += texture2DRect( src_tex, gl_FragCoord.xy - vec2(0.0, 3.2307692308) ) * 0.0702702703;
  
  gl_FragColor = FragmentColor;
}
