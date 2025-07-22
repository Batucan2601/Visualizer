#version 330

in vec3 fragNormal;
in vec3 fragPosition;

uniform vec3 lightPos;
uniform vec4 baseColor;

out vec4 finalColor;

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPosition);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 color = baseColor.rgb * diff + 0.1;  // basic ambient
    finalColor = vec4(color, baseColor.a);
}
