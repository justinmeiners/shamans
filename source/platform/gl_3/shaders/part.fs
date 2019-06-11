#define SPRITES_PER_ROW 16.0
#define SPRITE_SIZE (1.0 / SPRITES_PER_ROW)

uniform sampler2D u_atlas;

varying vec4 v_color;
varying float v_sprite;

void main()
{
    
    vec2 uv = vec2((SPRITE_SIZE * v_sprite) + (gl_PointCoord.x / SPRITES_PER_ROW), gl_PointCoord.y);
    
    vec4 diffuse = texture2D(u_atlas, uv);
    
    if (diffuse.a < 0.01)
    {
        discard;
    }
    
    gl_FragColor = v_color * diffuse;
}
