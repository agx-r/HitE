#include "swapchain.h"
#include <stdlib.h>
#include <string.h>

static const char *vert_shader_code
    = "#version 450\n"
      "layout(location = 0) out vec2 fragTexCoord;\n"
      "void main() {\n"
      "    vec2 positions[3] = vec2[](\n"
      "        vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0)\n"
      "    );\n"
      "    vec2 texCoords[3] = vec2[](\n"
      "        vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0)\n"
      "    );\n"
      "    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);\n"
      "    fragTexCoord = texCoords[gl_VertexIndex];\n"
      "}\n";

static const char *frag_shader_code
    = "#version 450\n"
      "layout(binding = 0) uniform sampler2D texSampler;\n"
      "layout(location = 0) in vec2 fragTexCoord;\n"
      "layout(location = 0) out vec4 outColor;\n"
      "void main() {\n"
      "    outColor = texture(texSampler, fragTexCoord);\n"
      "}\n";

result_t
swapchain_create (vulkan_context_t *context, GLFWwindow *window,
                  uint32_t width, uint32_t height, swapchain_t *swapchain)
{
  memset (swapchain, 0, sizeof (swapchain_t));

  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface (context->instance, window, NULL, &surface)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to create window surface");
    }

  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR (context->physical_device, surface,
                                             &capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR (context->physical_device, surface,
                                        &format_count, NULL);
  VkSurfaceFormatKHR *formats
      = malloc (format_count * sizeof (VkSurfaceFormatKHR));
  vkGetPhysicalDeviceSurfaceFormatsKHR (context->physical_device, surface,
                                        &format_count, formats);

  VkSurfaceFormatKHR surface_format = formats[0];

  for (uint32_t i = 0; i < format_count; i++)
    {
      if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM
          && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
          surface_format = formats[i];
          break;
        }
    }

  if (surface_format.format != VK_FORMAT_B8G8R8A8_UNORM)
    {
      for (uint32_t i = 0; i < format_count; i++)
        {
          if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB
              && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
              surface_format = formats[i];
              break;
            }
        }
    }
  free (formats);

  VkSwapchainCreateInfoKHR create_info = { 0 };
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = surface;
  create_info.minImageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0
      && create_info.minImageCount > capabilities.maxImageCount)
    {
      create_info.minImageCount = capabilities.maxImageCount;
    }
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent.width = width;
  create_info.imageExtent.height = height;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage
      = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.preTransform = capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  create_info.clipped = VK_TRUE;

  if (vkCreateSwapchainKHR (context->device, &create_info, NULL,
                            &swapchain->swapchain)
      != VK_SUCCESS)
    {
      vkDestroySurfaceKHR (context->instance, surface, NULL);
      return RESULT_ERROR (RESULT_ERROR_VULKAN, "Failed to create swapchain");
    }

  swapchain->format = surface_format.format;
  swapchain->extent.width = width;
  swapchain->extent.height = height;

  vkGetSwapchainImagesKHR (context->device, swapchain->swapchain,
                           &swapchain->image_count, NULL);
  swapchain->images = malloc (swapchain->image_count * sizeof (VkImage));
  vkGetSwapchainImagesKHR (context->device, swapchain->swapchain,
                           &swapchain->image_count, swapchain->images);

  swapchain->image_views
      = malloc (swapchain->image_count * sizeof (VkImageView));
  for (uint32_t i = 0; i < swapchain->image_count; i++)
    {
      VkImageViewCreateInfo view_info = { 0 };
      view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      view_info.image = swapchain->images[i];
      view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view_info.format = swapchain->format;
      view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      view_info.subresourceRange.baseMipLevel = 0;
      view_info.subresourceRange.levelCount = 1;
      view_info.subresourceRange.baseArrayLayer = 0;
      view_info.subresourceRange.layerCount = 1;

      vkCreateImageView (context->device, &view_info, NULL,
                         &swapchain->image_views[i]);
    }

  VkSemaphoreCreateInfo semaphore_info = { 0 };
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  vkCreateSemaphore (context->device, &semaphore_info, NULL,
                     &swapchain->image_available);
  vkCreateSemaphore (context->device, &semaphore_info, NULL,
                     &swapchain->render_finished);

  return RESULT_SUCCESS;
}

void
swapchain_destroy (vulkan_context_t *context, swapchain_t *swapchain)
{
  if (!context || !swapchain)
    return;

  vkDestroySemaphore (context->device, swapchain->image_available, NULL);
  vkDestroySemaphore (context->device, swapchain->render_finished, NULL);

  for (uint32_t i = 0; i < swapchain->image_count; i++)
    {
      vkDestroyImageView (context->device, swapchain->image_views[i], NULL);
    }

  free (swapchain->images);
  free (swapchain->image_views);

  vkDestroySwapchainKHR (context->device, swapchain->swapchain, NULL);
}

result_t
swapchain_present (vulkan_context_t *context, swapchain_t *swapchain,
                   const gpu_image_t *source_image)
{

  uint32_t image_index;
  VkResult result = vkAcquireNextImageKHR (
      context->device, swapchain->swapchain, UINT64_MAX,
      swapchain->image_available, VK_NULL_HANDLE, &image_index);

  if (result != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to acquire swapchain image");
    }

  vkQueueWaitIdle (context->graphics_queue);

  VkCommandBuffer cmd = vulkan_begin_single_time_commands (context);

  VkImageMemoryBarrier barrier = { 0 };
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = swapchain->images[image_index];
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  vkCmdPipelineBarrier (cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                        &barrier);

  VkImageBlit blit = { 0 };
  blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.srcSubresource.layerCount = 1;
  blit.srcOffsets[1].x = source_image->width;
  blit.srcOffsets[1].y = source_image->height;
  blit.srcOffsets[1].z = 1;
  blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.dstSubresource.layerCount = 1;
  blit.dstOffsets[1].x = swapchain->extent.width;
  blit.dstOffsets[1].y = swapchain->extent.height;
  blit.dstOffsets[1].z = 1;

  vkCmdBlitImage (cmd, source_image->image, VK_IMAGE_LAYOUT_GENERAL,
                  swapchain->images[image_index],
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                  VK_FILTER_LINEAR);

  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = 0;

  vkCmdPipelineBarrier (cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
                        NULL, 1, &barrier);

  vulkan_end_single_time_commands (context, cmd);

  VkPresentInfoKHR present_info = { 0 };
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swapchain->swapchain;
  present_info.pImageIndices = &image_index;

  result = vkQueuePresentKHR (context->graphics_queue, &present_info);
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN, "Failed to present");
    }

  return RESULT_SUCCESS;
}
