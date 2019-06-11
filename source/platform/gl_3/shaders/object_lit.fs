#define LIGHTS_PER_OBJECT 2

uniform sampler2D u_albedo;

uniform bool u_lightEnabled;
uniform vec3 u_lightColors[LIGHTS_PER_OBJECT];
uniform float u_visibility;

varying vec3 v_normal;
varying vec2 v_uv;
varying vec3 v_lightDir[LIGHTS_PER_OBJECT];
varying float v_lightAtten[LIGHTS_PER_OBJECT];

void main()
{
    vec3 diffuse = texture2D(u_albedo, v_uv.xy).xyz;

    if (u_lightEnabled)
    {
        float zenith = 0.33;
        float horizon = 0.15;
        
        float ambient = mix(horizon, zenith, v_normal.z);
        
        vec3 lighting = vec3(ambient, ambient, ambient);
        
        for (int i = 0; i < LIGHTS_PER_OBJECT; ++i)
        {
            float halfLambert = dot(normalize(v_normal), normalize(v_lightDir[i])) * 0.5 + 0.5;
            halfLambert *= halfLambert;
            
            lighting += u_lightColors[i] * v_lightAtten[i] * halfLambert;
        }
        
        diffuse *= clamp(lighting, 0.0, 1.0);
    }

    gl_FragColor = vec4(diffuse * u_visibility, 1.0);
}
