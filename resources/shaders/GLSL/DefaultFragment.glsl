#version 460 core

#define MAX_LIGHTS 2

#define LIGHT_TYPE_SUN   0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_PLANE 2
#define LIGHT_TYPE_SPOT  3

#define PHONG_EXPONENT 64

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec3 computednorm;
layout(location=3) in vec2 diffuseuv;
layout(location=4) in vec2 normaluv;
layout(location=5) in vec3 shadowposition[MAX_LIGHTS];

layout(push_constant) uniform PushConstants{
	mat4 cameravpmatrices;
	vec3 camerapos;
	vec2 standinguv;
	uint numlights;
} constants;
layout(set=0, binding=0) uniform LightUniformBuffer{
	mat4 vpmatrices;
	mat4 lspmatrix;
	vec3 position;
	float intensity;
	vec3 forward;
	int lighttype;
	vec4 color;
} lub[MAX_LIGHTS];
layout(set=0, binding=1) uniform samplerCube environmentsampler;
layout(set=1, binding=1) uniform sampler2D diffusesampler;
layout(set=1, binding=2) uniform sampler2D normalsampler;
layout(set=1, binding=3) uniform sampler2D heightsampler;
layout(set=1, binding=4) uniform sampler2DShadow shadowsampler[MAX_LIGHTS];

layout(location=0) out vec4 color;

void main() {
	const vec4 ambient = vec4(0.1, 0.1, 0.1, 1.);
	vec4 diffuse = texture(diffusesampler, diffuseuv);
	vec3 normaldir = normalize(normal + computednorm + texture(normalsampler, normaluv).xyz);
	vec3 lightdir, halfwaydir;
	float lambertian, specular;
	for (uint x = 0; x < MAX_LIGHTS; x++){
		if (x < constants.numlights){
			// float shadow = texture(shadowsampler[x], shadowposition[x] + vec3(0., 0., clamp(0.005 * tan(acos(dot(normaldir, vec3(1.)))), 0., 0.01)));
			float shadow = texture(shadowsampler[x], shadowposition[x]);
			if (shadow == 0) {
				color += lub[x].color * diffuse * ambient;
				continue;
			}
			// forward should probably be pre-normalized
			lightdir = (lub[x].lighttype == LIGHT_TYPE_POINT ? normalize(lub[x].position - position) : normalize(-lub[x].forward));
			lambertian = max(dot(lightdir, normaldir), 0);
			if (lambertian > 0) {
				halfwaydir = normalize(lightdir + normalize(constants.camerapos - position));
				specular = pow(max(dot(halfwaydir, normaldir), 0), PHONG_EXPONENT);
				if (lub[x].lighttype != LIGHT_TYPE_SUN) {
					float lightdistsq = pow(length(lub[x].position - position), 2);
					lambertian /= lightdistsq;
					specular /= lightdistsq;
				}
			}
			else {
				specular = 0;
			}
			color += lub[x].color * (lub[x].intensity * (diffuse * lambertian * shadow + specular * shadow) + diffuse * ambient);
		}
		else break;
	}
	color.a = 1;
}
