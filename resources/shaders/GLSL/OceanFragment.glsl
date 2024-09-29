#version 460 core

#define MAX_LIGHTS 2

#define LIGHT_TYPE_SUN   0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_PLANE 2
#define LIGHT_TYPE_SPOT  3

#define PHONG_EXPONENT 2

#define NUM_COARSE_STEPS 3
#define NUM_FINE_STEPS 3
#define NUM_BIN_STEPS 20

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;      //could we calc this in compute shader???
layout(location=2) in vec2 uv;

layout(push_constant) uniform PushConstants {
	mat4 cameravpmatrices;
	vec3 cameraposition;
	int numlights;
} constants;

layout(set=0, binding=0) uniform LightUniformBuffer {
	vec3 position;
	float intensity;
	vec3 forward;
	int lighttype;
	vec4 color;
} lightuniformbuffer[MAX_LIGHTS];

layout(set=0, binding=1) uniform samplerCube environmentsampler;
layout(set=1, binding=1) uniform sampler2D normalsampler;
layout(set=1, binding=2) uniform sampler2D ssrrcolorsampler;
layout(set=1, binding=3) uniform sampler2D ssrrdepthsampler;

layout(location=0) out vec4 color;

vec3 proj(vec4 v) {
	vec4 temp = constants.cameravpmatrices * v;
	return ((temp.xyz / temp.w) + vec3(1, 1, 0)) / vec3(2, 2, 1);
}

bool ssrOutOfBounds(vec3 r) {
	return r.x < 0 || r.x > 1 || r.y < 0 || r.y > 1 || r.z < 0 || r.z > 1;
}

bool clampSSR(inout vec3 r) {
	if (r.x < 0) {
		r.x = 0;
	} if (r.x > 1) {
		r.x = 1;
	} if (r.y < 0) {
		r.y = 0;
	} if (r.y > 1) {
		r.y = 1;
	} if (r.z < 0) {
		r.z = 0;
	} if (r.z > 1) {
		r.z = 1;
	}
	return r.x < 0 || r.x > 1 || r.y < 0 || r.y > 1 || r.z < 0 || r.z > 1;
}

void rayMarch2(inout vec3 r, in float clipdist) {
	vec4 ssrpreproj = vec4(position, 1), lastssrpreproj;
	vec3 ssr = proj(ssrpreproj), lastssr;
	float diststep = clipdist / float(NUM_COARSE_STEPS), depthsample;
	vec3 rx = vec3(0);
	// float a = diststep / float(NUM_COARSE_STEPS); // linear
	float a = clipdist / log(NUM_COARSE_STEPS + 1);

	/* Coarse Search, Logarithmically Distributed from 0 to clipdist */
	for (uint i = 0; i < NUM_COARSE_STEPS; i++) {
		lastssrpreproj = ssrpreproj;
		lastssr = ssr;
		// rx += r * diststep; // linear
		// rx += r * a * pow(i, 2); // quadratic
		rx += r * a * log(i + 1); // logarithmic
		ssrpreproj = vec4(position + rx, 1.);
		ssr = proj(ssrpreproj);
		if (ssrOutOfBounds(ssr)) break;
		depthsample = texture(ssrrdepthsampler, ssr.xy).r;
		if (depthsample < ssr.z) break;
	}

	/* Binary Search, from last in-bounds sample to next out-of-bounds sample */
	vec4 dir = normalize(ssrpreproj - lastssrpreproj);
	a = distance(ssrpreproj.xyz, lastssrpreproj.xyz) * 0.5;
	vec4 midpreproj = lastssrpreproj + a * dir;
	vec3 mid;
	for (uint k = 0; k < NUM_BIN_STEPS; k++) {
		a *= 0.5;
		mid = proj(midpreproj);
		depthsample = texture(ssrrdepthsampler, mid.xy).r;
		if (depthsample > mid.z) midpreproj += a * dir; 
		else midpreproj -= a * dir;
	}
	if (ssrOutOfBounds(mid)) {
		r = vec3(-1, 0, 0);
		return;
	}
	r = mid;
	return;
}

vec4 blendBounds(vec3 uv, vec4 primary, vec4 secondary) {
	vec4 result = primary;
	if (uv.x > 0.9) result = mix(secondary, result, (1 - uv.x) / 0.1);
	if (uv.y > 0.9) result = mix(secondary, result, (1 - uv.y) / 0.1);
	if (uv.x < 0.1) result = mix(secondary, result, uv.x / 0.1);
	if (uv.y < 0.1) result = mix(secondary, result, uv.y / 0.1);
	if (uv.z < 0.1) result = mix(secondary, result, uv.z / 0.1);
	return result;
}

void sampleSSRR(out vec4 combinedsample, in float eta, in vec3 n) {
	vec3 view = normalize(position - constants.cameraposition);
	vec3 reflectray = reflect(view, n),
		refractray = refract(view, n, eta);
	vec4 environmentsample = texture(environmentsampler, reflectray);
	rayMarch2(reflectray, 50.);
	// refract can have a lower clip (oceans tend to attenuate light faster than atmospheres)
	rayMarch2(refractray, 20.);
	float r0 = pow((1. - 1.33) / (1. + 1.33), 2.);
	float fresnel = r0 + (1. - r0) * pow(1. - dot(view, n), 5.);
	vec4 reflectcolor, refractcolor;
	/*
	if (reflectray.x == -1) combinedsample = vec4(0, 0.7, 0.7, 1);
	else if (reflectray.y == -1) combinedsample = vec4(0.9, 0.3, 0.3, 1);
	else combinedsample = texture(ssrrcolorsampler, reflectray.xy);
	*/
	// else combinedsample = vec4(reflectray.x, 0, reflectray.y, 1);
	/*
	if (refractray.x == -1) combinedsample = vec4(0, 0.7, 0.7, 1);
	else if (refractray.y == -1) combinedsample = vec4(0.9, 0.3, 0.3, 1);
	else combinedsample = texture(ssrrcolorsampler, refractray.xy);
	*/
	vec3 p = proj(vec4(position, 1));
	vec4 passthroughsample = texture(ssrrcolorsampler, p.xy);
	if (reflectray.x != -1.) {
		reflectcolor = texture(ssrrcolorsampler, reflectray.xy);
		reflectcolor = blendBounds(reflectray.xyz, reflectcolor, environmentsample);
	}
	else {
		reflectcolor = environmentsample;
	}
	if (refractray.x != -1.) {
		refractcolor = texture(ssrrcolorsampler, refractray.xy);
		refractcolor = blendBounds(refractray.xyz, refractcolor, passthroughsample);
	}
	else {
		refractcolor = passthroughsample;
	}
	combinedsample = mix(refractcolor, reflectcolor, 1. / fresnel);
}

void main() {
	vec3 normaldir = normalize(normal + texture(normalsampler, uv).xyz);

//     TODO: attenuation with depth is available using the length of the refracted ray
	vec4 reflectsample, refractsample;
	sampleSSRR(color, 1 / 1.33, normaldir);
}
