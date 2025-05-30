/* SPDX-FileCopyrightText: 2022 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "gpu_shader_compositor_texture_utilities.glsl"

void main()
{
  int2 texel = int2(gl_GlobalInvocationID.xy);

  float4 center_determinator = texture_load(determinator_tx, texel);

  /* Go over the pixels in the blur window of the specified radius around the center pixel, and for
   * pixels whose determinator is close enough to the determinator of the center pixel, accumulate
   * their color as well as their weights. */
  float accumulated_weight = 0.0f;
  float4 accumulated_color = float4(0.0f);
  for (int y = -radius; y <= radius; y++) {
    for (int x = -radius; x <= radius; x++) {
      float4 determinator = texture_load(determinator_tx, texel + int2(x, y));
      float difference = dot(abs(center_determinator - determinator).rgb, float3(1.0f)) / 3.0f;

      if (difference < threshold) {
        accumulated_weight += 1.0f;
        accumulated_color += texture_load(input_tx, texel + int2(x, y));
      }
    }
  }

  /* Write the accumulated color divided by the accumulated weight if any pixel in the window was
   * accumulated, otherwise, write a fallback black color. */
  float4 fallback = float4(float3(0.0f), 1.0f);
  float4 color = (accumulated_weight != 0.0f) ? (accumulated_color / accumulated_weight) :
                                                fallback;
  imageStore(output_img, texel, color);
}
