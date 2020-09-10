#pragma BLENDER_REQUIRE(common_globals_lib.glsl)
#pragma BLENDER_REQUIRE(common_view_lib.glsl)

in vec3 pos;
in vec2 u;
in int flag;

out float selectionFac;
noperspective out vec2 stipplePos;
flat out vec2 stippleStart;

void main()
{
  vec3 world_pos = point_object_to_world(vec3(u, 0.0));
  gl_Position = point_world_to_ndc(world_pos);
  /* Snap vertices to the pixel grid to reduce artifacts. */
  vec2 half_viewport_res = sizeViewport.xy * 0.5;
  vec2 half_pixel_offset = sizeViewportInv * 0.5;
  gl_Position.xy = floor(gl_Position.xy * half_viewport_res) / half_viewport_res +
                   half_pixel_offset;

  bool is_select = (flag & VERT_UV_SELECT) != 0;
  selectionFac = is_select ? 1.0 : 0.0;
  float depth = is_select ? 0.3 : 0.4;
  gl_Position.z = depth;

  /* Avoid precision loss. */
  stippleStart = stipplePos = 500.0 + 500.0 * (gl_Position.xy / gl_Position.w);
}
