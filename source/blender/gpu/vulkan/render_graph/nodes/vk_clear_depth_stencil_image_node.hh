/* SPDX-FileCopyrightText: 2024 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup gpu
 */

#pragma once

#include "vk_node_info.hh"

namespace blender::gpu::render_graph {

/**
 * Information stored inside the render graph node. See `VKRenderGraphNode`.
 */
struct VKClearDepthStencilImageData {
  VkImage vk_image;
  VkClearDepthStencilValue vk_clear_depth_stencil_value;
  VkImageSubresourceRange vk_image_subresource_range;
};

struct VKClearDepthStencilImageCreateInfo {
  VKClearDepthStencilImageData node_data;

  /**
   * All image aspects of the image that will be cleared.
   *
   * Used during the pipeline barriers as the full image aspect needs to be known for changing the
   * layout. Even when only one aspect is eventually cleared. */
  VkImageAspectFlags vk_image_aspects;
};

class VKClearDepthStencilImageNode : public VKNodeInfo<VKNodeType::CLEAR_DEPTH_STENCIL_IMAGE,
                                                       VKClearDepthStencilImageCreateInfo,
                                                       VKClearDepthStencilImageData,
                                                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                       VKResourceType::IMAGE> {
 public:
  /**
   * Update the node data with the data inside create_info.
   *
   * Has been implemented as a template to ensure all node specific data
   * (`VK*Data`/`VK*CreateInfo`) types can be included in the same header file as the logic. The
   * actual node data (`VKRenderGraphNode` includes all header files.)
   */
  template<typename Node, typename Storage>
  static void set_node_data(Node &node, Storage & /* storage */, const CreateInfo &create_info)
  {
    node.clear_depth_stencil_image = create_info.node_data;
  }

  /**
   * Extract read/write resource dependencies from `create_info` and add them to `node_links`.
   */
  void build_links(VKResourceStateTracker &resources,
                   VKRenderGraphNodeLinks &node_links,
                   const CreateInfo &create_info) override
  {
    ResourceWithStamp resource = resources.get_image_and_increase_stamp(
        create_info.node_data.vk_image);
    node_links.outputs.append({resource,
                               VK_ACCESS_TRANSFER_WRITE_BIT,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               create_info.vk_image_aspects});
  }

  /**
   * Build the commands and add them to the command_buffer.
   */
  void build_commands(VKCommandBufferInterface &command_buffer,
                      Data &data,
                      VKBoundPipelines & /*r_bound_pipelines*/) override
  {
    command_buffer.clear_depth_stencil_image(data.vk_image,
                                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                             &data.vk_clear_depth_stencil_value,
                                             1,
                                             &data.vk_image_subresource_range);
  }
};
}  // namespace blender::gpu::render_graph
