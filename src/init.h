//
// Created by Windows Vista on 8/18/2021.
//
#include "GLFW/glfw3.h"

#ifndef UNTITLED_FPS_VKINIT_H

#define REQUIRED_LAYER_COUNT 1
global const char* REQUIRED_LAYER_NAMES[REQUIRED_LAYER_COUNT] = {
    "VK_LAYER_KHRONOS_validation",
};
//NOTE(sean): If we dont support this then we just shouldnt run
#define REQUIRED_EXTENSION_COUNT 1
global const char* REQUIRED_EXTENSION_NAMES[REQUIRED_EXTENSION_COUNT] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

global const u32 MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
global b32 validation_enabled = 1;
#else
global b32 validation_enabled = 1;
#endif

global u32 supports_validation;

global void update_cursor_position(GLFWwindow* window, double xpos, double ypos) {
    GameState* state = glfwGetWindowUserPointer(window);

    f32 lastx, lasty;
    lastx = state->mouse_pos.x;
    lasty = state->mouse_pos.y;

    state->mouse_pos.x = (f32)xpos / 1024.0;
    state->mouse_pos.y = (f32)ypos / 1024.0;

    state->mouse_delta.x = lastx - state->mouse_pos.x;
    state->mouse_delta.y = lasty - state->mouse_pos.y;

    state->theta += state->mouse_delta.y * state->mouse_sensitivity;
    state->phi += state->mouse_delta.x * state->mouse_sensitivity;

    state->theta = f32_clamp(state->theta, 0.01, M_PI - 0.01);
}

void find_depth_image_format(GameState* state) {
    VkFormat formats[] = {VK_FORMAT_D16_UNORM};
    state->depth_image_format = find_supported_format(state->physical_device, 1, formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void create_depth_image(GameState* state) {
    create_image(
        state->device,
        state->physical_device,
        state->swapchain_extent.width,
        state->swapchain_extent.height,
        state->depth_image_format,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &state->depth_texture.image,
        &state->depth_texture.memory
    );

    create_image_view(
        state->device,
        state->depth_texture.image,
        state->depth_image_format,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        &state->depth_texture.view
    );
}

void create_generic_sampler(GameState* state) {
    create_image_sampler(state->device, state->physical_device, &state->generic_sampler);
}

void init_window(GameState* state) {
    // init glfw window
    if(!glfwInit()) { panic("Failed to initialize GLFW (somehow?)!"); };
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    state->window = glfwCreateWindow(state->window_width, state->window_height, "Spinning Cube", state->primary_monitor, 0);

    glfwSetInputMode(state->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if(glfwRawMouseMotionSupported()) {
        glfwSetInputMode(state->window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    glfwSetCursorPosCallback(state->window, update_cursor_position);
    glfwSetWindowUserPointer(state->window, state);
}

// check layers support
void check_layers_support(GameState* state) {
    if(!validation_enabled) return;

    u32 layer_count;
    VkLayerProperties* available_layers;

    vkEnumerateInstanceLayerProperties(&layer_count, 0);
    available_layers = sbmalloc(&state->scratch, layer_count * sizeof(VkLayerProperties));
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

    sbclear(&state->scratch);
}

void init_instance(GameState* state) {
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "Dread";
    app_info.engineVersion = VK_MAKE_VERSION(3, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};

    u32 glfw_ext_count = 0;
    const char** glfw_ext;
    glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

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
        debug_create_info.pUserData = &state->debug_callback_data;

        create_info.enabledLayerCount = REQUIRED_LAYER_COUNT;
        create_info.ppEnabledLayerNames = REQUIRED_LAYER_NAMES;
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_create_info;

        // GLFW frees the old glfw_ext so we don't need to do that.
        // We have to create our own, because realloc will *likely* move the location of the pointer.
        // GLFW then promptly throws a hissy-fit when we try to free this.
        usize new_count = glfw_ext_count + REQUIRED_LAYER_COUNT;
        const char** new_glfw_ext = sbmalloc(&state->scratch, new_count * sizeof(char**));
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

    if (vkCreateInstance(&create_info, 0, &state->instance) != VK_SUCCESS) {
        panic("Failed to create instance!");
    }

    // setup debug messenger
    if(supports_validation && validation_enabled) {
        PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(state->instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != 0) {
            func(state->instance, &debug_create_info, 0, &state->debug_messenger);
        } else {
            panic("Failed to create debug messenger!");
        }
    }

    sbclear(&state->scratch);
}

void init_surface(GameState* state) {
    if(glfwCreateWindowSurface(state->instance, state->window, 0, &state->surface) != 0) {
        panic("Failed to create window surface!");
    }
}

void init_physical_device(GameState* state) {
    u32 physical_device_count = 0;
    vkEnumeratePhysicalDevices(state->instance, &physical_device_count, 0);

    if (physical_device_count == 0) {
        panic("Failed to find GPUs with Vulkan support!");
    }

    VkPhysicalDevice* physical_devices = sbmalloc(&state->scratch, physical_device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(state->instance, &physical_device_count, physical_devices);

    //TODO(sean): make this algorithm smarter
    for(usize index = 0; index < physical_device_count; index += 1) {
        VkPhysicalDeviceProperties device_properties = {};
        VkPhysicalDeviceFeatures device_features = {};

        vkGetPhysicalDeviceProperties(physical_devices[index], &device_properties);
        vkGetPhysicalDeviceFeatures(physical_devices[index], &device_features);

        if(device_features.geometryShader != VK_FALSE && device_features.samplerAnisotropy != VK_FALSE) {
            state->physical_device = physical_devices[index];
            break;
        }
    }

    if(state->physical_device == 0) {
        panic("Failed to find suitable GPU!");
    }

    u32 graphics_queue_index = 0;
    u32 present_queue_index = 0;
    u32 queue_family_count = 0;
    VkQueueFamilyProperties* queue_families;

    vkGetPhysicalDeviceQueueFamilyProperties(state->physical_device, &queue_family_count, 0);

    queue_families = sbmalloc(&state->scratch, physical_device_count * sizeof(VkPhysicalDevice));
    vkGetPhysicalDeviceQueueFamilyProperties(state->physical_device, &queue_family_count, queue_families);

    b32 found_good_graphics_queue = 0;
    b32 found_good_present_queue = 0;
    for(usize index = 0; index < queue_family_count; index += 1) {
        if(queue_families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_queue_index = index;
            found_good_graphics_queue = 1;
        }

        VkBool32 present_supported;
        vkGetPhysicalDeviceSurfaceSupportKHR(state->physical_device, index, state->surface, &present_supported);

        if(present_supported) {
            present_queue_index = index;
            found_good_present_queue = 1;
        }

        if(found_good_graphics_queue && found_good_present_queue) {
            break;
        }
    }

    //TODO(sean): we might want to improve this set implementation
    state->queue_family_indices[0] = graphics_queue_index;
    state->queue_family_indices[1] = present_queue_index;
    u32 valid_queue_family_count;
    if(graphics_queue_index == present_queue_index) {
        valid_queue_family_count = 1;
    } else {
        valid_queue_family_count = 2;
    }
    u32* unique_queue_family_indices = sbmalloc(&state->scratch, valid_queue_family_count * sizeof(u32));

    VkDeviceQueueCreateInfo* queue_create_infos = sbmalloc(&state->scratch, valid_queue_family_count * sizeof(VkDeviceQueueCreateInfo));
    usize unique_queue_family_count = 1;

    //TODO(sean): try harder to get the same index
    unique_queue_family_indices[0] = graphics_queue_index;
    for(usize index = 1; index < valid_queue_family_count; index += 1) {
        u32 unique = 1;
        for(usize inter_index = 0; inter_index < unique_queue_family_count; inter_index += 1) {
            if(inter_index == index && state->queue_family_indices[inter_index] == state->queue_family_indices[index]) {
                unique = 0;
            }
        }
        if(unique == 1) {
            unique_queue_family_indices[unique_queue_family_count] = state->queue_family_indices[index];
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
    device_features.samplerAnisotropy = VK_TRUE; //TODO(sean): do proper device validation

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

    if(vkCreateDevice(state->physical_device, &device_create_info, 0, &state->device) != VK_SUCCESS) {
        panic("Failed to create logical device!");
    }

    vkGetDeviceQueue(state->device, graphics_queue_index, 0, &state->queue);
    vkGetDeviceQueue(state->device, present_queue_index, 0, &state->present_queue);

    sbclear(&state->scratch);
}

void pre_init_swapchain(GameState* state) {
    u32 surface_format_count;
    VkSurfaceFormatKHR* surface_formats;

    u32 present_mode_count;
    VkPresentModeKHR* present_modes;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->physical_device, state->surface, &state->surface_capabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(state->physical_device, state->surface, &surface_format_count, 0);

    if(surface_format_count != 0) {
        surface_formats = sbmalloc(&state->scratch, surface_format_count * sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(state->physical_device, state->surface, &surface_format_count, surface_formats);
    } else {
        panic("Could not find a suitable surface format!");
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(state->physical_device, state->surface, &present_mode_count, 0);

    if(present_mode_count != 0) {
        present_modes = sbmalloc(&state->scratch, surface_format_count * sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(state->physical_device, state->surface, &present_mode_count, present_modes);
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
        if( surface_formats[index].format == VK_FORMAT_B8G8R8A8_SRGB
            && surface_formats[index].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            ) {
            state->surface_format = surface_formats[index];
            adequate = true;
            break;
        }
    }
    if(adequate == false) {
        panic("Error finding appropriate surface format!");
    }

    adequate = false;
    for(usize index = 0; index < present_mode_count; index += 1) {
        if(present_modes[index] == VK_PRESENT_MODE_MAILBOX_KHR) {
            state->present_mode = present_modes[index];
            adequate = true;
            break;
        }
    }
    if(adequate == false) {
        state->present_mode = present_modes[0];
    }

    if( state->surface_capabilities.currentExtent.width != UINT32_MAX
        && state->surface_capabilities.currentExtent.height != UINT32_MAX
        ) {
        state->surface_extent = state->surface_capabilities.currentExtent;
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

    state->swapchain_image_count = state->surface_capabilities.minImageCount + 1;
    if(state->surface_capabilities.maxImageCount > 0 && state->swapchain_image_count > state->surface_capabilities.maxImageCount) { // cap to max images
        state->swapchain_image_count = state->surface_capabilities.maxImageCount;
    }

    state->swapchain_format = state->surface_format.format;
    state->swapchain_extent = state->surface_extent;

    sbclear(&state->scratch);
}

void init_swapchain(GameState* state) {
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = state->surface;
    swapchain_create_info.minImageCount = state->swapchain_image_count;
    swapchain_create_info.imageFormat = state->surface_format.format;
    swapchain_create_info.imageColorSpace = state->surface_format.colorSpace;
    swapchain_create_info.imageExtent = state->surface_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    //swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT; // use this for post processing textures

    if(state->queue_family_indices[0] != state->queue_family_indices[1]) {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = state->queue_family_indices;
    } else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // this offers better performance
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = 0;
    }

    swapchain_create_info.preTransform = state->surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = state->present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = 0;


    if (vkCreateSwapchainKHR(state->device, &swapchain_create_info, 0, &state->swapchain) != VK_SUCCESS) {
        panic("Failed to create swapchain!");
    }

    vkGetSwapchainImagesKHR(state->device, state->swapchain, &state->swapchain_image_count, 0);
    state->swapchain_images = sbmalloc(&state->swapchain_buffer, state->swapchain_image_count * sizeof(VkImage));
    vkGetSwapchainImagesKHR(state->device, state->swapchain, &state->swapchain_image_count, state->swapchain_images);

    state->swapchain_image_views = sbmalloc(&state->swapchain_buffer, state->swapchain_image_count * sizeof(VkImageView));

    for(usize index = 0; index < state->swapchain_image_count; index += 1) {
        create_image_view(state->device, state->swapchain_images[index], state->swapchain_format, VK_IMAGE_ASPECT_COLOR_BIT, &state->swapchain_image_views[index]);
    }
}

void create_shader_modules(GameState* state) {
    usize file_count = 6;
    FILE** files = sbmalloc(&state->scratch, file_count * sizeof(FILE*));
    char* file_names[] = {
        "../data/shaders/level.vert.spv",
        "../data/shaders/level.frag.spv",
        "../data/shaders/entity.vert.spv",
        "../data/shaders/entity.frag.spv",
        "../data/shaders/crosshair.vert.spv",
        "../data/shaders/crosshair.frag.spv"
    };
    char** buffers = sbmalloc(&state->scratch, file_count * sizeof(char*));
    usize* buffer_sizes = sbmalloc(&state->scratch, file_count * sizeof(usize));

    for (usize index = 0; index < file_count; index += 1) {
        files[index] = fopen(file_names[index], "rb");
        panic_if_zero(files[index], "Failed to open shader code files!");
    }

    for (usize index = 0; index < file_count; index += 1) {
        FILE *fp = files[index];

        fseek(fp, 0L, SEEK_END);
        usize fsize = ftell(fp);
        fseek(fp, 0L, SEEK_SET);

        buffers[index] = sbmalloc(&state->scratch, fsize);
        fread(buffers[index], fsize, 1, fp);
        buffer_sizes[index] = fsize;
    }

    for (usize index = 0; index < file_count; index += 1) {
        assert(fclose(files[index]) == 0);
    }

    create_shader_module(state->device, buffers[0], buffer_sizes[0], &state->level_modules.vertex);
    create_shader_module(state->device, buffers[1], buffer_sizes[1], &state->level_modules.fragment);

    create_shader_module(state->device, buffers[2], buffer_sizes[2], &state->entity_modules.vertex);
    create_shader_module(state->device, buffers[3], buffer_sizes[3], &state->entity_modules.fragment);

    create_shader_module(state->device, buffers[4], buffer_sizes[4], &state->crosshair_modules.vertex);
    create_shader_module(state->device, buffers[5], buffer_sizes[5], &state->crosshair_modules.fragment);

    sbclear(&state->scratch);
}


void create_level_graphics_pipeline(GameState* state) {
    PipelineOptions pipeline_options;
    pipeline_options.cull_mode = VK_CULL_MODE_BACK_BIT;
    pipeline_options.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    pipeline_options.color_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    pipeline_options.color_store_op = VK_ATTACHMENT_STORE_OP_STORE;
    pipeline_options.color_initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    pipeline_options.color_final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    pipeline_options.depth_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    pipeline_options.depth_store_op = VK_ATTACHMENT_STORE_OP_STORE;
    pipeline_options.depth_initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    pipeline_options.depth_final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    create_graphics_pipeline(
        state->device,
        state->level_modules.vertex,
        state->level_modules.fragment,
        level_vertex_attribute_description_count,
        vertex_attribute_descriptions,
        level_vertex_input_binding_description_count,
        vertex_binding_descriptions,
        state->swapchain_extent.width,
        state->swapchain_extent.height,
        1,
        &state->global_descriptor_set_layout,
        &pipeline_options,
        &state->level_pipeline,
        state->swapchain_format,
        state->depth_image_format
    );
}

void create_entity_graphics_pipeline(GameState* state) {
    PipelineOptions pipeline_options;
    pipeline_options.cull_mode = VK_CULL_MODE_BACK_BIT;
    pipeline_options.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    pipeline_options.color_load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
    pipeline_options.color_store_op = VK_ATTACHMENT_STORE_OP_STORE;
    pipeline_options.color_initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    pipeline_options.color_final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    pipeline_options.depth_load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
    pipeline_options.depth_store_op = VK_ATTACHMENT_STORE_OP_STORE;
    pipeline_options.depth_initial_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    pipeline_options.depth_final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    create_graphics_pipeline(
        state->device,
        state->entity_modules.vertex,
        state->entity_modules.fragment,
        entity_vertex_attribute_description_count,
        enemy_vertex_attribute_descriptions,
        entity_vertex_binding_description_count,
        enemy_vertex_binding_descriptions,
        state->swapchain_extent.width,
        state->swapchain_extent.height,
        1,
        &state->global_descriptor_set_layout,
        &pipeline_options,
        &state->entity_pipeline,
        state->swapchain_format,
        state->depth_image_format
    );
}

/*
void create_door_graphics_pipeline(GameState* state) {
    PipelineOptions pipeline_options;
    pipeline_options.cull_mode = VK_CULL_MODE_NONE;
    pipeline_options.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    pipeline_options.color_load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
    pipeline_options.color_store_op = VK_ATTACHMENT_STORE_OP_STORE;
    pipeline_options.color_initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    pipeline_options.color_final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    pipeline_options.depth_load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
    pipeline_options.depth_store_op = VK_ATTACHMENT_STORE_OP_STORE;
    pipeline_options.depth_initial_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    pipeline_options.depth_final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    create_graphics_pipeline(
        state->device,
        state->enemy_modules.vertex,
        state->enemy_modules.fragment,
        door_vertex_attribute_description_count,
        door_vertex_attribute_descriptions,
        door_vertex_binding_description_count,
        door_vertex_binding_descriptions,
        state->swapchain_extent.width,
        state->swapchain_extent.height,
        1,
        &state->ubo_sampler_descriptor_set_layout,
        &pipeline_options,
        &state->door_pipeline,
        state->swapchain_format,
        state->depth_image_format
    );
}
*/

void create_crosshair_graphics_pipeline(GameState* state) {
    PipelineOptions pipeline_options;
    pipeline_options.cull_mode = VK_CULL_MODE_FRONT_AND_BACK;
    pipeline_options.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    pipeline_options.color_load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
    pipeline_options.color_store_op = VK_ATTACHMENT_STORE_OP_STORE;
    pipeline_options.color_initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    pipeline_options.color_final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    pipeline_options.depth_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    pipeline_options.depth_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    pipeline_options.depth_initial_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    pipeline_options.depth_final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    create_graphics_pipeline(
        state->device,
        state->crosshair_modules.vertex,
        state->crosshair_modules.fragment,
        crosshair_vertex_attribute_description_count,
        crosshair_vertex_attribute_descriptions,
        crosshair_vertex_binding_description_count,
        crosshair_vertex_binding_descriptions,
        state->swapchain_extent.width,
        state->swapchain_extent.height,
        0,
        0,
        &pipeline_options,
        &state->crosshair_pipeline,
        state->swapchain_format,
        state->depth_image_format
    );
}

void create_swapchain_framebuffers(GameState* state) {
    state->swapchain_framebuffers = sbmalloc(&state->swapchain_buffer, state->swapchain_image_count * sizeof(VkFramebuffer));

    for (usize index = 0; index < state->swapchain_image_count; index += 1) {
        VkImageView attachments[] = {state->swapchain_image_views[index], state->depth_texture.view};

        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = state->level_pipeline.pass;
        framebuffer_create_info.attachmentCount = 2;
        framebuffer_create_info.pAttachments = attachments;
        framebuffer_create_info.width = state->swapchain_extent.width;
        framebuffer_create_info.height = state->swapchain_extent.height;
        framebuffer_create_info.layers = 1;

        if (vkCreateFramebuffer(state->device, &framebuffer_create_info, 0, &state->swapchain_framebuffers[index]) != VK_SUCCESS) {
            panic("Failed to create framebuffers!");
        }
    }
}

void init_command_pool(GameState* state) {
    if(state->command_pool == 0) {
        VkCommandPoolCreateInfo command_pool_create_info = {};
        command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info.queueFamilyIndex = state->queue_family_indices[0]; // graphics queue family
        command_pool_create_info.flags = 0;

        if (vkCreateCommandPool(state->device, &command_pool_create_info, 0, &state->command_pool) != VK_SUCCESS) {
            panic("Failed to create command pool!");
        }
    }
}

void create_sync_objects(GameState* state) {
    state->image_available_semaphores = sbmalloc(&state->semaphore_buffer, MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    state->render_finished_semaphores = sbmalloc(&state->semaphore_buffer, MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    state->in_flight_fences = sbmalloc(&state->semaphore_buffer, MAX_FRAMES_IN_FLIGHT * sizeof(VkFence));
    state->image_in_flight_fences = sbcalloc(&state->semaphore_buffer, 0, state->swapchain_image_count * sizeof(VkFence));

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(usize index = 0; index < MAX_FRAMES_IN_FLIGHT; index += 1) {
        if(vkCreateSemaphore(state->device, &semaphore_info, 0, &state->image_available_semaphores[index]) != VK_SUCCESS
            || vkCreateSemaphore(state->device, &semaphore_info, 0, &state->render_finished_semaphores[index]) != VK_SUCCESS
            || vkCreateFence(state->device, &fence_info, 0, &state->in_flight_fences[index]) != VK_SUCCESS
        ) {
            panic("Failed to create synchronization structures for a frame!");
        }
    }
}

void create_descriptor_set_layout(GameState* state) {
    VkDescriptorSetLayoutBinding ubo_layout_binding = {};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_layout_binding.pImmutableSamplers = 0;

    VkDescriptorSetLayoutBinding sampler_layout_binding = {};
    sampler_layout_binding.binding = 1;
    sampler_layout_binding.descriptorCount = 1;
    sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    sampler_layout_binding.pImmutableSamplers = 0;

    VkDescriptorSetLayoutBinding lights_layout_binding = {};
    lights_layout_binding.binding = 2;
    lights_layout_binding.descriptorCount = 1;
    lights_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    lights_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    lights_layout_binding.pImmutableSamplers = 0;

    VkDescriptorSetLayoutBinding bindings[descriptor_count] = {ubo_layout_binding, sampler_layout_binding, lights_layout_binding};
    VkDescriptorSetLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = descriptor_count;
    layout_create_info.pBindings = bindings;

    if(vkCreateDescriptorSetLayout(state->device, &layout_create_info, 0, &state->global_descriptor_set_layout) != VK_SUCCESS) {
        panic("Failed to create descriptor set layout!");
    }
}

void create_uniform_buffers(GameState* state) {
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);

    state->camera_uniforms = sbmalloc(&state->semaphore_buffer, state->swapchain_image_count * sizeof(Buffer));

    for(usize index = 0; index < state->swapchain_image_count; index += 1) {
        create_buffer(
            state->device,
            state->physical_device,
            buffer_size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &state->camera_uniforms[index].buffer,
            &state->camera_uniforms[index].memory
        );
    }
}

void create_crosshair_buffers(GameState* state) {
    create_device_local_and_staging_buffer(
        state->device,
        state->physical_device,
        state->queue,
        state->command_pool,
        sizeof(vec4) * 1,
        &state->crosshair_color,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        &state->crosshair_color_buffer,
        &state->crosshair_color_staging_buffer
    );
}

void init_staged_buffers(GameState* state) {
    sbinit(&state->scratch, 512 * 1024); // 512K
    sbinit(&state->swapchain_buffer, 4 * 1024); // 4K
    sbinit(&state->semaphore_buffer, 4 * 1024); // 4K

    sbinit(&state->level_buffer, 4 * 1024 * 1024); // 4M
    sbinit(&state->level_scratch_buffer, 512 * 1024); // 512K
}

void init_open_al_soft(GameState* state) {
    state->al_device = alcOpenDevice(0);
    assert(alcIsExtensionPresent(state->al_device,"ALC_ENUMERATION_EXT") == AL_TRUE);
    state->al_context = alcCreateContext(state->al_device, 0);
    assert(alcMakeContextCurrent(state->al_context) == ALC_TRUE);
}

void init_open_al_defaults(GameState* state) {
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
}

void init_defaults(GameState* state) {
    state->level_index = 0;

    //TODO(sean): load this from a config file
    state->mouse_sensitivity = 2.0;

    state->window_width = 960;
    state->window_height = 540;
    state->primary_monitor = 0;

    //TODO(sean): laod this from file
    state->theta = 1.15;
    state->phi = 0.75;
    state->gravity = 80.0;
    state->player_position.x = 0.0;
    state->player_position.y = 0.0;
    state->player_position.z = 0.0;
    state->max_door_open_time = 2.0f;
    state->enemy_simulation_radius = 20.0f;

    // engine constants
    state->sliding_threshold = 0.7;
    state->slide_gravity_factor = 0.8;

    state->player_health = 9000;
    state->player_speed = 10.0;
    state->player_jump_speed = 20.0;
    state->player_radius = 1.0;
    state->player_z_speed = 0.0f;

    state->crosshair_color = vec4_new(1.0, 1.0, 1.0, 1.0);

    state->door_activation_distance = 16.0f;
    state->door_move_speed = 4.0f;

    state->pistol_shoot_delay = 0.2f;
    state->pistol_reload_speed = 2.0f;
    state->loaded_pistol_ammo_count = 12;
    state->pistol_magazine_size = 12;

    state->enemy_shoot_delay = 2.0f;
}

#define UNTITLED_FPS_VKINIT_H

#endif //UNTITLED_FPS_VKINIT_H