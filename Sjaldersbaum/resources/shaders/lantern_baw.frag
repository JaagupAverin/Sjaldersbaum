uniform sampler2D texture;
uniform vec2 canvas_size;

uniform vec2  source;      // the origin of light
uniform float radius;      // the extent of light
uniform float brightness;  // multiplies final color

const float COLOR_FACTOR = 0.2;

void main()
{
    // m applies fading based on radius:
    float d = distance(gl_FragCoord.xy, source);
    float x = d / radius;
    float m = 1.0 / (1.0 + pow(x / 0.5, 6.0));

    vec2 px_pos = gl_FragCoord.xy / canvas_size;
    vec4 outColor = texture2D(texture, px_pos);
    
    float grey = 0.2 * outColor.r + 0.7 * outColor.g + 0.2 * outColor.b;

    gl_FragColor =
        vec4(((outColor.r * 1.2 * COLOR_FACTOR + grey * (1.15 - COLOR_FACTOR) - 0.2)) * brightness * m,
             ((outColor.g * 1.0 * COLOR_FACTOR + grey * (1.15 - COLOR_FACTOR) - 0.2)) * brightness * m,
             ((outColor.b * 0.8 * COLOR_FACTOR + grey * (1.15 - COLOR_FACTOR) - 0.2)) * brightness * m,
             1.0);
}