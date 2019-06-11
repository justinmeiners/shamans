#define ATLAS_COUNT 4

uniform sampler2D u_atlas[ATLAS_COUNT];

varying vec4 v_color;
varying vec2 v_uv;
varying float v_atlasId;

void main()
{
    vec4 color = vec4(0.0);
    
    for (int i = 0; i < ATLAS_COUNT; ++i)
    {
        vec4 atlasColor = texture2D(u_atlas[i], v_uv);
        float factor = max(v_atlasId - float(i - 1), 0.0);
        color = mix(color, atlasColor, factor);
    }

    gl_FragColor = color * v_color;
}
