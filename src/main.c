#define GLFW_INCLUDE_VULKAN

// std
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// self
#include "util.h"
#include "vkutil.h"
#include "vmmath.h"

// lib
#include <GLFW/glfw3.h>

#define REQUIRED_LAYER_COUNT 1
global const char* REQUIRED_LAYER_NAMES[REQUIRED_LAYER_COUNT] = {
        "VK_LAYER_KHRONOS_validation",
};
//TODO(sean): check that this is supported
#define REQUIRED_EXTENSION_COUNT 1
global const char* REQUIRED_EXTENSION_NAMES[REQUIRED_EXTENSION_COUNT] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

global const u32 MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
global b32 validation_enabled = 1;
#else
global b32 validation_enabled = 0;
#endif

// TODO(sean): move this to compile flag
global u32 supports_validation;

// TODO(sean): move this into a struct, none of it really works without the rest
// So it should be reasonable to move it to a struct
global u32 window_width = 800;
global u32 window_height = 600;
global GLFWwindow* window;
global VkInstance instance;
global VkDebugUtilsMessengerEXT debug_messenger;
global VkPhysicalDevice physical_device;
global VkDevice device;
global VkQueue queue;
global VkSurfaceKHR surface;
global VkQueue present_queue;
global VkSurfaceCapabilitiesKHR surface_capabilities;
global u32 queue_family_indices[2];
global VkSurfaceFormatKHR surface_format;
global VkPresentModeKHR present_mode;
global VkExtent2D surface_extent;
global VkSwapchainKHR swapchain;
global u32 swapchain_image_count;
global StagedBuffer swapchain_buffer;
global VkImage* swapchain_images;
global VkFormat swapchain_format;
global VkExtent2D swapchain_extent;
global VkImageView* swapchain_image_views;
global VkShaderModule vertex_module;
global VkShaderModule fragment_module;
global VkPipelineLayout pipeline_layout;
global VkRenderPass render_pass;
global VkPipeline graphics_pipeline;
global VkFramebuffer* swapchain_framebuffers;
global VkCommandPool command_pool;
global VkCommandBuffer* command_buffers;
global StagedBuffer semaphore_buffer;
global VkSemaphore* image_available_semaphores;
global VkSemaphore* render_finished_semaphores;
global VkFence* in_flight_fences;
global VkFence* image_in_flight_fences;
global usize current_frame_index;
global DebugCallbackData debug_callback_data;
global StagedBuffer scratch;

void recreate_swapchain();

void render() {
    vkWaitForFences(device, 1, &in_flight_fences[current_frame_index], VK_TRUE, UINT64_MAX);

    u32 image_index;
    VkResult result;

    result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphores[current_frame_index], 0, &image_index);

    if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        panic("Failed to acquire next swapchain image!");
    }

    if(image_in_flight_fences[current_frame_index] != 0) {
        vkWaitForFences(device, 1, &image_in_flight_fences[current_frame_index], VK_TRUE, UINT64_MAX);
    }
    image_in_flight_fences[current_frame_index] = in_flight_fences[current_frame_index];

    VkSemaphore wait_semaphores[] = {image_available_semaphores[current_frame_index]};
    VkSemaphore signal_semaphores[] = {render_finished_semaphores[current_frame_index]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[image_index];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    vkResetFences(device, 1, &in_flight_fences[current_frame_index]);

    if(vkQueueSubmit(queue, 1, &submit_info, in_flight_fences[current_frame_index]) != 0) {
        panic("Failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapchains[] = {swapchain};
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = 0;

    result = vkQueuePresentKHR(present_queue, &present_info);

    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreate_swapchain();
    } else if (result != VK_SUCCESS) {
        panic("Failed to present swapchain image!");
    }

    current_frame_index += 1;
    current_frame_index %= MAX_FRAMES_IN_FLIGHT;
}

void init_window() {
    // init glfw window
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(window_width, window_height, "Vulkan window", 0, 0);
}

// check layers support
// TODO(sean): move this to compile flag
void check_layers_support() {
    if(!validation_enabled) return;

    u32 layer_count;
    VkLayerProperties* available_layers;
    // TODO(sean): move this to compile flag

    vkEnumerateInstanceLayerProperties(&layer_count, 0);
    available_layers = sbmalloc(&scratch, layer_count * sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

    for(usize index = 0; index < layer_count; index += 1) {
        VkLayerProperties* current_layer = available_layers + index;

        for(usize layer_index = 0; layer_index < REQUIRED_LAYER_COUNT; layer_index += 1) {
            const char* layer_name = REQUIRED_LAYER_NAMES[layer_index];
            if(strcmp(layer_name, current_layer->layerName) == 0) {
                supports_validation = 1;
                break;
            }
        }
    }

    sbclear(&scratch);
}

void init_instance() {
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "Synapse";
    app_info.engineVersion = VK_MAKE_VERSION(3, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};

    u32 glfw_ext_count = 0;
    const char** glfw_ext;
    glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

    // TODO(sean): move this to compile flag
    if(supports_validation && validation_enabled) {
        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        debug_create_info.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = debug_callback;
        debug_create_info.pUserData = &debug_callback_data;

        create_info.enabledLayerCount = REQUIRED_LAYER_COUNT;
        create_info.ppEnabledLayerNames = REQUIRED_LAYER_NAMES;
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_create_info;

        // GLFW frees the old glfw_ext so we don't need to do that.
        // We have to create our own, because realloc will *likely* move the location of the pointer.
        // GLFW then promptly throws a hissy-fit when we try to free this.
        usize new_count = glfw_ext_count + REQUIRED_LAYER_COUNT;
        const char** new_glfw_ext = sbmalloc(&scratch, new_count * sizeof(char**));
        memcpy(new_glfw_ext, glfw_ext, new_count * sizeof(char*));
        glfw_ext = new_glfw_ext;
        glfw_ext[glfw_ext_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        glfw_ext_count = new_count;
    } else {
        create_info.enabledLayerCount = 0;
        create_info.pNext = 0;
    }

    create_info.enabledExtensionCount = glfw_ext_count;
    create_info.ppEnabledExtensionNames = glfw_ext;

    if (vkCreateInstance(&create_info, 0, &instance) != VK_SUCCESS) {
        panic("Failed to create instance!");
    }

    // setup debug messenger
    if(supports_validation && validation_enabled) {
        PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != 0) {
            func(instance, &debug_create_info, 0, &debug_messenger);
        } else {
            panic("Failed to create debug messenger!");
        }
    }

    sbclear(&scratch);
}

void init_surface() {
    if(glfwCreateWindowSurface(instance, window, 0, &surface) != 0) {
        panic("Failed to create window surface!");
    }
}

void init_physical_device() {
    u32 physical_device_count = 0;
    vkEnumeratePhysicalDevices(instance, &physical_device_count, 0);

    if (physical_device_count == 0) {
        panic("Failed to find GPUs with Vulkan support!");
    }

    VkPhysicalDevice* physical_devices = sbmalloc(&scratch, physical_device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices);

    //TODO(sean): make this algorithm smarter
    for(usize index = 0; index < physical_device_count; index += 1) {
        VkPhysicalDeviceProperties device_properties = {};
        VkPhysicalDeviceFeatures device_features = {};

        vkGetPhysicalDeviceProperties(physical_devices[index], &device_properties);
        vkGetPhysicalDeviceFeatures(physical_devices[index], &device_features);

        if(device_features.geometryShader) {
            physical_device = physical_devices[index];
            break;
        }
    }

    if(physical_device == 0) {
        panic("Failed to find suitable GPU!");
    }

    u32 graphics_queue_index = 0;
    u32 present_queue_index = 0;
    u32 queue_family_count = 0;
    VkQueueFamilyProperties* queue_families;

    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, 0);

    queue_families = sbmalloc(&scratch, physical_device_count * sizeof(VkPhysicalDevice));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);

    b32 found_good_graphics_queue = 0;
    b32 found_good_present_queue = 0;
    for(usize index = 0; index < queue_family_count; index += 1) {
        if(queue_families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_queue_index = index;
            found_good_graphics_queue = 1;
        }

        VkBool32 present_supported;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, index, surface, &present_supported);

        if(present_supported) {
            present_queue_index = index;
            found_good_present_queue = 1;
        }

        if(found_good_graphics_queue && found_good_present_queue) {
            break;
        }
    }

    //TODO(sean): see if we can improve this set implementation
    queue_family_indices[0] = graphics_queue_index;
    queue_family_indices[1] = present_queue_index;
    u32 valid_queue_family_count;
    if(graphics_queue_index == present_queue_index) {
        valid_queue_family_count = 1;
    } else {
        valid_queue_family_count = 2;
    }
    u32* unique_queue_family_indices = sbmalloc(&scratch, valid_queue_family_count * sizeof(u32));

    VkDeviceQueueCreateInfo* queue_create_infos = sbmalloc(&scratch, valid_queue_family_count * sizeof(VkDeviceQueueCreateInfo));
    usize unique_queue_family_count = 1;

    //TODO(sean): try harder to get the same index
    unique_queue_family_indices[0] = graphics_queue_index;
    for(usize index = 1; index < valid_queue_family_count; index += 1) {
        u32 unique = 1;
        for(usize inter_index = 0; inter_index < unique_queue_family_count; inter_index += 1) {
            if(inter_index == index && queue_family_indices[inter_index] == queue_family_indices[index]) {
                unique = 0;
            }
        }
        if(unique == 1) {
            unique_queue_family_indices[unique_queue_family_count] = queue_family_indices[index];
            unique_queue_family_count += 1;
        }
    }

    //TODO(sean): get queue family indices implementation to work with different queue indices
    f32 queue_priority = 1.0;
    for(usize index = 0; index < unique_queue_family_count; index += 1) {
        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = unique_queue_family_indices[index];
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;

        queue_create_infos[index] = queue_create_info;
    }

    VkPhysicalDeviceFeatures device_features = {};

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = unique_queue_family_count;
    device_create_info.pQueueCreateInfos = queue_create_infos;
    device_create_info.pEnabledFeatures = &device_features;

    if(supports_validation && validation_enabled) {
        device_create_info.enabledLayerCount = REQUIRED_LAYER_COUNT;
        device_create_info.ppEnabledLayerNames = REQUIRED_LAYER_NAMES;
    } else {
        device_create_info.enabledLayerCount = 0;
        device_create_info.enabledExtensionCount = 0;
    }

    //TODO(sean): actually check that this is supported
    device_create_info.enabledExtensionCount = REQUIRED_EXTENSION_COUNT;
    device_create_info.ppEnabledExtensionNames = REQUIRED_EXTENSION_NAMES; // implicitly supported by debug layers

    if(vkCreateDevice(physical_device, &device_create_info, 0, &device) != VK_SUCCESS) {
        panic("Failed to create logical device!");
    }

    vkGetDeviceQueue(device, graphics_queue_index, 0, &queue);
    vkGetDeviceQueue(device, present_queue_index, 0, &present_queue);

    sbclear(&scratch);
}

void pre_init_swapchain() {
    u32 surface_format_count;
    VkSurfaceFormatKHR* surface_formats;

    u32 present_mode_count;
    VkPresentModeKHR* present_modes;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, 0);

    if(surface_format_count != 0) {
        surface_formats = sbmalloc(&scratch, surface_format_count * sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats);
    } else {
        panic("Could not find a suitable surface format!");
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, 0);

    if(present_mode_count != 0) {
        present_modes = sbmalloc(&scratch, surface_format_count * sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes);
    } else {
        panic("Could not find a suitable present mode!");
    }

    b32 adequate = false;
    if(surface_format_count != 0 && present_mode_count != 0) {
        adequate = true;
    }

    if(adequate == false) {
        panic("Swapchain inadequate!");
    }

    // select from available surface formats and present modes
    adequate = false;
    for(usize index = 0; index < surface_format_count; index += 1) {
        //TODO(sean): figure out how i want to format this
        if(surface_formats[index].format == VK_FORMAT_B8G8R8A8_SRGB && surface_formats[index].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surface_format = surface_formats[index];
            adequate = true;
            break;
        }
    }
    if(adequate == false) {
        panic("Error finding appropriate surface format!");
    }

    adequate = false;
    for(usize index = 0; index < present_mode_count; index += 1) {
        if(present_modes[index] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            present_mode = present_modes[index];
            adequate = true;
            break;
        }
    }
    if(adequate == false) {
        present_mode = present_modes[0];
    }

    //TODO(sean): figure out how i want to format this
    if(surface_capabilities.currentExtent.width != UINT32_MAX && surface_capabilities.currentExtent.height != UINT32_MAX) {
        surface_extent = surface_capabilities.currentExtent;
    } else {
        //TODO(sean): handle this error, we need to reconstruct the swap chain size based on what glfw gives us
        panic("Failed to validate current swap chain extent!");
        /*
        i32 width, height;
        glfwGetFramebufferSize(window, &width, &height);
        swap_chain_extent.width = (u32)width;
        swap_chain_extent.height = (u32)height;
        */
    }

    swapchain_image_count = surface_capabilities.minImageCount + 1;
    if(surface_capabilities.maxImageCount > 0 && swapchain_image_count > surface_capabilities.maxImageCount) { // cap to max images
        swapchain_image_count = surface_capabilities.maxImageCount;
    }

    swapchain_format = surface_format.format;
    swapchain_extent = surface_extent;

    sbclear(&scratch);
}

void init_swapchain() {
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = surface;
    swapchain_create_info.minImageCount = swapchain_image_count;
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageExtent = surface_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    //swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT; // use this for post processing textures

    if (queue_family_indices[0] != queue_family_indices[1]) {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // this offers better performance
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = 0;
    }

    swapchain_create_info.preTransform = surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = 0;


    if (vkCreateSwapchainKHR(device, &swapchain_create_info, 0, &swapchain) != VK_SUCCESS) {
        panic("Failed to create swapchain!");
    }

    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, 0);
    swapchain_images = sbmalloc(&swapchain_buffer, swapchain_image_count * sizeof(VkImage));
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images);

    swapchain_image_views = sbmalloc(&swapchain_buffer, swapchain_image_count * sizeof(VkImageView));

    for(usize index = 0; index < swapchain_image_count; index += 1) {
        VkImageViewCreateInfo image_view_create_info = {};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image = swapchain_images[index];
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = swapchain_format;

        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;

        if(vkCreateImageView(device, &image_view_create_info, 0, &swapchain_image_views[index]) != 0) {
            panic("Failed to create image views!");
        }
    }
}

void create_shader_modules() {
    // load shaders
    usize file_count = 2;
    FILE **files = sbmalloc(&scratch, file_count * sizeof(FILE *));
    char *file_names[] = {"../vert.spv", "../frag.spv"};
    char **buffers = sbmalloc(&scratch, file_count * sizeof(char *));
    usize *buffer_sizes = sbmalloc(&scratch, file_count * sizeof(usize));

    for (usize index = 0; index < file_count; index += 1) {
        files[index] = fopen(file_names[index], "rb");
    }

    usize valid = 1;
    for (usize index = 0; index < file_count; index += 1) {
        if (files[index] == 0) {
            valid = 0;
        }
    }
    // TODO(sean): we do this a lot, see if we can move it out into something else
    if (valid == 0) {
        panic("Failed to open files!");
    }

    for (usize index = 0; index < file_count; index += 1) {
        FILE *fp = files[index];

        fseek(fp, 0L, SEEK_END);
        usize fsize = ftell(fp);
        fseek(fp, 0L, SEEK_SET);

        buffers[index] = sbmalloc(&scratch, fsize * sizeof(char));
        fread(buffers[index], fsize, sizeof(char), fp);
        buffer_sizes[index] = fsize;
    }

    //TODO(sean): create a better assert than stdlib
    for (usize index = 0; index < file_count; index += 1) {
        assert(fclose(files[index]) == 0);
    }

    create_shader_module(device, buffers[0], buffer_sizes[0], &vertex_module);
    create_shader_module(device, buffers[1], buffer_sizes[1], &fragment_module);

    sbclear(&scratch);
}

void create_pipeline() {
    VkPipelineShaderStageCreateInfo vertex_stage_create_info = {};
    vertex_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_stage_create_info.module = vertex_module;
    vertex_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_stage_create_info = {};
    fragment_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_stage_create_info.module = fragment_module;
    fragment_stage_create_info.pName = "main";

    // TODO(sean): formatting
    VkPipelineShaderStageCreateInfo shader_stages[] = { vertex_stage_create_info, fragment_stage_create_info };

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
    vertex_input_state_create_info.pVertexBindingDescriptions = 0;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
    vertex_input_state_create_info.pVertexAttributeDescriptions = 0;

    VkPipelineInputAssemblyStateCreateInfo vertex_input_assembly_create_info = {};
    vertex_input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    vertex_input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    vertex_input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32)swapchain_extent.width;
    viewport.height = (f32)swapchain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = swapchain_extent.width;
    scissor.extent.height = swapchain_extent.height;

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.lineWidth = 1.0f;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
    rasterization_state_create_info.depthBiasClamp = 0.0f;
    rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.minSampleShading = 1.0f;
    multisample_state_create_info.pSampleMask = 0;
    multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT
            | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state.blendEnable = VK_FALSE;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment_state;
    color_blend_state_create_info.blendConstants[0] = 0.0f;
    color_blend_state_create_info.blendConstants[1] = 0.0f;
    color_blend_state_create_info.blendConstants[2] = 0.0f;
    color_blend_state_create_info.blendConstants[3] = 0.0f;

    // TODO(sean): figure out if this need to be used for anything
    // VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };

    // VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    // dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    // dynamic_state_create_info.dynamicStateCount = 2;
    // dynamic_state_create_info.pDynamicStates = dynamic_states;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 0;
    pipeline_layout_create_info.pSetLayouts = 0;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = 0;

    if(vkCreatePipelineLayout(device, &pipeline_layout_create_info, 0, &pipeline_layout) != 0) {
        panic("Failed to create pipeline layout!");
    }

    VkAttachmentDescription color_attachment_desc = {};
    color_attachment_desc.format = swapchain_format;
    color_attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment_desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc = {};
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &color_attachment_ref;


    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &color_attachment_desc;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_desc;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pDependencies = &subpass_dependency;

    if(vkCreateRenderPass(device, &render_pass_create_info, 0, &render_pass) != 0) {
        panic("Failed to create render pass!");
    }

    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stages;
    pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &vertex_input_assembly_create_info;
    pipeline_create_info.pViewportState = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    pipeline_create_info.pMultisampleState = &multisample_state_create_info;
    pipeline_create_info.pDepthStencilState = 0;
    pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    pipeline_create_info.pDynamicState = 0;
    pipeline_create_info.layout = pipeline_layout;
    pipeline_create_info.renderPass = render_pass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = 0;
    pipeline_create_info.basePipelineIndex = -1;

    if(vkCreateGraphicsPipelines(device, 0, 1, &pipeline_create_info, 0, &graphics_pipeline) != 0) {
        panic("Failed to create graphics pipeline!");
    }
}

void create_swapchain_framebuffers() {
    swapchain_framebuffers = sbmalloc(&swapchain_buffer, swapchain_image_count * sizeof(VkFramebuffer));

    for (usize index = 0; index < swapchain_image_count; index += 1) {
        VkImageView attachments[] = {swapchain_image_views[index]};

        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = render_pass;
        framebuffer_create_info.attachmentCount = 1;
        framebuffer_create_info.pAttachments = attachments;
        framebuffer_create_info.width = swapchain_extent.width;
        framebuffer_create_info.height = swapchain_extent.height;
        framebuffer_create_info.layers = 1;

        if (vkCreateFramebuffer(device, &framebuffer_create_info, 0,
                                &swapchain_framebuffers[index]) != 0) {
            panic("Failed to create framebuffers!");
        }
    }
}

void create_command_buffers() {
    command_buffers = sbmalloc(&swapchain_buffer, swapchain_image_count * sizeof(VkCommandBuffer));

    if(command_pool == 0) {
        VkCommandPoolCreateInfo command_pool_create_info = {};
        command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info.queueFamilyIndex = queue_family_indices[0]; // graphics queue family
        command_pool_create_info.flags = 0;

        if (vkCreateCommandPool(device, &command_pool_create_info, 0, &command_pool) != 0) {
            panic("Failed to create command pool!");
        }
    }

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = (u32)swapchain_image_count;

    if(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, command_buffers) != 0) {
        panic("Failed to allocate command buffers!");
    }

    for(usize index = 0; index < swapchain_image_count; index += 1) {
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = 0;

        if (vkBeginCommandBuffer(command_buffers[index], &begin_info) != 0) {
            panic("Failed to begin recording command buffer!");
        }

        const VkClearValue render_clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

        VkRenderPassBeginInfo render_pass_begin_info = {};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.renderPass = render_pass;
        render_pass_begin_info.framebuffer = swapchain_framebuffers[index];
        render_pass_begin_info.renderArea.offset.x = 0;
        render_pass_begin_info.renderArea.offset.y = 0;
        render_pass_begin_info.renderArea.extent.width = swapchain_extent.width;
        render_pass_begin_info.renderArea.extent.height = swapchain_extent.height;
        render_pass_begin_info.pClearValues = &render_clear_color;
        render_pass_begin_info.clearValueCount = 1;

        vkCmdBeginRenderPass(command_buffers[index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(command_buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
        vkCmdDraw(command_buffers[index], 3, 1, 0, 0);

        vkCmdEndRenderPass(command_buffers[index]);

        if (vkEndCommandBuffer(command_buffers[index]) != 0) {
            panic("Failed to record command buffer!");
        }
    }
}

void create_semaphores() {
    //TODO(sean): see if we can reduce the overall amount of times we call malloc
    image_available_semaphores = sbmalloc(&semaphore_buffer, MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    render_finished_semaphores = sbmalloc(&semaphore_buffer, MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    in_flight_fences = sbmalloc(&semaphore_buffer, MAX_FRAMES_IN_FLIGHT * sizeof(VkFence));
    image_in_flight_fences = sbcalloc(&semaphore_buffer, 0, swapchain_image_count * sizeof(VkFence));

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(usize index = 0; index < MAX_FRAMES_IN_FLIGHT; index += 1)
    {
        if(vkCreateSemaphore(device, &semaphore_info, 0, &image_available_semaphores[index]) != 0
        || vkCreateSemaphore(device, &semaphore_info, 0, &render_finished_semaphores[index]) != 0
        || vkCreateFence(device, &fence_info, 0, &in_flight_fences[index]) != 0)
        {
            panic("Failed to create synchronization structures for a frame!");
        }
    }
}

void cleanup_swapchain_artifacts() {
    for(usize index = 0; index < swapchain_image_count; index += 1) {
        vkDestroyFramebuffer(device, swapchain_framebuffers[index], 0);
    }

    vkFreeCommandBuffers(device, command_pool, (u32)swapchain_image_count, command_buffers);
    vkDestroyPipeline(device, graphics_pipeline, 0);
    vkDestroyPipelineLayout(device, pipeline_layout, 0);
    vkDestroyRenderPass(device, render_pass, 0);

    for(usize index = 0; index < swapchain_image_count; index += 1) {
        vkDestroyImageView(device, swapchain_image_views[index], 0);
    }

    vkDestroySwapchainKHR(device, swapchain, 0);

    sbclear(&swapchain_buffer);
}

void recreate_swapchain() {
    vkDeviceWaitIdle(device);

    cleanup_swapchain_artifacts();

    pre_init_swapchain();
    init_swapchain();
    create_shader_modules();
    create_pipeline();
    create_swapchain_framebuffers();
    create_command_buffers();
}

int main() {
    sbinit(&scratch, 512 * 1024); // 512K
    sbinit(&swapchain_buffer, 4 * 1024); // 4K
    sbinit(&semaphore_buffer, 512); // 512b

    init_window();
    check_layers_support();
    init_instance();
    init_surface();
    init_physical_device();
    pre_init_swapchain();
    init_swapchain();
    create_shader_modules();
    create_pipeline();
    create_swapchain_framebuffers();
    create_command_buffers();
    create_semaphores();

    //Timer timer;
    //timer_init(&timer);

    while(!glfwWindowShouldClose(window)) {
        //timer_start(&timer);
        glfwPollEvents();
        render();
        //timer_end(&timer);
        //printf("Elapsed time: %lf s\n", timer.elapsed);
    }

    //TODO(sean): finish swapchain recreation steps

    // if swapchain unoptimal or out of date
    // recreate_swapchain = true
    // else
    // recreate_swapchain = false

    // if recreate_swapchain
    // cleanup_swapchain_artifacts();
    // create_swapchain();

    vkDeviceWaitIdle(device);

    /*
    // TODO(sean): We might need to recreate the window or recreate the vulkan context, so don't throw this away just yet
    glfwDestroyWindow(window);
    glfwTerminate();
    */

    return 0;
}