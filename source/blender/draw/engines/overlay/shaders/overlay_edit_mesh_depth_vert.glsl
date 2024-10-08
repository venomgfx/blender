/* SPDX-FileCopyrightText: 2017-2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "common_view_clipping_lib.glsl"
#include "common_view_lib.glsl"

void main()
{
  GPU_INTEL_VERTEX_SHADER_WORKAROUND

  vec3 world_pos = point_object_to_world(pos);
  vec3 view_pos = point_world_to_view(world_pos);
  gl_Position = point_view_to_ndc(view_pos);

  /* Offset Z position for retopology overlay. */
  gl_Position.z += get_homogenous_z_offset(view_pos.z, gl_Position.w, retopologyOffset);

  view_clipping_distances(world_pos);
}
