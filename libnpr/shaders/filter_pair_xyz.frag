/*****************************************************************************\

filter_pait_xyz.frag
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

uniform float g2_selection_scale;
uniform float h2_selection_scale;

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
    float g2magicscale = (0.9213 / 2.0) * g2_selection_scale;
    float h2magicscale = 0.9780 * h2_selection_scale;
    float h2a_d3_coef = (1.0 / -8.0) * h2magicscale * units3;
    float h2a_d1_coef = ((2.254 - 1.5) / 2.0) * h2magicscale * units1;
    float h2b_d3_coef = (1.0 / -8.0) * h2magicscale * units3;
    float h2b_d1_coef = ((0.7515 - 0.5) / 2.0) * h2magicscale * units1;

	float g2_coef = units2 * g2magicscale;

	// Apply filters separately for X, Y, Z components.
  
    vec4  d1_texel = texture2DRect(d1_x_image, gl_FragCoord.xy);
    vec4  d2_texel = texture2DRect(d2_x_image, gl_FragCoord.xy);
    vec4  d3_texel = texture2DRect(d3_x_image, gl_FragCoord.xy);

    float g2a = d2_texel.x * g2_coef;
    float g2b = d2_texel.y * g2_coef;
    float g2c = d2_texel.w * g2_coef;
        
    float h2a = d3_texel.x*h2a_d3_coef + d1_texel.x*h2a_d1_coef; 
    float h2b = d3_texel.y*h2b_d3_coef + d1_texel.z*h2b_d1_coef;
    float h2c = d3_texel.z*h2b_d3_coef + d1_texel.x*h2b_d1_coef; 
    float h2d = d3_texel.w*h2a_d3_coef + d1_texel.z*h2a_d1_coef;
    
    float C2_x, C3_x;
    C2_x = 0.5 * (g2a*g2a - g2c*g2c) + 0.46875 * (h2a*h2a - h2d*h2d)
         + 0.28125 * (h2b*h2b - h2c*h2c) + 0.1875 * (h2a*h2c - 
         h2b*h2d);
    C3_x = -g2a*g2b - g2b*g2c 
         - 0.9375*(h2c*h2d + h2a*h2b) - 
         1.6875*h2b*h2c - 0.1875*h2a*h2d;	

	
    float fC2_x = 0.5*(g2a*g2a - g2c*g2c);
    float fC3_x = -g2a*g2b - g2b*g2c;
    float sC2_x = 0.46875 * (h2a*h2a - h2d*h2d) 
         + 0.28125 * (h2b*h2b - h2c*h2c) + 0.1875 * (h2a*h2c - 
         h2b*h2d);
    float sC3_x = - 0.9375 * (h2c*h2d + h2a*h2b) - 
         1.6875*h2b*h2c - 0.1875*h2a*h2d;


		 
	d1_texel = texture2DRect(d1_y_image, gl_FragCoord.xy);
    d2_texel = texture2DRect(d2_y_image, gl_FragCoord.xy);
    d3_texel = texture2DRect(d3_y_image, gl_FragCoord.xy);

    g2a = d2_texel.x * g2_coef;
	g2b = d2_texel.y * g2_coef;
    g2c = d2_texel.w * g2_coef;
        
    h2a = d3_texel.x*h2a_d3_coef + d1_texel.x*h2a_d1_coef; 
    h2b = d3_texel.y*h2b_d3_coef + d1_texel.z*h2b_d1_coef;
    h2c = d3_texel.z*h2b_d3_coef + d1_texel.x*h2b_d1_coef; 
    h2d = d3_texel.w*h2a_d3_coef + d1_texel.z*h2a_d1_coef;
    
    float C2_y, C3_y;
    C2_y = 0.5 * (g2a*g2a - g2c*g2c) + 0.46875 * (h2a*h2a - h2d*h2d)
         + 0.28125 * (h2b*h2b - h2c*h2c) + 0.1875 * (h2a*h2c - 
         h2b*h2d);
    C3_y = -g2a*g2b - g2b*g2c 
         - 0.9375*(h2c*h2d + h2a*h2b) - 
         1.6875*h2b*h2c - 0.1875*h2a*h2d;
		 
	float fC2_y, fC3_y, sC2_y, sC3_y;
	fC2_y = 0.5*(g2a*g2a - g2c*g2c);
    fC3_y = -g2a*g2b - g2b*g2c;
    sC2_y = 0.46875 * (h2a*h2a - h2d*h2d) 
         + 0.28125 * (h2b*h2b - h2c*h2c) + 0.1875 * (h2a*h2c - 
         h2b*h2d);
    sC3_y = - 0.9375 * (h2c*h2d + h2a*h2b) - 
         1.6875*h2b*h2c - 0.1875*h2a*h2d;
		 

    d1_texel = texture2DRect(d1_z_image, gl_FragCoord.xy);
    d2_texel = texture2DRect(d2_z_image, gl_FragCoord.xy);
    d3_texel = texture2DRect(d3_z_image, gl_FragCoord.xy);

    g2a = d2_texel.x * g2_coef;
    g2b = d2_texel.y * g2_coef;
    g2c = d2_texel.w * g2_coef;
        
    h2a = d3_texel.x*h2a_d3_coef + d1_texel.x*h2a_d1_coef; 
    h2b = d3_texel.y*h2b_d3_coef + d1_texel.z*h2b_d1_coef;
    h2c = d3_texel.z*h2b_d3_coef + d1_texel.x*h2b_d1_coef; 
    h2d = d3_texel.w*h2a_d3_coef + d1_texel.z*h2a_d1_coef;
    
    float C2_z, C3_z;
    C2_z = 0.5 * (g2a*g2a - g2c*g2c) + 0.46875 * (h2a*h2a - h2d*h2d)
         + 0.28125 * (h2b*h2b - h2c*h2c) + 0.1875 * (h2a*h2c - 
         h2b*h2d);
    C3_z = -g2a*g2b - g2b*g2c 
         - 0.9375*(h2c*h2d + h2a*h2b) - 
         1.6875*h2b*h2c - 0.1875*h2a*h2d;

	float fC2_z, fC3_z, sC2_z, sC3_z;
	fC2_z = 0.5*(g2a*g2a - g2c*g2c);
    fC3_z = -g2a*g2b - g2b*g2c;
    sC2_z = 0.46875 * (h2a*h2a - h2d*h2d) 
         + 0.28125 * (h2b*h2b - h2c*h2c) + 0.1875 * (h2a*h2c - 
         h2b*h2d);
    sC3_z = - 0.9375 * (h2c*h2d + h2a*h2b) - 
         1.6875*h2b*h2c - 0.1875*h2a*h2d;

     
    float strength = sqrt(C2_x*C2_x + C2_y*C2_y + C2_z*C2_z + 
						  C3_x*C3_x + C3_y*C3_y + C3_z*C3_z);

    float tx = (C2_x + C2_y + C2_z) / strength;
    float ty = (C3_x + C3_y + C3_z) / strength;
    
    float theta = atan(ty, tx) / 2;
    
    float dirx = cos(theta);
    float diry = -sin(theta);


    float first = sqrt(fC2_x*fC2_x + fC2_y*fC2_y + fC2_z*fC2_z + 
					   fC3_x*fC3_x + fC3_y*fC3_y + fC3_z*fC3_z);

    float second = sqrt(sC2_x*sC2_x + sC2_y*sC2_y + sC2_z*sC2_z + 
					   sC3_x*sC3_x + sC3_y*sC3_y + sC3_z*sC3_z);    
    
    // Normalize by the square of the blur radius, to keep threshold values
    // more consistent over blur levels
    //float scalenorm = 1.0 / (blur_radius*blur_radius);
    //float scalenorm = 1.0 / blur_radius;
    float scalenorm = 1.0;

    vec4 out_color = vec4(strength*scalenorm, dirx, diry, 1.0);
    vec4 out_debug = vec4(first*scalenorm, second*scalenorm, 0.0, 1.0);
    vec4 out_orient = vec4(abs(dirx)*strength, abs(diry)*strength,0.0,1.0);

    gl_FragData[0] = out_color;
    gl_FragData[1] = out_debug;
    gl_FragData[2] = out_orient;
}
