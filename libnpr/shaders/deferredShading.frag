/*****************************************************************************\

deferredShafing.frag
Authors:
    Pierre Benard (pierre.benard@laposte.net),
    Forrester Cole (fcole@csail.mit.edu),
    Jingwan Lu (jingwanl@princeton.edu)
Copyright (c) 2012 Pierre Benard, Forrester Cole, Jingwan Lu

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform vec3 light_dir_world;
uniform sampler2D textureGrayScale;
uniform int toon_factor;
uniform int tex_height;
uniform int toon_mode;

varying vec4 vert_color;
varying vec3 vert_pos_camera;
varying vec3 vert_normal_camera;
varying vec3 vert_geom_flow;

vec3 phongShade(vec3 N, vec3 L, vec3 V,
           vec3 color, float Ka, float Kd, float Ks,
           vec3 lightCol, float shininess){

    vec3 final_color = color*Ka*lightCol;	//ambient

    float lambertTerm = dot(N,L);		//lambert

    if(lambertTerm > 0.0) {
        final_color += color*Kd*lambertTerm*lightCol; 	//diffuse
    }
    return final_color;
}

vec3 cellShade(float dotNL, vec3 color){
    float  toon_smooth = float(toon_factor)/tex_height;
    float  U = texture2D(textureGrayScale,vec2(dotNL,toon_smooth)).r;
    return U*color;
}

void main(void)
{
    vec3 light_color = vec3(1.0,1.0,1.0);

    vec3 N = normalize(vert_normal_camera);
    if(toon_mode == 1){
        float dotNL = dot(light_dir_world,N);
        gl_FragData[0].rgb = cellShade(dotNL,vert_color.rgb);
    }else{
        gl_FragData[0].rgb = phongShade(N,normalize(light_dir_world),normalize(vert_pos_camera),vert_color.xyz,0.4,0.9,2.0,light_color,128.0);
    }
    gl_FragData[0].a = 1.0;

    gl_FragData[1] = vec4(vert_pos_camera,1.0);

    gl_FragData[2] = vec4(vert_geom_flow,1.0);

    gl_FragData[3] = vec4(vert_pos_camera.z, vert_pos_camera.z,
                          vert_pos_camera.z,1);

    gl_FragData[4] = vert_color;
}
