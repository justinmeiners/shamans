uniform mat4 u_projection;

attribute vec2 a_vertex;
attribute vec2 a_uv0;
attribute vec4 a_color;

attribute float a_atlasId;

varying vec4 v_color;
varying vec2 v_uv;
varying float v_atlasId;

void main()
{ 
    v_color = a_color;
    v_uv = a_uv0;
    v_atlasId = a_atlasId;
    
    gl_Position = u_projection * vec4(a_vertex, 0.0, 1.0);
}
