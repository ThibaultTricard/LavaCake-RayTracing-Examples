#include "Framework/Framework.h"
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32 true
#include "glfw3.h"
#include "glfw3native.h"

#define RAYTRACING
#include "RayTracing/RayTracingPipeline.h"
#include "RayTracing/BottomLevelAS.h"

using namespace LavaCake;
using namespace LavaCake::Geometry;
using namespace LavaCake::Framework;
using namespace LavaCake::Core;
using namespace LavaCake::RayTracing;


int main() {

	ErrorCheck::PrintError();

	Window w("LavaCake: Raytracing HelloWorld", 1280 , 720);
	

	Device* d = Device::getDevice();
	d->initDevices(0, 1, w.m_windowParams);
	SwapChain* s = SwapChain::getSwapChain();
	s->init();
	VkExtent2D size = s->size();
	Queue* queue = d->getGraphicQueue(0);
	Queue* presentQueue = d->getPresentQueue();
	CommandBuffer  commandBuffer;
	commandBuffer.addSemaphore();


	//We define the stride format we need for the mesh here 
	//each vertex is a 3D position followed by a RGB color
	vertexFormat format = vertexFormat({ POS3 });

	//we create a indexed triangle mesh with the desired format
	Mesh_t* triangle = new IndexedMesh<TRIANGLE>(format);

	//adding 3 vertices
	triangle->appendVertex({ 1.0f,  1.0f, 0.0f });
	triangle->appendVertex({ -1.0f,  1.0f, 0.0f });
	triangle->appendVertex({0.0f, -1.0f, 0.0f });


	// we link the 3 vertices to define a triangle
	triangle->appendIndex(0);
	triangle->appendIndex(1);
	triangle->appendIndex(2);

	//creating an allocating a vertex buffer
	VertexBuffer* triangle_vertex_buffer = new VertexBuffer({ triangle });
	triangle_vertex_buffer->allocate(queue, commandBuffer, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

	VkTransformMatrixKHR transformMatrixKHR = { 1,0,0,0,
																						0,1,0,0,
																						0,0,1,0 };

	TransformBuffer* transform = new TransformBuffer(transformMatrixKHR);
	transform->allocate(queue, commandBuffer);

	BottomLevelAS blAS;
	blAS.addVertexBuffer(triangle_vertex_buffer, transform, true);
	blAS.allocate(queue, commandBuffer);

	TopLevelAS tlAS;


	tlAS.addInstance(&blAS, transformMatrixKHR, 0, 0);
	tlAS.alloctate(queue, commandBuffer, false);

	StorageImage output(size.width, size.height, 1, s->imageFormat());
	output.allocate(queue, commandBuffer);


	UniformBuffer proj;
	proj.addVariable("viewInverse", mat4({1.0,0.0,0.0,0.0,
																				 0.0,1.0,0.0,0.0,
																				 0.0,0.0,1.0,0.0,
																				 0.0,0.0,2.5,1.0 }));
	proj.addVariable("projInverse", mat4({ 1.0264,0.0,0.0,0.0,
																				 0.0,0.5774,0.0,0.0,
																				 0.0,0.0,0.0,-10.0,
																				 0.0,0.0,-1.0,10.0 }));

	proj.end();


	RayGenShaderModule rayGenShaderModule("Data/Shaders/HelloWorld/raygen.rgen.spv");
	MissShaderModule  missShaderModule("Data/Shaders/HelloWorld/miss.rmiss.spv");
	ClosestHitShaderModule closesHitShaderModule("Data/Shaders/HelloWorld/closesthit.rchit.spv");

	RayTracingPipeline rtPipeline(vec2u({ size.width, size.height }));
	rtPipeline.addAccelerationStructure(&tlAS, VK_SHADER_STAGE_RAYGEN_BIT_KHR , 0);
	rtPipeline.addStorageImage(&output, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1);
	rtPipeline.addUniformBuffer(&proj, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 2);

	rtPipeline.addRayGenModule(&rayGenShaderModule);
	rtPipeline.addMissModule(&missShaderModule);

	rtPipeline.startHitGroup();
	rtPipeline.setClosestHitModule(&closesHitShaderModule);
	rtPipeline.endHitGroup();

	rtPipeline.compile(queue, commandBuffer);


	//PostProcessQuad
	Mesh_t* quad = new IndexedMesh<TRIANGLE>(P3UV);

	quad->appendVertex({ -1.0,-1.0,0.0,0.0,0.0 });
	quad->appendVertex({ -1.0, 1.0,0.0,0.0,1.0 });
	quad->appendVertex({ 1.0, 1.0,0.0,1.0,1.0 });
	quad->appendVertex({ 1.0,-1.0,0.0,1.0,0.0 });

	quad->appendIndex(0);
	quad->appendIndex(1);
	quad->appendIndex(2);

	quad->appendIndex(2);
	quad->appendIndex(3);
	quad->appendIndex(0);



	VertexBuffer* quad_vertex_buffer = new VertexBuffer({ quad });
	quad_vertex_buffer->allocate(queue, commandBuffer);

	UniformBuffer* passNumber = new UniformBuffer();
	passNumber->addVariable("dimention", LavaCake::vec2u({ size.width, size.height }));
	passNumber->end();



	//renderPass
	RenderPass* showPass = new RenderPass();

	GraphicPipeline* pipeline = new GraphicPipeline(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));
	VertexShaderModule* vertexShader = new VertexShaderModule("Data/Shaders/HelloWorld/shader.vert.spv");
	FragmentShaderModule* fragmentShader = new FragmentShaderModule("Data/Shaders/HelloWorld/shader.frag.spv");
	pipeline->setVertexModule(vertexShader);
	pipeline->setFragmentModule(fragmentShader);
	pipeline->setVertices(quad_vertex_buffer);
	pipeline->addStorageImage(&output, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
	pipeline->addUniformBuffer(passNumber, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

	SubpassAttachment SA;
	SA.showOnScreen = true;
	SA.nbColor = 1;
	SA.storeColor = true;
	SA.useDepth = true;
	SA.showOnScreenIndex = 0;

	showPass->addSubPass({ pipeline }, SA);

	showPass->compile();

	FrameBuffer* frameBuffer = new FrameBuffer(s->size().width, s->size().height);
	showPass->prepareOutputFrameBuffer(queue, commandBuffer,*frameBuffer);

	std::vector<VkBufferMemoryBarrier> seed_memory_barriers;
	std::vector<VkBufferMemoryBarrier> print_memory_barriers;


	while (w.running()) {
		w.updateInput();

		VkDevice logical = d->getLogicalDevice();
		VkSwapchainKHR& swapchain = s->getHandle();
		
		commandBuffer.wait(2000000000);
		commandBuffer.resetFence();
		commandBuffer.beginRecord();

		proj.update(commandBuffer);
		rtPipeline.trace(commandBuffer);
		commandBuffer.endRecord();

		commandBuffer.submit(queue, {}, { });

		
		commandBuffer.wait(2000000000);
		commandBuffer.resetFence();

		SwapChainImage& image = s->acquireImage();

		std::vector<WaitSemaphoreInfo> wait_semaphore_infos = {};
		wait_semaphore_infos.push_back({
			image.getSemaphore(),                     // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT					// VkPipelineStageFlags   WaitingStage
			});


		commandBuffer.beginRecord();
		passNumber->update(commandBuffer);
		showPass->setSwapChainImage(*frameBuffer, image);


		showPass->draw(commandBuffer, *frameBuffer, vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.1f, 0.2f, 0.3f, 1.0f }, { 1.0f, 0 } });

		
		commandBuffer.endRecord();


		commandBuffer.submit(queue, wait_semaphore_infos, { commandBuffer.getSemaphore(0) });


		PresentInfo present_info = {
			swapchain,                                    // VkSwapchainKHR         Swapchain
			image.getIndex()                              // uint32_t               ImageIndex
		};

		if (!PresentImage(presentQueue->getHandle(), { commandBuffer.getSemaphore(0) }, { present_info })) {
			continue;
		}
		
	}
	d->end();
}
