/*****************************************************************************\

filter_pair_brute.frag
Authors: Forrester Cole (fcole@csail.mit.edu)
Copyright (c) 2012 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform sampler2DRect d1_x_image;
uniform sampler2DRect d2_x_image;
uniform sampler2DRect d3_x_image;
uniform sampler2DRect d1_y_image;
uniform sampler2DRect d2_y_image;
uniform sampler2DRect d3_y_image;
uniform sampler2DRect d1_z_image;
uniform sampler2DRect d2_z_image;
uniform sampler2DRect d3_z_image;
uniform float blur_radius;

void main()
{
    // Each derivative filter needs different units. The equivalent
    // blur kernel for each filter is slightly different due to the
    // repeated convolutions.
    // The results are quite sensitive to variations in the
    // unit values.

    float blur_grid = 3.0;
    float units1 = blur_radius + 3.0 / (2*blur_grid);
    float units2 = blur_radius + 5.0 / (2*blur_grid);
    units2 = units2 * units2;
    float units3 = blur_radius + 7.0 / (2*blur_grid);
    units3 = units3 * units3 * units3;

    // The magic numbers from Freeman's 1991 PAMI paper.
    float d2magicscale = 0.9213 / 2.0;
    float d3magicscale = 0.9780;
    float h2a_d3_coef = (1.0 / -8.0) * d3magicscale * units3;
    float h2a_d1_coef = ((2.254 - 1.5) / 2.0) * d3magicscale * units1;
    float h2b_d3_coef = (1.0 / -8.0) * d3magicscale * units3;
    float h2b_d1_coef = ((0.7515 - 0.5) / 2.0) * d3magicscale * units1;

	float g2_coef = units2 * d2magicscale;

    // Z component
  
    vec4  d1_texel = texture2DRect(d1_z_image, gl_FragCoord.xy);
    vec4  d2_texel = texture2DRect(d2_z_image, gl_FragCoord.xy);
    vec4  d3_texel = texture2DRect(d3_z_image, gl_FragCoord.xy);

    float g2a = d2_texel.x * g2_coef;
    float g2b = d2_texel.y * g2_coef;
    float g2c = d2_texel.w * g2_coef;
        
    float h2a = d3_texel.x*h2a_d3_coef + d1_texel.x*h2a_d1_coef; 
    float h2b = d3_texel.y*h2b_d3_coef + d1_texel.z*h2b_d1_coef;
    float h2c = d3_texel.z*h2b_d3_coef + d1_texel.x*h2b_d1_coef; 
    float h2d = d3_texel.w*h2a_d3_coef + d1_texel.z*h2a_d1_coef;
    
    float max_t = 0;
    float max_e = 0;
    
    for (float theta = 0; theta < 3.1415; theta += 0.5) {
        float cos_t = cos(theta);
        float sin_t = sin(theta);

        float kga = cos_t * cos_t;
        float kgb = -2 * cos_t * sin_t;
        float kgc = sin_t * sin_t;

        float kha = cos_t * cos_t * cos_t;
        float khb = -3 * cos_t * cos_t * sin_t;
        float khc = -3 * cos_t * sin_t * sin_t;
        float khd = -sin_t * sin_t * sin_t;

        float G2z = kga * g2a + kgb * g2b + kgc * g2c;
        float H2z = kha * h2a + khb * h2b + khc * h2c + khd * h2d;

        float E = G2z * G2z + H2z * H2z;
        if (E > max_e) {
            max_e = E;
            max_t = theta;
        }
    }
    
    float dirx = cos(max_t);
    float diry = -sin(max_t);
    float strength = max_e;

    float first = 0;
    float second = 0;

    vec4 out_color = vec4(strength, dirx, diry, 1.0);
    vec4 out_debug = vec4(first, second, 0.0, 1.0);
    vec4 out_orient = vec4(abs(dirx)*strength, abs(diry)*strength,0.0,1.0);

    gl_FragData[0] = out_color;
    gl_FragData[1] = out_debug;
    gl_FragData[2] = out_orient;
}

