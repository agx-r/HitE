#ifndef HITE_VULKAN_CORE_H
#define HITE_VULKAN_CORE_H

#include "../core/types.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct
{
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
  VkPhysicalDevice physical_device;
  VkDevice device;
  VkQueue graphics_queue;
  VkQueue compute_queue;
  VkCommandPool command_pool;
  VkCommandPool compute_command_pool;

  uint32_t graphics_family;
  uint32_t compute_family;

  VkPhysicalDeviceProperties device_properties;
  VkPhysicalDeviceMemoryProperties memory_properties;
} vulkan_context_t;

typedef struct
{
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkDeviceSize size;
  void *mapped;
} gpu_buffer_t;

typedef struct
{
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
  VkSampler sampler;
  uint32_t width;
  uint32_t height;
  VkFormat format;
} gpu_image_t;

result_t vulkan_init (vulkan_context_t *context, bool enable_validation);
void vulkan_cleanup (vulkan_context_t *context);

result_t gpu_buffer_create (vulkan_context_t *context, VkDeviceSize size,
                            VkBufferUsageFlags usage,
                            VkMemoryPropertyFlags properties,
                            gpu_buffer_t *buffer);
void gpu_buffer_destroy (vulkan_context_t *context, gpu_buffer_t *buffer);
result_t gpu_buffer_upload (vulkan_context_t *context, gpu_buffer_t *buffer,
                            const void *data, VkDeviceSize size);

result_t gpu_image_create (vulkan_context_t *context, uint32_t width,
                           uint32_t height, VkFormat format,
                           VkImageUsageFlags usage, gpu_image_t *image);
void gpu_image_destroy (vulkan_context_t *context, gpu_image_t *image);

VkCommandBuffer vulkan_begin_single_time_commands (vulkan_context_t *context);
void vulkan_end_single_time_commands (vulkan_context_t *context,
                                      VkCommandBuffer command_buffer);

uint32_t vulkan_find_memory_type (vulkan_context_t *context,
                                  uint32_t type_filter,
                                  VkMemoryPropertyFlags properties);

#endif
