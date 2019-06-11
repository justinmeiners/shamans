
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

attribute vec3 a_vertex;

varying float v_visibility;

void main()
{
    gl_Position = u_projection * u_view * u_model * vec4(a_vertex, 1.0);
}
