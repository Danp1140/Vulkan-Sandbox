#include "GraphicsHandler.h"

#define VK_INIT_GUARD {if (GraphicsHandler::vulkaninfo.logicaldevice == VK_NULL_HANDLE) return;}
#define COMP_INIT_GUARD {if (scratchbuffer.image == VK_NULL_HANDLE) return;}

#define COMPOSITING_SCRATCH_DEPTH_BUFFER_FORMAT VK_FORMAT_D32_SFLOAT

class CompositingOp {
private:
	static uint8_t numcompops;
protected:
	static TextureInfo scratchbuffer, scratchdepthbuffer;
public:
	CompositingOp ();
	~CompositingOp ();

	static void init();
	static void terminate();
	static const VkDescriptorImageInfo getScratchDII() {return scratchbuffer.getDescriptorImageInfo();}
	static const VkDescriptorImageInfo getScratchDepthDII() {return scratchdepthbuffer.getDescriptorImageInfo();}
};

/*
 * SSRR is fully static, should not be instantiated
 */
class SSRR : public CompositingOp {
private:
	// RP & FB for any draws using the SSRR scratch buffers
	static VkRenderPass renderpass;
	static VkFramebuffer* framebuffers;
public:
	SSRR () = delete;

	static void init ();
	static void terminate ();

	static const VkRenderPass getRenderpass () {return renderpass;}
	static const VkFramebuffer getFramebuffer (uint8_t scii) {return framebuffers[scii];}

	static VkDependencyInfoKHR getPreCopyDependency ();
	static VkDependencyInfoKHR getPostCopyDependency ();
	// separated because they can't be combined into one recordImgCpy op
	static void recordCopy (VkCommandBuffer& cb);
	static void recordDepthCopy (VkCommandBuffer& cb);
};

typedef enum PostProcOpTypes {
	POST_PROC_OP_TONEMAP,
	POST_PROC_OP_LUMINANCE_CUTOFF,
	POST_PROC_OP_BLOOM,
	POST_PROC_OP_DOWNSAMPLE,
	POST_PROC_OP_UPSAMPLE
} PostProcOpTypes;
typedef uint32_t PostProcOpType; // if not for its use in PCs, this could p be a uint8_t

typedef struct PostProcPushConstants {
	PostProcOpType optype;
	float exposure;
	glm::uvec2 numinvocs, scext;
	uint32_t dfdepth; // would like to make this a uint8_t if possible
} PostProcPushConstants;

class PostProcOp : public CompositingOp {
private:
	static uint8_t numpostprocops;
	static PipelineInfo graphicspipeline, computepipeline;
	static VkDescriptorSet ds;

	static void createGraphicsPipeline ();
	static void createComputePipeline ();
	static void initDescriptorSets ();
	static void updateDescriptorSets ();
public:
	PostProcOp ();
	~PostProcOp ();
};

class Bloom : public PostProcOp {

};
