﻿
#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include "Application.h"
#include <tiny_obj_loader.h>
#include <stb_image.h>


void Application::run() {
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

void Application::initWindow() {
	glfwInit();
	// Defaults to OpenGL hence specifying no API
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(WIDTH, HEIGHT, APPLICATION_NAME, nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Application::initVulkan() {
	createVulkanInstance();
	createVulkanSurface();
	pickVulkanPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createSwapChainImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createDepthResources();
	createFramebuffers();
	createGraphicsCommandPool();
	createTransferCommandPool();
	createTransferCommandBuffer();
	createTextureImage();
	createTextureImageView();
	createTextureSampler();
	load3DModel();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createGraphicsCommandBuffers();
	createSynchronizationObjects();
}

void Application::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrame();
	}
	// Wait for the logical device to finish operations before destroying the window
	vkDeviceWaitIdle(vulkanLogicalDevice);
}

void Application::cleanup() {
	cleanupSwapChain();

	vkDestroySampler(vulkanLogicalDevice, textureSampler, nullptr);
	vkDestroyImageView(vulkanLogicalDevice, textureImageView, nullptr);
	vkDestroyImage(vulkanLogicalDevice, textureImage, nullptr);
	vkFreeMemory(vulkanLogicalDevice, textureDeviceMemory, nullptr);

	// Destroy the UBOs
	for (size_t i{ 0 }; i < uniformBuffers.size(); i++) {
		vkDestroyBuffer(vulkanLogicalDevice, uniformBuffers.at(i), nullptr);
		vkFreeMemory(vulkanLogicalDevice, uniformBuffersMemory.at(i), nullptr);
		uniformBuffersMapped.at(i) = nullptr;
	}
	vkDestroyDescriptorPool(vulkanLogicalDevice, vulkanDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(vulkanLogicalDevice, vulkanDescriptorSetLayout, nullptr);

	vkDestroyPipeline(vulkanLogicalDevice, vulkanGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(vulkanLogicalDevice, vulkanPipelineLayout, nullptr);
	vkDestroyRenderPass(vulkanLogicalDevice, vulkanRenderPass, nullptr);

	// Destroy the vertex & index buffer and de-allocate the memory allocated for them:
	vkDestroyBuffer(vulkanLogicalDevice, indexBuffer, nullptr);
	vkFreeMemory(vulkanLogicalDevice, indexBufferMemory, nullptr);
	vkDestroyBuffer(vulkanLogicalDevice, vertexBuffer, nullptr);
	vkFreeMemory(vulkanLogicalDevice, vertexBufferMemory, nullptr);

	// Destroy synchronization objects
	for (size_t i{ 0 }; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(vulkanLogicalDevice, imageAvailableSemaphores.at(i), nullptr);
		vkDestroySemaphore(vulkanLogicalDevice, renderFinishedSemaphores.at(i), nullptr);
		vkDestroyFence(vulkanLogicalDevice, inFlightFences.at(i), nullptr);
	}
	// Destroy command buffer pools
	vkDestroyCommandPool(vulkanLogicalDevice, vulkanGraphicsCommandPool, nullptr);
	vkDestroyCommandPool(vulkanLogicalDevice, vulkanTransferCommandPool, nullptr);

	vkDestroyDevice(vulkanLogicalDevice, nullptr);
	vkDestroySurfaceKHR(vulkanInstance, vulkanSurface, nullptr);
	// Destroy Vulkan instance just before the program terminates
	vkDestroyInstance(vulkanInstance, nullptr);

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::createVulkanInstance() {
	// Check if validation layers were enabled. If so, check if they're all supported
	if (enableVulkanValidationLayers == true) {
		std::cout << "> Vulkan validation layers requested.\n";
		if (!checkValidationLayersSupport())
			throw std::runtime_error("RUNTIME ERROR: Not all validation layers requested are available!");
		else
			std::cout << "> All requested validation layers are supported.\n";
	}

	// [Optional struct] Provides metadata of the application
	VkApplicationInfo vulkanAppInfo{};
	vulkanAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vulkanAppInfo.pApplicationName = APPLICATION_NAME;
	vulkanAppInfo.apiVersion = VK_MAKE_VERSION(1, 4, 3);
	vulkanAppInfo.pEngineName = "No Engine";
	vulkanAppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	vulkanAppInfo.apiVersion = VK_API_VERSION_1_4;

	// Vulkan needs extensions to deal with GLFW (GLFW provides handy methods to get these extension names)
	uint32_t glfwExtensionsCount{};
	const char** glfwExtensionNames;
	glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

#ifdef NDEBUG
	// Release Mode:
#else
	// Debug Mode:
	// List down all the available Vulkan instance extensions
	uint32_t vulkanExtensionsCount{};
	vkEnumerateInstanceExtensionProperties(nullptr, &vulkanExtensionsCount, nullptr);
	std::vector<VkExtensionProperties> vulkanExtensions(vulkanExtensionsCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &vulkanExtensionsCount, vulkanExtensions.data());
	std::cout << "\nDEBUG LOG: Available Vulkan Extensions:\n";
	for (const VkExtensionProperties& extensionProperty : vulkanExtensions) {
		std::cout << "\t" << extensionProperty.extensionName << " (version: " << extensionProperty.specVersion << ")\n";
	}

	// List down all the GLFW extensions required for Vulkan
	std::cout << "\nDEBUG LOG: Required GLFW Extensions for Vulkan:\n";
	bool glfwExtensionFound{ false };
	for (int i{ 0 }; i < glfwExtensionsCount; i++) {
		std::cout << "\t" << glfwExtensionNames[i];
		// Print if the GLFW extensions are available in the Vulkan instance extensions
		for (const VkExtensionProperties& extensionProperty : vulkanExtensions) {
			if (strcmp(extensionProperty.extensionName, glfwExtensionNames[i]) == 0) {
				std::cout << " - (SUPPORTED BY VULKAN INSTANCE)\n";
				glfwExtensionFound = true;
				break;
			}
		}
		if (!glfwExtensionFound) {
			std::cout << "\t(!UNSUPPORTED!)\n";
			glfwExtensionFound = false;
			throw std::runtime_error("RUNTIME ERROR: Unsupported GLFW extensions found!");
		}
	}
	std::cout << "\n";
#endif

	// [Required struct] Tells Vulkan how to create the instance
	VkInstanceCreateInfo vulkanCreateInfo{};
	vulkanCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vulkanCreateInfo.pApplicationInfo = &vulkanAppInfo;
	vulkanCreateInfo.enabledExtensionCount = glfwExtensionsCount;
	vulkanCreateInfo.ppEnabledExtensionNames = glfwExtensionNames;
	vulkanCreateInfo.enabledLayerCount = 0;                          // Disabling validation layers for now (they help in debugging Vulkan)

	// Creates the instance based on the Create Info struct, and stores the instance in the specified instance variable (3rd arg)
	VkResult result = vkCreateInstance(&vulkanCreateInfo, nullptr, &vulkanInstance);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create Vulkan instance!");
	}
	std::cout << "> Vulkan instance created successfully.\n";

}

void Application::createVulkanSurface() {
	VkResult result = glfwCreateWindowSurface(vulkanInstance, window, nullptr, &vulkanSurface);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create Vulkan surface!");
	}
}

void Application::pickVulkanPhysicalDevice() {
	// Get the list of physical devices (GPUs) available in the system that support Vulkan
	uint32_t physicalDevicesCount{};
	vkEnumeratePhysicalDevices(vulkanInstance, &physicalDevicesCount, nullptr);
	if (physicalDevicesCount == 0) {
		throw std::runtime_error("RUNTIME ERROR: Failed to find physical devices that support Vulkan!");
	}
	std::vector<VkPhysicalDevice> physicalDevicesList(physicalDevicesCount);
	vkEnumeratePhysicalDevices(vulkanInstance, &physicalDevicesCount, physicalDevicesList.data());

	VkPhysicalDeviceProperties physicalDeviceProperties;
	// Find the most suitable GPU
	for (const VkPhysicalDevice& physicalDevice : physicalDevicesList) {
		// Prioritize picking the Discrete GPU (if it exists)
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		if (isPhysicalDeviceSuitable(physicalDevice)) {
			vulkanPhysicalDevice = physicalDevice;
			// Suitable discrete GPU found. Choose that as our Vulkan physical device.
			if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				break;
			}
		}
	}
	if (vulkanPhysicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("RUNTIME ERROR: No suitable physical device found!");
	}

#ifdef NDEBUG
	// Release Mode:
#else
	// Debug Mode:
	vkGetPhysicalDeviceProperties(vulkanPhysicalDevice, &physicalDeviceProperties);
	std::cout << "> Vulkan picked the physical device (GPU): '" << physicalDeviceProperties.deviceName << "'\n";
#endif

}

void Application::createLogicalDevice() {
	// Logical Device is created based on the Physical Device selected
	if (vulkanPhysicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("RUNTIME ERROR: Unable to create Vulkan logical device! Physical device is NULL or hasn't been created yet...");
	}
	// Get the actual indices of the queue families that we'll need (in this case Graphics queue)
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(vulkanPhysicalDevice);

	// Specify to Vulkan which Queues must be created and how many of them, along with the priority of each queue
	std::set<uint32_t> requiredQueueFamilies = { 
		queueFamilyIndices.graphicsFamily.value(),        // graphics queue family
		queueFamilyIndices.presentationFamily.value(),    // presenttaion queue family
		queueFamilyIndices.transferFamily.value()         // transfer queue family
	};
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

	float queuePriority{ 1.0f };
	for (uint32_t queueFamily : requiredQueueFamilies) {
		// Create a Vulkan queue create info struct for each queue family
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		// Append the struct to the vector of queue create info structs (pushes a copy)
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Specifying the physical device features we'll be using (eg. geometry shader)
	VkPhysicalDeviceFeatures physicalDeviceFeatures{};

	// Specify how to create the Logical Device to Vulkan
	VkDeviceCreateInfo createDeviceInfo{};
	createDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createDeviceInfo.pQueueCreateInfos = queueCreateInfos.data();
	createDeviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createDeviceInfo.pEnabledFeatures = &physicalDeviceFeatures;
	createDeviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createDeviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createDeviceInfo.enabledLayerCount = 0;
	if (enableVulkanValidationLayers) {
		createDeviceInfo.enabledLayerCount = static_cast<uint32_t>(vulkanValidationLayers.size());
		createDeviceInfo.ppEnabledLayerNames = vulkanValidationLayers.data();
	}

	// Create the Logical Device
	VkResult result = vkCreateDevice(vulkanPhysicalDevice, &createDeviceInfo, nullptr, &vulkanLogicalDevice);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create Vulkan logical device!");
	}

	// Logical Device is successfully created...
	std::cout << "> Vulkan logical device successfully created.\n";

	// Get the queue handles:
	vkGetDeviceQueue(vulkanLogicalDevice, queueFamilyIndices.graphicsFamily.value(), 0, &deviceGraphicsQueue);
	vkGetDeviceQueue(vulkanLogicalDevice, queueFamilyIndices.presentationFamily.value(), 0, &devicePresentationQueue);
	vkGetDeviceQueue(vulkanLogicalDevice, queueFamilyIndices.transferFamily.value(), 0, &deviceTransferQueue);
	std::cout << "> Retrieved queue handles.\n";

}

void Application::recreateSwapChain() {
	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	// Don't touch resources that may still be in use
	vkDeviceWaitIdle(vulkanLogicalDevice);

	cleanupSwapChain();

	createSwapChain();
	createSwapChainImageViews();
	createDepthResources();
	createFramebuffers();
	std::cout << "> Recreated swapchain successfully.\n";
}

void Application::cleanupSwapChain() {
	// Destroy the depth images
	vkDestroyImageView(vulkanLogicalDevice, depthImageView, nullptr);
	vkDestroyImage(vulkanLogicalDevice, depthImage, nullptr);
	vkFreeMemory(vulkanLogicalDevice, depthImageMemory, nullptr);

	// Delete all the framebuffers
	for (auto framebuffer : vulkanSwapChainFramebuffers) {
		vkDestroyFramebuffer(vulkanLogicalDevice, framebuffer, nullptr);
	}
	// Destroy the swapchain image-views
	for (VkImageView imageView : vulkanSwapChainImageViews) {
		vkDestroyImageView(vulkanLogicalDevice, imageView, nullptr);
	}
	vkDestroySwapchainKHR(vulkanLogicalDevice, vulkanSwapChain, nullptr);
}

void Application::createSwapChain() {
	// Safety Check (although will never reach here)
	if (vulkanPhysicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create SwapChain! Physical Device is NULL.");
	}
	if (vulkanLogicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create SwapChain! Logical Device is NULL.");
	}

	// Get supported swapchain properties from the physical device (GPU)
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(vulkanPhysicalDevice);

	// Choose and set the desired properties of our swapchain
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.surfaceFormats);
	VkPresentModeKHR presentationMode = chooseSwapPresentationMode(swapChainSupport.presentationModes);
	VkExtent2D swapExtent = chooseSwapExtent(swapChainSupport.surfaceCapabilities);

	// We would like one image more than the min supported images in the swapchain by the device (ensured that its clamped)
	uint32_t swapChainImagesCount = std::clamp(swapChainSupport.surfaceCapabilities.minImageCount + 1, swapChainSupport.surfaceCapabilities.minImageCount, swapChainSupport.surfaceCapabilities.maxImageCount);

	// Specify how to create the SwapChain:
	VkSwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = vulkanSurface;
	swapChainCreateInfo.minImageCount = swapChainImagesCount;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent = swapExtent;
	swapChainCreateInfo.imageArrayLayers = 1;  // Layers in each image (will always be 1, unless building a stereoscopic 3D application)
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	// Need to specify how to handle swapchain images that will be used across multiple queue families (eg: graphics & presentation queues)
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(vulkanPhysicalDevice);
	uint32_t indices[] = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentationFamily.value() };
	if (queueFamilyIndices.graphicsFamily.value() != queueFamilyIndices.presentationFamily.value()) {
		// The graphics and presentation families are different
		// We'll go with CONCURRENT mode (less efficient but Vulkan automatically handles resource sharing and ownership transfer)
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.pQueueFamilyIndices = indices;
		swapChainCreateInfo.queueFamilyIndexCount = 2;

	}
	else {
	 // The graphics and presentation families are the same
	 // Go with EXCLUSIVE mode (most efficient and best performance)
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;  // optional
		swapChainCreateInfo.queueFamilyIndexCount = 0;      // optional
	}
	// Not applying any pre-transform to swapchain images (used in mobile applications)
	swapChainCreateInfo.preTransform = swapChainSupport.surfaceCapabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;  // No blending with other windows in the window system
	swapChainCreateInfo.presentMode = presentationMode;
	swapChainCreateInfo.clipped = VK_TRUE;  // Don't care about pixels that are obscured by say, other windows for example
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;  // Swapchain might be unoptimized over time, recreating required. Specify the old one here.

	// Create the SwapChain:
	VkResult result = vkCreateSwapchainKHR(vulkanLogicalDevice, &swapChainCreateInfo, nullptr, &vulkanSwapChain);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create the SwapChain!");
	}
	std::cout << "> Vulkan swapchain created successfully.\n";

	// Store the swapchain image-format and extent in member variables:
	vulkanSwapChainImageFormat = surfaceFormat.format;
	vulkanSwapChainImageColorspace = surfaceFormat.colorSpace;
	vulkanSwapChainExtent = swapExtent;

	// Retrieve the handles to the swapchain images
	vkGetSwapchainImagesKHR(vulkanLogicalDevice, vulkanSwapChain, &swapChainImagesCount, nullptr);
	vulkanSwapChainImages.resize(swapChainImagesCount);
	vkGetSwapchainImagesKHR(vulkanLogicalDevice, vulkanSwapChain, &swapChainImagesCount, vulkanSwapChainImages.data());
	std::cout << "> Retrieved swapchain image handles.\n";

}

void Application::createSwapChainImageViews() {
	// FResize the vector holding the image-views to fit the number of images
	vulkanSwapChainImageViews.resize(vulkanSwapChainImages.size());

	// Iterate through the swapchain images (not views)
	for (size_t i{ 0 }; i < vulkanSwapChainImages.size(); i++) {
		// Create the image-view for each swapchain image
		vulkanSwapChainImageViews.at(i) = createImageView(vulkanSwapChainImages.at(i), vulkanSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	std::cout << "> Created image-views for swapchain images successfully.\n";
}

void Application::createRenderPass() {
	// We'll just have one color buffer attachment for our framebuffer represented by one of the swapchain images
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = vulkanSwapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;  // No MSAA (hence 1 sample per pixel)
	// Color buffer load and store specification:
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// We're not using the stencil buffer right now:
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// Initial and Final states of the Images before and after the render pass
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Images to be used for presentation in swap chain

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	// Subpasses (We'll have only one subpass for now)
	// There can be multiple subpasses when there's postprocessing needed, for example.
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;  // Stating that this is a graphics subpass
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.colorAttachmentCount = 1;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	// Subpass Dependency management:
	VkSubpassDependency subpassDependency{};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


	// Render Pass creation
	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pAttachments = attachments.data();
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;
	renderPassCreateInfo.dependencyCount = 1;

	VkResult result = vkCreateRenderPass(vulkanLogicalDevice, &renderPassCreateInfo, nullptr, &vulkanRenderPass);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create render pass!");
	}
	std::cout << "> Created render pass successfully.\n";

}

void Application::createDescriptorSetLayout() {
	// Specify the UBO Layout Binding
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;  // For image sampling descriptors (optional)

	// Specify the Texture Sampler Layout Binding
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	// Layout bindings
	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	// Create the Descriptor Set Layout
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	descriptorSetLayoutCreateInfo.pBindings = bindings.data();

	VkResult result = vkCreateDescriptorSetLayout(vulkanLogicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &vulkanDescriptorSetLayout);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create Descriptor Set Layout!");
	}
	std::cout << "> Created Vulkan descriptor set layout successfully.\n";

}

void Application::createGraphicsPipeline() {
	// Read in the compiled Vertex and Fragment shaders (Spir-V)
	auto vertShaderCode = readFile("shaders/vert.spv");
	auto fragShaderCode = readFile("shaders/frag.spv");

	// Create the shader modules from the compiled shader code and assign them to their respective pipeline stages
	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";  // the entry point of the shader code

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";  // the entry point of the shader code

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// Describing the vertex input to the Vulkan vertex shader
	auto vertexBindingDecription = Vertex::getBindingDescription();
	auto vertexAttributeDescription = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexDataInputInfo{};
	vertexDataInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexDataInputInfo.pVertexBindingDescriptions = &vertexBindingDecription;
	vertexDataInputInfo.vertexBindingDescriptionCount = 1;
	vertexDataInputInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();
	vertexDataInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescription.size());


	// Input assembler creation description:
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	// Viewport definition
	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float)vulkanSwapChainExtent.width;
	viewport.height = (float)vulkanSwapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// Scissor definition (we'll just draw the entire framebuffer for now)
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = vulkanSwapChainExtent;

	// Make certain parts of the pipeline dynamic [Viewport and Scissor]
	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicPipelineCreateInfo{};
	dynamicPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicPipelineCreateInfo.pDynamicStates = dynamicStates.data();
	dynamicPipelineCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());

	// Telling Vulkan about our Viewports and Scissors (only mention the counts for a dynamic version of the two)
	// If we need a static viewport and scissor (unlikely), just pass the reference to the viewport and scissor now
	// Since we're using a dynamic viewport and scissor, we'll be passing them later using 'vkCmd' commands
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.scissorCount = 1;

	// Rasterizer info
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;  // Disable any output to the framebuffer (geometry never passes through the rasterizer)
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = RASTERIZER_CULL_MODE;
	// Vulkan uses the order of the vertices when projected on to the screen to determine front/back faces
	// We're telling Vulkan what order (clockwise/anti-clockwise) must be treated as front face [OBJ, GLTF etc. use anti-clockwise]
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// Multisampling (Anti-Aliasing) - Disabled for now
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// Depth Stencil state create info
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	// Color Blend attachment properties (per attached framebuffer)
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// Opaque rendering (no color blending) - What this means is new colors simply overwrite the old colors of the frame buffer
	colorBlendAttachment.blendEnable = VK_FALSE;

	// Color Blending stage properties (global color blend settings)
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.attachmentCount = 1;

	// Defining the Pipeline layout (specifies the 'uniforms' (global shader variables) that can be changed at runtime)
	// Creating an empty pipeline layout for now
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;  // Descriptor set layouts count
	pipelineLayoutCreateInfo.pSetLayouts = &vulkanDescriptorSetLayout;  // Descriptor set layouts
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	// Create the pipeline layout
	VkResult result = vkCreatePipelineLayout(vulkanLogicalDevice, &pipelineLayoutCreateInfo, nullptr, &vulkanPipelineLayout);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create pipeline layout!");
	}
	std::cout << "> Created pipeline layout successfully.\n";


	// Specify how to create the Graphics Pipeline:
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.pStages = shaderStages;  // The vertex and fragment shader stages (array created above)
	graphicsPipelineCreateInfo.stageCount = 2;
	// Specifying each stage of the pipeline:
	graphicsPipelineCreateInfo.pVertexInputState = &vertexDataInputInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizer;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampling;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlending;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicPipelineCreateInfo;
	// Pipeline layout:
	graphicsPipelineCreateInfo.layout = vulkanPipelineLayout;
	// Render pass and Sub passes:
	graphicsPipelineCreateInfo.renderPass = vulkanRenderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	// Possible to derive a pipeline more efficienbtly from an existing pipeline (if they share a lot in common)
	// We won't be using this feature here, since we have only one graphics pipeline
	graphicsPipelineCreateInfo.basePipelineHandle = nullptr;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	// Create the Graphics Pipeline:
	result = vkCreateGraphicsPipelines(
		vulkanLogicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &vulkanGraphicsPipeline
	);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create Vulkan Graphics Pipeline!");
	}
	std::cout << "> Vulkan graphics pipeline created successfully.\n";

	vkDestroyShaderModule(vulkanLogicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(vulkanLogicalDevice, fragShaderModule, nullptr);
}

void Application::createFramebuffers() {
	// Each framebuffer wraps a SwapChain image view
	vulkanSwapChainFramebuffers.resize(vulkanSwapChainImageViews.size());

	for (size_t i{ 0 }; i < vulkanSwapChainImageViews.size(); i++) {
		VkImageView framebufferAttachments[] = {
			// We're only attaching the Color attachment for now 
			// Can have Depth, Stencil and Resolve[MSAA] in the future per framebuffer
			vulkanSwapChainImageViews[i],
			depthImageView
		};

		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = vulkanRenderPass;
		framebufferCreateInfo.pAttachments = framebufferAttachments;
		framebufferCreateInfo.attachmentCount = 2;  // Color and Depth attachments
		framebufferCreateInfo.width = vulkanSwapChainExtent.width;
		framebufferCreateInfo.height = vulkanSwapChainExtent.height;
		framebufferCreateInfo.layers = 1;  // 2D Images (Will be > 1 for Stereoscopic 3D)

		VkResult result = vkCreateFramebuffer(vulkanLogicalDevice, &framebufferCreateInfo, nullptr, &vulkanSwapChainFramebuffers.at(i));
		if (result != VK_SUCCESS) {
			throw std::runtime_error("RUNTIME ERROR: Failed to create Framebuffers!");
		}
		std::cout << "> Created Vulkan swapchain framebuffer for view " << i << " successfully.\n";

	}
}

/// <summary>
/// A custom abstraction that helps create the various buffers we need (eg: vertex buffer, staging buffer).
/// </summary>
/// <param name="logicalDevice"> = The Vulkan logical device instance. </param>
/// <param name="size"> = Size in bytes needed to allocate the buffer. </param>
/// <param name="usage"> = Specifies the usage of this buffer (eg: VK_BUFFER_USAGE_VERTEX_BUFFER_BIT). vert</param>
/// <param name="queueFamilyIndices"> = (Optional) Pass the indices of the queue families that can access this buffer. Passing this field will switch the 'sharingMode' of the buffer to CONCURRENT mode instead of EXCLUSIVE mode. </param>
/// <param name="memoryProperties"> = The memory properties of this buffer (eg: HOST_VISIBLE, DEVICE_LOCAL, etc.) </param>
/// <param name="outVkBuffer"> = (Output) The resultant buffer. </param>
/// <param name="outBufferMemory"> = (Output) The resultant memory allocated for the buffer. </param>
/// <param name="queueFamilyIndices"> = (Optional Param) The indices of the queue families that will be sharing this buffer. </param>
void Application::createBuffer(VkDevice logicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkBuffer& outVkBuffer, VkDeviceMemory& outBufferMemory, const std::vector<uint32_t>& queueFamilyIndices) {

	// Specify the buffer creation
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	if (queueFamilyIndices.empty()) {
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferCreateInfo.pQueueFamilyIndices = nullptr;
		bufferCreateInfo.queueFamilyIndexCount = 0;
	}
	else {
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
		bufferCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
	}

	// Create the buffer
	VkResult result = vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &outVkBuffer);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create buffer!");
	}

	// Get memory requirememts for this buffer
	VkMemoryRequirements memRequirements{};
	vkGetBufferMemoryRequirements(logicalDevice, outVkBuffer, &memRequirements);

#ifdef NDEBUG
// Release mode
#else
	// Debug mode
	std::cout << "\nDEBUG LOG: Suitable memory-type bits fetched for buffer allocation (binary) = " << 
		std::bitset<32>(memRequirements.memoryTypeBits) << "\n";
#endif

	// Allocate the memory for this buffer
	VkMemoryAllocateInfo memAllocateInfo{};
	memAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocateInfo.allocationSize = memRequirements.size;
	memAllocateInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, memoryProperties);

	result = vkAllocateMemory(logicalDevice, &memAllocateInfo, nullptr, &outBufferMemory);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to allocate device memory for buffer!");
	}

	// Bind the allocated memory and the buffer created
	vkBindBufferMemory(logicalDevice, outVkBuffer, outBufferMemory, 0);
}

/// <summary>
/// A custom abstraction that helps create Vulkan Images (VkImage) and allocate memory for them (VkDeviceMemory).
/// </summary>
/// <param name="logicalDevice"> = The Vulkan logical device instance. </param>
/// <param name="width"> = Width of the image (number of texels in row). </param>
/// <param name="height"> = Height of the image (number of texels in column).</param>
/// <param name="imageFormat"> = The format of the image. (eg: VK_FORMAT_R8G8B8A8_SRGB) </param>
/// <param name="imageTiling"> = The tiling behaviour of the image. (VK_IMAGE_TILING_LINEAR or VK_IMAGE_TILING_OPTIMAL) </param>
/// <param name="usageFlags"> = The flags indicating the intended use for this image. (eg: VK_IMAGE_USAGE_SAMPLED_BIT) </param>
/// <param name="memoryProperties"> = The memory properties of the allocated image. (eg: VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) </param>
/// <param name="outImage"> = (Output) The resulting image. </param>
/// <param name="outImageDeviceMemory"> = (Output) The resulting image device memory. </param>
/// <param name="queueFamilyIndices"> = (Optional Param) The indices of the queue families that will be sharing this image. </param>
void Application::create2DVulkanImage(VkDevice logicalDevice, uint32_t width, uint32_t height, VkFormat imageFormat, VkImageTiling imageTiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryProperties, VkImage& outImage, VkDeviceMemory& outImageDeviceMemory, const std::vector<uint32_t>& queueFamilyIndices) {

	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = imageFormat;
	imageCreateInfo.tiling = imageTiling;  // For efficient access in our shader
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usageFlags;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.flags = 0;
	if (queueFamilyIndices.empty()) {
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.pQueueFamilyIndices = nullptr;
		imageCreateInfo.queueFamilyIndexCount = 0;
	}
	else {
		imageCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		imageCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
		imageCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
	}

	VkResult result = vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &outImage);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create VkImage type member 'textureImage'");
	}
	std::cout << "> Created Vulkan texture image successfully.\n";

	// Allocate memory for the created image
	VkMemoryRequirements imageMemoryRequirements;
	vkGetImageMemoryRequirements(logicalDevice, outImage, &imageMemoryRequirements);

	VkMemoryAllocateInfo imageMemoryAllocInfo{};
	imageMemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	imageMemoryAllocInfo.allocationSize = imageMemoryRequirements.size;
	imageMemoryAllocInfo.memoryTypeIndex = findMemoryType(imageMemoryRequirements.memoryTypeBits, memoryProperties);
	result = vkAllocateMemory(logicalDevice, &imageMemoryAllocInfo, nullptr, &outImageDeviceMemory);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to allocate memory for image!");
	}
	std::cout << "> Allocated memory for Vulkan image successfully.\n";

	vkBindImageMemory(logicalDevice, outImage, outImageDeviceMemory, 0);

}

VkFormat Application::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(vulkanPhysicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("RUNTIME ERROR: Failed to find supported format!");
}

VkFormat Application::findDepthFormat() {
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool Application::hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Application::createDepthResources() {
	VkFormat depthFormat = findDepthFormat();

	create2DVulkanImage(vulkanLogicalDevice, vulkanSwapChainExtent.width, vulkanSwapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
	depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

}

VkImageView Application::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(vulkanLogicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void Application::createTextureSampler() {
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(vulkanPhysicalDevice, &properties);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(vulkanLogicalDevice, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
	std::cout << "> Created texture sampler successfully.\n";

}

/// @brief Used for recording ONE_TIME_SUBMIT commands to the Transfer Command Buffer.
void Application::beginSingleTimeTransferCommands() {
	// Reset the command buffer before recording
	vkResetCommandBuffer(vulkanTransferCommandBuffer, 0);

	// Begin the transfer command buffer
	VkCommandBufferBeginInfo transferCommandBufferBeginInfo{};
	transferCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	transferCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(vulkanTransferCommandBuffer, &transferCommandBufferBeginInfo);
}

/// @brief Used for ending the recorded ONE_TIME_SUBMIT commands of the Transfer Command Buffer.
void Application::submitAndEndSingleTimeTransferCommands() {
	// End the command buffer (stop recording, as it only contains the one copy command)
	vkEndCommandBuffer(vulkanTransferCommandBuffer);

	// Submit the command to the transfer queue and wait for its completion
	VkSubmitInfo submitCommandInfo{};
	submitCommandInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitCommandInfo.pCommandBuffers = &vulkanTransferCommandBuffer;
	submitCommandInfo.commandBufferCount = 1;

	vkQueueSubmit(deviceTransferQueue, 1, &submitCommandInfo, nullptr);
	vkQueueWaitIdle(deviceTransferQueue);  // Waits for the transfer queue to become idle
	/*
		NOTE:
		A fence would allow you to schedule multiple transfers simultaneously and wait for all of them complete,
		instead of executing one at a time. That may give the driver more opportunities to optimize.
	*/
}

/// @brief Function to copy the contents from one buffer to another.
void Application::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	beginSingleTimeTransferCommands();
	// Record the copy command to the transfer command buffer
	VkBufferCopy bufferCopy{};
	bufferCopy.srcOffset = 0;
	bufferCopy.dstOffset = 0;
	bufferCopy.size = size;
	vkCmdCopyBuffer(vulkanTransferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopy);
	submitAndEndSingleTimeTransferCommands();
}

void Application::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	beginSingleTimeTransferCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		vulkanTransferCommandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	submitAndEndSingleTimeTransferCommands();
}

void Application::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	beginSingleTimeTransferCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		vulkanTransferCommandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	submitAndEndSingleTimeTransferCommands();
}

bool Application::isPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice) {
	// We're deeming a GPU as suitable if it has the Queue Families that we need (eg. Graphics family)
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	bool deviceExtensionsSupported = checkPhysicalDeviceExtensionsSupport(physicalDevice);
	bool swapChainSupportAdequate{ false };

	if (deviceExtensionsSupported) {
		SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(physicalDevice);
		// If we have atleast 1 surface format and 1 presentation mode supported, thats adequate for now
		swapChainSupportAdequate = !swapChainSupportDetails.surfaceFormats.empty() && !swapChainSupportDetails.presentationModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	// IMPORTANT: Need to check for 'swapChainSupportAdequate' AFTER 'deviceExtensionsSupported' due to the way AND conditions work
	return indices.isComplete() && deviceExtensionsSupported && swapChainSupportAdequate && supportedFeatures.samplerAnisotropy;
}

/// @brief Finds the required Queue Family indices in the Physical-Device.
/// @brief Tries to find a Transfer queue family thats different from a Graphics queue family.
QueueFamilyIndices Application::findQueueFamilies(VkPhysicalDevice physicalDevice) {
	QueueFamilyIndices indices;

	// Get the list of queue families
	uint32_t queueFamilyCount{};
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	// Need to find at least one queue family that supports VK_QUEUE_GRAPHICS_BIT
	// And one queue family that supports VK_QUEUE_TRANSFER_BIT that is not the same family as the VK_QUEUE_GRAPHICS_BIT
	bool foundGraphicsQueue{ false };
	bool foundPresentationQueue{ false };
	size_t i{ 0 };
	for (const auto& queueFamily : queueFamilies) {
		// Check for presentation support by the queue family
		VkBool32 presentationSupport{ false };
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, vulkanSurface, &presentationSupport);

		// Check if queue family supports graphics queue
		if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !foundGraphicsQueue) {
			indices.graphicsFamily = i;
			foundGraphicsQueue = true;
		}
		// If current queue family supports presentation, set its family index
		if (presentationSupport && !foundPresentationQueue) {
			indices.presentationFamily = i;
			foundPresentationQueue = true;
		}
		// Look for dedicated transfer queue families (no graphics queue)
		if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
			indices.transferFamily = i;
		}
		++i;
	}

	// If couldn't find a dedicated transfer queue, fallback to the graphics queue family
	// Note: Graphics queue families & Compute queue families are guaranteed to have a Transfer queue family
	if (!indices.transferFamily.has_value()) {
		indices.transferFamily = indices.graphicsFamily.value();
	}

	// Check if all the queue families were found
	if (!indices.isComplete()) {
		throw std::runtime_error("RUNTIME ERROR: Failed to find required Queue Families!");
	}

#ifdef NDEBUG
	// Release Mode
#else
	// Debug Mode
	if (!queueFamiliesLoggedToDebugAlready) {
		std::cout << "\nDEBUG LOG: Assigned the following queue families:\n";
		std::cout << "DEBUG LOG: Graphics queue family = " << indices.graphicsFamily.value() << "\n";
		std::cout << "DEBUG LOG: Presentation queue family = " << indices.presentationFamily.value() << "\n";
		std::cout << "DEBUG LOG: Transfer queue family = " << indices.transferFamily.value() << "\n\n";
		queueFamiliesLoggedToDebugAlready = true;
	}
#endif

	return indices;
}

SwapChainSupportDetails Application::querySwapChainSupport(VkPhysicalDevice physicalDevice) {
	SwapChainSupportDetails swapchainSupportDetails{};

	// Get the surface capabilities of the physical device
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, vulkanSurface, &swapchainSupportDetails.surfaceCapabilities);

	// Get the supported surface formats by the physical device
	uint32_t surfaceFormatsCount{};
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vulkanSurface, &surfaceFormatsCount, nullptr);
	if (surfaceFormatsCount != 0) {
		swapchainSupportDetails.surfaceFormats.resize(surfaceFormatsCount);
	}
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vulkanSurface, &surfaceFormatsCount, swapchainSupportDetails.surfaceFormats.data());

	// Get the supported presentation modes by the physical device
	uint32_t presentationModesCount{};
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vulkanSurface, &presentationModesCount, nullptr);
	if (presentationModesCount != 0) {
		swapchainSupportDetails.presentationModes.resize(presentationModesCount);
	}
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vulkanSurface, &presentationModesCount, swapchainSupportDetails.presentationModes.data());

	return swapchainSupportDetails;
}

VkSurfaceFormatKHR Application::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats) {
	for (const VkSurfaceFormatKHR& availableSurfaceFormat : availableSurfaceFormats) {
		// We'd prefer to have the BGR (8-bit) format and the SRGB non linear color space for our surface
		if (availableSurfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableSurfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
			// If supported, return it
			return availableSurfaceFormat;
		}
	}
	// Desired format not supported (return the first supported one)
	// Can go deeper and perform ranking of the available formats and colorspaces to pick the best too...
	return availableSurfaceFormats.at(0);
}

VkPresentModeKHR Application::chooseSwapPresentationMode(const std::vector<VkPresentModeKHR>& availablePresentationModes) {
	// We'll prefer the MAILBOX presentation mode (suitable for desktop apps, but not for mobile apps due to power consumption & wastage of images)
	for (const VkPresentModeKHR& presentationMode : availablePresentationModes) {
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentationMode;
		}
	}
	// FIFO is guaranteed to exist, if we couldn't find the best one
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) {
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		// If its not max, that means the GPU requires a fixed width & height for the swapchain (eg: mobile GPUs)
		return surfaceCapabilities.currentExtent;
	}
	int width{};
	int height{};
	glfwGetFramebufferSize(window, &width, &height);

	VkExtent2D actualExtent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};
	actualExtent.width = std::clamp(actualExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

	return actualExtent;
}

/// @brief Checks if all the validation layers requested are supported by Vulkan or not.
/// @return boolean True if all the validation layers are supported. False otherwise.
bool Application::checkValidationLayersSupport() {
	// Get all the available Vulkan instance layers
	uint32_t layerCount{};
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availabeInstanceLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availabeInstanceLayers.data());

	// Check if all the requested layers are supported (found) or not
	for (const char* requestedLayer : vulkanValidationLayers) {
		bool layerFound{ false };
		for (const auto& availableLayerProperty : availabeInstanceLayers) {
			if (strcmp(requestedLayer, availableLayerProperty.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) {
			// Requested validation layers were NOT found
			return false;
		}
	}

	// All requested validation layers were found
	return true;
}

bool Application::checkPhysicalDeviceExtensionsSupport(VkPhysicalDevice physicalDevice) {
	// Get the available device extensions
	uint32_t availableExtensionsCount{};
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableExtensionsCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(availableExtensionsCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableExtensionsCount, availableExtensions.data());

	// Create a set of required extensions (copies unique contents from deviceExtensions member and stores)
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	// Tick off all the required extensions that are already there
	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	// Return true if all the required extensions are already existing (set is empty; everything ticked off)
	return requiredExtensions.empty();
}

/// @brief Creates and returns a VkShaderModule wrapper around the Spir-V compiled shader code.
VkShaderModule Application::createShaderModule(const std::vector<char>& compiledShaderCode) {
	VkShaderModuleCreateInfo shaderModuleCreateInfo{};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = compiledShaderCode.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(compiledShaderCode.data());

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(vulkanLogicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create shader module!");
	}
	return shaderModule;
}

/// @brief Creates the pool for creating graphics command buffers from.
void Application::createGraphicsCommandPool() {
	// Fetch the queue families of the GPU
	QueueFamilyIndices queueFamilies = findQueueFamilies(vulkanPhysicalDevice);

	// Specify how to create the Graphics Command Pool
	VkCommandPoolCreateInfo graphicsCommandPoolCreateInfo{};
	graphicsCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	// This flag provides more fine grain control over individual command buffer in this pool
	graphicsCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	// Each command pool can only allocate command buffers that are submitted on a single type of queue (In this case Graphics queue for draw cmds)
	graphicsCommandPoolCreateInfo.queueFamilyIndex = queueFamilies.graphicsFamily.value();


	// Create the Graphics Command Pool
	VkResult result = vkCreateCommandPool(vulkanLogicalDevice, &graphicsCommandPoolCreateInfo, nullptr, &vulkanGraphicsCommandPool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create Graphics command pool.");
	}
	std::cout << "> Created Vulkan graphics command pool successfully.\n";
}

/// @brief Creates the pool for creating transfer command buffers from.
void Application::createTransferCommandPool() {
	// Fetch the queue families from the GPU
	QueueFamilyIndices queueFamilies = findQueueFamilies(vulkanPhysicalDevice);

	// Specify how to create the Transfer Command Pool
	VkCommandPoolCreateInfo transferCommandPoolCreateInfo{};
	transferCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	// This flag provides more fine grain control over individual command buffer in this pool
	transferCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	transferCommandPoolCreateInfo.queueFamilyIndex = queueFamilies.transferFamily.value();

	// Create the Transfer Command Pool
	VkResult result = vkCreateCommandPool(vulkanLogicalDevice, &transferCommandPoolCreateInfo, nullptr, &vulkanTransferCommandPool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create Transfer command pool!");
	}
	std::cout << "> Created Vulkan transfer command pool successfully.\n";

}

void Application::createVertexBuffer() {
	// Find the queue family indices that share the buffer
	QueueFamilyIndices indices = findQueueFamilies(vulkanPhysicalDevice);
	std::vector<uint32_t> queueFamilyIndices = { indices.graphicsFamily.value(), indices.transferFamily.value() };

	// Size of the buffer needed for vertex and staging buffers
	VkDeviceSize bufferSize = sizeof(vertices.at(0)) * vertices.size();

	// Create Staging Buffer on CPU Visible Memory (Host will upload the vertex data into this buffer)
	VkBuffer stagingBuffer{};
	VkDeviceMemory stagingBufferMemory{};
	createBuffer(
		vulkanLogicalDevice,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  // Buffer can be used as Source in a memory transfer operation.
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);
	void* data;
	vkMapMemory(vulkanLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	// Copy the vertex data to the mapped memory via memcpy:
	memcpy(data, vertices.data(), (size_t)bufferSize);
	// Unmap the memory after writing the data
	vkUnmapMemory(vulkanLogicalDevice, stagingBufferMemory);


	// Create the Vertex buffer	on GPU-Only Visible Memory
	createBuffer(
		vulkanLogicalDevice,
		bufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBuffer,
		vertexBufferMemory,
		queueFamilyIndices
	);

	// Copy the data from Staging Buffer to Vertex Buffer (CPU Visible memory -> High performance memory)
	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	// Destroy the Staging buffer
	vkDestroyBuffer(vulkanLogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(vulkanLogicalDevice, stagingBufferMemory, nullptr);

}

void Application::createIndexBuffer() {
	// Find the queue family indices that will share the Index Buffer
	QueueFamilyIndices queueFamilies = findQueueFamilies(vulkanPhysicalDevice);
	std::vector<uint32_t> queueFamilyIndices = {queueFamilies.graphicsFamily.value(), queueFamilies.transferFamily.value()};

	// Buffer size of the index buffer
	VkDeviceSize bufferSize = sizeof(indices.at(0)) * indices.size();

	// Create the Staging Buffer (host visible)
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(
		vulkanLogicalDevice,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);
	void* data;
	vkMapMemory(vulkanLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(vulkanLogicalDevice, stagingBufferMemory);

	// Create the Index Buffer (device local)
	createBuffer(
		vulkanLogicalDevice,
		bufferSize,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBuffer,
		indexBufferMemory,
		queueFamilyIndices
	);

	// Copy the data from Staging to Index buffer (Host Visible -> High Performance Memory)
	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	// Destroy the staging buffer and free the memory allocated to it
	vkDestroyBuffer(vulkanLogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(vulkanLogicalDevice, stagingBufferMemory, nullptr);

}

void Application::createUniformBuffers() {
	// Find the queue family indices that will share the Index Buffer
	//QueueFamilyIndices queueFamilies = findQueueFamilies(vulkanPhysicalDevice);
	//std::vector<uint32_t> queueFamilyIndices = { queueFamilies.graphicsFamily.value(), queueFamilies.transferFamily.value() };

	// Memory required for the UBO
	VkDeviceSize uboBufferSize = sizeof(UniformBufferObject);

	// Resize the Uniform Buffers for MAX_FRAMES_IN_FLIGHT
	uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i{ 0 }; i < MAX_FRAMES_IN_FLIGHT; i++) {
		// Create the Uniform Buffer
		createBuffer(
			vulkanLogicalDevice,
			uboBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniformBuffers.at(i),
			uniformBuffersMemory.at(i)
			//queueFamilyIndices
		);

		// Persistent Memory Mapping (we'll be accessing the UBO every draw call)
		vkMapMemory(vulkanLogicalDevice, uniformBuffersMemory.at(i), 0, uboBufferSize, 0, &uniformBuffersMapped.at(i));
	}
}

void Application::createDescriptorPool() {
	// Type of descriptors in our pool, plus the size of the pool
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	// Create the Descriptor pool
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkResult result = vkCreateDescriptorPool(vulkanLogicalDevice, &descriptorPoolCreateInfo, nullptr, &vulkanDescriptorPool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to create Descriptor Pool!");
	}
	std::cout << "> Created Vulkan descriptor pool successfully.\n";
}

void Application::createDescriptorSets() {
	// Descriptor Layout for each Descriptor Set
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT, vulkanDescriptorSetLayout);

	// Allocate Descriptor Sets
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocInfo.descriptorPool = vulkanDescriptorPool;
	descriptorSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	descriptorSetAllocInfo.pSetLayouts = descriptorSetLayouts.data();

	vulkanDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	VkResult result = vkAllocateDescriptorSets(vulkanLogicalDevice, &descriptorSetAllocInfo, vulkanDescriptorSets.data());
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to allocate Descriptor Sets!");
	}
	std::cout << "> Created Vulkan descriptor sets successfully.\n";

	// Populate every descriptor
	for (size_t i{ 0 }; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImageView;
		imageInfo.sampler = textureSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = vulkanDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = vulkanDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(vulkanLogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

		std::cout << "> Updated descriptor set " << i << "\n";
	}
}

void Application::updateUniformBuffers(uint32_t currentImage) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	
	// viking room model
	if (MODEL_PATH == viking_room_model_path) {
		ubo.model = glm::rotate(glm::mat4(1.0f), deltaTime * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), vulkanSwapChainExtent.width / (float)vulkanSwapChainExtent.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;
	}
	// viking house model
	else {
		ubo.model = glm::rotate(glm::mat4(1.0f), deltaTime * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(60.0f, 40.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(35.0f), vulkanSwapChainExtent.width / (float)vulkanSwapChainExtent.height, 0.1f, 200.0f);
		ubo.proj[1][1] *= -1;
	}

	memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}


/// @brief Creates Graphics command buffers from the graphics command pool.
void Application::createGraphicsCommandBuffers() {

	vulkanGraphicsCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	// Specify how to allocate command buffers and from which command pool
	/*
		Note:
		- VK_COMMAND_BUFFER_LEVEL_PRIMARY:		Can be submitted to a queue for execution, but cannot be called from other command buffers.
		- VK_COMMAND_BUFFER_LEVEL_SECONDARY:	Cannot be submitted directly, but can be called from primary command buffers
	*/
	VkCommandBufferAllocateInfo graphicsCommandBuffersAllocateInfo{};
	graphicsCommandBuffersAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	graphicsCommandBuffersAllocateInfo.commandPool = vulkanGraphicsCommandPool;
	graphicsCommandBuffersAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	graphicsCommandBuffersAllocateInfo.commandBufferCount = (uint32_t)vulkanGraphicsCommandBuffers.size();

	// Allocate the Graphics Command Buffer(s):
	VkResult result = vkAllocateCommandBuffers(vulkanLogicalDevice, &graphicsCommandBuffersAllocateInfo, vulkanGraphicsCommandBuffers.data());
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to allocate Graphics Command Buffers from the Graphics Command Pool!\n");
	}
	std::cout << "> Allocated Vulkan graphics command buffer(s), from graphics command pool successfully.\n";

}

/// @brief Creates Transfer command buffer from the transfer command pool.
void Application::createTransferCommandBuffer() {
	// Specify how and from which pool to allocate this buffer
	VkCommandBufferAllocateInfo transferCommandBufferAllocateInfo{};
	transferCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	transferCommandBufferAllocateInfo.commandPool = vulkanTransferCommandPool;
	transferCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	transferCommandBufferAllocateInfo.commandBufferCount = 1;

	// Allocate the Transfer Command Buffer
	VkResult result = vkAllocateCommandBuffers(vulkanLogicalDevice, &transferCommandBufferAllocateInfo, &vulkanTransferCommandBuffer);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to allocate Transfer Command Buffer from the Transfer Command Pool!\n");
	}
	std::cout << "> Allocated Vulkan transfer command buffer, from transfer command pool successfully.\n";

}

/// @brief Function that writes the commands we want to execute into a command buffer.
/// @param commandBuffer: The command buffer (VkCommandBuffer object) that you want to write the command to.
/// @param swapChainImageIndex: The index of the SwapChain image that you want to write to.
void Application::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t swapChainImageIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;  // optional
	beginInfo.pInheritanceInfo = nullptr;  // optional (only relevant for Secondary Command Buffers)

	VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to begin recording Command Buffer!");
	}

	// Begin the Render Pass
	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = vulkanRenderPass;
	renderPassBeginInfo.framebuffer = vulkanSwapChainFramebuffers.at(swapChainImageIndex);
	renderPassBeginInfo.renderArea.offset = { 0,0 };
	renderPassBeginInfo.renderArea.extent = vulkanSwapChainExtent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassBeginInfo.pClearValues = clearValues.data();  // Clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR (black in this case)
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());


	// Begin the render pass (commands will be embedded in the Primary command buffer itself. No usage of secondary cmd buffers)
	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Bind the Graphics Pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanGraphicsPipeline);

	// Bind the Vertex Buffer
	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	// Bind the Index Buffer
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);


	// We specified viewport and scissor state for this pipeline to be dynamic. 
	// So we need to set them in the command buffer before issuing our draw command.
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	viewport.width = static_cast<float>(vulkanSwapChainExtent.width);
	viewport.height = static_cast<float>(vulkanSwapChainExtent.height);
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = vulkanSwapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	// Bind descriptor sets
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelineLayout, 0, 1, &vulkanDescriptorSets[currentFrame], 0, nullptr);

	// Issue the Draw command for the Triangle
	// Use 1 for instanceCount if NOT using instanced rendering
	//vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

	// End the Render Pass
	vkCmdEndRenderPass(commandBuffer);

	// Finished recording the Command Buffer:
	result = vkEndCommandBuffer(commandBuffer);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to record Command Buffer!");
	}

}

/// @brief The render loop.
void Application::drawFrame() {

	// At the start of the frame, we want to wait until the previous frame has finished, 
	// so that the command buffer and semaphores are available to use.
	vkWaitForFences(vulkanLogicalDevice, 1, &inFlightFences.at(currentFrame), VK_TRUE, UINT64_MAX);

	// Acquiring an image from the SwapChain
	uint32_t swapChainImageIndex{};
	VkResult result = vkAcquireNextImageKHR(vulkanLogicalDevice, vulkanSwapChain, UINT64_MAX, imageAvailableSemaphores.at(currentFrame), VK_NULL_HANDLE, &swapChainImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("RUNTIME ERROR: Failed to acquire the next image from the swapchain!");
	}

	// Updating the Uniform Buffers
	updateUniformBuffers(currentFrame);

	// Only reset the fence if we are submitting work (avoiding a potential Deadlock)
	// After waiting, we need to manually reset the fence to the 'unisgnalled' state
	vkResetFences(vulkanLogicalDevice, 1, &inFlightFences.at(currentFrame));

	// Recording the Command Buffer
	vkResetCommandBuffer(vulkanGraphicsCommandBuffers.at(currentFrame), 0);
	recordCommandBuffer(vulkanGraphicsCommandBuffers.at(currentFrame), swapChainImageIndex);

	// Submit the command buffer:
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores.at(currentFrame) };  // wait semaphores
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores.at(currentFrame) };  // signal semaphores
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };  // pipeline wait stages

	VkSubmitInfo commandBufferSubmitInfo{};  // command submit info
	commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	commandBufferSubmitInfo.waitSemaphoreCount = 1;
	commandBufferSubmitInfo.signalSemaphoreCount = 1;
	commandBufferSubmitInfo.pWaitSemaphores = waitSemaphores;
	commandBufferSubmitInfo.pWaitDstStageMask = waitStages;
	commandBufferSubmitInfo.pSignalSemaphores = signalSemaphores;
	commandBufferSubmitInfo.pCommandBuffers = &vulkanGraphicsCommandBuffers.at(currentFrame);
	commandBufferSubmitInfo.commandBufferCount = 1;

	result = vkQueueSubmit(deviceGraphicsQueue, 1, &commandBufferSubmitInfo, inFlightFences.at(currentFrame));
	if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to submit draw command buffer to graphics queue!");
	}

	// Presentation
	VkSwapchainKHR swapChains[] = { vulkanSwapChain };

	VkPresentInfoKHR presentationInfo{};
	presentationInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentationInfo.pWaitSemaphores = signalSemaphores;
	presentationInfo.waitSemaphoreCount = 1;
	presentationInfo.pSwapchains = swapChains;
	presentationInfo.swapchainCount = 1;  // Will almost always be only 1
	presentationInfo.pImageIndices = &swapChainImageIndex;
	presentationInfo.pResults = nullptr; // optional: Allows specifying a VkResult array to check for success of presentation in each swapchain

	result = vkQueuePresentKHR(devicePresentationQueue, &presentationInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frameBufferResized) {
		frameBufferResized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("RUNTIME ERROR: Failed to present SwapChaim images to the Queue!");
	}

	// Increment the frame
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}

uint32_t Application::findMemoryType(uint32_t typefilter, VkMemoryPropertyFlags properties) {
	// Get the memory properties of the GPU
	VkPhysicalDeviceMemoryProperties memoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(vulkanPhysicalDevice, &memoryProperties);

	/*
		NOTE:
		Physical Device Memory properties contains 2 arrays: 
		- memoryHeaps, and
		- memoryTypes
		[Refer the Vulkan Hardware Capability Viewer tool]
		Memory Heaps are distinct memory resources (Example: VRAM and Swap-Space CPU RAM for when VRAM runs out)
		Each memoryHeap contains multiple memory types.
	*/

	// Try and find a memory-type that is suitable for the buffer
	for (uint32_t i{ 0 }; i < memoryProperties.memoryTypeCount; i++) {
		if ((typefilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			// Found a suitable memory type for the vertex buffer
			// Also found a suitable memory type to write our buffer data to

#ifdef NDEBUG
			// Release Mode
#else
			// Debug Mode
			std::cout << "DEBUG LOG: Found suitable memory-type for Buffer Memory allocation.\n";
			std::cout << "DEBUG LOG: Requested Memory Properties = ";
			if (properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
				std::cout << "DEVICE_LOCAL  ";
			if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
				std::cout << "HOST_VISIBLE  ";
			if (properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
				std::cout << "HOST_COHERENT  ";
			if (properties & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
				std::cout << "HOST_CACHED  ";
			if (properties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
				std::cout << "LAZILY_ALLOCATED  ";
			if (properties & VK_MEMORY_PROPERTY_PROTECTED_BIT)
				std::cout << "PROTECTED  ";
			if (properties & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
				std::cout << "DEVICE_COHERENT_AMD  ";
			if (properties & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
				std::cout << "DEVICE_UNCACHED_AMD  ";
			std::cout << "\n";
			std::cout << "DEBUG LOG: Chosen Memory Type = " << i << " (Refer VHCV for more info)\n\n";
#endif

			return i;
		}
	}
	throw std::runtime_error("RUNTIME ERROR: Failed to find suitable memory type!");

	return 0;
}

/// @brief Load and image and upload it into a Vulkan image object
void Application::createTextureImage() {
	int textureWidth{};
	int textureHeight{};
	int textureChannels{};

	const char* imagePath = "textures/Vulkan_Texture.png";

	// Load the texture image
	stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
	if (!pixels) {
		throw std::runtime_error("RUNTIME ERROR: Failed to load texture image!");
	}
	std::cout << "> Loaded texture image successfully.\n";

	// Create staging buffer
	VkDeviceSize imageSize = textureWidth * textureHeight * 4;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(
		vulkanLogicalDevice,
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);
	
	// Copy the data from the loaded image to the buffer
	void* stagingBufferData{ nullptr };
	vkMapMemory(vulkanLogicalDevice, stagingBufferMemory, 0, imageSize, 0, &stagingBufferData);
	memcpy(stagingBufferData, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(vulkanLogicalDevice, stagingBufferMemory);

	// Clean up the retrieved pixel data (no longer needed since its copied into staging buffer)
	stbi_image_free(pixels);


	// Create the Vulkan Image that will contain the pixel data from the staging buffer, and will be read from by our shader
	QueueFamilyIndices queueFamilies = findQueueFamilies(vulkanPhysicalDevice);
	std::vector<uint32_t> queueFamilyIndices = { queueFamilies.graphicsFamily.value(), queueFamilies.transferFamily.value() };
	create2DVulkanImage(
		vulkanLogicalDevice,
		textureWidth,
		textureHeight,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		textureImage,
		textureDeviceMemory,
		queueFamilyIndices
	);

	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(vulkanLogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(vulkanLogicalDevice, stagingBufferMemory, nullptr);
}

void Application::createTextureImageView() {
	textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

/// @brief Load a 3D Model using tinyobjloader library
void Application::load3DModel() {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			if (index.texcoord_index >= 0) {
				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}
			else {
				vertex.texCoord = { 0.0f, 0.0f };
			}

			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}

}

void Application::createSynchronizationObjects() {

	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Creates the fence initialized to the signaled state (for the first drawFrame call)

	// Create the Synchronization Objects per frame
	for (size_t i{ 0 }; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(vulkanLogicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores.at(i)) != VK_SUCCESS) {
			throw std::runtime_error("RUNTIME ERROR: Failed to create 'imageAvailableSemaphore' for frame: " + i);
		}
		if (vkCreateSemaphore(vulkanLogicalDevice, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores.at(i)) != VK_SUCCESS) {
			throw std::runtime_error("RUNTIME ERROR: Failed to create 'renderFinishedSemaphore' for frame: " + i);
		}
		if (vkCreateFence(vulkanLogicalDevice, &fenceCreateInfo, nullptr, &inFlightFences.at(i)) != VK_SUCCESS) {
			throw std::runtime_error("RUNTIME ERROR: Failed to create 'inFlightFence' for frame: " + i);
		}
	}

	std::cout << "> Created Vulkan synchronization objects successfully.\n";

}

/// @brief Callback used by GLFW when a window resize occurs (see 'initWindow' method).
void Application::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto application = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	application->frameBufferResized = true;
}

/// @brief Reads all the bytes from a specified file and return them in a byte array (vector).
std::vector<char> Application::readFile(const std::string& fileName) {
	// Benefit of starting at end of file is that we can immediately get the size and allocate a buffer accordingly
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("RUNTIME ERROR: Failed to open file '" + fileName + "'.");
	}
	// File is now open..
	// 
	// Get the file size and create a buffer of that size
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	// Read the contents of the whole file at once, into the buffer
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	// Close the file
	file.close();
	return buffer;
}