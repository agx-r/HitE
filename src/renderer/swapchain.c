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

  if (glfwCreateWindowSurface (context->instance, window, NULL,
                               &swapchain->surface)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to create window surface");
    }

  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR (
      context->physical_device, swapchain->surface, &capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR (
      context->physical_device, swapchain->surface, &format_count, NULL);
  VkSurfaceFormatKHR *formats
      = malloc (format_count * sizeof (VkSurfaceFormatKHR));
  vkGetPhysicalDeviceSurfaceFormatsKHR (
      context->physical_device, swapchain->surface, &format_count, formats);

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
  create_info.surface = swapchain->surface;
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
      vkDestroySurfaceKHR (context->instance, swapchain->surface, NULL);
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

  swapchain->image_available_semaphores
      = malloc (swapchain->image_count * sizeof (VkSemaphore));
  swapchain->render_finished_semaphores
      = malloc (swapchain->image_count * sizeof (VkSemaphore));

  for (uint32_t i = 0; i < swapchain->image_count; i++)
    {
      if (vkCreateSemaphore (context->device, &semaphore_info, NULL,
                             &swapchain->image_available_semaphores[i])
          != VK_SUCCESS)
        {

          for (uint32_t j = 0; j < i; j++)
            {
              vkDestroySemaphore (context->device,
                                  swapchain->image_available_semaphores[j],
                                  NULL);
            }
          free (swapchain->image_available_semaphores);
          free (swapchain->render_finished_semaphores);
          return RESULT_ERROR (RESULT_ERROR_VULKAN,
                               "Failed to create image_available semaphore");
        }

      if (vkCreateSemaphore (context->device, &semaphore_info, NULL,
                             &swapchain->render_finished_semaphores[i])
          != VK_SUCCESS)
        {

          for (uint32_t j = 0; j <= i; j++)
            {
              vkDestroySemaphore (context->device,
                                  swapchain->image_available_semaphores[j],
                                  NULL);
            }
          for (uint32_t j = 0; j < i; j++)
            {
              vkDestroySemaphore (context->device,
                                  swapchain->render_finished_semaphores[j],
                                  NULL);
            }
          free (swapchain->image_available_semaphores);
          free (swapchain->render_finished_semaphores);
          return RESULT_ERROR (RESULT_ERROR_VULKAN,
                               "Failed to create render_finished semaphore");
        }
    }

  VkFenceCreateInfo fence_info = { 0 };
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  if (vkCreateFence (context->device, &fence_info, NULL,
                     &swapchain->command_fence)
      != VK_SUCCESS)
    {

      for (uint32_t i = 0; i < swapchain->image_count; i++)
        {
          vkDestroySemaphore (context->device,
                              swapchain->image_available_semaphores[i], NULL);
          vkDestroySemaphore (context->device,
                              swapchain->render_finished_semaphores[i], NULL);
        }
      free (swapchain->image_available_semaphores);
      free (swapchain->render_finished_semaphores);
      return RESULT_ERROR (RESULT_ERROR_VULKAN, "Failed to create fence");
    }

  return RESULT_SUCCESS;
}

void
swapchain_destroy (vulkan_context_t *context, swapchain_t *swapchain)
{
  if (!context || !swapchain)
    return;

  if (context->device)
    {
      vkDeviceWaitIdle (context->device);
    }

  if (swapchain->image_available_semaphores)
    {
      for (uint32_t i = 0; i < swapchain->image_count; i++)
        {
          vkDestroySemaphore (context->device,
                              swapchain->image_available_semaphores[i], NULL);
        }
      free (swapchain->image_available_semaphores);
    }

  if (swapchain->render_finished_semaphores)
    {
      for (uint32_t i = 0; i < swapchain->image_count; i++)
        {
          vkDestroySemaphore (context->device,
                              swapchain->render_finished_semaphores[i], NULL);
        }
      free (swapchain->render_finished_semaphores);
    }

  if (swapchain->command_fence != VK_NULL_HANDLE)
    {
      vkDestroyFence (context->device, swapchain->command_fence, NULL);
    }

  for (uint32_t i = 0; i < swapchain->image_count; i++)
    {
      vkDestroyImageView (context->device, swapchain->image_views[i], NULL);
    }

  free (swapchain->images);
  free (swapchain->image_views);

  if (swapchain->swapchain != VK_NULL_HANDLE)
    {
      vkDestroySwapchainKHR (context->device, swapchain->swapchain, NULL);
    }

  if (swapchain->surface != VK_NULL_HANDLE && context->instance)
    {
      vkDestroySurfaceKHR (context->instance, swapchain->surface, NULL);
    }
}

result_t
swapchain_present (vulkan_context_t *context, swapchain_t *swapchain,
                   const gpu_image_t *source_image)
{

  uint32_t image_index;

  vkQueueWaitIdle (context->graphics_queue);

  VkResult result = vkAcquireNextImageKHR (
      context->device, swapchain->swapchain, UINT64_MAX,
      swapchain->image_available_semaphores[0], VK_NULL_HANDLE, &image_index);

  VkSemaphore image_available_sem = swapchain->image_available_semaphores[0];

  if (result != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to acquire swapchain image");
    }

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

  vkEndCommandBuffer (cmd);

  vkWaitForFences (context->device, 1, &swapchain->command_fence, VK_TRUE,
                   UINT64_MAX);
  vkResetFences (context->device, 1, &swapchain->command_fence);

  VkSubmitInfo submit_info = { 0 };
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &image_available_sem;
  submit_info.pWaitDstStageMask = wait_stages;

  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd;

  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores
      = &swapchain->render_finished_semaphores[image_index];

  if (vkQueueSubmit (context->graphics_queue, 1, &submit_info,
                     swapchain->command_fence)
      != VK_SUCCESS)
    {
      vkFreeCommandBuffers (context->device, context->command_pool, 1, &cmd);
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to submit command buffer");
    }

  VkPresentInfoKHR present_info = { 0 };
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores
      = &swapchain->render_finished_semaphores[image_index];

  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swapchain->swapchain;
  present_info.pImageIndices = &image_index;

  result = vkQueuePresentKHR (context->graphics_queue, &present_info);

  vkWaitForFences (context->device, 1, &swapchain->command_fence, VK_TRUE,
                   UINT64_MAX);
  vkFreeCommandBuffers (context->device, context->command_pool, 1, &cmd);

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN, "Failed to present");
    }

  return RESULT_SUCCESS;
}
