#version 330 core

out vec4 FragColor;

uniform float u_time;
uniform vec2 u_resolution;

void main() {
  vec3 col = vec3(0.5f);
  vec2 uv = gl_FragCoord.xy / u_resolution;

  col.xy = uv;
  col.z = sin(u_time) * 0.5f + 0.5f;

  FragColor = vec4(col, 1.0f);
}
