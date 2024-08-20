#version 460 core

#define MAX_LIGHTS 2

#define LIGHT_TYPE_SUN   0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_PLANE 2
#define LIGHT_TYPE_SPOT  3

#define PHONG_EXPONENT 2

#define NUM_COARSE_STEPS 50
#define NUM_FINE_STEPS 50
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

void rayMarch(inout vec3 r, in float clipdist, in float marchstep) {
	vec4 lastssrpreproj, ssrpreproj = vec4(position, 1);
	vec3 ssr = proj(ssrpreproj), lastssr;
	float coarsedist = clipdist / float(NUM_COARSE_STEPS);
	vec3 rx = vec3(0);
	bool advance = false;
	for (uint i = 0; i < NUM_COARSE_STEPS; i++) {
		lastssr = ssr;
		lastssrpreproj = ssrpreproj;
		rx += r * coarsedist;
		ssrpreproj = vec4(position + rx, 1.);
		ssr = proj(ssrpreproj);
		/*
		if (ssr.z > 0.999) {
			r = vec3(-1, 0, 0);
			return;
		}
		*/
		if (ssr.x < 0 || ssr.x > 1 || ssr.y < 0 || ssr.y > 1 || ssr.z > 0.999) {
			advance = true;
		}
		if (advance || texture(ssrrdepthsampler, ssr.xy).r < ssr.z) {
			advance = false;
			float finedist = distance(lastssrpreproj, ssrpreproj) / float(NUM_FINE_STEPS);
			rx = vec3(0);
			vec3 newpos = lastssrpreproj.xyz;
			for (uint j = 0; j < NUM_FINE_STEPS; j++) {
				lastssr = ssr;
				lastssrpreproj = ssrpreproj;
				rx += r * finedist;
				ssrpreproj = vec4(newpos + rx, 1);
				ssr = proj(ssrpreproj);
				if (ssr.x < 0 || ssr.x > 1 || ssr.y < 0 || ssr.y > 1 || ssr.z > 0.999) {
					// r = vec3(-1, 0, 0);
					// return;
					advance = true;
				}
				if (advance || texture(ssrrdepthsampler, ssr.xy).r < ssr.z) {
					// r = ssr.xyz;
					// return;
					vec4 midpreproj;
					vec3 mid;
					for (uint k = 0; k < NUM_BIN_STEPS; k++) {
						midpreproj = lastssrpreproj + 0.5 * (ssrpreproj - lastssrpreproj);
						mid = proj(midpreproj);
						if (texture(ssrrdepthsampler, mid.xy).r > mid.z) {
							if (ssrOutOfBounds(mid)) {
								r = vec3(-1, 0, 0);
								return;
							}
							lastssrpreproj = midpreproj;
						}
						else ssrpreproj = midpreproj;
					}
					/*
					if (ssrOutOfBounds(mid)) {
						r = vec3(-1, 0, 0);
						return;
					}
					*/
					r = mid;
					return;
				}
			}
		}
	}
	r = vec3(0, -1, 0);
}

void rayMarch2(inout vec3 r, in float clipdist) {
	vec4 ssrpreproj = vec4(position, 1), lastssrpreproj;
	vec3 ssr = proj(ssrpreproj), lastssr;
	float diststep = clipdist / float(NUM_COARSE_STEPS), depthsample;
	vec3 rx = vec3(0);
	float a = diststep / float(NUM_COARSE_STEPS);
	// float a = clipdist / log(NUM_COARSE_STEPS);
	for (uint i = 0; i < NUM_COARSE_STEPS; i++) {
		lastssrpreproj = ssrpreproj;
		lastssr = ssr;
		// rx += r * diststep; // linear
		rx += r * a * pow(i, 2); // quadratic
		// below would be ideal but unsure of how to handle the first step
		/*
		if (i > 0) rx += r * a * log(i); // logarithmic (i believe this distributes most evenly on screen, b/c proj uses z divide
		else rx += r;
		*/
		ssrpreproj = vec4(position + rx, 1.);
		ssr = proj(ssrpreproj);
		depthsample = texture(ssrrdepthsampler, ssr.xy).r;
		if (ssrOutOfBounds(ssr) || depthsample < ssr.z) {
			vec4 midpreproj;
			vec3 mid;
			for (uint k = 0; k < NUM_BIN_STEPS; k++) {
				midpreproj = lastssrpreproj + 0.5 * (ssrpreproj - lastssrpreproj);
				mid = proj(midpreproj);
				depthsample = texture(ssrrdepthsampler, mid.xy).r;
				if (depthsample > mid.z) {
					lastssrpreproj = midpreproj;
				}
				else {
					// if (ssrOutOfBounds(mid) || depthsample == 1) {
					if (ssrOutOfBounds(mid)) {
						r = vec3(-1, 0, 0);
						return;
					}
					ssrpreproj = midpreproj;
				}
			}
			// if (ssrOutOfBounds(mid) || depthsample == 1) {
			if (ssrOutOfBounds(mid)) {
				r = vec3(-1, 0, 0);
				return;
			}
			r = mid;
			return;
		}
	}
	r = vec3(0, -1, 0);
}

void sampleSSRR(out vec4 combinedsample, in float eta, in vec3 n) {
	vec3 view = normalize(position - constants.cameraposition);
	vec3 reflectray = reflect(view, n),
		refractray = refract(view, n, eta);
	rayMarch2(reflectray, 1000.);
	// refract can have a lower clip (oceans tend to attenuate light faster than atmospheres)
	rayMarch2(refractray, 50.);
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
	if (reflectray.x != -1.) {
		reflectcolor = texture(ssrrcolorsampler, reflectray.xy);
	}
	else {
		reflectcolor = texture(environmentsampler, reflectray);
	}
	if (refractray.x != -1.) {
		refractcolor = texture(ssrrcolorsampler, refractray.xy);
		/*
		combinedsample = mix(refractcolor, reflectcolor, 1. / fresnel);
		combinedsample.a = 1.;
		*/
	}
	else {
		vec3 p = proj(vec4(position, 1));
		refractcolor = texture(ssrrcolorsampler, p.xy);
		/*
		combinedsample = fresnel * reflectcolor;
		combinedsample.a = 1. / fresnel;
		*/
	}
	combinedsample = mix(refractcolor, reflectcolor, 1. / fresnel);
}

void main() {
	vec3 normaldir = normalize(normal + texture(normalsampler, uv).xyz);
//    vec3 halfwaydir=normalize(lightuniformbuffer[0].position-position+constants.cameraposition-position);
	vec3 halfwaydir = normalize(lightuniformbuffer[0].position + constants.cameraposition - position);
	vec4 specular = lightuniformbuffer[0].color * pow(max(dot(normaldir, halfwaydir), 0.), PHONG_EXPONENT);

//    // TODO: implement blending when samples are not available; perhaps integrate with more advanced prediction of offscreen rays
//    // TODO: attenuation with depth is available using the length of the refracted ray
	vec4 reflectsample, refractsample;
	sampleSSRR(color, 1. / 1.33, normaldir);
}
