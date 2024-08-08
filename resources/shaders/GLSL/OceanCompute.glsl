#version 460 core

layout(local_size_x=1, local_size_y=1) in;

layout(push_constant) uniform PushConstants{
    float time;
}constants;
layout(set=0, binding=0, r32f) uniform image2D imageout;
/*
 * okay, so this is super janky. i tried to use c++-side VK_FORMAT_R32G32B32A32_SFLOAT and glsl-side rgba32f,
 * but imageLoad simply returned 0 for all of velocitymap. so, i tried a few different formats, many aren't
 * supported for image store/load ops, so i finally went with VK_FORMAT_R32_SFLOAT and rgba8...it seems to at
 * least mostly work, but its kinda wack...i should investigate fixes to do this more properly, issue is most
 * likely to be in c++, not glsl
 *
 * now its falling apart when i try to do per-component access...i wish somebody would tell me about how glsl
 * converts my arbitrary format into a vec4, cause right now i have no idea how my values are getting loaded/
 * stored...worst case scenario im thinking is just bitshift rgba into a 32-bit float
 */
layout(set=0, binding=1, rgba32f) uniform image2D velocitymap;

void main() {
//    vec4 velocity=imageLoad(velocitymap, ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y));
//    velocity+=vec4(0.1, 0.01, 0.1, 1.0);
//    velocity.y+=0.01;
    //note that non-square ocean mesh may distort calculation
    //need to do some work to establish scale
//    imageStore(imageout, ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y),
//        vec4(sin(float(gl_GlobalInvocationID.x)*0.1+constants.time*5.0)*0.25
//            +cos(float(gl_GlobalInvocationID.y)*0.2+constants.time*7.0)*0.1));



// switched for troubleshooting, add back later
    imageStore(imageout, ivec2(gl_GlobalInvocationID.xy), vec4(1.5));
    // before we implement wind modeling (or even after), if we wanna make this look nicer we can have a universal
    // brownian noise texture available on the GPU
	/*
    imageStore(imageout,
                ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y),
                vec4(1+sin(float(gl_GlobalInvocationID.y * 10.)*0.02+constants.time*2.0)*0.1));
		*/






//      imageStore(
//            imageout, ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y),
//            vec4(
//                (
//                    1.+sin(float(gl_GlobalInvocationID.y)+constants.time)
//                )
//                *0.1
//                )
//            );
//    imageStore(imageout, ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y), vec4(velocity.y));
//    imageStore(velocitymap, ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y), velocity);
}
