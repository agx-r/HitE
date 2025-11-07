#include "vulkan_core.h"
#include "../core/logger.h"
#include <stdlib.h>
#include <string.h>

static const char *validation_layers[] = { "VK_LAYER_KHRONOS_validation" };

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback (VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                VkDebugUtilsMessageTypeFlagsEXT type,
                const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                void *user_data)
{
  (void)type;
  (void)user_data;

  if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
      log_level_t level
          = (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
                ? LOG_LEVEL_ERROR
                : LOG_LEVEL_WARNING;
      logger_log (level, "Vulkan", "%s", callback_data->pMessage);
    }
  return VK_FALSE;
}

result_t
vulkan_init (vulkan_context_t *context, bool enable_validation)
{
  memset (context, 0, sizeof (vulkan_context_t));

  VkApplicationInfo app_info = { 0 };
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Hite Engine";
  app_info.applicationVersion = VK_MAKE_VERSION (1, 0, 0);
  app_info.pEngineName = "Hite";
  app_info.engineVersion = VK_MAKE_VERSION (1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_2;

  VkInstanceCreateInfo create_info = { 0 };
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  uint32_t glfw_extension_count = 0;
  const char **glfw_extensions
      = glfwGetRequiredInstanceExtensions (&glfw_extension_count);

  const char **extensions
      = malloc ((glfw_extension_count + 1) * sizeof (char *));
  for (uint32_t i = 0; i < glfw_extension_count; i++)
    {
      extensions[i] = glfw_extensions[i];
    }
  extensions[glfw_extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

  create_info.enabledExtensionCount = glfw_extension_count + 1;
  create_info.ppEnabledExtensionNames = extensions;

  if (enable_validation)
    {
      create_info.enabledLayerCount = 1;
      create_info.ppEnabledLayerNames = validation_layers;
    }

  VkResult result = vkCreateInstance (&create_info, NULL, &context->instance);
  free ((void *)extensions);

  if (result != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to create Vulkan instance");
    }

  if (enable_validation)
    {
      VkDebugUtilsMessengerCreateInfoEXT debug_create_info = { 0 };
      debug_create_info.sType
          = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
      debug_create_info.messageSeverity
          = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
      debug_create_info.messageType
          = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
      debug_create_info.pfnUserCallback = debug_callback;

      PFN_vkCreateDebugUtilsMessengerEXT func
          = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr (
              context->instance, "vkCreateDebugUtilsMessengerEXT");
      if (func)
        {
          func (context->instance, &debug_create_info, NULL,
                &context->debug_messenger);
        }
    }

  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices (context->instance, &device_count, NULL);
  if (device_count == 0)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN, "No Vulkan devices found");
    }

  VkPhysicalDevice *devices
      = malloc (device_count * sizeof (VkPhysicalDevice));
  if (devices == NULL)
    {
      LOG_ERROR("Vulkan", "malloc (); for devices failed!");
      exit (1);
    }
  vkEnumeratePhysicalDevices (context->instance, &device_count, devices);
  context->physical_device = devices[0];
  free (devices);

  vkGetPhysicalDeviceProperties (context->physical_device,
                                 &context->device_properties);
  vkGetPhysicalDeviceMemoryProperties (context->physical_device,
                                       &context->memory_properties);

  LOG_INFO ("Vulkan", "Using device: %s",
            context->device_properties.deviceName);

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties (context->physical_device,
                                            &queue_family_count, NULL);

  VkQueueFamilyProperties *queue_families
      = malloc (queue_family_count * sizeof (VkQueueFamilyProperties));

  if (queue_families == NULL)
    {
      LOG_ERROR("Vulkan", "malloc (); for device families failed!");
      exit(1);
    }
    
  vkGetPhysicalDeviceQueueFamilyProperties (
      context->physical_device, &queue_family_count, queue_families);

  context->graphics_family = UINT32_MAX;
  context->compute_family = UINT32_MAX;

  for (uint32_t i = 0; i < queue_family_count; i++)
    {
      if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
          context->graphics_family = i;
        }
      if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
          context->compute_family = i;
        }
    }
  free (queue_families);

  if (context->graphics_family == UINT32_MAX
      || context->compute_family == UINT32_MAX)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Required queue families not found");
    }

  float queue_priority = 1.0f;
  VkDeviceQueueCreateInfo queue_create_infos[2] = { 0 };
  uint32_t queue_create_count = 1;

  queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_infos[0].queueFamilyIndex = context->graphics_family;
  queue_create_infos[0].queueCount = 1;
  queue_create_infos[0].pQueuePriorities = &queue_priority;

  if (context->compute_family != context->graphics_family)
    {
      queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_create_infos[1].queueFamilyIndex = context->compute_family;
      queue_create_infos[1].queueCount = 1;
      queue_create_infos[1].pQueuePriorities = &queue_priority;
      queue_create_count = 2;
    }

  VkPhysicalDeviceFeatures device_features = { 0 };
  device_features.shaderFloat64 = VK_TRUE;

  const char *device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

  VkDeviceCreateInfo device_create_info = { 0 };
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.queueCreateInfoCount = queue_create_count;
  device_create_info.pQueueCreateInfos = queue_create_infos;
  device_create_info.pEnabledFeatures = &device_features;
  device_create_info.enabledExtensionCount = 1;
  device_create_info.ppEnabledExtensionNames = device_extensions;

  if (vkCreateDevice (context->physical_device, &device_create_info, NULL,
                      &context->device)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to create logical device");
    }

  vkGetDeviceQueue (context->device, context->graphics_family, 0,
                    &context->graphics_queue);
  vkGetDeviceQueue (context->device, context->compute_family, 0,
                    &context->compute_queue);

  VkCommandPoolCreateInfo pool_info = { 0 };
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = context->graphics_family;

  if (vkCreateCommandPool (context->device, &pool_info, NULL,
                           &context->command_pool)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to create command pool");
    }

  pool_info.queueFamilyIndex = context->compute_family;
  if (vkCreateCommandPool (context->device, &pool_info, NULL,
                           &context->compute_command_pool)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to create compute command pool");
    }

  return RESULT_SUCCESS;
}

void
vulkan_cleanup (vulkan_context_t *context)
{
  if (!context)
    return;

  if (context->device)
    {
      vkDestroyCommandPool (context->device, context->command_pool, NULL);
      vkDestroyCommandPool (context->device, context->compute_command_pool,
                            NULL);
      vkDestroyDevice (context->device, NULL);
    }

  if (context->debug_messenger)
    {
      PFN_vkDestroyDebugUtilsMessengerEXT func
          = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr (
              context->instance, "vkDestroyDebugUtilsMessengerEXT");
      if (func)
        {
          func (context->instance, context->debug_messenger, NULL);
        }
    }

  if (context->instance)
    {
      vkDestroyInstance (context->instance, NULL);
    }
}

uint32_t
vulkan_find_memory_type (vulkan_context_t *context, uint32_t type_filter,
                         VkMemoryPropertyFlags properties)
{
  for (uint32_t i = 0; i < context->memory_properties.memoryTypeCount; i++)
    {
      if ((type_filter & (1 << i))
          && (context->memory_properties.memoryTypes[i].propertyFlags
              & properties)
                 == properties)
        {
          return i;
        }
    }
  return UINT32_MAX;
}

result_t
gpu_buffer_create (vulkan_context_t *context, VkDeviceSize size,
                   VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                   gpu_buffer_t *buffer)
{
  VkBufferCreateInfo buffer_info = { 0 };
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer (context->device, &buffer_info, NULL, &buffer->buffer)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN, "Failed to create buffer");
    }

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements (context->device, buffer->buffer,
                                 &mem_requirements);

  VkMemoryAllocateInfo alloc_info = { 0 };
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex = vulkan_find_memory_type (
      context, mem_requirements.memoryTypeBits, properties);

  if (vkAllocateMemory (context->device, &alloc_info, NULL, &buffer->memory)
      != VK_SUCCESS)
    {
      vkDestroyBuffer (context->device, buffer->buffer, NULL);
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to allocate buffer memory");
    }

  vkBindBufferMemory (context->device, buffer->buffer, buffer->memory, 0);

  buffer->size = size;
  buffer->mapped = NULL;

  return RESULT_SUCCESS;
}

void
gpu_buffer_destroy (vulkan_context_t *context, gpu_buffer_t *buffer)
{
  if (!context || !buffer)
    return;

  if (buffer->mapped)
    {
      vkUnmapMemory (context->device, buffer->memory);
    }

  vkDestroyBuffer (context->device, buffer->buffer, NULL);
  vkFreeMemory (context->device, buffer->memory, NULL);
}

result_t
gpu_buffer_upload (vulkan_context_t *context, gpu_buffer_t *buffer,
                   const void *data, VkDeviceSize size)
{
  void *mapped;
  if (vkMapMemory (context->device, buffer->memory, 0, size, 0, &mapped)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN, "Failed to map buffer memory");
    }

  memcpy (mapped, data, size);
  vkUnmapMemory (context->device, buffer->memory);

  return RESULT_SUCCESS;
}

result_t
gpu_image_create (vulkan_context_t *context, uint32_t width, uint32_t height,
                  VkFormat format, VkImageUsageFlags usage, gpu_image_t *image)
{
  VkImageCreateInfo image_info = { 0 };
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent.width = width;
  image_info.extent.height = height;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.format = format;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_info.usage = usage;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage (context->device, &image_info, NULL, &image->image)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN, "Failed to create image");
    }

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements (context->device, image->image,
                                &mem_requirements);

  VkMemoryAllocateInfo alloc_info = { 0 };
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex
      = vulkan_find_memory_type (context, mem_requirements.memoryTypeBits,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory (context->device, &alloc_info, NULL, &image->memory)
      != VK_SUCCESS)
    {
      vkDestroyImage (context->device, image->image, NULL);
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to allocate image memory");
    }

  vkBindImageMemory (context->device, image->image, image->memory, 0);

  VkImageViewCreateInfo view_info = { 0 };
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = image->image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = format;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  if (vkCreateImageView (context->device, &view_info, NULL, &image->view)
      != VK_SUCCESS)
    {
      vkDestroyImage (context->device, image->image, NULL);
      vkFreeMemory (context->device, image->memory, NULL);
      return RESULT_ERROR (RESULT_ERROR_VULKAN, "Failed to create image view");
    }

  image->width = width;
  image->height = height;
  image->format = format;

  return RESULT_SUCCESS;
}

void
gpu_image_destroy (vulkan_context_t *context, gpu_image_t *image)
{
  if (!context || !image)
    return;

  vkDestroyImageView (context->device, image->view, NULL);
  vkDestroyImage (context->device, image->image, NULL);
  vkFreeMemory (context->device, image->memory, NULL);
}

VkCommandBuffer
vulkan_begin_single_time_commands (vulkan_context_t *context)
{
  VkCommandBufferAllocateInfo alloc_info = { 0 };
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = context->command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers (context->device, &alloc_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info = { 0 };
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer (command_buffer, &begin_info);

  return command_buffer;
}

void
vulkan_end_single_time_commands (vulkan_context_t *context,
                                 VkCommandBuffer command_buffer)
{
  vkEndCommandBuffer (command_buffer);

  VkSubmitInfo submit_info = { 0 };
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  vkQueueSubmit (context->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle (context->graphics_queue);

  vkFreeCommandBuffers (context->device, context->command_pool, 1,
                        &command_buffer);
}
