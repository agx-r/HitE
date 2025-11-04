#ifndef HITE_SWAPCHAIN_H
#define HITE_SWAPCHAIN_H

#include "vulkan_core.h"
#include <GLFW/glfw3.h>

typedef struct
{
  VkSwapchainKHR swapchain;
  VkImage *images;
  VkImageView *image_views;
  VkFramebuffer *framebuffers;
  uint32_t image_count;
  VkFormat format;
  VkExtent2D extent;

  VkRenderPass render_pass;
  VkPipeline graphics_pipeline;
  VkPipelineLayout pipeline_layout;
  VkDescriptorSetLayout descriptor_set_layout;
  VkDescriptorPool descriptor_pool;
  VkDescriptorSet descriptor_set;

  VkSemaphore image_available;
  VkSemaphore render_finished;
} swapchain_t;

result_t swapchain_create (vulkan_context_t *context, GLFWwindow *window,
                           uint32_t width, uint32_t height,
                           swapchain_t *swapchain);
void swapchain_destroy (vulkan_context_t *context, swapchain_t *swapchain);

result_t swapchain_present (vulkan_context_t *context, swapchain_t *swapchain,
                            const gpu_image_t *source_image);

#endif // HITE_SWAPCHAIN_H
