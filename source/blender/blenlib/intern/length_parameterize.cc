/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup bli
 */

#include "BLI_length_parameterize.hh"
#include "BLI_task.hh"
#include <algorithm>

namespace blender::length_parameterize {

void sample_uniform(const Span<float> accumulated_segment_lengths,
                    const bool include_last_point,
                    MutableSpan<int> r_segment_indices,
                    MutableSpan<float> r_factors)
{
  const int count = r_segment_indices.size();
  BLI_assert(count > 0);
  BLI_assert(accumulated_segment_lengths.size() >= 1);
  BLI_assert(
      std::is_sorted(accumulated_segment_lengths.begin(), accumulated_segment_lengths.end()));

  if (count == 1) {
    r_segment_indices[0] = 0;
    r_factors[0] = 0.0f;
    return;
  }
  const float total_length = accumulated_segment_lengths.last();
  const float step_length = total_length / (count - include_last_point);
  threading::parallel_for(IndexRange(count), 512, [&](const IndexRange range) {
    SampleSegmentHint hint;
    for (const int i : range) {
      /* Use minimum to avoid issues with floating point accuracy. */
      const float sample_length = std::min(total_length, i * step_length);
      sample_at_length(
          accumulated_segment_lengths, sample_length, r_segment_indices[i], r_factors[i], &hint);
    }
  });
}

void sample_uniform_reverse(const Span<float> accumulated_segment_lengths,
                            const bool include_first_point,
                            MutableSpan<int> r_segment_indices,
                            MutableSpan<float> r_factors)
{
  const int count = r_segment_indices.size();
  BLI_assert(count > 0);
  BLI_assert(accumulated_segment_lengths.size() >= 1);
  BLI_assert(
      std::is_sorted(accumulated_segment_lengths.begin(), accumulated_segment_lengths.end()));

  if (count == 1) {
    r_segment_indices[0] = accumulated_segment_lengths.size() - 1;
    r_factors[0] = 1.0f;
    return;
  }
  const float total_length = accumulated_segment_lengths.last();
  const float step_length = total_length / (count - include_first_point);
  threading::parallel_for(IndexRange(count), 512, [&](const IndexRange range) {
    SampleSegmentHint hint;
    for (const int i : range) {
      /* Use maximum to avoid issues with floating point accuracy. */
      const float sample_length = std::max(0.0f, total_length - i * step_length);
      sample_at_length(
          accumulated_segment_lengths, sample_length, r_segment_indices[i], r_factors[i], &hint);
    }
  });
}

void sample_at_lengths(const Span<float> accumulated_segment_lengths,
                       const Span<float> sample_lengths,
                       MutableSpan<int> r_segment_indices,
                       MutableSpan<float> r_factors)
{
  BLI_assert(
      std::is_sorted(accumulated_segment_lengths.begin(), accumulated_segment_lengths.end()));
  BLI_assert(std::is_sorted(sample_lengths.begin(), sample_lengths.end()));

  const int count = sample_lengths.size();
  BLI_assert(count == r_segment_indices.size());
  BLI_assert(count == r_factors.size());

  threading::parallel_for(IndexRange(count), 512, [&](const IndexRange range) {
    SampleSegmentHint hint;
    for (const int i : range) {
      const float sample_length = sample_lengths[i];
      sample_at_length(
          accumulated_segment_lengths, sample_length, r_segment_indices[i], r_factors[i], &hint);
    }
  });
}

}  // namespace blender::length_parameterize
