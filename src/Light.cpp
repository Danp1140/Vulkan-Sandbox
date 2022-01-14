//
// Created by Daniel Paavola on 6/17/21.
//

#include "Light.h"

Light::Light(){
	position=glm::vec3(0, 5, 5);
	forward=glm::vec3(0, 0, -1);
	up=glm::vec3(0, 1, 0);
	color=glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	intensity=5.0f;
	type=LIGHT_TYPE_SUN;
	shadowtype=SHADOW_TYPE_UNIFORM;
	shadowmapresolution=1024;
	worldspacescenebb[0]=glm::vec3(-100., -100., -100.);
	worldspacescenebb[1]=glm::vec3(100., 100., 100.);
	for(uint32_t x=0;x<GraphicsHandler::vulkaninfo.numswapchainimages;x++) GraphicsHandler::changeflags[x]|=LIGHT_CHANGE_FLAG_BIT;
	recalculateProjectionMatrix();
	recalculateViewMatrix();
	GraphicsHandler::VKHelperInitTexture(
			&shadowmap,
			shadowmapresolution, 0,
			VK_FORMAT_D32_SFLOAT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			TEXTURE_TYPE_SHADOWMAP,
			VK_IMAGE_VIEW_TYPE_2D,
			GraphicsHandler::depthsampler);
	initRenderpassAndFramebuffer();
}

Light::Light(glm::vec3 p, glm::vec3 f, float i, glm::vec4 c, int smr, LightType t){
	position=p;
	forward=f;
	up=glm::vec3(0, 1, 0);
	color=c;
	intensity=i;
	type=t;
	shadowmapresolution=smr;
	shadowtype=SHADOW_TYPE_UNIFORM;
	worldspacescenebb[0]=glm::vec3(-100., -100., -100.);
	worldspacescenebb[1]=glm::vec3(100., 100., 100.);
	for(uint32_t x=0;x<GraphicsHandler::vulkaninfo.numswapchainimages;x++) GraphicsHandler::changeflags[x]|=LIGHT_CHANGE_FLAG_BIT;
	recalculateProjectionMatrix();
	recalculateViewMatrix();
	GraphicsHandler::VKHelperInitTexture(
			&shadowmap,
			shadowmapresolution, 0,
			VK_FORMAT_D32_SFLOAT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			TEXTURE_TYPE_SHADOWMAP,
			VK_IMAGE_VIEW_TYPE_2D,
			GraphicsHandler::depthsampler);
	initRenderpassAndFramebuffer();
}

void Light::initRenderpassAndFramebuffer(){
	VkAttachmentDescription shadowattachmentdescription{
			0,
			VK_FORMAT_D32_SFLOAT,
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
	VkAttachmentReference shadowattachmentreference{0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
	VkSubpassDescription shadowsubpassdescription{
			0,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			0,
			nullptr,
			0,
			nullptr,
			nullptr,
			&shadowattachmentreference,
			0,
			nullptr
	};
	VkSubpassDependency shadowsubpassdependencies[2]{{
             VK_SUBPASS_EXTERNAL,
             0,
             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
             VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
             VK_ACCESS_SHADER_READ_BIT,
             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
             0
     }, {
             0,
             VK_SUBPASS_EXTERNAL,
             VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
             VK_ACCESS_SHADER_READ_BIT,
             0
     }};
	VkRenderPassCreateInfo shadowrenderpasscreateinfo{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			nullptr,
			0,
			1,
			&shadowattachmentdescription,
			1,
			&shadowsubpassdescription,
			2,
			&shadowsubpassdependencies[0]
	};
	vkCreateRenderPass(GraphicsHandler::vulkaninfo.logicaldevice, &shadowrenderpasscreateinfo, nullptr, &shadowrenderpass);
	VkFramebufferCreateInfo shadowframebuffercreateinfo{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr,
			0,
			shadowrenderpass,
			1,
			&shadowmap.imageview,
			shadowmapresolution, shadowmapresolution, 1
	};
	vkCreateFramebuffer(GraphicsHandler::vulkaninfo.logicaldevice, &shadowframebuffercreateinfo, nullptr, &shadowframebuffer);
}

void Light::recalculateProjectionMatrix(){
	if(type==LIGHT_TYPE_SUN){
		glm::vec3 min, max, *b=new glm::vec3[8];
		GraphicsHandler::makeRectPrism(&b, worldspacescenebb[0], worldspacescenebb[1]);
		for(uint8_t i=0;i<8;i++){
			if(shadowtype==SHADOW_TYPE_LIGHT_SPACE_PERSPECTIVE){
				b[i]=GraphicsHandler::mat4TransformVec3(shadowpushconstants.lspmatrix, b[i]);
			}
			b[i]=GraphicsHandler::mat4TransformVec3(viewmatrix, b[i]);
			if(i==0){
				min=b[i];
				max=b[i];
			}
			for(uint8_t x=0;x<3;x++){
				if(b[i][x]<min[x]) min[x]=b[i][x];
				else if(b[i][x]>max[x]) max[x]=b[i][x];
			}
		}
		delete[] b;
		projectionmatrix=glm::ortho<float>(min.x, max.x, max.y, min.y, 0., 100.);
	}
//	if(type==LIGHT_TYPE_SUN) projectionmatrix=glm::ortho<float>(-20., 20., -20., 20., 0., 100.);
	else if(type==LIGHT_TYPE_PLANE){
		projectionmatrix=glm::ortho<float>(-5, 5, -5, 5, 0, 100);
		projectionmatrix[1][1]*=-1.;
	}
	else{
		projectionmatrix=glm::perspective<float>(0.785f, 1.0f, 1.0f, 100.0f);
		projectionmatrix[1][1]*=-1.;
	}
	shadowpushconstants={projectionmatrix*viewmatrix, shadowpushconstants.lspmatrix};
}

void Light::recalculateViewMatrix(){
	if(shadowtype==SHADOW_TYPE_LIGHT_SPACE_PERSPECTIVE){
		glm::mat4 lspviewnotranslate = lspview;
		lspviewnotranslate[3][0] = 0.; lspviewnotranslate[3][1] = 0.; lspviewnotranslate[3][2] = 0.;
		viewmatrix = glm::lookAt<float>(GraphicsHandler::mat4TransformVec3(lspview, position),
		                                GraphicsHandler::mat4TransformVec3(lspview, position + forward),
		                                GraphicsHandler::mat4TransformVec3(lspviewnotranslate, glm::vec3(0., 1., 0.)));
	}
	else{
		viewmatrix=glm::lookAt(position, position+forward, up);
	}
	shadowpushconstants={projectionmatrix*viewmatrix, shadowpushconstants.lspmatrix};
}
void Light::recalculateLSPSMMatrix(glm::vec3 cameraforward, glm::vec3 camerapos, glm::vec3*b, uint8_t numb){
	//----------------------------
	//----------------------------
	// this area has gotten *very* bloated, so i'm gonna rewrite it cleaner up here.
	glm::mat4 lspsmmatrix;
	if(shadowtype==SHADOW_TYPE_UNIFORM){lspsmmatrix=glm::mat4(1.);}
	else{
		// prelim world-space min-maxing, could be made more efficient
		glm::vec3 min, max;
		for(uint8_t i=0;i<numb;i++){
			if(i==0){
				min=b[i];
				max=b[i];
			}
			for(uint8_t x=0;x<3;x++){
				if(b[i][x]<min[x]) min[x]=b[i][x];
				else if(b[i][x]>max[x]) max[x]=b[i][x];
			}
		}
		// definition of lsp frustum's "view" position, in my own way, ensuring nothing
		// lies behind it
		glm::vec3 P=camerapos, v=-lsorthobasis[0];
		float t=0.;
		for(uint8_t x=0;x<3;x++){
			if(v[x]<0.){
				if(t*v[x]+P[x]>min[x]){
					t=(min[x]-P[x])/v[x];
				}
			}else{
				if(t*v[x]+P[x]<max[x]){
					t=(max[x]-P[x])/v[x];
				}
			}
		}
		glm::vec3 lsppos=P+v*t;
		GraphicsHandler::troubleshootingsstrm<<SHIFT_TRIPLE(lsppos)<<std::endl;
		GraphicsHandler::troubleshootingsstrm<<"t: "<<t<<std::endl;
		// initial definition of lsp matrices defining lsp frustum
		lspprojection=glm::frustum<float>(-1., 1., -1., 1., t, t-0.5);
		lspview=glm::lookAt(lsppos,
		                    lsppos+lsorthobasis[0],
		                    lsorthobasis[1]);
		// composition of lsp projection and view into lspsmmatrix, characterizing final lsp frustum
		lspsmmatrix=lspprojection*lspview;
	}
	// final insertion of matrices into push constants for rendering
	shadowpushconstants={projectionmatrix*viewmatrix, lspsmmatrix};
	//----------------------------
	//----------------------------
}

void Light::setPosition(glm::vec3 p){
	position=p;
	for(uint32_t x=0;x<GraphicsHandler::vulkaninfo.numswapchainimages;x++) GraphicsHandler::changeflags[x]|=LIGHT_CHANGE_FLAG_BIT;
	recalculateViewMatrix();
}

void Light::setForward(glm::vec3 f){
	forward=f;
	for(uint32_t x=0;x<GraphicsHandler::vulkaninfo.numswapchainimages;x++) GraphicsHandler::changeflags[x]|=LIGHT_CHANGE_FLAG_BIT;
	recalculateViewMatrix();
}

void Light::setWorldSpaceSceneBB(glm::vec3 min, glm::vec3 max){
	worldspacescenebb[0]=min;
	worldspacescenebb[1]=max;
	recalculateViewMatrix();
	recalculateProjectionMatrix();
}

void Light::recalculateLSOrthoBasis(glm::vec3 cameraforward, glm::vec3 camerapos, glm::vec3*b, uint8_t numb){
	lsorthobasis[1]=glm::normalize(-forward);
	lsorthobasis[0]=glm::normalize(cameraforward-glm::proj(cameraforward, lsorthobasis[1]));
	lsorthobasis[2]=glm::cross(lsorthobasis[0], lsorthobasis[1]);
	recalculateLSPSMMatrix(cameraforward, camerapos, b, numb);
	recalculateViewMatrix();
	recalculateProjectionMatrix();
}