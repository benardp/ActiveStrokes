/*****************************************************************************\

gaussian_blur.frag
Authors: Forrester Cole (fcole@csail.mit.edu)
Copyright (c) 2012 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform sampler2DRect colors;
uniform float kernel_radius;
uniform vec2 filter_direction;

void main()
{
    float x = gl_FragCoord.x;
    float y = gl_FragCoord.y;

    vec4 accum_color = vec4(0.0, 0.0, 0.0, 0.0);

    float grid_radius = 3.0;
    // Compensation for the non-zero width of our derivative operator.
    float derivative_width = 3.0;
    float gwidth = 2.0*grid_radius*kernel_radius + derivative_width;
    float units = gwidth / (2.0*grid_radius);

    // sigma of the Gaussian as defined by Freeman is sqrt(0.5).       
    float sigma = units * sqrt(0.5);
    float exp_scale = 1.0 / (2*sigma*sigma);
    float norm = 1.0 / sqrt(2.0 * 3.1415 * sigma * sigma); 

    for (float i = -gwidth/2.0; i <= gwidth/2.0; i++)
    {
        float weight = norm * exp(-1.0*i*i*exp_scale);
        vec2 sample = vec2(x, y) + filter_direction * i;

        vec4 texel = weight * texture2DRect(colors, sample);

        accum_color += texel;
    }

    gl_FragColor = accum_color;
}
