#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <algorithm>
#include <optional>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <vector>
#include <bitset>
#include <chrono>
#include <array>
#include <set>


// Forward declarations
struct QueueFamilyIndices;
struct SwapChainSupportDetails;
struct Vertex;
struct UniformBufferObject;

// APPLICATION CLASS
class Application {
public:
	void run();

private:
	// Members:
	GLFWwindow* window;
	const uint32_t WIDTH{ 800 };
	const uint32_t HEIGHT{ 600 };
	const char* APPLICATION_NAME = "Vulkan Application";

	VkInstance vulkanInstance = VK_NULL_HANDLE;
	const int MAX_FRAMES_IN_FLIGHT{ 2 };
	uint32_t currentFrame{ 0 };
	VkPhysicalDevice vulkanPhysicalDevice = VK_NULL_HANDLE;
	VkDevice vulkanLogicalDevice = VK_NULL_HANDLE;
	VkQueue deviceGraphicsQueue = VK_NULL_HANDLE;  // for draw commands
	VkQueue devicePresentationQueue = VK_NULL_HANDLE;  // for presentation of images onto screen
	VkQueue deviceTransferQueue = VK_NULL_HANDLE;  // used for buffer copying
	VkSurfaceKHR vulkanSurface = VK_NULL_HANDLE;
	VkSwapchainKHR vulkanSwapChain = VK_NULL_HANDLE;
	VkFormat vulkanSwapChainImageFormat;
	VkColorSpaceKHR vulkanSwapChainImageColorspace;
	VkExtent2D vulkanSwapChainExtent;
	VkRenderPass vulkanRenderPass = VK_NULL_HANDLE;	
	VkPipelineLayout vulkanPipelineLayout = VK_NULL_HANDLE;
	VkPipeline vulkanGraphicsPipeline = VK_NULL_HANDLE;
	VkCommandPool vulkanGraphicsCommandPool = VK_NULL_HANDLE;  // graphics command pool
	std::vector<VkCommandBuffer> vulkanGraphicsCommandBuffers;  // graphics command buffers (size based on frames in flight)
	VkCommandPool vulkanTransferCommandPool = VK_NULL_HANDLE;  // transfer command pool
	VkCommandBuffer vulkanTransferCommandBuffer = VK_NULL_HANDLE; // transfer command buffer
	std::vector<VkImage> vulkanSwapChainImages;
	std::vector<VkImageView> vulkanSwapChainImageViews;
	std::vector<VkFramebuffer> vulkanSwapChainFramebuffers;
	VkBuffer vertexBuffer = VK_NULL_HANDLE;  // vertex buffer
	VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
	VkBuffer indexBuffer = VK_NULL_HANDLE;  // index buffer
	VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
	std::vector<VkBuffer> uniformBuffers;  // uniform buffers (size based on frames in flight)
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;
	VkDescriptorSetLayout vulkanDescriptorSetLayout = VK_NULL_HANDLE;  // descriptor set layout
	VkDescriptorPool vulkanDescriptorPool = VK_NULL_HANDLE;  // descriptor pool
	std::vector<VkDescriptorSet> vulkanDescriptorSets;  // descriptor sets

	// Texture properties
	VkImage textureImage = VK_NULL_HANDLE;
	VkImageView textureImageView = VK_NULL_HANDLE;
	VkSampler textureSampler = VK_NULL_HANDLE;
	VkDeviceMemory textureDeviceMemory = VK_NULL_HANDLE;

	// Depth properties
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	// Synchronization objects:
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector <VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	bool frameBufferResized{ false };
	// Validation layers are now common for instance and devices:
	const std::vector<const char*> vulkanValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	// List of physical device extensions to check/enable:
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

#ifdef NDEBUG 
	// Release Mode:
	const bool enableVulkanValidationLayers = false;
#else         
	// Debug Mode:
	const bool enableVulkanValidationLayers = true;
	bool queueFamiliesLoggedToDebugAlready = false;
#endif       

	// Member Methods:
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();

	// Helper Methods:
	void createVulkanInstance();
	void createVulkanSurface();
	void pickVulkanPhysicalDevice();
	void createLogicalDevice();
	void recreateSwapChain();
	void cleanupSwapChain();
	void createSwapChain();
	void createSwapChainImageViews();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createFramebuffers();
	VkShaderModule createShaderModule(const std::vector<char>& compiledShaderCode);
	void createGraphicsCommandPool();
	void createTransferCommandPool();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void updateUniformBuffers(uint32_t currentImage);
	void createGraphicsCommandBuffers();
	void createTransferCommandBuffer();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t swapChainImageIndex);
	void createSynchronizationObjects();
	void drawFrame();

	void createBuffer(VkDevice logicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkBuffer& outVkBuffer, VkDeviceMemory& outBufferMemory, const std::vector<uint32_t>& queueFamilyIndices = {});
	void create2DVulkanImage(VkDevice logicalDevice, uint32_t width, uint32_t height, VkFormat imageFormat, VkImageTiling imageTiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryProperties, VkImage& outImage, VkDeviceMemory& outImageDeviceMemory, const std::vector<uint32_t>& queueFamilyIndices = {});
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	bool hasStencilComponent(VkFormat format);
	void createDepthResources();
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void createTextureSampler();
	void beginSingleTimeTransferCommands();
	void submitAndEndSingleTimeTransferCommands();
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	bool isPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats);
	VkPresentModeKHR chooseSwapPresentationMode(const std::vector<VkPresentModeKHR>& availablePresentationModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
	bool checkValidationLayersSupport();
	bool checkPhysicalDeviceExtensionsSupport(VkPhysicalDevice physicalDevice);
	uint32_t findMemoryType(uint32_t typefilter, VkMemoryPropertyFlags properties);
	void createTextureImage();
	void createTextureImageView();

	// static methods:
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	static std::vector<char> readFile(const std::string& fileName);

};


// Struct definitions:

/// @brief Custom struct that holds the queue indices for various device queue families.
struct QueueFamilyIndices {
	/// @brief The index of the Graphics queue family (if any) of the GPU.
	std::optional<uint32_t> graphicsFamily;
	/// @brief The index of the Presentation queue family (if any) of the GPU.
	std::optional<uint32_t> presentationFamily;
	/// @brief The index of the Transfer queue family (if any) of the GPU.
	std::optional<uint32_t> transferFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentationFamily.has_value() && transferFamily.has_value();
	}
};

/// @brief Custom struct that holds the data relevant to the Swapchain support of the Physical Device (GPU).
struct SwapChainSupportDetails {
	/// @brief The capabilities of the surface supported by our GPU (eg: min/max images in Swapchain).
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	/// @brief List of pixel formats and color spaces supported by our GPU (eg: SRGB color space).
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	/// @brief List of presentation modes supported for the Swapchain (eg: FIFO, Mailbox etc.).
	std::vector<VkPresentModeKHR> presentationModes;
};

///@brief Attributes to describe a vertex for the Vertex Shader.
struct Vertex {
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texCoord;

	/// @brief Tell Vulkan how to pass this data format to the vertex shader once it's been uploaded into GPU memory.
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingInputDescription{};
		bindingInputDescription.binding = 0;
		bindingInputDescription.stride = sizeof(Vertex);
		bindingInputDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingInputDescription;
	}

	// @brief Describes how to extract a vertex attribute from a chunk of vertex data originating from a binding description.
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		// We have two attributes, position and color, so we need two attribute description structs.
		std::array<VkVertexInputAttributeDescription, 3> inputAttributeDescriptions{};

		// For position data
		inputAttributeDescriptions.at(0).binding = 0;
		inputAttributeDescriptions.at(0).location = 0;  // The corresponding shader layout location for: inPosition
		inputAttributeDescriptions.at(0).format = VK_FORMAT_R32G32B32_SFLOAT;  // Yes, we reuse color formats to specify vec2 of floats
		inputAttributeDescriptions.at(0).offset = offsetof(Vertex, position);  // The offset in bytes from start of member: 'position' in the Vertex struct

		// For color data
		inputAttributeDescriptions.at(1).binding = 0;
		inputAttributeDescriptions.at(1).location = 1;  // The corresponding shader layout location for: inColor
		inputAttributeDescriptions.at(1).format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3 of floats
		inputAttributeDescriptions.at(1).offset = offsetof(Vertex, color);  // The offset in bytes from start of member: 'color' in the Vertex struct

		// For texture coordinates
		inputAttributeDescriptions.at(2).binding = 0;
		inputAttributeDescriptions.at(2).location = 2;
		inputAttributeDescriptions.at(2).format = VK_FORMAT_R32G32_SFLOAT;
		inputAttributeDescriptions.at(2).offset = offsetof(Vertex, texCoord);

		return inputAttributeDescriptions;
	}
};

// UBO definition
struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

// Indexed Vertex data of the rectangle for the vertex buffer
const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};
const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4
};


