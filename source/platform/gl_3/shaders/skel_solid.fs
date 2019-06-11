
uniform vec4 u_color;
uniform float u_visibility;

void main()
{
    gl_FragColor = vec4(u_color.xyz * u_visibility, u_color.a);
}

