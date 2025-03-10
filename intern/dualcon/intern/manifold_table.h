/* SPDX-FileCopyrightText: 2002-2022 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __MANIFOLD_TABLE_H__
#define __MANIFOLD_TABLE_H__

struct ManifoldIndices {
  int comps;
  int pairs[12][2];
};

extern const ManifoldIndices manifold_table[256];

#endif /* __MANIFOLD_TABLE_H__ */
