#define MAX_JOINTS 48
#define MAX_WEIGHTS 3
#define LIGHTS_PER_OBJECT 2

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

attribute vec3 a_normal;
attribute vec2 a_uv0;

uniform vec3 u_jointOrigins[MAX_JOINTS];
uniform vec4 u_jointRotations[MAX_JOINTS];

uniform vec3 u_lightPoints[LIGHTS_PER_OBJECT];
uniform float u_lightRadii[LIGHTS_PER_OBJECT];

attribute vec3 a_weightJoints;
attribute vec4 a_weight0;
attribute vec4 a_weight1;
attribute vec4 a_weight2;

varying vec3 v_normal;
varying vec2 v_uv;

varying vec3 v_lightDir[LIGHTS_PER_OBJECT];
varying float v_lightAtten[LIGHTS_PER_OBJECT];

vec3 quatRotate(const vec4 quat, const vec3 vec)
{
    vec3 t = cross(quat.xyz, vec) * 2.0;
    return vec + t * quat.w + cross(quat.xyz, t);
}

void main()
{
    vec3 vert = vec3(0.0, 0.0, 0.0);
    vec3 norm = vec3(0.0, 0.0, 0.0);
    
    vec4 weights[MAX_WEIGHTS];
    weights[0] = a_weight0;
    weights[1] = a_weight1;
    weights[2] = a_weight2;
    
    for (int i = 0; i < MAX_WEIGHTS; ++i)
    {
        int joint = int(a_weightJoints[i]);
        vec3 transformed = u_jointOrigins[joint] + quatRotate(u_jointRotations[joint], weights[i].xyz);
        vert += transformed * weights[i].w;
        transformed = quatRotate(u_jointRotations[joint], a_normal);
        norm += transformed * weights[i].w;
    }
        
    v_normal = norm;
    v_uv = a_uv0;
    
    vec4 worldVert = u_model * vec4(vert, 1.0);
    
    for (int i = 0; i < LIGHTS_PER_OBJECT; ++i)
    {
        v_lightDir[i] = u_lightPoints[i] - worldVert.xyz;
        vec3 normDir = v_lightDir[i] / u_lightRadii[i];
        v_lightAtten[i] = max(1.0 - dot(normDir, normDir), 0.0);
    }

    gl_Position = u_projection * u_view * worldVert;
}
