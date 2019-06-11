
#define OBSERVERS_MAX 6

/* Uber Visibility */
uniform vec3 u_observerPositions[OBSERVERS_MAX];
uniform float u_observerRadiiSq[OBSERVERS_MAX];

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;


attribute vec3 a_vertex;
attribute vec3 a_normal;
attribute vec2 a_uv0;
attribute vec2 a_uv1;

varying vec2 v_uvs[2];
varying float v_visibility;

void main()
{
    v_uvs[0] = a_uv0;
    v_uvs[1] = a_uv1;
    
    highp vec4 worldVert = u_model * vec4(a_vertex, 1.0);
    
    float visibility = 0.0;
    
    for (int i = 0; i < OBSERVERS_MAX; ++i)
    {
        vec3 dirVec = u_observerPositions[i] - worldVert.xyz;
        float distSq = dot(dirVec, dirVec) + 0.000001;
        float value = u_observerRadiiSq[i] / (distSq * 1.8);
        visibility += value * value;
    }
    
    v_visibility = min(visibility * visibility, 1.0);
    
    gl_Position = u_projection * u_view * worldVert;
}
