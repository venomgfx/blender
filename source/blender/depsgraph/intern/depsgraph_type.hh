/* SPDX-FileCopyrightText: 2013 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup depsgraph
 *
 * Data-types for internal use in the Depsgraph
 *
 * All of these data-types are only really used within the "core" depsgraph.
 * In particular, node types declared here form the structure of operations
 * in the graph.
 */

#pragma once

#include <cstdint>

/* TODO(sergey): Ideally we'll just use char* and statically allocated strings
 * to avoid any possible overhead caused by string (re)allocation/formatting. */

struct CustomData_MeshMasks;

namespace blender::deg {

/* Source of the dependency graph node update tag.
 *
 * NOTE: This is a bit mask, so accumulation of sources is possible.
 *
 * TODO(sergey): Find a better place for this. */
enum eUpdateSource {
  /* Update is caused by a time change. */
  DEG_UPDATE_SOURCE_TIME = (1 << 0),
  /* Update caused by user directly or indirectly influencing the node. */
  DEG_UPDATE_SOURCE_USER_EDIT = (1 << 1),
  /* Update is happening as a special response for the relations update. */
  DEG_UPDATE_SOURCE_RELATIONS = (1 << 2),
  /* Update is happening due to visibility change. */
  DEG_UPDATE_SOURCE_VISIBILITY = (1 << 3),
  /**
   * Update is happening because some side effect of the reevaluation is necessary. For example,
   * baking intermediate geometry nodes happens during normal evaluation but does not actually
   * change the final result. For baking we still want to reevaluate it.
   */
  DEG_UPDATE_SOURCE_SIDE_EFFECT_REQUEST = (1 << 4),
};

/* C++ wrapper around DNA's CustomData_MeshMasks struct. */
struct DEGCustomDataMeshMasks {
  uint64_t vert_mask;
  uint64_t edge_mask;
  uint64_t face_mask;
  uint64_t loop_mask;
  uint64_t poly_mask;

  DEGCustomDataMeshMasks() : vert_mask(0), edge_mask(0), face_mask(0), loop_mask(0), poly_mask(0)
  {
  }

  explicit DEGCustomDataMeshMasks(const CustomData_MeshMasks *other);

  DEGCustomDataMeshMasks &operator|=(const DEGCustomDataMeshMasks &other)
  {
    this->vert_mask |= other.vert_mask;
    this->edge_mask |= other.edge_mask;
    this->face_mask |= other.face_mask;
    this->loop_mask |= other.loop_mask;
    this->poly_mask |= other.poly_mask;
    return *this;
  }

  DEGCustomDataMeshMasks operator|(const DEGCustomDataMeshMasks &other) const
  {
    DEGCustomDataMeshMasks result;
    result.vert_mask = this->vert_mask | other.vert_mask;
    result.edge_mask = this->edge_mask | other.edge_mask;
    result.face_mask = this->face_mask | other.face_mask;
    result.loop_mask = this->loop_mask | other.loop_mask;
    result.poly_mask = this->poly_mask | other.poly_mask;
    return result;
  }

  bool operator==(const DEGCustomDataMeshMasks &other) const
  {
    return (this->vert_mask == other.vert_mask && this->edge_mask == other.edge_mask &&
            this->face_mask == other.face_mask && this->loop_mask == other.loop_mask &&
            this->poly_mask == other.poly_mask);
  }

  bool operator!=(const DEGCustomDataMeshMasks &other) const
  {
    return !(*this == other);
  }

  static DEGCustomDataMeshMasks MaskVert(const uint64_t vert_mask)
  {
    DEGCustomDataMeshMasks result;
    result.vert_mask = vert_mask;
    return result;
  }

  static DEGCustomDataMeshMasks MaskEdge(const uint64_t edge_mask)
  {
    DEGCustomDataMeshMasks result;
    result.edge_mask = edge_mask;
    return result;
  }

  static DEGCustomDataMeshMasks MaskFace(const uint64_t face_mask)
  {
    DEGCustomDataMeshMasks result;
    result.face_mask = face_mask;
    return result;
  }

  static DEGCustomDataMeshMasks MaskLoop(const uint64_t loop_mask)
  {
    DEGCustomDataMeshMasks result;
    result.loop_mask = loop_mask;
    return result;
  }

  static DEGCustomDataMeshMasks MaskPoly(const uint64_t poly_mask)
  {
    DEGCustomDataMeshMasks result;
    result.poly_mask = poly_mask;
    return result;
  }
};

}  // namespace blender::deg
