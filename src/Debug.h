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

	void updateDescriptorSet (const TextureInfo& t);
};

class Lines {
private:
	BufferInfo vertexbuffer;
};
