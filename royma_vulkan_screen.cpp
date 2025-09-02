
#include <platform/vulkan/royma_vulkan_screen.h>
#include <platform/vulkan/royma_vulkan_mesh.h>
#include <mesh/royma_font.h>
#include <platform/vulkan/royma_vulkan_texture.h>
#include <render/royma_shader_source.h>
#include <light/royma_directional_light.h>
#include <light/royma_point_light.h>
#include <light/royma_spot_light.h>
#include <render/royma_deferred_render_routine.h>
#include <platform/vulkan/royma_vulkan_renderer.h>

#define VULKAN_VALIDATION

namespace royma
{
	VkAllocationCallbacks* g_pVkAllocationCallbacks = nullptr;

	VulkanScreen::DeviceInfo VulkanScreen::s_deviceInfo;

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
#ifdef VULKAN_VALIDATION
		Log::global()->record(string().fromAscii(pCallbackData->pMessage));
		assert(messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
#endif // VULKAN_VALIDATION		
		
		return VK_FALSE;
	}

#ifdef NV_DEBUG
#define AFTERMATH_CHECK_ERROR(c) verify(GFSDK_Aftermath_Result_Success == (c))

	// Static wrapper for the GPU crash dump handler. See the 'Handling GPU crash dump Callbacks' section for details.
	static void NvGpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData)
	{
		// Create a GPU crash dump decoder object for the GPU crash dump.
		GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};
		AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
			GFSDK_Aftermath_Version_API,
			pGpuCrashDump,
			gpuCrashDumpSize,
			&decoder));

		GFSDK_Aftermath_GpuCrashDump_DeviceInfo deviceInfo = {};
		GFSDK_Aftermath_GpuCrashDump_GetDeviceInfo(decoder, &deviceInfo);
		switch (deviceInfo.status)
		{
		case GFSDK_Aftermath_Device_Status_Active:
			Log::global()->record(L"The GPU is still active, and hasn't gone down");
			assert(false);
			break;
		case GFSDK_Aftermath_Device_Status_Timeout:
			Log::global()->record(L"A long running shader/operation has caused a GPU timeout. Reconfiguring the timeout length might help tease out the problem");
			assert(false);
			break;
		case GFSDK_Aftermath_Device_Status_OutOfMemory:
			Log::global()->record(L"Run out of memory to complete operations");
			assert(false);
			break;
		case GFSDK_Aftermath_Device_Status_PageFault:
		{
			Log::global()->record(L"An invalid VA access has caused a fault");
			assert(false);
			// Query GPU page fault information.
			GFSDK_Aftermath_GpuCrashDump_PageFaultInfo pageFaultInfo = {};
			GFSDK_Aftermath_Result result = GFSDK_Aftermath_GpuCrashDump_GetPageFaultInfo(decoder, &pageFaultInfo);

			if (GFSDK_Aftermath_SUCCEED(result) && result != GFSDK_Aftermath_Result_NotAvailable)
			{
				// Print information about the GPU page fault.
				/*Utility::Printf("GPU page fault at 0x%016llx", pageFaultInfo.faultingGpuVA);
				if (pageFaultInfo.bHasResourceInfo)
				{
					Utility::Printf("Fault in resource starting at 0x%016llx", pageFaultInfo.resourceInfo.gpuVa);
					Utility::Printf("Size of resource: (w x h x d x ml) = {%u, %u, %u, %u} = %llu bytes",
						pageFaultInfo.resourceInfo.width,
						pageFaultInfo.resourceInfo.height,
						pageFaultInfo.resourceInfo.depth,
						pageFaultInfo.resourceInfo.mipLevels,
						pageFaultInfo.resourceInfo.size);
					Utility::Printf("Format of resource: %u", pageFaultInfo.resourceInfo.format);
					Utility::Printf("Resource was destroyed: %d", pageFaultInfo.resourceInfo.bWasDestroyed);
				}*/
			}
			break;
		}
		case GFSDK_Aftermath_Device_Status_Stopped:
			Log::global()->record(L"The GPU has stopped executing");
			assert(false);
			break;
		case GFSDK_Aftermath_Device_Status_Reset:
			Log::global()->record(L"The device has been reset");
			assert(false);
			break;
		case GFSDK_Aftermath_Device_Status_Unknown:
			Log::global()->record(L"Unknown problem - likely using an older driver incompatible with this Aftermath feature");
			assert(false);
			break;
		case GFSDK_Aftermath_Device_Status_DmaFault:
			Log::global()->record(L"An invalid rendering call has percolated through the driver");
			assert(false);
			break;
		default:
			assert(false);
		}

		// First query active shaders count.
		uint32_t shaderCount = 0;
		GFSDK_Aftermath_Result result = GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfoCount(decoder, &shaderCount);

		if (GFSDK_Aftermath_SUCCEED(result) && result != GFSDK_Aftermath_Result_NotAvailable)
		{
			// Allocate buffer for results.
			std::vector<GFSDK_Aftermath_GpuCrashDump_ShaderInfo> shaderInfos(shaderCount);

			// Query active shaders information.
			result = GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfo(decoder, shaderCount, shaderInfos.data());

			if (GFSDK_Aftermath_SUCCEED(result))
			{
				assert(false);
				// Print information for each active shader
				/*for (const GFSDK_Aftermath_GpuCrashDump_ShaderInfo& shaderInfo : shaderInfos)
				{
					Utility::Printf("Active shader: ShaderHash = 0x%016llx ShaderInstance = 0x%016llx Shadertype = %u",
						shaderInfo.shaderHash,
						shaderInfo.shaderInstance,
						shaderInfo.shaderType);
				}*/
			}
		}

		// Flags controlling what to include in the JSON data
		const uint32_t jsonDecoderFlags =
			GFSDK_Aftermath_GpuCrashDumpDecoderFlags_SHADER_INFO |          // Include information about active shaders.
			GFSDK_Aftermath_GpuCrashDumpDecoderFlags_WARP_STATE_INFO |      // Include information about active shader warps.
			GFSDK_Aftermath_GpuCrashDumpDecoderFlags_SHADER_MAPPING_INFO;   // Try to map shader instruction addresses to shader lines.

		// Query the size of the required results buffer
		/*
		uint32_t jsonSize = 0;
		GFSDK_Aftermath_Result result = GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
			decoder,
			jsonDecoderFlags,                                            // The flags controlling what information to include in the JSON.
			GFSDK_Aftermath_GpuCrashDumpFormatterFlags_CONDENSED_OUTPUT, // Generate condensed out, i.e. omit all unnecessary whitespace.
			ShaderDebugInfoLookupCallback,                               // Callback function invoked to find shader debug information data.
			ShaderLookupCallback,                                        // Callback function invoked to find shader binary data by shader hash.
			ShaderInstructionsLookupCallback,                            // Callback function invoked to find shader binary shader data by instructions hash.
			ShaderSourceDebugDataLookupCallback,                         // Callback function invoked to find shader source debug data by shader DebugName.
			&m_gpuCrashDumpTracker,                                      // User data that will be provided to the above callback functions.
			&jsonSize);                                                  // Result of the call: size in bytes of the generated JSON data.

		if (GFSDK_Aftermath_SUCCEED(result) && result != GFSDK_Aftermath_Result_NotAvailable)
		{
			// Allocate buffer for results.
			std::vector<char> json(jsonSize);

			// Query the generated JSON data taht si cached inside the decoder object.
			result = GFSDK_Aftermath_GpuCrashDump_GetJSON(
				decoder,
				json.size(),
				json.data());
			if (GFSDK_Aftermath_SUCCEED(result))
			{
				assert(false);
				//Utility::Printf("JSON: %s", json.data());
			}
		}
		*/
		AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder));
	}

	// Static wrapper for the shader debug information handler. See the 'Handling Shader Debug Information callbacks' section for details.
	static void NvShaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData)
	{
		// Get shader debug information identifier.
		GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier = {};
		AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderDebugInfoIdentifier(GFSDK_Aftermath_Version_API, pShaderDebugInfo, shaderDebugInfoSize, &identifier));
	}

	// Static wrapper for the GPU crash dump description handler. See the 'Handling GPU Crash Dump Description Callbacks' section for details.
	static void NvCrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* pUserData)
	{
		/*GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
		pGpuCrashTracker->OnDescription(addDescription);*/
	}
#endif //NV_DEBUG

	void VulkanScreen::createInstance()
	{
		uint nExtensionCount;
		VKCALL(vkEnumerateInstanceExtensionProperties(nullptr, &nExtensionCount, nullptr));
		Buffer<VkExtensionProperties> extensionProperties(nExtensionCount);
		VKCALL(vkEnumerateInstanceExtensionProperties(nullptr, &nExtensionCount, extensionProperties.get()));

		uint nLayerCount;
		VKCALL(vkEnumerateInstanceLayerProperties(&nLayerCount, nullptr));
		Buffer<VkLayerProperties> layerProperties(nLayerCount);
		VKCALL(vkEnumerateInstanceLayerProperties(&nLayerCount, layerProperties.get()));

		VkApplicationInfo appInfo;
		memset(&appInfo, 0, sizeof(VkApplicationInfo));
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "";
		appInfo.pEngineName = "royma";
		appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);

#if ROYMA_SYSTEM_PLATFORM_IS(ROYMA_SYSTEM_PLATFORM_WINDOWS)
		const char* szNativeSufaceExtensionName = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#else
#endif //ROYMA_SYSTEM_PLATFORM
		
#ifdef ROYMA_DEBUG
		Buffer<const char*> instanceExtensionNames = { szNativeSufaceExtensionName, VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
		Buffer<const char*> instanceLayersNames = { "VK_LAYER_KHRONOS_validation" };
#else
		Buffer<const char*> instanceExtensionNames = { szNativeSufaceExtensionName, VK_KHR_SURFACE_EXTENSION_NAME };
#endif // ROYMA_DEBUG

		VkInstanceCreateInfo createInfo;
		memset(&createInfo, 0, sizeof(VkInstanceCreateInfo));
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = castval<uint>(instanceExtensionNames.count());
		createInfo.ppEnabledExtensionNames = instanceExtensionNames.get();
#ifdef ROYMA_DEBUG
		createInfo.enabledLayerCount = castval<uint>(instanceLayersNames.count());
		createInfo.ppEnabledLayerNames = instanceLayersNames.get();
#endif // ROYMA_DEBUG
		
		VKCALL(vkCreateInstance(&createInfo, g_pVkAllocationCallbacks, &m_vkInstance));

#ifdef ROYMA_DEBUG
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugCallback;
		debugCreateInfo.pUserData = nullptr; // Optional

		auto createDebugUtils = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkCreateDebugUtilsMessengerEXT");
		assert(createDebugUtils != nullptr);
		VKCALL(createDebugUtils(m_vkInstance, &debugCreateInfo, g_pVkAllocationCallbacks, &m_debugMessenger));
#endif // ROYMA_DEBUG

		uint nPhysicalDeviceCount;
		VKCALL(vkEnumeratePhysicalDevices(m_vkInstance, &nPhysicalDeviceCount, nullptr));
		Buffer<VkPhysicalDevice> physicalDevices(nPhysicalDeviceCount);
		VKCALL(vkEnumeratePhysicalDevices(m_vkInstance, &nPhysicalDeviceCount, physicalDevices.get()));

		Buffer<VkPhysicalDeviceProperties> physicalDeviceProperties(nPhysicalDeviceCount);
		Buffer<VkPhysicalDeviceMemoryProperties> physicalDeviceMemoryProperties(nPhysicalDeviceCount);
		Buffer<VkPhysicalDeviceFeatures> physicalDeviceFeatures(nPhysicalDeviceCount);
		
		for (uint i = 0; i < nPhysicalDeviceCount; ++i)
		{
			auto hDevice = physicalDevices[i];
			vkGetPhysicalDeviceProperties(hDevice, &physicalDeviceProperties[i]);
			vkGetPhysicalDeviceFeatures(hDevice, &physicalDeviceFeatures[i]);
			vkGetPhysicalDeviceMemoryProperties(hDevice, &physicalDeviceMemoryProperties[i]);

			VkFormatProperties formatProp;
			vkGetPhysicalDeviceFormatProperties(hDevice, VK_FORMAT_D32_SFLOAT_S8_UINT, &formatProp);
			if (formatProp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT && formatProp.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{

			}
			else
			{
				throw;
			}

			if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == physicalDeviceProperties[i].deviceType)
			{
				m_physicalDevices.insert({ hDevice, i }, m_physicalDevices.begin());
			}
			else if (VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU == physicalDeviceProperties[i].deviceType)
			{
				m_physicalDevices.insert({ hDevice, i });
			}
		}

		for (const auto& item : m_physicalDevices)
		{
			auto hDevice = item.first;
			auto nIndex = item.second;
			s_deviceInfo.physicalDevice = hDevice;

			const auto& memProp = physicalDeviceMemoryProperties[nIndex];
			for (uint k = 0; k < memProp.memoryTypeCount; k++)
			{
				if (memProp.memoryTypes[k].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
				{
					if (memProp.memoryTypes[k].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
					{
						if (s_deviceInfo.updateMemoryTypeIndex == 0xffffFFFF)
						{
							s_deviceInfo.updateMemoryTypeIndex = k;
						}
					}
					else if (memProp.memoryTypes[k].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
					{
						if (s_deviceInfo.constMemoryTypeIndex == 0xffffFFFF)
						{
							s_deviceInfo.constMemoryTypeIndex = k;
						}
					}					
					else
					{
						if (s_deviceInfo.commitMemoryTypeIndex == 0xffffFFFF)
						{
							s_deviceInfo.commitMemoryTypeIndex = k;
						}
					}
				}
				else if (memProp.memoryTypes[k].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
				{
					if (s_deviceInfo.videoMemoryTypeIndex == 0xffffFFFF)
					{
						s_deviceInfo.videoMemoryTypeIndex = k;
					}
				}
			}
			if (s_deviceInfo.commitMemoryTypeIndex == 0xffffFFFF)
			{
				s_deviceInfo.commitMemoryTypeIndex = s_deviceInfo.constMemoryTypeIndex;
			}

			uint nQueueFamilyCount;
			vkGetPhysicalDeviceQueueFamilyProperties(hDevice, &nQueueFamilyCount, nullptr);
			Buffer<VkQueueFamilyProperties> queueFamilyProperties(nQueueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(hDevice, &nQueueFamilyCount, queueFamilyProperties.get());

			Buffer<VkDeviceQueueCreateInfo> queueInfo(nQueueFamilyCount);
			Buffer<float> queuePriorities = { 1.0f, 1.0f, 1.0f, 0.5f, 0.0f };
			memset(queueInfo.data(), 0, nQueueFamilyCount * sizeof(VkDeviceQueueCreateInfo));
			for (uint q = 0; q < nQueueFamilyCount; ++q)
			{
				const auto& queueFamilyProperty = queueFamilyProperties[q];
				bool bIsComputeQueue = false;
				bool bIsTransferQueue = false;
				if (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
				}
				else if (queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT)
				{
					bIsComputeQueue = true;
				}
				else if (queueFamilyProperty.queueFlags & VK_QUEUE_TRANSFER_BIT)
				{
					bIsTransferQueue = true;
				}

				VkDeviceQueueCreateInfo& info = queueInfo[q];
				info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				info.queueFamilyIndex = q;
				info.queueCount = bIsComputeQueue ? 2 : 1;
				if (bIsComputeQueue)
				{
					info.pQueuePriorities = &queuePriorities[2];
				}
				else if (bIsTransferQueue)
				{
					info.pQueuePriorities = &queuePriorities[1];
				}
				else
				{
					info.pQueuePriorities = &queuePriorities[0];
				}
			}

			VkPhysicalDeviceFeatures requiredFeatures;
			memset(&requiredFeatures, 0, sizeof(VkPhysicalDeviceFeatures));

			uint nExtensionCount;
			VKCALL(vkEnumerateDeviceExtensionProperties( hDevice, nullptr, &nExtensionCount, nullptr));
			Buffer<VkExtensionProperties> extensionProperties(nExtensionCount);
			VKCALL(vkEnumerateDeviceExtensionProperties( hDevice, nullptr, &nExtensionCount, extensionProperties.get()));

#ifdef NV_DEBUG
			/// <Nsight Aftermath>
			Buffer<const char*> deviceExtensionNames = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME, VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME };
			// Set up device creation info for Aftermath feature flag configuration.
			VkDeviceDiagnosticsConfigFlagsNV aftermathFlags =
				VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |      // Enable tracking of resources.
				VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV |  // Capture call stacks for all draw calls, compute dispatches, and resource copies.
				VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV;       // Generate debug information for shaders.
			VkDeviceDiagnosticsConfigCreateInfoNV aftermathInfo = {};
			aftermathInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			aftermathInfo.flags = aftermathFlags;
			///
#else
			Buffer<const char*> deviceExtensionNames = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
#endif //NV_DEBUG

			VkDeviceCreateInfo deviceInfo;
			memset(&deviceInfo, 0, sizeof(VkDeviceCreateInfo));
			deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceInfo.queueCreateInfoCount = nQueueFamilyCount;
			deviceInfo.pQueueCreateInfos = queueInfo.get();
			deviceInfo.pEnabledFeatures = &requiredFeatures;
			deviceInfo.enabledExtensionCount = castval<uint>(deviceExtensionNames.count());
			deviceInfo.ppEnabledExtensionNames = deviceExtensionNames.get();
#ifdef NV_DEBUG
			deviceInfo.pNext = &aftermathInfo;
#endif //NV_DEBUG

			VKCALL(vkCreateDevice(hDevice, &deviceInfo, g_pVkAllocationCallbacks, &s_deviceInfo.device));

			for (uint i = 0; i < nQueueFamilyCount; i++)
			{
#if ROYMA_SYSTEM_PLATFORM_IS(ROYMA_SYSTEM_PLATFORM_WINDOWS)
				auto bSupportPresent = vkGetPhysicalDeviceWin32PresentationSupportKHR(hDevice, i);
#else
#endif //ROYMA_SYSTEM_PLATFORM

				const auto& queueFamilyProperty = queueFamilyProperties[i];
				if (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					assert(queueFamilyProperty.queueCount > 0);
					s_deviceInfo.graphicsQueueFamilyIndex = i;
					vkGetDeviceQueue(s_deviceInfo.device, i, 0, &s_deviceInfo.graphicsQueue);
				}
				else if (queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT)
				{
					assert(queueFamilyProperty.queueCount > 1);
					s_deviceInfo.computeQueueFamilyIndex = i;
					vkGetDeviceQueue(s_deviceInfo.device, i, 0, &s_deviceInfo.shortComputeQueue);
					vkGetDeviceQueue(s_deviceInfo.device, i, 1, &s_deviceInfo.longComputeQueue);
				}
				else if (queueFamilyProperty.queueFlags & VK_QUEUE_TRANSFER_BIT)
				{
					//[Todo] Can only support one transfer queue, merge streamIn and out into one queue
					assert(queueFamilyProperty.queueCount > 0);
					s_deviceInfo.transferQueueFamilyIndex = i;
					vkGetDeviceQueue(s_deviceInfo.device, i, 0, &s_deviceInfo.streamInQueue);
					//vkGetDeviceQueue(s_deviceInfo.device, i, 1, &s_deviceInfo.streamOutQueue);
				}
			}
			if (0xFFFFffff == s_deviceInfo.transferQueueFamilyIndex)
			{
				s_deviceInfo.transferQueueFamilyIndex = s_deviceInfo.graphicsQueueFamilyIndex;
				s_deviceInfo.streamInQueue = s_deviceInfo.graphicsQueue;
				s_deviceInfo.streamOutQueue = s_deviceInfo.graphicsQueue;
			}
			if (0xFFFFffff == s_deviceInfo.computeQueueFamilyIndex)
			{
				s_deviceInfo.computeQueueFamilyIndex = s_deviceInfo.graphicsQueueFamilyIndex;
				s_deviceInfo.shortComputeQueue = s_deviceInfo.graphicsQueue;
				s_deviceInfo.longComputeQueue = s_deviceInfo.graphicsQueue;
			}

#if ROYMA_SYSTEM_PLATFORM_IS(ROYMA_SYSTEM_PLATFORM_WINDOWS)
			VkWin32SurfaceCreateInfoKHR surfaceInfo;
			memset(&surfaceInfo, 0, sizeof(VkWin32SurfaceCreateInfoKHR));
			surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			surfaceInfo.hinstance = (HINSTANCE)Platform::global()->getProcessId();
			surfaceInfo.hwnd = (HWND)Platform::global()->getWindowId();

			VKCALL(vkCreateWin32SurfaceKHR(m_vkInstance, &surfaceInfo, g_pVkAllocationCallbacks, &m_surface));
#else
#endif //ROYMA_SYSTEM_PLATFORM

			uint nSurfaceFormatCount;
			VKCALL(vkGetPhysicalDeviceSurfaceFormatsKHR(hDevice, m_surface, &nSurfaceFormatCount, nullptr));
			Buffer<VkSurfaceFormatKHR> surfaceFormats(nSurfaceFormatCount);
			VKCALL(vkGetPhysicalDeviceSurfaceFormatsKHR(hDevice, m_surface, &nSurfaceFormatCount, surfaceFormats.get()));

			VkSurfaceCapabilitiesKHR surfaceCapabilities;
			VKCALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(hDevice, m_surface, &surfaceCapabilities));
			m_nWidth = surfaceCapabilities.currentExtent.width;
			m_nHeight = surfaceCapabilities.currentExtent.height;

			VkSwapchainCreateInfoKHR swapchainInfo;
			memset(&swapchainInfo, 0, sizeof(VkSwapchainCreateInfoKHR));
			swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapchainInfo.surface = m_surface;
			swapchainInfo.minImageCount = 3;
			swapchainInfo.imageFormat = surfaceFormats[0].format;
			swapchainInfo.imageColorSpace = surfaceFormats[0].colorSpace;
			swapchainInfo.imageExtent = { castval<uint>(width()), castval<uint>(height()) };
			swapchainInfo.imageArrayLayers = 1;
			swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
			swapchainInfo.preTransform = surfaceCapabilities.currentTransform;
			swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapchainInfo.clipped = VK_TRUE;
			swapchainInfo.oldSwapchain = m_swapchain;
			
			VkBool32 bSupportedSurface;
			VKCALL(vkGetPhysicalDeviceSurfaceSupportKHR(hDevice, s_deviceInfo.graphicsQueueFamilyIndex, m_surface, &bSupportedSurface));
			assert(bSupportedSurface);

			switch (swapchainInfo.imageFormat)
			{
			case VK_FORMAT_R8G8B8A8_UNORM:
				m_swapchainColorFormat = COLOR_FORMAT::RGBA_LDR;
				break;
			case VK_FORMAT_B8G8R8A8_UNORM:
				m_swapchainColorFormat = COLOR_FORMAT::BGRA_LDR;
				break;
			default:
				assert(false);
			}

			s_deviceInfo.timestampPeriod = physicalDeviceProperties[nIndex].limits.timestampPeriod;
			s_deviceInfo.maxSharedMemorySize = physicalDeviceProperties[nIndex].limits.maxComputeSharedMemorySize;
			
			if (hDevice == m_physicalDevices.front().first)
			{
				VKCALL(vkCreateSwapchainKHR(s_deviceInfo.device, &swapchainInfo, g_pVkAllocationCallbacks, &m_swapchain));

				uint nImageCount;
				VKCALL(vkGetSwapchainImagesKHR(s_deviceInfo.device, m_swapchain, &nImageCount, nullptr));
				m_swapchainImages.realloc(nImageCount);
				VKCALL(vkGetSwapchainImagesKHR(s_deviceInfo.device, m_swapchain, &nImageCount, m_swapchainImages.get()));

				VkSemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo;
				memset(&imageAcquiredSemaphoreCreateInfo, 0, sizeof(VkSemaphoreCreateInfo));
				imageAcquiredSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

				m_submitSemaphores.realloc(nImageCount * m_maxSubmitionCount);
				for (uint k = 0; k < nImageCount * m_maxSubmitionCount; k++)
				{
					VKCALL(vkCreateSemaphore(s_deviceInfo.device, &imageAcquiredSemaphoreCreateInfo, g_pVkAllocationCallbacks, &m_submitSemaphores[k]));
				}

				m_currentSwapchainImageIndex = nImageCount - 1;
			}

			break;
		}
#ifdef NV_DEBUG
		GFSDK_Aftermath_EnableGpuCrashDumps(GFSDK_Aftermath_Version_API, GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan, GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,
			NvGpuCrashDumpCallback, NvShaderDebugInfoCallback, NvCrashDumpDescriptionCallback, nullptr);
#endif //NV_DEBUG
	}

	void VulkanScreen::createCommandPools()
	{
		VkCommandPoolCreateInfo poolInfo;
		memset(&poolInfo, 0, sizeof(VkCommandPoolCreateInfo));
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

		uint nThreads = Platform::global()->getShortTaskThreadCount();
		s_deviceInfo.graphicsCommandPools.resize(nThreads);
		s_deviceInfo.computeCommandPools.resize(nThreads);
		s_deviceInfo.transferCommandPools.resize(nThreads);

		for (uint i = 0; i < nThreads; ++i)
		{
			poolInfo.queueFamilyIndex = s_deviceInfo.graphicsQueueFamilyIndex;
			VKCALL(vkCreateCommandPool(s_deviceInfo.device, &poolInfo, g_pVkAllocationCallbacks, &s_deviceInfo.graphicsCommandPools[i]));

			poolInfo.queueFamilyIndex = s_deviceInfo.computeQueueFamilyIndex;
			VKCALL(vkCreateCommandPool(s_deviceInfo.device, &poolInfo, g_pVkAllocationCallbacks, &s_deviceInfo.computeCommandPools[i]));

			poolInfo.queueFamilyIndex = s_deviceInfo.transferQueueFamilyIndex;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			VKCALL(vkCreateCommandPool(s_deviceInfo.device, &poolInfo, g_pVkAllocationCallbacks, &s_deviceInfo.transferCommandPools[i]));
		}		
	}

	void VulkanScreen::createDescriptorPool()
	{
		Buffer<VkDescriptorPoolSize> dsPoolSizes(2);
		dsPoolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 180 };
		dsPoolSizes[1] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 65526 * 3 };

		uint nTotalDsCount = 0;
		for (slong i = 0; i < dsPoolSizes.count(); i++)
		{
			nTotalDsCount += dsPoolSizes[i].descriptorCount;
		}

		VkDescriptorPoolCreateInfo dsPoolInfo;
		memset(&dsPoolInfo, 0, sizeof(VkDescriptorPoolCreateInfo));
		dsPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		//dsPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		dsPoolInfo.maxSets = nTotalDsCount;
		dsPoolInfo.poolSizeCount = castval<uint>(dsPoolSizes.count());
		dsPoolInfo.pPoolSizes = dsPoolSizes.get();

		VKCALL(vkCreateDescriptorPool(s_deviceInfo.device, &dsPoolInfo, g_pVkAllocationCallbacks, &s_deviceInfo.descriptorPool));
	}

	void VulkanScreen::createQueryPools()
	{
		
	}

	void VulkanScreen::createGraphicsCommandBuffer(VkCommandBuffer* pCBuffer, uint nCount)
	{
		VkCommandBufferAllocateInfo commandBufferInfo;
		memset(&commandBufferInfo, 0, sizeof(VkCommandBufferAllocateInfo));
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferInfo.commandPool = s_deviceInfo.graphicsCommandPools[getCurrentThreadId() - THREAD_ID_SHORT_TASK0];
		commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferInfo.commandBufferCount = nCount;

		VKCALL(vkAllocateCommandBuffers(s_deviceInfo.device, &commandBufferInfo, pCBuffer));
	}

	void VulkanScreen::createComputeCommandBuffer(VkCommandBuffer* pCBuffer, uint nCount)
	{
		VkCommandBufferAllocateInfo commandBufferInfo;
		memset(&commandBufferInfo, 0, sizeof(VkCommandBufferAllocateInfo));
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferInfo.commandPool = s_deviceInfo.computeCommandPools[getCurrentThreadId() - THREAD_ID_SHORT_TASK0];
		commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferInfo.commandBufferCount = nCount;

		VKCALL(vkAllocateCommandBuffers(s_deviceInfo.device, &commandBufferInfo, pCBuffer));
	}

	void VulkanScreen::createTransferCommandBuffer(VkCommandBuffer* pCBuffer, uint nCount)
	{		
		VkCommandBufferAllocateInfo commandBufferInfo;
		memset(&commandBufferInfo, 0, sizeof(VkCommandBufferAllocateInfo));
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferInfo.commandPool = s_deviceInfo.transferCommandPools[getCurrentThreadId() - THREAD_ID_SHORT_TASK0];
		commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferInfo.commandBufferCount = nCount;

		VKCALL(vkAllocateCommandBuffers(s_deviceInfo.device, &commandBufferInfo, pCBuffer));
	}

	void VulkanScreen::prepareRenderingTasks()
	{

	}

	strong<Screen> VulkanScreen::initialize(HWND hWnd, int nWidth, int nHeight)
	{
		HRESULT hr = S_OK;

		if (!global())
		{
			Screen::setGlobalInstance(strong<VulkanScreen>(new VulkanScreen(hWnd, nWidth, nHeight)));
			auto spScreen = cast<VulkanScreen>(Screen::global());
			spScreen->calibrateGpuClock(THREAD_ID_GPU_GRAPHICS_QUEUE);
			
			spScreen->buildSamplerDescriptorSet();

			spScreen->m_swapchainTextures.realloc(spScreen->m_swapchainImages.count());
			for (slong i = 0; i < spScreen->m_swapchainTextures.count(); i++)
			{
				strong<VulkanTexture> spTexture(new VulkanTexture);
				spTexture->createTexture2D(spScreen->m_swapchainImages[i], spScreen->width(), spScreen->height(), spScreen->m_swapchainColorFormat);
				spScreen->m_swapchainTextures[i] = spTexture;
			}

			spScreen->m_gBuffer.realloc(GBUFFER_INDEX_COUNT);

			spScreen->m_gBuffer[GBUFFER_INDEX_COLOR] = ResourceManager::createWithId<Texture2D>(L"GBUFFER_COLOR", nWidth, nHeight, TEXTURE_USAGE::RENDER_TARGET, COLOR_FORMAT::RGBA_LDR);
			spScreen->m_gBuffer[GBUFFER_INDEX_COLOR]->setMipCount(1);
			spScreen->m_gBuffer[GBUFFER_INDEX_COLOR]->commit();

			spScreen->m_gBuffer[GBUFFER_INDEX_SURFACE] = ResourceManager::createWithId<Texture2D>(L"GBUFFER_SURFACE", nWidth, nHeight, TEXTURE_USAGE::RENDER_TARGET, COLOR_FORMAT::RGBA_LDR);
			spScreen->m_gBuffer[GBUFFER_INDEX_SURFACE]->setMipCount(1);
			spScreen->m_gBuffer[GBUFFER_INDEX_SURFACE]->commit();

			spScreen->m_gBuffer[GBUFFER_INDEX_EMISSION] = ResourceManager::createWithId<Texture2D>(L"GBUFFER_EMISSION", nWidth, nHeight, TEXTURE_USAGE::RENDER_TARGET, COLOR_FORMAT::RGBA_LDR);
			spScreen->m_gBuffer[GBUFFER_INDEX_EMISSION]->setMipCount(1);
			spScreen->m_gBuffer[GBUFFER_INDEX_EMISSION]->commit();

			spScreen->m_gBuffer[GBUFFER_INDEX_NORMAL] = ResourceManager::createWithId<Texture2D>(L"GBUFFER_NORMAL", nWidth, nHeight, TEXTURE_USAGE::RENDER_TARGET, COLOR_FORMAT::RG_HDR);
			spScreen->m_gBuffer[GBUFFER_INDEX_NORMAL]->setMipCount(1);
			spScreen->m_gBuffer[GBUFFER_INDEX_NORMAL]->commit();

			spScreen->m_gBuffer[GBUFFER_INDEX_DEPTH_STENCIL] = ResourceManager::createWithId<Texture2D>(L"GBUFFER_DEPTH_STENCIL", nWidth, nHeight, TEXTURE_USAGE::DEPTH_STENCIL_BUFFER, COLOR_FORMAT::DEPTH_STENCIL);
			spScreen->m_gBuffer[GBUFFER_INDEX_DEPTH_STENCIL]->setMipCount(1);
			spScreen->m_gBuffer[GBUFFER_INDEX_DEPTH_STENCIL]->commit();

			spScreen->m_accumulationBuffer = ResourceManager::createWithId<Texture2D>(L"ACCUMULATION_BUFFER", nWidth, nHeight, TEXTURE_USAGE::RENDER_TARGET, COLOR_FORMAT::RGBA_HDR);
			spScreen->m_accumulationBuffer->setMipCount(1);
			spScreen->m_accumulationBuffer->commit();

			spScreen->m_defaultDepthStencilBuffer = ResourceManager::createWithId<Texture2D>(L"DEFAUTL_DEPTH_STENCIL", nWidth, nHeight, TEXTURE_USAGE::DEPTH_STENCIL_BUFFER, COLOR_FORMAT::DEPTH_STENCIL);
			spScreen->m_defaultDepthStencilBuffer->setMipCount(1);
			spScreen->m_defaultDepthStencilBuffer->commit();

			Light::commit(4096, 4096);

			const string strBasePath = Platform::global()->getShaderPath();
			strong<ShaderSource> spShaderSource;

			Buffer<SVertexLayout> planeLayout(2);
			planeLayout[0] = SVertexLayout(VS_SEMANTIC::POSITION, 0, 3, 0);
			planeLayout[1] = SVertexLayout(VS_SEMANTIC::TEXCOORD, 0, 2, 3);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"plane"));
			spShaderSource->loadSourceCode(formatString(L"%?plane.hlsl", strBasePath));
			spShaderSource->setVertexLayout(planeLayout);
			spShaderSource->setShaderType(SHADER_TYPE::VERTEX_SHADER);

			Buffer<SVertexLayout> lineLayout(1);
			lineLayout[0] = SVertexLayout(VS_SEMANTIC::POSITION, 0, 3, 0);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"line"));
			spShaderSource->loadSourceCode(formatString(L"%?line.hlsl", strBasePath));
			spShaderSource->setVertexLayout(lineLayout);
			spShaderSource->setShaderType(SHADER_TYPE::VERTEX_SHADER);

			Buffer<SVertexLayout> staticTransformLayout(5);
			staticTransformLayout[0] = (SVertexLayout(VS_SEMANTIC::POSITION, 0, 3, 0));
			staticTransformLayout[1] = (SVertexLayout(VS_SEMANTIC::NORMAL, 0, 3, 3));
			staticTransformLayout[2] = (SVertexLayout(VS_SEMANTIC::TANGENT, 0, 3, 6));
			staticTransformLayout[3] = (SVertexLayout(VS_SEMANTIC::BITANGENT, 0, 3, 9));
			staticTransformLayout[4] = (SVertexLayout(VS_SEMANTIC::TEXCOORD, 0, 2, 12));

			Buffer<SVertexLayout> skinnedTransformLayout(9);
			skinnedTransformLayout[0] = (SVertexLayout(VS_SEMANTIC::POSITION, 0, 3, 0));
			skinnedTransformLayout[1] = (SVertexLayout(VS_SEMANTIC::NORMAL, 0, 3, 3));
			skinnedTransformLayout[2] = (SVertexLayout(VS_SEMANTIC::TANGENT, 0, 3, 6));
			skinnedTransformLayout[3] = (SVertexLayout(VS_SEMANTIC::BITANGENT, 0, 3, 9));
			skinnedTransformLayout[4] = (SVertexLayout(VS_SEMANTIC::TEXCOORD, 0, 2, 12));
			skinnedTransformLayout[5] = (SVertexLayout(VS_SEMANTIC::BLENDINDICES, 0, 4, 14));
			skinnedTransformLayout[6] = (SVertexLayout(VS_SEMANTIC::BLENDINDICES, 1, 4, 18));
			skinnedTransformLayout[7] = (SVertexLayout(VS_SEMANTIC::BLENDWEIGHTS, 0, 4, 22));
			skinnedTransformLayout[8] = (SVertexLayout(VS_SEMANTIC::BLENDWEIGHTS, 1, 4, 26));

			Buffer<SVertexLayout> terrainTransformLayout(2);
			terrainTransformLayout[0] = (SVertexLayout(VS_SEMANTIC::POSITION, 0, 3, 0));
			terrainTransformLayout[1] = (SVertexLayout(VS_SEMANTIC::TEXCOORD, 0, 2, 3));

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"static_transform"));
			spShaderSource->loadSourceCode(formatString(L"%?static_transform.hlsl", strBasePath));
			spShaderSource->setVertexLayout(staticTransformLayout);
			spShaderSource->setShaderType(SHADER_TYPE::VERTEX_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"terrain_transform"));
			spShaderSource->loadSourceCode(formatString(L"%?terrain_transform.hlsl", strBasePath));
			spShaderSource->setVertexLayout(terrainTransformLayout);
			spShaderSource->setShaderType(SHADER_TYPE::VERTEX_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"skinned_transform"));
			spShaderSource->loadSourceCode(formatString(L"%?skinned_transform.hlsl", strBasePath));
			spShaderSource->setVertexLayout(skinnedTransformLayout);
			spShaderSource->setShaderType(SHADER_TYPE::VERTEX_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"paint"));
			spShaderSource->loadSourceCode(formatString(L"%?paint.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::PIXEL_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"rectangle"));
			spShaderSource->loadSourceCode(formatString(L"%?rectangle.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::PIXEL_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"font"));
			spShaderSource->loadSourceCode(formatString(L"%?font.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::PIXEL_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"emit"));
			spShaderSource->loadSourceCode(formatString(L"%?emit.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::PIXEL_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"tint"));
			spShaderSource->loadSourceCode(formatString(L"%?tint.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::PIXEL_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"directional_light"));
			spShaderSource->loadSourceCode(formatString(L"%?directional_light.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::PIXEL_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"point_light"));
			spShaderSource->loadSourceCode(formatString(L"%?point_light.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::PIXEL_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"spot_light"));
			spShaderSource->loadSourceCode(formatString(L"%?spot_light.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::PIXEL_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"ambient_light"));
			spShaderSource->loadSourceCode(formatString(L"%?ambient_light.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::PIXEL_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"ambient_light_pre_integration"));
			spShaderSource->loadSourceCode(formatString(L"%?ambient_light_pre_integration.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::PIXEL_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"esm"));
			spShaderSource->loadSourceCode(formatString(L"%?esm.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::PIXEL_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"shadow_mask"));
			spShaderSource->loadSourceCode(formatString(L"%?shadow_mask.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::PIXEL_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"tone_mapping"));
			spShaderSource->loadSourceCode(formatString(L"%?tone_mapping.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::PIXEL_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"terrain_partition"));
			spShaderSource->loadSourceCode(formatString(L"%?terrain_partition.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::HULL_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"terrain_height"));
			spShaderSource->loadSourceCode(formatString(L"%?terrain_height.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::DOMAIN_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"water_material"));
			spShaderSource->loadSourceCode(formatString(L"%?water_material.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::PIXEL_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"tone_mapping_histogram"));
			spShaderSource->loadSourceCode(formatString(L"%?tone_mapping_histogram.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::COMPUTE_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"skeletal_animation"));
			spShaderSource->loadSourceCode(formatString(L"%?skeletal_animation.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::COMPUTE_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"radix_sort"));
			spShaderSource->loadSourceCode(formatString(L"%?radix_sort.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::COMPUTE_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"radix_sort_partition"));
			spShaderSource->loadSourceCode(formatString(L"%?radix_sort_partition.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::COMPUTE_SHADER);

			spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), L"radix_sort_local"));
			spShaderSource->loadSourceCode(formatString(L"%?radix_sort_local.hlsl", strBasePath));
			spShaderSource->setShaderType(SHADER_TYPE::COMPUTE_SHADER);

			auto spMaterialTemplate = ResourceManager::createWithId<MaterialTemplate>(L"DefaultMaterial");
			spMaterialTemplate->setMaterialShader(L"default_material.mat");
			Buffer<SShaderResourceBindInfo> defaultTextureLayout =
			{
				{ SShaderResourceBindInfo::TEXTURE, 0, nullptr, 0, 0 },
				{ SShaderResourceBindInfo::TEXTURE, 1, nullptr, 0, 0 },
			};
			spMaterialTemplate->setTextureLayout(defaultTextureLayout);
			spMaterialTemplate->setNativeTextureLayout(spScreen->buildMeshTextureLayout(defaultTextureLayout));

			Map<utf8, utf8> shaderMacros;
			shaderMacros["NORMAL_MAP"] = "";

			spMaterialTemplate = ResourceManager::createWithId<MaterialTemplate>(L"MetalMaterial");
			spMaterialTemplate->setShaderMacros(shaderMacros);
			spMaterialTemplate->setMaterialShader(L"metal.mat");			
			spMaterialTemplate->setTextureLayout(defaultTextureLayout);
			spMaterialTemplate->setNativeTextureLayout(spScreen->buildMeshTextureLayout(defaultTextureLayout));

			spMaterialTemplate = ResourceManager::createWithId<MaterialTemplate>(L"GoldMaterial");
			spMaterialTemplate->setMaterialShader(L"gold.mat");
			spMaterialTemplate->setTextureLayout(defaultTextureLayout);
			spMaterialTemplate->setNativeTextureLayout(spScreen->buildMeshTextureLayout(defaultTextureLayout));

			spMaterialTemplate = ResourceManager::createWithId<MaterialTemplate>(L"GlossMaterial");
			spMaterialTemplate->setMaterialShader(L"gloss.mat");
			spMaterialTemplate->setTextureLayout(defaultTextureLayout);
			spMaterialTemplate->setNativeTextureLayout(spScreen->buildMeshTextureLayout(defaultTextureLayout));

			spMaterialTemplate = ResourceManager::createWithId<MaterialTemplate>(L"RectangleMaterial");
			Buffer<SShaderResourceBindInfo> rectangleTextureLayout =
			{
				{ SShaderResourceBindInfo::TEXTURE, 0, nullptr, 0, 0 },
			};
			spMaterialTemplate->setTextureLayout(rectangleTextureLayout);
			spMaterialTemplate->setNativeTextureLayout(spScreen->buildMeshTextureLayout(rectangleTextureLayout));

			auto spRectangleMaterialTemplate = ResourceManager::getResource<MaterialTemplate>(L"RectangleMaterial");
			spScreen->m_nullTextureSet = spScreen->buildShaderResourceBindInfo(Buffer<SShaderResourceBindInfo>(), (VkDescriptorSetLayout)spRectangleMaterialTemplate->getNativeTextureLayout(), spScreen->m_nullTextureSet);
		}

		return global();
	}

	VulkanScreen::VulkanScreen(HWND hWnd, int nWidth, int nHeight) : Screen(nWidth, nHeight)
	{
		createInstance();

		createCommandPools();

		createDescriptorPool();

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VKCALL(vkCreateFence(s_deviceInfo.device, &fenceInfo, g_pVkAllocationCallbacks, &m_presentFence));
		VKCALL(vkCreateFence(s_deviceInfo.device, &fenceInfo, g_pVkAllocationCallbacks, &m_transientRoutineFence));
	}

	void VulkanScreen::waitResourceReady()
	{
		VKCALL(vkQueueWaitIdle(s_deviceInfo.graphicsQueue));
	}

	Buffer<strong<IDrawableTexture>> VulkanScreen::getBackBuffer() const
	{
		return m_swapchainTextures;
	}

	uint VulkanScreen::getCurrentBackBufferIndex() const
	{
		return m_currentSwapchainImageIndex;
	}

	double VulkanScreen::duration(const EventProfiler::ProfileEvent& query) const
	{
		return (query.endTicks - query.startTicks) / 1000.0 * s_deviceInfo.timestampPeriod;
	}

	inline uint VulkanScreen::getSharedMemorySize()
	{
		return s_deviceInfo.maxSharedMemorySize;
	}

	VkSampler VulkanScreen::getSampler(uint nSlot)
	{
		return m_samplers[nSlot];
	}

	VkDescriptorSetLayout VulkanScreen::buildMeshTextureLayout(Buffer<SShaderResourceBindInfo> meshTextureBindInfo)
	{
		VkDescriptorSetLayout meshTextureLayout;

		Buffer<VkDescriptorSetLayoutBinding> meshBindInfos(meshTextureBindInfo.count());
		for (uint i = 0; i < meshBindInfos.count(); ++i)
		{
			auto& bindInfo = meshBindInfos[i];
			bindInfo.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			bindInfo.binding = meshTextureBindInfo[i].bindIndex;
			bindInfo.descriptorCount = 1;
			bindInfo.stageFlags = VK_SHADER_STAGE_ALL;
			bindInfo.pImmutableSamplers = nullptr;
		}

		VkDescriptorSetLayoutCreateInfo meshLayoutInfo;
		memset(&meshLayoutInfo, 0, sizeof(VkDescriptorSetLayoutCreateInfo));
		meshLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		meshLayoutInfo.bindingCount = castval<uint>(meshBindInfos.count());
		meshLayoutInfo.pBindings = meshBindInfos.get();

		VKCALL(vkCreateDescriptorSetLayout(VulkanScreen::s_deviceInfo.device, &meshLayoutInfo, g_pVkAllocationCallbacks, &meshTextureLayout));

		return meshTextureLayout;
	}

	VkDescriptorSetLayout VulkanScreen::getMeshAnimationBufferLayout()
	{
		Buffer<SShaderResourceBindInfo> constBufferInfo =
		{
			{ SShaderResourceBindInfo::STRUCTURED_BUFFER, 0, nullptr, 0, 0 },
			{ SShaderResourceBindInfo::STRUCTURED_BUFFER, 1, nullptr, 0, 0 },
			{ SShaderResourceBindInfo::STRUCTURED_BUFFER, 2, nullptr, 0, 0 },
			{ SShaderResourceBindInfo::STRUCTURED_BUFFER, 3, nullptr, 0, 0 },
			{ SShaderResourceBindInfo::STRUCTURED_BUFFER, 4, nullptr, 0, 0 },
			{ SShaderResourceBindInfo::STRUCTURED_BUFFER, 5, nullptr, 0, 0 },
			{ SShaderResourceBindInfo::STRUCTURED_BUFFER, 6, nullptr, 0, 0 },
			{ SShaderResourceBindInfo::STRUCTURED_BUFFER, 7, nullptr, 0, 0 },
		};

		if (!m_meshAnimationBufferLayout)
		{
			Buffer<VkDescriptorSetLayoutBinding> constBufferBindInfos(constBufferInfo.count());
			for (uint i = 0; i < constBufferBindInfos.count(); ++i)
			{
				auto& bindInfo = constBufferBindInfos[i];
				bindInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				bindInfo.binding = constBufferInfo[i].bindIndex;
				bindInfo.descriptorCount = 1;
				bindInfo.stageFlags = VK_SHADER_STAGE_ALL;
				bindInfo.pImmutableSamplers = nullptr;
			}

			VkDescriptorSetLayoutCreateInfo constBufferLayoutInfo;
			memset(&constBufferLayoutInfo, 0, sizeof(VkDescriptorSetLayoutCreateInfo));
			constBufferLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			constBufferLayoutInfo.bindingCount = castval<uint>(constBufferBindInfos.count());
			constBufferLayoutInfo.pBindings = constBufferBindInfos.get();

			VKCALL(vkCreateDescriptorSetLayout(s_deviceInfo.device, &constBufferLayoutInfo, g_pVkAllocationCallbacks, &m_meshAnimationBufferLayout));
		}

		return m_meshAnimationBufferLayout;
	}

	VulkanScreen::~VulkanScreen()
	{
		VKCALL(vkDeviceWaitIdle(s_deviceInfo.device));

		for (slong i = 0; i < m_submitSemaphores.count(); i++)
		{
			vkDestroySemaphore(s_deviceInfo.device, m_submitSemaphores[i], g_pVkAllocationCallbacks);
		}
		vkDestroyFence(s_deviceInfo.device, m_presentFence, g_pVkAllocationCallbacks);
		vkDestroyFence(s_deviceInfo.device, m_transientRoutineFence, g_pVkAllocationCallbacks);
		vkDestroySwapchainKHR(s_deviceInfo.device, m_swapchain, g_pVkAllocationCallbacks);
		for (slong i = 0; i < m_swapchainTextures.count(); i++)
		{
			m_swapchainTextures[i] = nullptr;
		}
		for (uint i = 0; i < Platform::global()->getShortTaskThreadCount(); ++i)
		{
			vkDestroyCommandPool(s_deviceInfo.device, s_deviceInfo.graphicsCommandPools[i], g_pVkAllocationCallbacks);
			vkDestroyCommandPool(s_deviceInfo.device, s_deviceInfo.computeCommandPools[i], g_pVkAllocationCallbacks);
			vkDestroyCommandPool(s_deviceInfo.device, s_deviceInfo.transferCommandPools[i], g_pVkAllocationCallbacks);
		}
		vkDestroyDescriptorSetLayout(s_deviceInfo.device, m_meshTextureLayout, g_pVkAllocationCallbacks);
		vkDestroyDescriptorSetLayout(s_deviceInfo.device, m_samplerLayout, g_pVkAllocationCallbacks);
		vkDestroyDescriptorPool(s_deviceInfo.device, s_deviceInfo.descriptorPool, g_pVkAllocationCallbacks);
		for (size_t i = 0; i < array_length(m_samplers); ++i)
		{
			vkDestroySampler(s_deviceInfo.device, m_samplers[i], g_pVkAllocationCallbacks);
		}
		vkDestroyDevice(s_deviceInfo.device, g_pVkAllocationCallbacks);
#ifdef ROYMA_DEBUG
		auto destroyDebugUtils = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkDestroyDebugUtilsMessengerEXT");
		assert(destroyDebugUtils != nullptr);
		destroyDebugUtils(m_vkInstance, m_debugMessenger, g_pVkAllocationCallbacks);
#endif //ROYMA_DEBUG
		vkDestroyInstance(m_vkInstance, g_pVkAllocationCallbacks);
#ifdef NV_DEBUG
		AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_DisableGpuCrashDumps());
#endif //NV_DEBUG
	}

	void VulkanScreen::buildSamplerDescriptorSet()
	{
		SSamplerState samplerState;
		samplerState.scalingFilter = SAMPLER_FILTER::LINEAR;
		samplerState.mipFilter = SAMPLER_FILTER::LINEAR;
		samplerState.addressMode = SAMPLER_ADDRESS_MODE::WRAP;
		samplerState.comparison = COMPARE_OPERATION::NEVER;
		setSampler(0, samplerState);

		samplerState.addressMode = SAMPLER_ADDRESS_MODE::CLAMP;
		setSampler(1, samplerState);

		samplerState.addressMode = SAMPLER_ADDRESS_MODE::BORDER;
		samplerState.borderColor = LinearColor(0.0f, 0.0f, 0.0f);
		setSampler(2, samplerState);

		samplerState.addressMode = SAMPLER_ADDRESS_MODE::BORDER;		
		samplerState.borderColor = LinearColor(exp(SHADOW_POWER), 0.0f, 0.0f);
		setSampler(3, samplerState);

		samplerState.scalingFilter = SAMPLER_FILTER::POINT;
		samplerState.mipFilter = SAMPLER_FILTER::LINEAR;
		samplerState.addressMode = SAMPLER_ADDRESS_MODE::CLAMP;
		setSampler(4, samplerState);

		Buffer<VkDescriptorSetLayoutBinding> samplerBindInfos(5);
		Buffer<SShaderResourceBindInfo> shaderResBindInfo(samplerBindInfos.count());
		for (uint i = 0; i < samplerBindInfos.count(); ++i)
		{
			auto& bindInfo = samplerBindInfos[i];
			bindInfo.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			bindInfo.binding = i;			
			bindInfo.descriptorCount = 1;
			bindInfo.stageFlags = VK_SHADER_STAGE_ALL;
			bindInfo.pImmutableSamplers = nullptr;//&m_samplers[i];

			shaderResBindInfo[i] = { SShaderResourceBindInfo::SAMPLER, i, getSampler(i), 0, 0 };
		}

		VkDescriptorSetLayoutCreateInfo samplerLayoutInfo;
		memset(&samplerLayoutInfo, 0, sizeof(VkDescriptorSetLayoutCreateInfo));
		samplerLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		samplerLayoutInfo.bindingCount = castval<uint>(samplerBindInfos.count());
		samplerLayoutInfo.pBindings = samplerBindInfos.get();

		VKCALL(vkCreateDescriptorSetLayout(s_deviceInfo.device, &samplerLayoutInfo, g_pVkAllocationCallbacks, &m_samplerLayout));
		
		m_samplerSet = buildShaderResourceBindInfo(shaderResBindInfo, m_samplerLayout, m_samplerSet);
	}

	bool VulkanScreen::present(strong<RenderRoutine> spRoutine)
	{
		uint nCurrentSwapchainSemaphoreIndex = m_currentSwapchainImageIndex;
		VKCALL(vkAcquireNextImageKHR(s_deviceInfo.device, m_swapchain, UINT64_MAX, m_submitSemaphores[nCurrentSwapchainSemaphoreIndex * m_maxSubmitionCount + m_submitionCount], VK_NULL_HANDLE, &m_currentSwapchainImageIndex));
		++m_submitionCount;

		spRoutine->dispatch();
		spRoutine->finish();
		
		auto commandBuffers = spRoutine->gatherCommandBuffers();
		if (commandBuffers.count())
		{
			VkSubmitInfo graphicsSubmitInfo;
			memset(&graphicsSubmitInfo, 0, sizeof(VkSubmitInfo));
			graphicsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			graphicsSubmitInfo.commandBufferCount = castval<uint>(commandBuffers.count());
			graphicsSubmitInfo.pCommandBuffers = (VkCommandBuffer*)commandBuffers.get();
			VkPipelineStageFlags waitStageFlags[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			if (m_submitionCount > 1)
			{				
				graphicsSubmitInfo.pWaitDstStageMask = waitStageFlags;
				graphicsSubmitInfo.waitSemaphoreCount = 2;
				graphicsSubmitInfo.pWaitSemaphores = m_submitSemaphores.get()
					+ nCurrentSwapchainSemaphoreIndex * m_maxSubmitionCount + m_submitionCount - 2;
			}
			else
			{
				VkPipelineStageFlags waitStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				graphicsSubmitInfo.pWaitDstStageMask = &waitStageFlag;
				graphicsSubmitInfo.waitSemaphoreCount = 1;
				graphicsSubmitInfo.pWaitSemaphores = m_submitSemaphores.get()
					+ nCurrentSwapchainSemaphoreIndex * m_maxSubmitionCount;
			}
			graphicsSubmitInfo.signalSemaphoreCount = 1;
			graphicsSubmitInfo.pSignalSemaphores = m_submitSemaphores.get()
				+ nCurrentSwapchainSemaphoreIndex * m_maxSubmitionCount + m_submitionCount;
			
			VKCALL(vkQueueSubmit(s_deviceInfo.graphicsQueue, 1, &graphicsSubmitInfo, m_presentFence));
			++m_submitionCount;

			VkResult presentResult;
			VkPresentInfoKHR presentInfo;
			memset(&presentInfo, 0, sizeof(VkPresentInfoKHR));
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &m_swapchain;
			presentInfo.pImageIndices = &m_currentSwapchainImageIndex;
			presentInfo.pResults = &presentResult;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = m_submitSemaphores.get()
				+ nCurrentSwapchainSemaphoreIndex * m_maxSubmitionCount + m_submitionCount - 1;

			VKCALL(vkQueuePresentKHR(s_deviceInfo.graphicsQueue, &presentInfo));
			VKCALL(presentResult);
			VKCALL(vkWaitForFences(s_deviceInfo.device, 1, &m_presentFence, VK_FALSE, UINT64_MAX));
			VKCALL(vkResetFences(s_deviceInfo.device, 1, &m_presentFence));
			uint nThreads = Platform::global()->getShortTaskThreadCount();
			for (uint i = 0; i < nThreads; i++)
			{
				VKCALL(vkResetCommandPool(s_deviceInfo.device, s_deviceInfo.graphicsCommandPools[i], 0));
				VKCALL(vkResetCommandPool(s_deviceInfo.device, s_deviceInfo.computeCommandPools[i], 0));
				VKCALL(vkResetCommandPool(s_deviceInfo.device, s_deviceInfo.transferCommandPools[i], 0));
			}
			
#ifdef ROYMA_PROFILE
			if (EventProfiler::isCollecting())
			{
				auto gpuClockQuery = spRoutine->gatherGpuClockQuery();
				for (const auto& item : gpuClockQuery)
				{
					double fDuration = duration(item.second);
					EventProfiler::global()->recordEvent(item.second);
				}
			}
#endif // ROYMA_PROFILE
		}
		++m_renderFrameCount;
		m_submitionCount = 0;

		return true;
	}

	void VulkanScreen::flush(strong<RenderRoutine> spRoutine)
	{
		spRoutine->dispatch();
		spRoutine->finish([this, spRoutine]()
			{
				auto commandBuffers = spRoutine->gatherCommandBuffers();
				if (!commandBuffers.count())
				{
					return;
				}

				VkSubmitInfo graphicsSubmitInfo;
				memset(&graphicsSubmitInfo, 0, sizeof(VkSubmitInfo));
				graphicsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				graphicsSubmitInfo.commandBufferCount = castval<uint>(commandBuffers.count());
				graphicsSubmitInfo.pCommandBuffers = (VkCommandBuffer*)commandBuffers.get();
				VkPipelineStageFlags waitStageFlag = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
				graphicsSubmitInfo.pWaitDstStageMask = &waitStageFlag;
				graphicsSubmitInfo.waitSemaphoreCount = !!m_submitionCount;
				graphicsSubmitInfo.pWaitSemaphores = m_submitSemaphores.get()
					+ m_currentSwapchainImageIndex * m_maxSubmitionCount + m_submitionCount - 1;
				graphicsSubmitInfo.signalSemaphoreCount = 1;
				graphicsSubmitInfo.pSignalSemaphores = m_submitSemaphores.get()
					+ m_currentSwapchainImageIndex * m_maxSubmitionCount + m_submitionCount;

				VkQueue queue;
				switch (spRoutine->getGpuQueueId())
				{
				case royma::THREAD_ID_GPU_GRAPHICS_QUEUE:
					queue = s_deviceInfo.graphicsQueue;
					break;
				case royma::THREAD_ID_GPU_SHORT_COMPUTE_QUEUE:
					queue = s_deviceInfo.shortComputeQueue;
					break;
				case royma::THREAD_ID_GPU_LONG_COMPUTE_QUEUE:
					queue = s_deviceInfo.longComputeQueue;
					break;
				case royma::THREAD_ID_GPU_STREAM_IN_QUEUE:
					queue = s_deviceInfo.streamInQueue;
					break;
				case royma::THREAD_ID_GPU_STREAM_OUT_QUEUE:
					queue = s_deviceInfo.streamOutQueue;
					break;
				default:
					assert(false);
				}

				VKCALL(vkQueueSubmit(queue, 1, &graphicsSubmitInfo, m_transientRoutineFence));
				++m_submitionCount;
				VKCALL(vkWaitForFences(s_deviceInfo.device, 1, &m_transientRoutineFence, VK_FALSE, UINT64_MAX));
				VKCALL(vkResetFences(s_deviceInfo.device, 1, &m_transientRoutineFence));
			});
	}

	void VulkanScreen::drawLine(const float2 startPoint, const float2 endPoint, float fWidth, const SDirtyRect& dirtyRect, const LinearColor& color)
	{
		//m_solidRectImage->drawLine(startPoint, endPoint, fWidth, dirtyRect, color);
	}

	void VulkanScreen::drawLine(const float3& pos, const float3& vec, const LinearColor& color)
	{
		auto spGeom = m_line->getFirstGeometry();
		assert(spGeom != nullptr);

		matrix mWorld;
		float fLength = vec.length();
		float3 vDir = float3((1.0f / fLength) * vec);

		if (vDir != float3(0.0, 0.0, 1.0) && vDir != float3(0.0, 0.0, -1.0))
		{
			mWorld.r[0] = setw(fLength * normalize3(cross(vDir, float3(0.0, 0.0, 1.0))), 0.0);
			mWorld.r[1] = setw(fLength * vDir, 0.0);
			mWorld.r[2] = setw(fLength * normalize3(cross(mWorld.r[0], mWorld.r[1])), 0.0);
			mWorld.r[3] = setw(pos, 1.0);
		}
		else
		{
			mWorld.r[3] = setw(pos, 1.0);
			mWorld.r[2] = setw(fLength * normalize3(cross(float3(1.0, 0.0, 0.0), vDir)), 0.0);
			mWorld.r[1] = setw(fLength * vDir, 0.0);
			mWorld.r[0] = setw(fLength * normalize3(cross(mWorld.r[1], mWorld.r[2])), 0.0);
		}

		spGeom->Material.EmissiveColor = color;
		m_line->draw(mWorld);
	}

	void VulkanScreen::drawCurve(Buffer<float2> bufCurve, const SDirtyRect& dirtyRect, const LinearColor& color)
	{
		//m_solidRectImage->drawCurve(bufCurve, dirtyRect, color);
	}

	void VulkanScreen::drawSolidRect(const SRect& dstRect, const SDirtyRect& dirtyRect, const LinearColor& color)
	{
		//m_solidRectImage->draw(dstRect, dirtyRect, color, false);
	}

	void VulkanScreen::drawSolidRect(const SRect& dstRect, const SDirtyRect& dirtyRect, strong<Texture2D> spTexture)
	{
		//m_solidRectImage->getFirstGeometry()->getDrawableMesh()->setTexture(0, spTexture);
		//m_solidRectImage->draw(dstRect, dirtyRect, LinearColor(1.0f, 1.0f, 1.0f));
	}

	float2 VulkanScreen::getNearPlaneWorldSpaceSize(const Camera* pCamera) const
	{
		if (pCamera->isPerspective())
		{
			float n = pCamera->getNearPlane();
			float tanHalfFov = tanf(0.5f * pCamera->getFov());
			float h = 2.0f * n * tanHalfFov;
			float w = pCamera->getAspect() * h;

			return float2(w, h);
		}
		else
		{
			return float2(pCamera->getWorldSpaceHalfWidth(), pCamera->getWorldSpaceHalfHeight());
		}
	}

	royma::matrix VulkanScreen::orthographicMatrix(strong<Camera> spCamera) const
	{
		matrix mProj;

		matrix mZupToYup;
		mZupToYup.r[0] = xyzw(1.0, 0.0, 0.0, 0.0);
		mZupToYup.r[1] = xyzw(0.0, 0.0, 1.0, 0.0);
		mZupToYup.r[2] = xyzw(0.0, 1.0, 0.0, 0.0);
		mZupToYup.r[3] = xyzw(0.0, 0.0, 0.0, 1.0);

		float n = spCamera->getNearPlane();
		float f = spCamera->getFarPlane();

		float2 worldSpaceSize = getNearPlaneWorldSpaceSize(spCamera.get());
		float w = 1.0f / (worldSpaceSize.x);
		float h = 1.0f / (worldSpaceSize.y);

		mProj.r[0] = xyzw(w, 0.0f, 0.0f, 0.0f);
		mProj.r[1] = xyzw(0.0f, h, 0.0f, 0.0f);
		mProj.r[2] = xyzw(0.0f, 0.0f, 1.0f / (f - n), 0.0f);
		mProj.r[3] = xyzw(0.0f, 0.0f, -n / (f - n), 1.0f);

		return mZupToYup *= mProj;
	}

	royma::matrix VulkanScreen::orthographicMatrixNative(strong<Camera> spCamera) const
	{
		matrix mProj;

		matrix mZupToYUp;
		mZupToYUp.r[0] = xyzw(1.0, 0.0, 0.0, 0.0);
		mZupToYUp.r[1] = xyzw(0.0, 0.0, 1.0, 0.0);
		mZupToYUp.r[2] = xyzw(0.0, 1.0, 0.0, 0.0);
		mZupToYUp.r[3] = xyzw(0.0, 0.0, 0.0, 1.0);

		float n = spCamera->getNearPlane();
		float f = spCamera->getFarPlane();

		float2 worldSpaceSize = getNearPlaneWorldSpaceSize(spCamera.get());
		float w = 1.0f / (worldSpaceSize.x);
		float h = 1.0f / (worldSpaceSize.y);

		mProj.r[0] = xyzw(w, 0.0f, 0.0f, 0.0f);
		mProj.r[1] = xyzw(0.0f, h, 0.0f, 0.0f);
		mProj.r[2] = xyzw(0.0f, 0.0f, 1.0f / (f - n), 0.0f);
		mProj.r[3] = xyzw(0.0f, 0.0f, -n / (f - n), 1.0f);

		matrix mFlipY;
		mFlipY.r[0] = xyzw(1.0, 0.0, 0.0, 0.0);
		mFlipY.r[1] = xyzw(0.0, -1.0, 0.0, 0.0);
		mFlipY.r[2] = xyzw(0.0, 0.0, 1.0, 0.0);
		mFlipY.r[3] = xyzw(0.0, 0.0, 0.0, 1.0);

		mZupToYUp *= mProj;
		mZupToYUp *= mFlipY;

		return mZupToYUp;
	}

	royma::matrix VulkanScreen::perspectiveMatrix(strong<Camera> spCamera) const
	{
		matrix mZupToYUp;
		mZupToYUp.r[0] = xyzw(1.0, 0.0, 0.0, 0.0);
		mZupToYUp.r[1] = xyzw(0.0, 0.0, 1.0, 0.0);
		mZupToYUp.r[2] = xyzw(0.0, 1.0, 0.0, 0.0);
		mZupToYUp.r[3] = xyzw(0.0, 0.0, 0.0, 1.0);

		matrix mProj;

		float w = 1.0f / (spCamera->getAspect() * tan(spCamera->getFov() / 2.0f));
		float h = 1.0f / tan(spCamera->getFov() / 2.0f);
		float n = spCamera->getNearPlane();
		float f = spCamera->getFarPlane();
		float q = f / (f - n);

		mProj.r[0] = xyzw(w, 0.0, 0.0, 0.0);
		mProj.r[1] = xyzw(0.0, h, 0.0, 0.0);
		mProj.r[2] = xyzw(0.0, 0.0, q, 1.0);
		mProj.r[3] = xyzw(0.0, 0.0, -q * n, 0.0);

		return mZupToYUp *= mProj;
	}

	matrix VulkanScreen::perspectiveMatrixNative(strong<Camera> spCamera) const
	{
		matrix mZupToYUp;
		mZupToYUp.r[0] = xyzw(1.0, 0.0, 0.0, 0.0);
		mZupToYUp.r[1] = xyzw(0.0, 0.0, 1.0, 0.0);
		mZupToYUp.r[2] = xyzw(0.0, 1.0, 0.0, 0.0);
		mZupToYUp.r[3] = xyzw(0.0, 0.0, 0.0, 1.0);

		matrix mProj;

		float w = 1.0f / (spCamera->getAspect() * tan(spCamera->getFov() / 2.0f));
		float h = 1.0f / tan(spCamera->getFov() / 2.0f);
		float n = spCamera->getNearPlane();
		float f = spCamera->getFarPlane();
		float q = f / (f - n);

		mProj.r[0] = xyzw(w, 0.0, 0.0, 0.0);
		mProj.r[1] = xyzw(0.0, h, 0.0, 0.0);
		mProj.r[2] = xyzw(0.0, 0.0, q, 1.0);
		mProj.r[3] = xyzw(0.0, 0.0, -q * n, 0.0);

		matrix mFlipY;
		mFlipY.r[0] = xyzw(1.0, 0.0, 0.0, 0.0);
		mFlipY.r[1] = xyzw(0.0, -1.0, 0.0, 0.0);
		mFlipY.r[2] = xyzw(0.0, 0.0, 1.0, 0.0);
		mFlipY.r[3] = xyzw(0.0, 0.0, 0.0, 1.0);

		mZupToYUp *= mProj;
		mZupToYUp *= mFlipY;

		return mZupToYUp;
	}

	void VulkanScreen::setSampler(uint nSlot, const SSamplerState& sampler)
	{
		VkSamplerCreateInfo samplerInfo = {};
		VkSamplerAddressMode addressMode;
		switch (sampler.addressMode)
		{
		case SAMPLER_ADDRESS_MODE::WRAP:
			addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			break;
		case SAMPLER_ADDRESS_MODE::CLAMP:
			addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			break;
		case SAMPLER_ADDRESS_MODE::BORDER:
			addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			break;
		case SAMPLER_ADDRESS_MODE::MIRROR:
			addressMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			break;
		default:
			assert(false);
		}

		VkFilter scaleFilter;
		switch (sampler.scalingFilter)
		{
		case SAMPLER_FILTER::POINT:
			scaleFilter = VK_FILTER_NEAREST;
			break;
		case SAMPLER_FILTER::LINEAR:
			scaleFilter = VK_FILTER_LINEAR;
			break;
		default:
			assert(false);
		}

		VkSamplerMipmapMode mipFilter;
		switch (sampler.mipFilter)
		{
		case SAMPLER_FILTER::POINT:
			mipFilter = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			break;
		case SAMPLER_FILTER::LINEAR:
			mipFilter = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			break;
		default:
			assert(false);
		}

		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = addressMode;
		samplerInfo.magFilter = samplerInfo.minFilter = scaleFilter;
		samplerInfo.mipmapMode = mipFilter;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 13.0f;
		VKCALL(vkCreateSampler(s_deviceInfo.device, &samplerInfo, g_pVkAllocationCallbacks, &m_samplers[nSlot]));
	}

	VkDescriptorSet VulkanScreen::buildShaderResourceBindInfo(Buffer<SShaderResourceBindInfo> shaderResBindInfo, VkDescriptorSetLayout layout, VkDescriptorSet descSet)
	{
		if (!descSet)
		{
			VkDescriptorSetAllocateInfo descSetAllocInfo = {};
			descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descSetAllocInfo.descriptorPool = s_deviceInfo.descriptorPool;
			descSetAllocInfo.descriptorSetCount = 1;
			descSetAllocInfo.pSetLayouts = &layout;

			VKCALL(vkAllocateDescriptorSets(s_deviceInfo.device, &descSetAllocInfo, &descSet));
		}

		Buffer<VkWriteDescriptorSet> descWrites(shaderResBindInfo.count());
		Buffer<VkDescriptorBufferInfo> descBufInfos(descWrites.count());
		Buffer<VkDescriptorImageInfo> descImgInfos(descWrites.count());
		for (slong i = 0; i < shaderResBindInfo.count(); ++i)
		{
			auto& descWrite = descWrites[i];
			descWrite = {};
			switch (shaderResBindInfo[i].type)
			{
			case SShaderResourceBindInfo::CONST_BUFFER:
				descWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descBufInfos[i] = { (VkBuffer)shaderResBindInfo[i].resourceHandle, 0, VK_WHOLE_SIZE };
				descWrite.pBufferInfo = &descBufInfos[i];
				break;
			case SShaderResourceBindInfo::STRUCTURED_BUFFER:
				descWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descBufInfos[i] = { (VkBuffer)shaderResBindInfo[i].resourceHandle, 0, VK_WHOLE_SIZE };
				descWrite.pBufferInfo = &descBufInfos[i];
				break;
			case SShaderResourceBindInfo::TEXTURE:
				descWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				descImgInfos[i] = { VK_NULL_HANDLE, (VkImageView)shaderResBindInfo[i].resourceHandle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
				descWrite.pImageInfo = &descImgInfos[i];
				break;
			case SShaderResourceBindInfo::RWTEXTURE:
				descWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				descImgInfos[i] = { VK_NULL_HANDLE, (VkImageView)shaderResBindInfo[i].resourceHandle, VK_IMAGE_LAYOUT_GENERAL };
				descWrite.pImageInfo = &descImgInfos[i];
				break;
			case SShaderResourceBindInfo::RWTEXTURE_READ:
				descWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				descImgInfos[i] = { VK_NULL_HANDLE, (VkImageView)shaderResBindInfo[i].resourceHandle, VK_IMAGE_LAYOUT_GENERAL };
				descWrite.pImageInfo = &descImgInfos[i];
				break;
			case SShaderResourceBindInfo::SAMPLER:
				descWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				descImgInfos[i] = { (VkSampler)shaderResBindInfo[i].resourceHandle, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED };
				descWrite.pImageInfo = &descImgInfos[i];
				break;
			default:
				assert(false);
			}
			descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrite.dstSet = descSet;
			descWrite.dstBinding = shaderResBindInfo[i].bindIndex;
			descWrite.dstArrayElement = 0;
			descWrite.descriptorCount = 1;
		}

		vkUpdateDescriptorSets(s_deviceInfo.device, castval<uint>(descWrites.count()), descWrites.get(), 0, nullptr);

		return descSet;
	}
}