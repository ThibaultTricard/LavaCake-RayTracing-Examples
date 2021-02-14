#include "Framework/Framework.h"
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32 true
#include "glfw3.h"
#include "glfw3native.h"


#include "RayTracing/RayTracingPipeline.h"
#include "RayTracing/BottomLevelAS.h"

using namespace LavaCake;
using namespace LavaCake::Geometry;
using namespace LavaCake::Framework;
using namespace LavaCake::Core;
using namespace LavaCake::RayTracing;

struct material_t {
	float diff[3];
	float emis[3];
};

float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

int main() {

	ErrorCheck::PrintError();

	Window w("LavaCake: Raytracing HelloWorld", 500 , 500);
	

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
	vertexFormat format = vertexFormat({ POS3, F1 });


	//we create a indexed triangle mesh with the desired format
	
	std::vector<Data*> materials;

	//light material
	materials.push_back(new v<float,8>({ 0.0f,0.0f,0.0f,1.0f,1.0f,1.0f,0.0f, 0.0f }));
	//white material
	materials.push_back(new v<float, 8>({ 1.0f,1.0f,1.0f,0.0f,0.0f,0.0f,0.0f, 0.0f }));
	//green material
	materials.push_back(new v<float, 8>({ 0.0f,1.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 0.0f }));
	//red material
	materials.push_back(new v<float, 8>({ 1.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 0.0f }));

	UniformBuffer materialBuffer;
	materialBuffer.addVariable("material",&materials);
	materialBuffer.end();


	std::vector<Data*> samples;
	srand(time(NULL));
	for (unsigned int i = 0; i < 64; i++)
	{
		vec3f noise({
			(float)rand() / (float)RAND_MAX * 2.0f - 1.0f,
			(float)rand() / (float)RAND_MAX * 2.0f - 1.0f,
			(float)rand() / (float)RAND_MAX });

		noise = Normalize(noise) ;

		samples.push_back(new vec4f({ noise[0], noise[1], noise[2],0.0 }));
	}

	UniformBuffer sampleBuffer;
	sampleBuffer.addVariable("sample", &samples);
	sampleBuffer.end();

	Mesh_t* light = new IndexedMesh<TRIANGLE>(format);

	light->appendVertex({ 343.0, 547.8, 227.0, 0.0f });
	light->appendVertex({ 343.0, 547.8, 332.0, 0.0f });
	light->appendVertex({ 213.0, 547.8, 332.0, 0.0f });
	light->appendVertex({ 213.0, 547.8, 227.0, 0.0f });

	light->appendIndex(0);
	light->appendIndex(1);
	light->appendIndex(2);

	light->appendIndex(2);
	light->appendIndex(3);
	light->appendIndex(0);

	Mesh_t* floor = new IndexedMesh<TRIANGLE>(format);
	floor->appendVertex({ 552.8f,  0.0f, 0.0f, 1.0f });
	floor->appendVertex({ 0.0f,		 0.0f, 0.0f, 1.0f });
	floor->appendVertex({ 0.0f,		 0.0f, 559.2f, 1.0f });
	floor->appendVertex({ 549.6f,  0.0f, 559.2f, 1.0f });

	floor->appendIndex(0);
	floor->appendIndex(1);
	floor->appendIndex(2);

	floor->appendIndex(2);
	floor->appendIndex(3);
	floor->appendIndex(0);


	Mesh_t* ceiling = new IndexedMesh<TRIANGLE>(format);

	ceiling->appendVertex({ 556.0f,  548.8f, 0.0, 1.0f });
	ceiling->appendVertex({ 556.0f,  548.8f, 559.2f, 1.0f });
	ceiling->appendVertex({ 0.0f,    548.8f, 559.2f, 1.0f });
	ceiling->appendVertex({ 0.0f,    548.8f,   0.0, 1.0f });

	ceiling->appendIndex(0);
	ceiling->appendIndex(1);
	ceiling->appendIndex(2);

	ceiling->appendIndex(2);
	ceiling->appendIndex(3);
	ceiling->appendIndex(0);


	Mesh_t* backWall = new IndexedMesh<TRIANGLE>(format);

	backWall->appendVertex({ 549.6f,  0.0f,			559.2f, 1.0f });
	backWall->appendVertex({ 0.0f,    0.0f,			559.2f, 1.0f });
	backWall->appendVertex({ 0.0f,    548.8f,		559.2f, 1.0f });
	backWall->appendVertex({ 556.0f,  548.8f,   559.2f, 1.0f });

	backWall->appendIndex(0);
	backWall->appendIndex(1);
	backWall->appendIndex(2);

	backWall->appendIndex(2);
	backWall->appendIndex(3);
	backWall->appendIndex(0);


	Mesh_t* rightWall = new IndexedMesh<TRIANGLE>(format);

	rightWall->appendVertex({ 0.0f,    0.0f,			559.2f, 2.0f });
	rightWall->appendVertex({ 0.0f,    0.0f,				0.0f, 2.0f });
	rightWall->appendVertex({ 0.0f,    548.8f,			0.0f, 2.0f });
	rightWall->appendVertex({ 0.0f,    548.8f,    559.2f, 2.0f });

	rightWall->appendIndex(0);
	rightWall->appendIndex(1);
	rightWall->appendIndex(2);

	rightWall->appendIndex(2);
	rightWall->appendIndex(3);
	rightWall->appendIndex(0);


	Mesh_t* leftWall = new IndexedMesh<TRIANGLE>(format);

	leftWall->appendVertex({ 552.8f,    0.0f,			   0.0f, 3.0f });
	leftWall->appendVertex({ 549.6f,    0.0f,			 559.2f, 3.0f });
	leftWall->appendVertex({ 556.0f,    548.8f,		 559.2f, 3.0f });
	leftWall->appendVertex({ 556.0f,    548.8f,      0.0f, 3.0f });

	leftWall->appendIndex(0);
	leftWall->appendIndex(1);
	leftWall->appendIndex(2);

	leftWall->appendIndex(2);
	leftWall->appendIndex(3);
	leftWall->appendIndex(0);


	

	Mesh_t* box1 = new IndexedMesh<TRIANGLE>(format);

	box1->appendVertex({ 130.0, 165.0,  65.0, 1.0f, 1.0f, 1.0f });
	box1->appendVertex({ 82.0, 165.0, 225.0, 1.0f, 1.0f, 1.0f });
	box1->appendVertex({ 240.0, 165.0, 272.0, 1.0f, 1.0f, 1.0f });
	box1->appendVertex({ 290.0, 165.0, 114.0, 1.0f, 1.0f, 1.0f });

	box1->appendVertex({ 552.8f,    0.0f,			   0.0f, 1.0f, 1.0f, 1.0f });
	box1->appendVertex({ 549.6f,    0.0f,			 559.2f, 1.0f, 1.0f, 1.0f });
	box1->appendVertex({ 556.0f,    548.8f,		 559.2f, 1.0f, 1.0f, 1.0f });
	box1->appendVertex({ 556.0f,    548.8f,      0.0f, 1.0f, 1.0f, 1.0f });



	//creating an allocating a vertex buffer
	VertexBuffer* triangle_vertex_buffer = new VertexBuffer({ light,ceiling, leftWall, rightWall, floor, backWall });
	triangle_vertex_buffer->allocate(queue, commandBuffer, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

	VkTransformMatrixKHR transformMatrixKHR = { 1,0,0,0,
																						0,1,0,0,
																						0,0,1,0 };

	VkTransformMatrixKHR transformMatrixKHR2= { 1,0,0,0,
																						0,1,0,0,
																						0,0,1,0 };


	TransformBuffer* transform = new TransformBuffer(transformMatrixKHR2);
	transform->allocate(queue, commandBuffer);

	BottomLevelAS blAS;
	blAS.addVertexBuffer(triangle_vertex_buffer, transform, true);
	blAS.allocate(queue, commandBuffer);

	TopLevelAS tlAS;


	tlAS.addInstance(&blAS, transformMatrixKHR, 0, 0);
	tlAS.alloctate(queue, commandBuffer, false);
	//we create a indexed triangle mesh with the desired format
	

	StorageImage output(size.width, size.height, 1, s->imageFormat());
	output.allocate(queue, commandBuffer);

	UniformBuffer proj;

	proj.addVariable("CameraPos", &vec4f({ 278, 273, -80,0 }));
	proj.addVariable("Direction", &vec4f({ 0, 0, 1,0 }));
	proj.addVariable("horizontal", &vec4f({ 1, 0, 0,0 }));
	proj.addVariable("Up", &vec3f({ 0, 1, 0}));
	proj.addVariable("focale", 0.35f);
	proj.addVariable("width", 0.025f);
	proj.addVariable("height", 0.025f);

	proj.end();


	RayGenShaderModule rayGenShaderModule("Data/Shaders/CornellBox/raygen.rgen.spv");
	MissShaderModule  missShaderModule("Data/Shaders/CornellBox/miss.rmiss.spv");
	ClosestHitShaderModule closesHitShaderModule("Data/Shaders/CornellBox/closesthit.rchit.spv");

	RayTracingPipeline rtPipeline(vec2u({ size.width, size.height }));
	rtPipeline.addAccelerationStructure(&tlAS, VK_SHADER_STAGE_RAYGEN_BIT_KHR| VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0);
	rtPipeline.addStorageImage(&output, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1);
	rtPipeline.addUniformBuffer(&proj, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 2);
	rtPipeline.addBuffer(&triangle_vertex_buffer->getVertexBuffer(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 3);
	rtPipeline.addBuffer(&triangle_vertex_buffer->getIndexBuffer(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 4);
	rtPipeline.addUniformBuffer(&materialBuffer, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 5);
	rtPipeline.addUniformBuffer(&sampleBuffer, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 6);
	rtPipeline.addRayGenModule(&rayGenShaderModule);
	rtPipeline.addMissModule(&missShaderModule);

	rtPipeline.startHitGroup();
	rtPipeline.setClosestHitModule(&closesHitShaderModule);
	rtPipeline.endHitGroup();

	rtPipeline.setMaxRecursion(10);

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
	passNumber->addVariable("dimention", &LavaCake::vec2u({ size.width, size.height }));
	passNumber->end();



	//renderPass
	RenderPass* showPass = new RenderPass();

	GraphicPipeline* pipeline = new GraphicPipeline(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));
	VertexShaderModule* vertexShader = new VertexShaderModule("Data/Shaders/HelloWorld/shader.vert.spv");
	FragmentShaderModule* fragmentShader = new FragmentShaderModule("Data/Shaders/HelloWorld/shader.frag.spv");
	pipeline->setVextexModule(vertexShader);
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
	showPass->prepareOutputFrameBuffer(*frameBuffer);

	std::vector<VkBufferMemoryBarrier> seed_memory_barriers;
	std::vector<VkBufferMemoryBarrier> print_memory_barriers;


	while (w.running()) {
		w.updateInput();

		VkDevice logical = d->getLogicalDevice();
		VkSwapchainKHR& swapchain = s->getHandle();
		
		commandBuffer.wait(2000000000);
		commandBuffer.resetFence();
		commandBuffer.beginRecord();

		materialBuffer.update(commandBuffer);
		sampleBuffer.update(commandBuffer);
		proj.update(commandBuffer);
		rtPipeline.trace(commandBuffer);
		commandBuffer.endRecord();

		if (!SubmitCommandBuffersToQueue(queue->getHandle(), {}, { commandBuffer.getHandle() }, {}, commandBuffer.getFence())) {
			continue;
		}

		
		commandBuffer.wait(2000000000);
		commandBuffer.resetFence();

		SwapChainImage& image = s->AcquireImage();

		std::vector<WaitSemaphoreInfo> wait_semaphore_infos = {};
		wait_semaphore_infos.push_back({
			image.getSemaphore(),                     // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT					// VkPipelineStageFlags   WaitingStage
			});


		commandBuffer.beginRecord();
		passNumber->update(commandBuffer);
		showPass->setSwapChainImage(*frameBuffer, image);


		showPass->draw(commandBuffer.getHandle(), frameBuffer->getHandle(), vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.1f, 0.2f, 0.3f, 1.0f }, { 1.0f, 0 } });

		
		commandBuffer.endRecord();


		if (!SubmitCommandBuffersToQueue(queue->getHandle(), wait_semaphore_infos, { commandBuffer.getHandle() }, { commandBuffer.getSemaphore(0) }, commandBuffer.getFence())) {
			continue;
		}


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