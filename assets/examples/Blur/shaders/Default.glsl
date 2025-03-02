#version 330 core

out vec4 FragColor;

uniform float u_mode;
uniform vec2 u_resolution;
uniform sampler2D u_cat;
uniform float rangeMult = 4.0;

const int wLen = 10;

float weights[wLen] = float[wLen]( 0.2, 0.18, 0.15, 0.12, 0.10, 0.08, 0.07, 0.04, 0.02, 0.01);
float weightMult = 0.27;

void main() {
  vec2 uv = gl_FragCoord.xy / u_resolution.xy;
  FragColor = texture(u_cat, uv) * 0.5;
  if (u_mode < 1) {
    for (int i = 0; i < wLen; ++i) {
      FragColor += texture(u_cat, uv + vec2(rangeMult / u_resolution.x, 0)) * weights[i] * weightMult;
      FragColor += texture(u_cat, uv - vec2(rangeMult / u_resolution.x, 0)) * weights[i] * weightMult;
    }
  } else {
    for (int i = 0; i < wLen; ++i) {
      FragColor += texture(u_cat, uv + vec2(0, rangeMult / u_resolution.y)) * weights[i] * weightMult;
      FragColor += texture(u_cat, uv - vec2(0, rangeMult / u_resolution.y)) * weights[i]  * weightMult;
    }
  }
}
