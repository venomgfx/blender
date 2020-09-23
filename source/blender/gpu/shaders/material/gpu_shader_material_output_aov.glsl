void node_output_aov_color(vec4 color, float value, out Closure result)
{
  result = CLOSURE_DEFAULT;
#ifndef VOLUMETRICS
  result.radiance = color.rgb;
#endif
}

void node_output_aov_value(vec4 color, float value, out Closure result)
{
  result = CLOSURE_DEFAULT;
#ifndef VOLUMETRICS
  result.radiance = vec3(value);
#endif
}
