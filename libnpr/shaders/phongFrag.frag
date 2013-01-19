/*****************************************************************************\

phongFrag.frag
Authors: Pierre Benard (pierre.benard@laposte.net)
Copyright (c) 2012 Benard Pierre

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform vec3 light_dir_world;

varying vec4 vert_color;
varying vec4 vert_pos_view;
varying vec3 vert_normal_view;

vec3 shade(vec3 N, vec3 L, vec3 V, 
           vec3 color, float Ka, float Kd, float Ks,
           vec3 lightCol, float shininess){

    vec3 final_color = color*Ka*lightCol;	//ambient

    float lambertTerm = dot(N,L);		//lambert

    if(lambertTerm > 0.0) {
        final_color += color*Kd*lambertTerm*lightCol; 	//diffuse

        vec3 R = reflect(L,N);
        float specular = pow(max(dot(R,V), 0.0), shininess);
        final_color += color*Ks*lightCol*(specular);	//specular
    }

    return final_color;
}

void main(void)
{
    vec3 light_color = vec3(1.0,1.0,1.0);

    gl_FragColor.rgb = shade(normalize(vert_normal_view), normalize(light_dir_world),normalize(vert_pos_view).xyz,vert_color.xyz,0.4,0.9,2.0,light_color,128.0);
    gl_FragColor.a = 1.0;
}
