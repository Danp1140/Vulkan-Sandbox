#version 460 core

layout(location=0) in vec2 uv;

layout(binding=0, set=0) uniform sampler2D texturesampler;

layout(location=0) out vec4 color;

void main(){
//    color=vec4(vec3(texture(texturesampler, vec2(uv.x, -uv.y)).r), 1.);
//    color=vec4(texture(texturesampler, vec2(uv.x, -uv.y)).rgb, 1.);
//    float rval=texture(texturesampler, vec2(uv.x, -uv.y)).r;
//    color=vec4(0., 0., 1., 1.)*rval+vec4(0., 1., 0., 1.)*(1.-rval);
    /*float depth = texture(texturesampler, vec2(uv.x, -uv.y)).r;
    color = vec4(0., 0., 1., 1.) * depth + vec4(0., 1., 0., 1.) * (1. - depth);
    if (depth > 0.999f && depth < 1.f) {
        color = vec4(1., 0., 0., 1.);
    }*/
	// color = texture(texturesampler, uv * vec2(1., -1.));
	color = texture(texturesampler, 2 * uv * vec2(1., -1.));
	color.a = 1.;
}
