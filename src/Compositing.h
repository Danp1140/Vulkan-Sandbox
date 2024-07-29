#include "GraphicsHandler.h"

#define VK_INIT_GUARD {if (GraphicsHandler::vulkaninfo.logicaldevice == VK_NULL_HANDLE) return;}
#define COMPOSITING_SCRATCH_DEPTH_BUFFER_FORMAT VK_FORMAT_D32_SFLOAT
class CompositingOp {
private:
	static uint8_t numcompops;
protected:
	static TextureInfo scratchbuffer, scratchdepthbuffer;
public:
	CompositingOp ();
	~CompositingOp ();
};

class SSRR : public CompositingOp {
public:
	VkDependencyInfoKHR getPreCopyDependency ();
	VkDependencyInfoKHR getPostCopyDependency ();
	void recordCopy (VkCommandBuffer& cb);
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
