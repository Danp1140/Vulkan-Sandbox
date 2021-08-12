echo "compiling default shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/DefaultVertex.glsl -o SPIRV/defaultvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/DefaultFragment.glsl -o SPIRV/defaultfrag.spv
echo "compiling text shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/TextVertex.glsl -o SPIRV/textvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/TextFragment.glsl -o SPIRV/textfrag.spv
echo "compiling skybox shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/SkyboxVertex.glsl -o SPIRV/skyboxvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/SkyboxFragment.glsl -o SPIRV/skyboxfrag.spv
echo "compiling ocean shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/OceanVertex.glsl -o SPIRV/oceanvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=tesc GLSL/OceanTessellationControl.glsl -o SPIRV/oceantesc.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=tese GLSL/OceanTessellationEvaluation.glsl -o SPIRV/oceantese.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/OceanFragment.glsl -o SPIRV/oceanfrag.spv

#od -x vert.spv
#od -x frag.spv

