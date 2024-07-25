path="/Users/danp/Desktop/C Coding/VulkanSandbox/resources/shaders"
cd "$path"
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
echo "compiling texmon shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/TexMonVertex.glsl -o SPIRV/texmonvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/TexMonFragment.glsl -o SPIRV/texmonfrag.spv
echo "compiling line shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/LineVertex.glsl -o SPIRV/linevert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/LineFragment.glsl -o SPIRV/linefrag.spv
echo "compiling compositing shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/CompositingVertex.glsl -o SPIRV/compositingvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/CompositingFragment.glsl -o SPIRV/compositingfrag.spv
echo "compiling TG shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=comp GLSL/TGVoxelCompute2.glsl -o SPIRV/tgvoxelcomp.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/VoxelTroubleshootingVertex.glsl -o SPIRV/voxeltroubleshootingvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/VoxelTroubleshootingFragment.glsl -o SPIRV/voxeltroubleshootingfrag.spv
echo "compiling post-proc shaders"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=comp GLSL/PostProcessingCompute.glsl -o SPIRV/postproccomp.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/PostProcessingVertex.glsl -o SPIRV/postprocvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/PostProcessingFragment.glsl -o SPIRV/postprocfrag.spv

#/Users/danp/downloads/install/bin/spirv-dis "/Users/danp/Desktop/C Coding/VulkanSandbox/resources/shaders/SPIRV/voxeltroubleshootingvert.spv"

#od -x SPIRV/voxeltroubleshootingvert.spv

