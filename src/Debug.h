#include "GraphicsHandler.h"

class TextureMonitor {
private:
	VkDescriptorSet ds;
	static PipelineInfo graphicspipeline;

	static void createPipeline ();
public:
	TextureMonitor ();
	~TextureMonitor ();

	static void init ();
	static void terminate ();
	static void recordDraw (cbRecData data, VkCommandBuffer& cb);
	static const PipelineInfo& getPipeline () {return graphicspipeline;}

	const VkDescriptorSet& getDescriptorSet () const {return ds;}

	void updateDescriptorSet (const VkDescriptorImageInfo& dii);
};

typedef struct LinesPushConstants {
	glm::mat4 vp;
} LinesPushConstants;

class Lines {
private:
	BufferInfo vertexbuffer;
	LinesPushConstants pushconstants;

	static PipelineInfo graphicspipeline;

	static void createPipeline ();
public:
	Lines ();
	~Lines ();

	const VkBuffer& getVertexBuffer () const {return vertexbuffer.buffer;}
	const uint16_t getNumVertices () const {return vertexbuffer.numelems;}
	void updatePCs (LinesPushConstants&& p) {pushconstants = std::move(p);}
	LinesPushConstants* getPCPtr () {return &pushconstants;}

	static void init ();
	static void terminate ();
	static void recordDraw (cbRecData data, VkCommandBuffer& cb);
	static const PipelineInfo& getPipeline () {return graphicspipeline;}
};
