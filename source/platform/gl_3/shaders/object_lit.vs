
#define LIGHTS_PER_OBJECT 2

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

uniform bool u_lightEnabled;
uniform vec3 u_lightPoints[LIGHTS_PER_OBJECT];
uniform float u_lightRadii[LIGHTS_PER_OBJECT];

attribute vec3 a_vertex;
attribute vec3 a_normal;
attribute vec2 a_uv0;

varying vec3 v_normal;
varying vec2 v_uv;
varying vec3 v_lightDir[LIGHTS_PER_OBJECT];
varying float v_lightAtten[LIGHTS_PER_OBJECT];

void main()
{
    vec4 worldVert = u_model * vec4(a_vertex, 1.0);
    
    v_normal = (u_model * vec4(a_normal, 0.0)).xyz;
    v_uv = a_uv0;
    
    if (u_lightEnabled)
    {
        for (int i = 0; i < LIGHTS_PER_OBJECT; ++i)
        {
            v_lightDir[i] = u_lightPoints[i] - worldVert.xyz;
            vec3 normDir = v_lightDir[i] / u_lightRadii[i];
            v_lightAtten[i] = max(1.0 - dot(normDir, normDir), 0.0);
        }
    }
    
    gl_Position = u_projection * u_view * worldVert;
}
