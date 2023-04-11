#version 330 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

#define NUM_OF_POINT_LIGHTS 2

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirectionalLight {
    vec3 direction;

    vec3 specular;
    vec3 ambient;
    vec3 diffuse;
};

struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Spotlight {
    vec3 position;
    vec3 direction;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float cutOff;
    float outerCutOff;
    };

in vec3 Normal;
in vec3 FragPos;
in vec2 texCoords;

uniform DirectionalLight directionalLight;
uniform Material material;
uniform vec3 viewPos;
uniform Spotlight spotlight;
uniform PointLight pointLights[NUM_OF_POINT_LIGHTS];
uniform bool spotlightOn;

vec3 calcDirectionalLight(DirectionalLight light, vec3 norm, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);

    vec3 ambient = texture(material.diffuse, texCoords).xxx * light.ambient;
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, texCoords).xxx;
    vec3 specular = light.specular * spec * texture(material.specular, texCoords).xxx;

    return (ambient + diffuse + specular);
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

    float d = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear*d + light.quadratic*(d*d));

    vec3 ambient = texture(material.diffuse, texCoords).rgb * light.ambient;
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, texCoords).rgb;
    vec3 specular = light.specular * spec * texture(material.specular, texCoords).rgb;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}

vec3 calcSpotlight(Spotlight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

    float d = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear*d + light.quadratic*d*d);

    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, texCoords).rgb;
    vec3 specular = light.specular * spec * texture(material.specular, texCoords).rgb;
    vec3 ambient = texture(material.diffuse, texCoords).rgb * light.ambient;
    diffuse *= intensity * attenuation;
    specular *= intensity* attenuation;
    ambient *= intensity* attenuation ;
    return (diffuse + specular + ambient);

}
void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = vec3(0.0f, 0.0f, 0.0f);
    result += calcDirectionalLight(directionalLight, norm, viewDir);
    for (int i = 0; i < NUM_OF_POINT_LIGHTS; i++){
        result += calcPointLight(pointLights[i], norm, FragPos, viewDir);
    }

    if(spotlightOn){
         result += calcSpotlight(spotlight, norm, FragPos, viewDir);
    }
   FragColor = vec4(result, 1.0);

   float brightness = dot(FragColor.rgb, vec3(0.2126, 0.0, 0.222));
   if(brightness > 1.0)
        BrightColor = vec4(FragColor.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}