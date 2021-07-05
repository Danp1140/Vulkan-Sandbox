#/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc Test.vert -o vert.spv
#/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc Test.frag -o frag.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslangValidator -V -H Test.vert -o vert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslangValidator -V -H Test.frag -o frag.spv
#/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslangValidator -h