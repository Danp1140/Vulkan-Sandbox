echo "compiling default shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/DefaultVertex.glsl -o SPIRV/defaultvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=tesc GLSL/DefaultTessellationControl.glsl -o SPIRV/defaulttesc.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=tese GLSL/DefaultTessellationEvaluation.glsl -o SPIRV/defaulttese.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/DefaultFragment.glsl -o SPIRV/defaultfrag.spv
echo "compiling text shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/TextVertex.glsl -o SPIRV/textvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/TextFragment.glsl -o SPIRV/textfrag.spv
echo "compiling skybox shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/SkyboxVertex.glsl -o SPIRV/skyboxvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/SkyboxFragment.glsl -o SPIRV/skyboxfrag.spv
echo "compiling ocean shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=comp GLSL/OceanCompute.glsl -o SPIRV/oceancomp.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/OceanVertex.glsl -o SPIRV/oceanvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=tesc GLSL/OceanTessellationControl.glsl -o SPIRV/oceantesc.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=tese GLSL/OceanTessellationEvaluation.glsl -o SPIRV/oceantese.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/OceanFragment.glsl -o SPIRV/oceanfrag.spv
echo "compiling grass shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/GrassVertex.glsl -o SPIRV/grassvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=tesc GLSL/GrassTessellationControl.glsl -o SPIRV/grasstesc.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=tese GLSL/GrassTessellationEvaluation.glsl -o SPIRV/grasstese.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/GrassFragment.glsl -o SPIRV/grassfrag.spv
echo "compiling shadowmap shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/ShadowmapVertex.glsl -o SPIRV/shadowmapvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/ShadowmapFragment.glsl -o SPIRV/shadowmapfrag.spv
echo "compiiling texmon shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/TexMonVertex.glsl -o SPIRV/texmonvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/TexMonFragment.glsl -o SPIRV/texmonfrag.spv

#od -x SPIRV/shadowmapfrag.spv

