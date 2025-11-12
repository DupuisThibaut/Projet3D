#version 330 core

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D textureSampler;
uniform int materialType;
uniform vec3 color;

uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform float lightIntensity;

out vec4 FragColor;

void main() {
vec3 albedo;
    if (materialType == 0) {
        albedo = texture(textureSampler, TexCoords).rgb;
    } else if (materialType == 1) {
        albedo = color;
    } else {
        albedo = vec3(1.0);
    }

    // lighting
    vec3 norm = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos); // direction to light
    vec3 ambient = 0.1 * lightColor * albedo * lightIntensity;

    float diff = max(dot(norm, L), 0.0);
    vec3 diffuse = diff * lightColor * albedo * lightIntensity;

    // simple specular (Blinn-Phong)
    vec3 viewDir = normalize(-FragPos); // camera at origin in view-space
    vec3 halfDir = normalize(L + viewDir);
    float specAngle = max(dot(norm, halfDir), 0.0);
    float shininess = 32.0;
    vec3 specular = pow(specAngle, shininess) * lightColor * lightIntensity;

    vec3 result = ambient + diffuse + specular;

    FragColor = vec4(result, 1.0);
}
