/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#ifdef GPU_SHADER
#  pragma once
#  include "gpu_glsl_cpp_stubs.hh"

#  include "gpu_shader_fullscreen_info.hh"
#endif

#include "overlay_common_info.hh"

GPU_SHADER_CREATE_INFO(overlay_antialiasing)
DO_STATIC_COMPILATION()
SAMPLER(0, DEPTH_2D, depthTex)
SAMPLER(1, FLOAT_2D, colorTex)
SAMPLER(2, FLOAT_2D, lineTex)
PUSH_CONSTANT(bool, doSmoothLines)
FRAGMENT_OUT(0, float4, fragColor)
FRAGMENT_SOURCE("overlay_antialiasing_frag.glsl")
ADDITIONAL_INFO(gpu_fullscreen)
ADDITIONAL_INFO(draw_globals)
GPU_SHADER_CREATE_END()

GPU_SHADER_CREATE_INFO(overlay_xray_fade)
DO_STATIC_COMPILATION()
SAMPLER(0, DEPTH_2D, depthTex)
SAMPLER(1, DEPTH_2D, xrayDepthTex)
PUSH_CONSTANT(float, opacity)
FRAGMENT_OUT(0, float4, fragColor)
FRAGMENT_SOURCE("overlay_xray_fade_frag.glsl")
ADDITIONAL_INFO(gpu_fullscreen)
SAMPLER(2, DEPTH_2D, xrayDepthTexInfront)
SAMPLER(3, DEPTH_2D, depthTexInfront)
GPU_SHADER_CREATE_END()
