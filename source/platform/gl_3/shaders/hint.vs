uniform mat4 u_projection;
uniform mat4 u_view;

attribute vec3 a_vertex;
attribute vec3 a_color;

varying vec3 v_color;

void main()
{ 
    v_color = a_color;
    
    gl_Position = u_projection * u_view * vec4(a_vertex, 1.0);
}
