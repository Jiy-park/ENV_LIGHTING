#version 330 core

out vec4 fragColor;

in vec3 normal;
in vec3 position;

uniform vec3 cameraPos;
uniform samplerCube skybox;

void main() {
    vec3 I = normalize(position - cameraPos);
    vec3 R = reflect(I, normalize(normal));
    fragColor = vec4(texture(skybox, R).rgb, 1.0);
}
////////////////////////////////////env_map
////////////////////////////////////lighting
#version 330 core
in vec3 normal;
in vec2 texCoord;
in vec3 position;
out vec4 fragColor;
 
uniform float env_scale;

uniform vec3 viewPos;
uniform samplerCube skybox;

struct Light {
    vec3 position;
    vec3 direction;
    vec2 cutoff;
    vec3 attenuation;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Light light;
 
struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};
uniform Material material;

	
void main() {
    vec3 texColor = texture2D(material.diffuse, texCoord).xyz;
    vec3 ambient = texColor * light.ambient;
 	
    float dist = length(light.position - position);
    vec3 distPoly = vec3(1.0, dist, dist*dist);
    float attenuation = 1.0 / dot(distPoly, light.attenuation);
    vec3 lightDir = (light.position - position) / dist;

    vec3 result = ambient;
    float theta = dot(lightDir, normalize(-light.direction));
    float intensity = clamp((theta - light.cutoff[1]) / (light.cutoff[0] - light.cutoff[1]),  0.0, 1.0);
 
    float snv_scale =0.0;


    if (intensity>0.0) {
        vec3 I = normalize(position - viewPos);
        vec3 R = reflect(I, normalize(normal));
        vec3 pixelNorm = normalize(normal);
        float diff = max(dot(pixelNorm, lightDir), 0.0);
        vec3 diffuse = diff * texColor * light.diffuse;   
        vec3 specColor = texture2D(material.specular, texCoord).xyz;
        vec3 viewDir = normalize(viewPos - position);
        vec3 reflectDir = reflect(-lightDir, pixelNorm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        vec3 specular = spec * specColor * light.specular;    
        
        result += ((diffuse + specular) * texture(skybox, R).rgb) * intensity * env_scale;
    }

    result *= attenuation;
    fragColor = vec4(result, 1.0);
    //fragColor = vec4(vec3(gl_FragCoord.z), 1.0);
}