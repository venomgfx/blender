/* SPDX-FileCopyrightText: 2019-2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "infos/overlay_extra_info.hh"

FRAGMENT_SHADER_CREATE_INFO(overlay_extra_groundline)

#include "overlay_common_lib.glsl"
#include "select_lib.glsl"

void main()
{
  fragColor = finalColor;
#ifdef IS_SPOT_CONE
  lineOutput = float4(0.0f);
#else
  lineOutput = pack_line_data(gl_FragCoord.xy, edgeStart, edgePos);
  select_id_output(select_id);
#endif
}
