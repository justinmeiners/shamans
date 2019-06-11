
uniform sampler2D u_albedo;
uniform sampler2D u_lightmap;

varying vec2 v_uvs[2];
varying float v_visibility;

void main()
{
    vec3 diffuse = texture2D(u_albedo, v_uvs[0].xy).xyz;
    
    vec3 lumen = texture2D(u_lightmap, vec2(v_uvs[1].x, 1.0 - v_uvs[1].y)).xyz;
    diffuse *= lumen * v_visibility * (gl_FrontFacing ? 1.0 : 0.0);
    
    gl_FragColor = vec4(diffuse, 1.0);
}
