#define M_PI 3.141592
#define M_2_PI 6.283184

#define EFFECT_BLOOD 1
#define EFFECT_DEATH 2
#define EFFECT_PURPLE_MAGIC 3
#define EFFECT_RED_MAGIC 4
#define EFFECT_HEAL 5
#define EFFECT_MIND_CONTROL 6
#define EFFECT_SPAWN 7
#define EFFECT_TELEPORT 8
#define EFFECT_ACID 9

#define CONTROL_POINTS_MAX 4

attribute float a_index;

uniform vec3 u_controlPoints[CONTROL_POINTS_MAX];

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

uniform int u_partEffect;
uniform int u_partCount;
uniform float u_time;

uniform float u_nearPlane;

varying vec4 v_color;
varying float v_sprite;


float rand2(vec2 co)
{
    return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

float quadIn(float t)
{
    return t*t;
}

float cubicIn(float t)
{
    return t*t*t;
}

float cubicOut(float t)
{
    t -= 1.0;
    return t*t*t + 1.0;
}

float expIn(float t)
{
    return pow(2.0, 10.0 * (t-1.0));
}

float expOut(float t)
{
    return -pow(2.0, -10.0 * t) + 1.0;
}


void main()
{
    int index = int(a_index);
    float value = a_index / float(u_partCount);
    float pointSize = 0.0;
    
    vec4 world_position = vec4(0.0, 0.0, 0.0, 1.0);
    
    if (u_partEffect == EFFECT_BLOOD)
    {
        float x = rand2(vec2(value, 0.5));
        float y = rand2(vec2(value, 0.25));
        float z = rand2(vec2(value, 0.1));
                
        float red = rand2(vec2(value, 0.2));
        
        v_color = vec4(red * 0.5 + 0.5, 0.0, 0.0, 1.0);
        
        vec3 dir = vec3(x * 2.0 - 1.0, y * 2.0 - 1.0, z * 2.0 - 1.0);
        
        vec3 local_position = dir * cubicOut(u_time) * 0.8;
        world_position = u_model * vec4(local_position, 1.0);
        v_sprite = 3.0;
        pointSize = 18.0;
        
    }
    else if (u_partEffect == EFFECT_DEATH)
    {
        if (index == 0)
        {
            v_color = vec4(1.0, 1.0, 1.0, 1.0 - u_time);
            v_sprite = 2.0;
            world_position = u_model * vec4(0.0, 0.0, 0.0, 1.0);

            pointSize = 120.0;
        }
        else
        {
            float x = cos(value * M_2_PI);
            float y = sin(value * M_2_PI);
            
            float size = rand2(vec2(value, 0.21));
            float gray = rand2(vec2(value, 0.2)) * 0.2;
            
            v_color = vec4(0.4 - gray, 0.03, 0.35, 1.0);
            
            vec3 dir = vec3(x, y, 0.0);
            
            vec3 local_position = dir * (cubicOut(u_time) + 0.5 * size) * (size * 2.0 + 2.0);
            world_position = u_model * vec4(local_position, 1.0);
            
            if (size > 0.5)
            {
                v_sprite = 0.0;
            }
            else
            {
                v_sprite = 1.0;
            }

            pointSize = (30.0 + 15.0 * size) * (1.0 - expIn(u_time));
        }
    }
    else if (u_partEffect == EFFECT_PURPLE_MAGIC)
    {
        float modifier = rand2(vec2(value, 0.15));
        float modifier2 = rand2(vec2(value, 0.08));
        
        vec3 point = u_controlPoints[0];
        point.x += modifier;
        point.y += modifier2;
        
        vec4 origin = u_model * vec4(0.0, 0.0, 0.0, 1.0);
        world_position = vec4(mix(origin.xyz, point, quadIn(value)), 1.0);

        v_color = vec4(1.0 - modifier * 0.5, 0.0, 1.0 - modifier2 * .25, 1.0);
        v_sprite = 3.0;
        
        pointSize = (1.0 - u_time) * 25.0 + 15.0 * modifier;
    }
    else if (u_partEffect == EFFECT_RED_MAGIC)
    {
        float modifier = rand2(vec2(value, 0.15));
        float modifier2 = rand2(vec2(value, 0.08));
        
        vec3 point = u_controlPoints[0];
        point.x += modifier;
        point.y += modifier2;
        
        vec4 origin = u_model * vec4(0.0, 0.0, 0.0, 1.0);
        world_position = vec4(mix(origin.xyz, point, quadIn(value)), 1.0);

        v_color = vec4(1.0 - modifier * 0.5, 0.0, modifier2 * .1, 1.0);
        v_sprite = 3.0;
        
        pointSize = (1.0 - u_time) * 15.0 + 10.0 * modifier;
    }
    else if (u_partEffect == EFFECT_HEAL)
    {
        vec4 origin = u_model * vec4(0.0, 0.0, 0.0, 1.0);
        world_position = vec4(mix(origin.xyz, u_controlPoints[0], value), 1.0);

        float col = rand2(vec2(value, 0.43));
        v_color = vec4(1.0, 0.3 + col * 0.2, 0.3 + col * 0.2, 1.0);

        v_sprite = 3.0;
        pointSize = 18.0 + 4.0 * col - u_time * 9.0;
    }
    else if (u_partEffect == EFFECT_MIND_CONTROL)
    {
        float x = rand2(vec2(value, 0.5));
        float y = rand2(vec2(value, 0.25));
        float z = rand2(vec2(value, 0.1));
        
        float white = rand2(vec2(value, 0.2));
        
        v_color = vec4(1.0, white * 0.25 + 0.25, 0.0, 1.0);
        
        vec3 start = vec3(x * 2.0 - 1.0, y * 2.0 - 1.0, z * 1.0 - 0.5);
        
        vec3 local_position = start * 2.0 + vec3(0.0, 0.0, white * 7.0) * smoothstep(0.0, 1.0, u_time);
        world_position = u_model * vec4(local_position, 1.0);

        v_sprite = 6.0;
        pointSize = 12.0 + white * 5.0;
    }
    else if (u_partEffect == EFFECT_SPAWN)
    {
        float dist = clamp(u_time + value / 2.0, 0.0, 1.0);
        
        float x = cos((value + u_time) * M_PI * 3.0) * 3.0 * (1.0 - u_time);
        float y = sin((value + u_time) * M_PI * 3.0) * 3.0 * (1.0 - u_time);

        float z = value * 6.0 - 2.0;
        
        v_sprite = mix(8.0, 6.0, mod(a_index, 2.0));
        v_color = vec4(0.6 + ((1.0 - u_time) * 0.5), 0.0, 1.0  * rand2(vec2(value, 0.7)) * 1.5, 1.0);

        pointSize = 16.0 + 10.0 * rand2(vec2(value, 0.3));
        
        world_position = u_model * vec4(x, y, z, 1.0);
    }
    else if (u_partEffect == EFFECT_TELEPORT)
    {
        float theta = rand2(vec2(value, 0.1)) * M_PI * 2.0;
        float phi = rand2(vec2(value, 0.09)) * M_PI * 0.55;
        float hue = rand2(vec2(value, 0.02));

        float dist = 1.0 + (1.0 - cubicOut(u_time)) * (4.0 * rand2(vec2(value, 0.06)) + 1.0);

        float r = dist * sin(phi);
        
        float z = 3.0 * dist * cos(phi);
        float x = r * cos(theta);
        float y = r * sin(theta);
        
        v_sprite = 5.0;
        v_color = vec4(hue * 0.2, 0.0, hue * 0.5, 1.0);
        
        pointSize = 18.0 + 3.0 * phi;
        world_position = u_model * vec4(x, y, z, 1.0);
    }
    else if (u_partEffect == EFFECT_ACID)
    {
        float theta = rand2(vec2(value, 0.241)) * M_PI * 2.0;
        float phi = rand2(vec2(value, 0.03)) * M_PI * 0.55;
        float hue = rand2(vec2(value, 0.2));
        
        float dist = 10.0 * cubicOut(u_time) * (rand2(vec2(value, 0.06)) + 0.5);
        
        float r = dist * sin(phi);
        
        float z = dist * cos(phi);
        float x = r * cos(theta);
        float y = r * sin(theta);
        
        v_sprite = 8.0;
        v_color = vec4(0.0, hue * 0.5 + 0.5, theta * 0.1, 1.0);
        
        pointSize = 50.0 + 30.0 * hue;
        world_position = u_model * vec4(x, y, z, 1.0);
    }
    
    
    gl_Position = u_projection * u_view * world_position;
    gl_PointSize = (pointSize * u_nearPlane * 10.0) / gl_Position.w;
}
