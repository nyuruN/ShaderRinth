#version 330 core

out vec4 FragColor;

uniform float u_time;
uniform vec2 u_resolution;
uniform sampler2D u_cat;

mat2x2 rot2D(float a) {
  return mat2x2(cos(a), sin(a),
            sin(a), -cos(a));
}

void main() {
  vec3 col = vec3(0.5f);
  vec2 uv = (2.0*gl_FragCoord.xy - u_resolution.xy)/u_resolution.x;

  uv *= rot2D(u_time);

  col.rgb = texture(u_cat, uv*(1+round(2*fract(.7*u_time)))).rgb;
  col.x += sin(u_time*5.) * 0.2f + 0.1f;
  col.y += sin(u_time*1.2) * 0.2f + 0.1f;
  col.z += sin(u_time*.5) * 0.4f + 0.1f;

  FragColor = vec4(col, 1.0f);
}
