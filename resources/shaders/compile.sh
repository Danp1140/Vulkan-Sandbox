/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc Test.vert -o vert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc Test.frag -o frag.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert TextVertexShader.glsl -o textvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag TextFragmentShader.glsl -o textfrag.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag TextFragmentShader.glsl -o textfrag.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc Skybox.vert -o skyboxvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc Skybox.frag -o skyboxfrag.spv
#/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -h

#od -x vert.spv
#od -x frag.spv

