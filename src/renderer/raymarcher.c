#include "raymarcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *
read_file (const char *filename, size_t *out_size)
{
  FILE *file = fopen (filename, "rb");
  if (!file)
    return NULL;

  fseek (file, 0, SEEK_END);
  size_t size = ftell (file);
  fseek (file, 0, SEEK_SET);

  char *buffer = malloc (size);
  if (!buffer)
    {
      fclose (file);
      return NULL;
    }

  fread (buffer, 1, size, file);
  fclose (file);

  if (out_size)
    *out_size = size;
  return buffer;
}

result_t
raymarcher_create (vulkan_context_t *context, uint32_t width, uint32_t height,
                   raymarcher_t *raymarcher)
{
  memset (raymarcher, 0, sizeof (raymarcher_t));
  raymarcher->vk_context = context;
  raymarcher->width = width;
  raymarcher->height = height;
  raymarcher->max_objects = MAX_SDF_OBJECTS;

  result_t result = gpu_image_create (
      context, width, height, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
          | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      &raymarcher->output_color_depth);

  if (result.code != RESULT_OK)
    return result;

  result = gpu_image_create (
      context, width, height, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      &raymarcher->output_normal);

  if (result.code != RESULT_OK)
    return result;

  result = gpu_image_create (context, width, height, VK_FORMAT_R8G8B8A8_UNORM,
                             VK_IMAGE_USAGE_STORAGE_BIT
                                 | VK_IMAGE_USAGE_SAMPLED_BIT
                                 | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                             &raymarcher->output_final);

  if (result.code != RESULT_OK)
    return result;

  result = gpu_buffer_create (context, sizeof (lighting_uniforms_t),
                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              &raymarcher->lighting_uniform_buffer);

  if (result.code != RESULT_OK)
    return result;

  VkCommandBuffer cmd = vulkan_begin_single_time_commands (context);

  VkImageMemoryBarrier barriers[3] = { 0 };

  barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barriers[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
  barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].image = raymarcher->output_color_depth.image;
  barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barriers[0].subresourceRange.baseMipLevel = 0;
  barriers[0].subresourceRange.levelCount = 1;
  barriers[0].subresourceRange.baseArrayLayer = 0;
  barriers[0].subresourceRange.layerCount = 1;
  barriers[0].srcAccessMask = 0;
  barriers[0].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

  barriers[1] = barriers[0];
  barriers[1].image = raymarcher->output_normal.image;

  barriers[2] = barriers[0];
  barriers[2].image = raymarcher->output_final.image;

  vkCmdPipelineBarrier (cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0,
                        NULL, 3, barriers);

  vulkan_end_single_time_commands (context, cmd);

  result = gpu_buffer_create (context, sizeof (raymarch_uniforms_t),
                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              &raymarcher->uniform_buffer);

  if (result.code != RESULT_OK)
    return result;

  result = gpu_buffer_create (context, sizeof (sdf_object_t) * MAX_SDF_OBJECTS,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              &raymarcher->sdf_objects_buffer);

  if (result.code != RESULT_OK)
    return result;

  VkDescriptorSetLayoutBinding bindings[4] = { 0 };

  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[2].binding = 2;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[2].descriptorCount = 1;
  bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[3].binding = 3;
  bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[3].descriptorCount = 1;
  bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutCreateInfo layout_info = { 0 };
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = 4;
  layout_info.pBindings = bindings;

  if (vkCreateDescriptorSetLayout (context->device, &layout_info, NULL,
                                   &raymarcher->descriptor_set_layout)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to create descriptor set layout");
    }

  VkDescriptorPoolSize pool_sizes[3] = { 0 };
  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[0].descriptorCount = 2;
  pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  pool_sizes[1].descriptorCount = 5;
  pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  pool_sizes[2].descriptorCount = 2;

  VkDescriptorPoolCreateInfo pool_info = { 0 };
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.poolSizeCount = 3;
  pool_info.pPoolSizes = pool_sizes;
  pool_info.maxSets = 2;

  if (vkCreateDescriptorPool (context->device, &pool_info, NULL,
                              &raymarcher->descriptor_pool)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to create descriptor pool");
    }

  VkDescriptorSetAllocateInfo alloc_info = { 0 };
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = raymarcher->descriptor_pool;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &raymarcher->descriptor_set_layout;

  if (vkAllocateDescriptorSets (context->device, &alloc_info,
                                &raymarcher->descriptor_set)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to allocate descriptor set");
    }

  VkDescriptorBufferInfo uniform_buffer_info = { 0 };
  uniform_buffer_info.buffer = raymarcher->uniform_buffer.buffer;
  uniform_buffer_info.offset = 0;
  uniform_buffer_info.range = sizeof (raymarch_uniforms_t);

  VkDescriptorImageInfo color_depth_image_info = { 0 };
  color_depth_image_info.imageView = raymarcher->output_color_depth.view;
  color_depth_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  VkDescriptorImageInfo normal_image_info = { 0 };
  normal_image_info.imageView = raymarcher->output_normal.view;
  normal_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  VkDescriptorBufferInfo sdf_buffer_info = { 0 };
  sdf_buffer_info.buffer = raymarcher->sdf_objects_buffer.buffer;
  sdf_buffer_info.offset = 0;
  sdf_buffer_info.range = sizeof (sdf_object_t) * MAX_SDF_OBJECTS;

  VkWriteDescriptorSet writes[4] = { 0 };
  writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[0].dstSet = raymarcher->descriptor_set;
  writes[0].dstBinding = 0;
  writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writes[0].descriptorCount = 1;
  writes[0].pBufferInfo = &uniform_buffer_info;

  writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[1].dstSet = raymarcher->descriptor_set;
  writes[1].dstBinding = 1;
  writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  writes[1].descriptorCount = 1;
  writes[1].pImageInfo = &color_depth_image_info;

  writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[2].dstSet = raymarcher->descriptor_set;
  writes[2].dstBinding = 2;
  writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[2].descriptorCount = 1;
  writes[2].pBufferInfo = &sdf_buffer_info;

  writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[3].dstSet = raymarcher->descriptor_set;
  writes[3].dstBinding = 3;
  writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  writes[3].descriptorCount = 1;
  writes[3].pImageInfo = &normal_image_info;

  vkUpdateDescriptorSets (context->device, 4, writes, 0, NULL);

  VkPipelineLayoutCreateInfo pipeline_layout_info = { 0 };
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = 1;
  pipeline_layout_info.pSetLayouts = &raymarcher->descriptor_set_layout;

  if (vkCreatePipelineLayout (context->device, &pipeline_layout_info, NULL,
                              &raymarcher->pipeline_layout)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to create pipeline layout");
    }

  VkCommandBufferAllocateInfo cmd_alloc_info = { 0 };
  cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmd_alloc_info.commandPool = context->compute_command_pool;
  cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmd_alloc_info.commandBufferCount = 1;

  if (vkAllocateCommandBuffers (context->device, &cmd_alloc_info,
                                &raymarcher->compute_command_buffer)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to allocate command buffer");
    }

  VkFenceCreateInfo fence_info = { 0 };
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if (vkCreateFence (context->device, &fence_info, NULL,
                     &raymarcher->compute_fence)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN, "Failed to create fence");
    }

  return RESULT_SUCCESS;
}

void
raymarcher_destroy (raymarcher_t *raymarcher)
{
  if (!raymarcher || !raymarcher->vk_context)
    return;

  vulkan_context_t *context = raymarcher->vk_context;

  if (raymarcher->compute_fence)
    vkDestroyFence (context->device, raymarcher->compute_fence, NULL);
  if (raymarcher->compute_pipeline)
    vkDestroyPipeline (context->device, raymarcher->compute_pipeline, NULL);
  if (raymarcher->pipeline_layout)
    vkDestroyPipelineLayout (context->device, raymarcher->pipeline_layout,
                             NULL);
  if (raymarcher->descriptor_pool)
    vkDestroyDescriptorPool (context->device, raymarcher->descriptor_pool,
                             NULL);
  if (raymarcher->descriptor_set_layout)
    vkDestroyDescriptorSetLayout (context->device,
                                  raymarcher->descriptor_set_layout, NULL);
  if (raymarcher->compute_shader)
    vkDestroyShaderModule (context->device, raymarcher->compute_shader, NULL);

  if (raymarcher->uniform_buffer.buffer)
    gpu_buffer_destroy (context, &raymarcher->uniform_buffer);
  if (raymarcher->sdf_objects_buffer.buffer)
    gpu_buffer_destroy (context, &raymarcher->sdf_objects_buffer);
  if (raymarcher->output_color_depth.image)
    gpu_image_destroy (context, &raymarcher->output_color_depth);
  if (raymarcher->output_normal.image)
    gpu_image_destroy (context, &raymarcher->output_normal);
  if (raymarcher->output_final.image)
    gpu_image_destroy (context, &raymarcher->output_final);
  if (raymarcher->lighting_uniform_buffer.buffer)
    gpu_buffer_destroy (context, &raymarcher->lighting_uniform_buffer);
  if (raymarcher->lighting_shader)
    vkDestroyShaderModule (context->device, raymarcher->lighting_shader, NULL);
  if (raymarcher->lighting_pipeline)
    vkDestroyPipeline (context->device, raymarcher->lighting_pipeline, NULL);
  if (raymarcher->lighting_pipeline_layout)
    vkDestroyPipelineLayout (context->device,
                             raymarcher->lighting_pipeline_layout, NULL);
  if (raymarcher->lighting_descriptor_set_layout)
    vkDestroyDescriptorSetLayout (
        context->device, raymarcher->lighting_descriptor_set_layout, NULL);
}

result_t
raymarcher_load_shader (raymarcher_t *raymarcher, const char *shader_path)
{
  size_t code_size;
  char *code = read_file (shader_path, &code_size);
  if (!code)
    {
      return RESULT_ERROR (RESULT_ERROR_FILE_NOT_FOUND,
                           "Failed to load shader");
    }

  VkShaderModuleCreateInfo create_info = { 0 };
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code_size;
  create_info.pCode = (const uint32_t *)code;

  if (vkCreateShaderModule (raymarcher->vk_context->device, &create_info, NULL,
                            &raymarcher->compute_shader)
      != VK_SUCCESS)
    {
      free (code);
      return RESULT_ERROR (RESULT_ERROR_SHADER_COMPILATION,
                           "Failed to create shader module");
    }

  free (code);

  VkComputePipelineCreateInfo pipeline_info = { 0 };
  pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipeline_info.stage.sType
      = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  pipeline_info.stage.module = raymarcher->compute_shader;
  pipeline_info.stage.pName = "main";
  pipeline_info.layout = raymarcher->pipeline_layout;

  if (vkCreateComputePipelines (raymarcher->vk_context->device, VK_NULL_HANDLE,
                                1, &pipeline_info, NULL,
                                &raymarcher->compute_pipeline)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to create compute pipeline");
    }

  return RESULT_SUCCESS;
}

result_t
raymarcher_execute (raymarcher_t *raymarcher,
                    const raymarch_uniforms_t *uniforms)
{
  vulkan_context_t *context = raymarcher->vk_context;

  vkWaitForFences (context->device, 1, &raymarcher->compute_fence, VK_TRUE,
                   UINT64_MAX);
  vkResetFences (context->device, 1, &raymarcher->compute_fence);

  result_t result = gpu_buffer_upload (context, &raymarcher->uniform_buffer,
                                       uniforms, sizeof (raymarch_uniforms_t));
  if (result.code != RESULT_OK)
    return result;

  VkCommandBufferBeginInfo begin_info = { 0 };
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  vkBeginCommandBuffer (raymarcher->compute_command_buffer, &begin_info);

  vkCmdBindPipeline (raymarcher->compute_command_buffer,
                     VK_PIPELINE_BIND_POINT_COMPUTE,
                     raymarcher->compute_pipeline);
  vkCmdBindDescriptorSets (
      raymarcher->compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
      raymarcher->pipeline_layout, 0, 1, &raymarcher->descriptor_set, 0, NULL);

  uint32_t group_count_x = (raymarcher->width + 7) / 8;
  uint32_t group_count_y = (raymarcher->height + 7) / 8;
  vkCmdDispatch (raymarcher->compute_command_buffer, group_count_x,
                 group_count_y, 1);

  vkEndCommandBuffer (raymarcher->compute_command_buffer);

  VkSubmitInfo submit_info = { 0 };
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &raymarcher->compute_command_buffer;

  if (vkQueueSubmit (context->compute_queue, 1, &submit_info,
                     raymarcher->compute_fence)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to submit compute commands");
    }

  return RESULT_SUCCESS;
}

const gpu_image_t *
raymarcher_get_color_depth (const raymarcher_t *raymarcher)
{
  return &raymarcher->output_color_depth;
}

const gpu_image_t *
raymarcher_get_final (const raymarcher_t *raymarcher)
{
  return &raymarcher->output_final;
}

result_t
raymarcher_load_lighting_shader (raymarcher_t *raymarcher,
                                 const char *shader_path)
{
  size_t code_size;
  char *code = read_file (shader_path, &code_size);
  if (!code)
    {
      return RESULT_ERROR (RESULT_ERROR_FILE_NOT_FOUND,
                           "Failed to load lighting shader");
    }

  VkShaderModuleCreateInfo create_info = { 0 };
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code_size;
  create_info.pCode = (const uint32_t *)code;

  if (vkCreateShaderModule (raymarcher->vk_context->device, &create_info, NULL,
                            &raymarcher->lighting_shader)
      != VK_SUCCESS)
    {
      free (code);
      return RESULT_ERROR (RESULT_ERROR_SHADER_COMPILATION,
                           "Failed to create lighting shader module");
    }

  free (code);

  VkDescriptorSetLayoutBinding bindings[6] = { 0 };

  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[2].binding = 2;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[2].descriptorCount = 1;
  bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[3].binding = 4;
  bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[3].descriptorCount = 1;
  bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[4].binding = 5;
  bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[4].descriptorCount = 1;
  bindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutCreateInfo layout_info = { 0 };
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = 5;
  layout_info.pBindings = bindings;

  if (vkCreateDescriptorSetLayout (raymarcher->vk_context->device,
                                   &layout_info, NULL,
                                   &raymarcher->lighting_descriptor_set_layout)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to create lighting descriptor set layout");
    }

  VkDescriptorSetAllocateInfo alloc_info = { 0 };
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = raymarcher->descriptor_pool;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &raymarcher->lighting_descriptor_set_layout;

  if (vkAllocateDescriptorSets (raymarcher->vk_context->device, &alloc_info,
                                &raymarcher->lighting_descriptor_set)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to allocate lighting descriptor set");
    }

  VkDescriptorBufferInfo lighting_uniform_buffer_info = { 0 };
  lighting_uniform_buffer_info.buffer
      = raymarcher->lighting_uniform_buffer.buffer;
  lighting_uniform_buffer_info.offset = 0;
  lighting_uniform_buffer_info.range = sizeof (lighting_uniforms_t);

  VkDescriptorImageInfo color_depth_read_info = { 0 };
  color_depth_read_info.imageView = raymarcher->output_color_depth.view;
  color_depth_read_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  VkDescriptorImageInfo normal_read_info = { 0 };
  normal_read_info.imageView = raymarcher->output_normal.view;
  normal_read_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  VkDescriptorImageInfo final_write_info = { 0 };
  final_write_info.imageView = raymarcher->output_final.view;
  final_write_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  VkDescriptorBufferInfo sdf_buffer_info = { 0 };
  sdf_buffer_info.buffer = raymarcher->sdf_objects_buffer.buffer;
  sdf_buffer_info.offset = 0;
  sdf_buffer_info.range = sizeof (sdf_object_t) * MAX_SDF_OBJECTS;

  VkWriteDescriptorSet writes[5] = { 0 };
  writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[0].dstSet = raymarcher->lighting_descriptor_set;
  writes[0].dstBinding = 0;
  writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writes[0].descriptorCount = 1;
  writes[0].pBufferInfo = &lighting_uniform_buffer_info;

  writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[1].dstSet = raymarcher->lighting_descriptor_set;
  writes[1].dstBinding = 1;
  writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  writes[1].descriptorCount = 1;
  writes[1].pImageInfo = &color_depth_read_info;

  writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[2].dstSet = raymarcher->lighting_descriptor_set;
  writes[2].dstBinding = 2;
  writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  writes[2].descriptorCount = 1;
  writes[2].pImageInfo = &normal_read_info;

  writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[3].dstSet = raymarcher->lighting_descriptor_set;
  writes[3].dstBinding = 4;
  writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  writes[3].descriptorCount = 1;
  writes[3].pImageInfo = &final_write_info;

  writes[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[4].dstSet = raymarcher->lighting_descriptor_set;
  writes[4].dstBinding = 5;
  writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[4].descriptorCount = 1;
  writes[4].pBufferInfo = &sdf_buffer_info;

  vkUpdateDescriptorSets (raymarcher->vk_context->device, 5, writes, 0, NULL);

  VkPipelineLayoutCreateInfo pipeline_layout_info = { 0 };
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = 1;
  pipeline_layout_info.pSetLayouts
      = &raymarcher->lighting_descriptor_set_layout;

  if (vkCreatePipelineLayout (raymarcher->vk_context->device,
                              &pipeline_layout_info, NULL,
                              &raymarcher->lighting_pipeline_layout)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to create lighting pipeline layout");
    }

  VkComputePipelineCreateInfo pipeline_info = { 0 };
  pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipeline_info.stage.sType
      = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  pipeline_info.stage.module = raymarcher->lighting_shader;
  pipeline_info.stage.pName = "main";
  pipeline_info.layout = raymarcher->lighting_pipeline_layout;

  if (vkCreateComputePipelines (raymarcher->vk_context->device, VK_NULL_HANDLE,
                                1, &pipeline_info, NULL,
                                &raymarcher->lighting_pipeline)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to create lighting compute pipeline");
    }

  return RESULT_SUCCESS;
}

result_t
raymarcher_execute_lighting (raymarcher_t *raymarcher,
                             const lighting_uniforms_t *uniforms)
{
  vulkan_context_t *context = raymarcher->vk_context;

  vkWaitForFences (context->device, 1, &raymarcher->compute_fence, VK_TRUE,
                   UINT64_MAX);
  vkResetFences (context->device, 1, &raymarcher->compute_fence);

  result_t result
      = gpu_buffer_upload (context, &raymarcher->lighting_uniform_buffer,
                           uniforms, sizeof (lighting_uniforms_t));
  if (result.code != RESULT_OK)
    return result;

  VkCommandBufferBeginInfo begin_info = { 0 };
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  vkBeginCommandBuffer (raymarcher->compute_command_buffer, &begin_info);

  VkImageMemoryBarrier barriers[2] = { 0 };
  barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
  barriers[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
  barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  barriers[0].image = raymarcher->output_color_depth.image;
  barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barriers[0].subresourceRange.levelCount = 1;
  barriers[0].subresourceRange.layerCount = 1;

  barriers[1] = barriers[0];
  barriers[1].image = raymarcher->output_normal.image;

  vkCmdPipelineBarrier (
      raymarcher->compute_command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 2, barriers);

  vkCmdBindPipeline (raymarcher->compute_command_buffer,
                     VK_PIPELINE_BIND_POINT_COMPUTE,
                     raymarcher->lighting_pipeline);
  vkCmdBindDescriptorSets (raymarcher->compute_command_buffer,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           raymarcher->lighting_pipeline_layout, 0, 1,
                           &raymarcher->lighting_descriptor_set, 0, NULL);

  uint32_t group_count_x = (raymarcher->width + 7) / 8;
  uint32_t group_count_y = (raymarcher->height + 7) / 8;
  vkCmdDispatch (raymarcher->compute_command_buffer, group_count_x,
                 group_count_y, 1);

  vkEndCommandBuffer (raymarcher->compute_command_buffer);

  VkSubmitInfo submit_info = { 0 };
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &raymarcher->compute_command_buffer;

  if (vkQueueSubmit (context->compute_queue, 1, &submit_info,
                     raymarcher->compute_fence)
      != VK_SUCCESS)
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN,
                           "Failed to submit lighting compute commands");
    }

  return RESULT_SUCCESS;
}
