/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup bmesh
 *
 * Removes isolated geometry regions without creating holes in the mesh.
 */

#include "MEM_guardedalloc.h"

#include "BLI_math_vector.h"
#include "BLI_stack.h"
#include "BLI_vector.hh"

#include "bmesh.hh"
#include "bmesh_tools.hh"

#include "intern/bmesh_operators_private.hh"

using blender::Vector;

/* ***_ISGC: mark for garbage-collection */

#define FACE_MARK 1
#define FACE_ORIG 2
#define FACE_NEW 4
#define FACE_TAG 8

#define EDGE_MARK 1
#define EDGE_TAG 2
#define EDGE_ISGC 8

#define VERT_MARK 1
#define VERT_MARK_PAIR 4
#define VERT_TAG 2
#define VERT_ISGC 8
#define VERT_MARK_TEAR 16

static bool UNUSED_FUNCTION(check_hole_in_region)(BMesh *bm, BMFace *f)
{
  BMWalker regwalker;
  BMIter liter2;
  BMLoop *l2, *l3;
  BMFace *f2;

  /* Checks if there are any unmarked boundary edges in the face region. */

  BMW_init(&regwalker,
           bm,
           BMW_ISLAND,
           BMW_MASK_NOP,
           BMW_MASK_NOP,
           FACE_MARK,
           BMW_FLAG_NOP,
           BMW_NIL_LAY);

  for (f2 = static_cast<BMFace *>(BMW_begin(&regwalker, f)); f2;
       f2 = static_cast<BMFace *>(BMW_step(&regwalker)))
  {
    BM_ITER_ELEM (l2, &liter2, f2, BM_LOOPS_OF_FACE) {
      l3 = l2->radial_next;
      if (BMO_face_flag_test(bm, l3->f, FACE_MARK) != BMO_face_flag_test(bm, l2->f, FACE_MARK)) {
        if (!BMO_edge_flag_test(bm, l2->e, EDGE_MARK)) {
          return false;
        }
      }
    }
  }
  BMW_end(&regwalker);

  return true;
}

static void bm_face_split(BMesh *bm, const short oflag, bool use_edge_delete)
{
  BLI_Stack *edge_delete_verts;
  BMIter iter;
  BMVert *v;

  if (use_edge_delete) {
    edge_delete_verts = BLI_stack_new(sizeof(BMVert *), __func__);
  }

  BM_ITER_MESH (v, &iter, bm, BM_VERTS_OF_MESH) {
    if (BMO_vert_flag_test(bm, v, oflag)) {
      if (BM_vert_is_edge_pair(v) == false) {
        BMIter liter;
        BMLoop *l;
        BM_ITER_ELEM (l, &liter, v, BM_LOOPS_OF_VERT) {
          if (l->f->len > 3) {
            if (BMO_vert_flag_test(bm, l->next->v, oflag) == 0 &&
                BMO_vert_flag_test(bm, l->prev->v, oflag) == 0)
            {
              BM_face_split(bm, l->f, l->next, l->prev, nullptr, nullptr, true);
            }
          }
        }

        if (use_edge_delete) {
          BLI_stack_push(edge_delete_verts, &v);
        }
      }
    }
  }

  if (use_edge_delete) {
    while (!BLI_stack_is_empty(edge_delete_verts)) {
      /* remove surrounding edges & faces */
      BLI_stack_pop(edge_delete_verts, &v);
      while (v->e) {
        BM_edge_kill(bm, v->e);
      }
    }
    BLI_stack_free(edge_delete_verts);
  }
}

void bmo_dissolve_faces_exec(BMesh *bm, BMOperator *op)
{
  BMOIter oiter;
  BMFace *f;
  BMWalker regwalker;

  const bool use_verts = BMO_slot_bool_get(op->slots_in, "use_verts");

  if (use_verts) {
    /* tag verts that start out with only 2 edges,
     * don't remove these later */
    BMIter viter;
    BMVert *v;

    BM_ITER_MESH (v, &viter, bm, BM_VERTS_OF_MESH) {
      BMO_vert_flag_set(bm, v, VERT_MARK, !BM_vert_is_edge_pair(v));
    }
  }

  BMO_slot_buffer_flag_enable(bm, op->slots_in, "faces", BM_FACE, FACE_MARK | FACE_TAG);

  /* List of regions which are themselves a list of faces. */
  Vector<Vector<BMFace *>> regions;

  /* collect region */
  BMO_ITER (f, &oiter, op->slots_in, "faces", BM_FACE) {
    if (!BMO_face_flag_test(bm, f, FACE_TAG)) {
      continue;
    }

    BMW_init(&regwalker,
             bm,
             BMW_ISLAND_MANIFOLD,
             BMW_MASK_NOP,
             BMW_MASK_NOP,
             FACE_MARK,
             /* no need to check BMW_FLAG_TEST_HIDDEN, faces are already marked by the bmo. */
             BMW_FLAG_NOP,
             BMW_NIL_LAY);

    /* Check there are at least two faces before creating the array. */
    BMFace *faces_init[2];
    if ((faces_init[0] = static_cast<BMFace *>(BMW_begin(&regwalker, f))) &&
        (faces_init[1] = static_cast<BMFace *>(BMW_step(&regwalker))))
    {
      Vector<BMFace *> faces;
      faces.append(faces_init[0]);
      faces.append(faces_init[1]);

      BMFace *f_iter;
      while ((f_iter = static_cast<BMFace *>(BMW_step(&regwalker)))) {
        faces.append(f_iter);
      }

      for (BMFace *face : faces) {
        BMO_face_flag_disable(bm, face, FACE_TAG);
        BMO_face_flag_enable(bm, face, FACE_ORIG);
      }

      regions.append_as(std::move(faces));
    }

    BMW_end(&regwalker);
  }

  /* track how many faces we should end up with */
  int totface_target = bm->totface;

  for (Vector<BMFace *> &faces : regions) {
    const int64_t faces_len = faces.size();

    BMFace *f_double;

    BMFace *f_new = BM_faces_join(bm, faces.data(), faces_len, true, &f_double);

    if (LIKELY(f_new)) {

      /* All the joined faces are gone and the fresh f_new represents their union. */
      totface_target -= faces_len - 1;

      if (UNLIKELY(f_double)) {
        /* `BM_faces_join()` succeeded, but there is a double. Keep the pre-existing face
         * and retain its custom-data. Remove the newly made merge result.  */
        BM_face_kill(bm, f_new);
        totface_target -= 1;
        f_new = f_double;
      }

      /* Un-mark the joined face to ensure it is not garbage collected later. */
      BMO_face_flag_disable(bm, f_new, FACE_ORIG);

      /* Mark the joined face so it can be added to the selection later. */
      BMO_face_flag_enable(bm, f_new, FACE_NEW);
    }
    else {
      /* `BM_faces_join()` failed. */

      /* NOTE: prior to 3.0 this raised an error: "Could not create merged face".
       * Change behavior since it's not useful to fail entirely when a single face-group
       * can't be merged into one face. Continue with other face groups instead.
       *
       * This could optionally do a partial merge, where some faces are joined. */

      /* Prevent these faces from being removed. */
      for (BMFace *face : faces) {
        BMO_face_flag_disable(bm, face, FACE_ORIG);
      }
    }
  }

  /* Typically no faces need to be deleted */
  if (totface_target != bm->totface) {
    BMO_op_callf(bm, op->flag, "delete geom=%ff context=%i", FACE_ORIG, DEL_FACES);
  }

  if (use_verts) {
    BMIter viter;
    BMVert *v, *v_next;

    BM_ITER_MESH_MUTABLE (v, v_next, &viter, bm, BM_VERTS_OF_MESH) {
      if (BMO_vert_flag_test(bm, v, VERT_MARK)) {
        if (BM_vert_is_edge_pair(v)) {
          BM_vert_collapse_edge(bm, v->e, v, true, true, true);
        }
      }
    }
  }

  BLI_assert(!BMO_error_occurred_at_level(bm, BMO_ERROR_FATAL));

  BMO_slot_buffer_from_enabled_flag(bm, op, op->slots_out, "region.out", BM_FACE, FACE_NEW);
}

void bmo_dissolve_edges_exec(BMesh *bm, BMOperator *op)
{
  // BMOperator fop;
  BMOIter eiter;
  BMIter iter;
  BMEdge *e, *e_next;
  BMVert *v, *v_next;

  const bool use_verts = BMO_slot_bool_get(op->slots_in, "use_verts");
  const bool use_face_split = BMO_slot_bool_get(op->slots_in, "use_face_split");

  if (use_face_split) {
    BMO_slot_buffer_flag_enable(bm, op->slots_in, "edges", BM_EDGE, EDGE_TAG);

    BM_ITER_MESH (v, &iter, bm, BM_VERTS_OF_MESH) {
      BMIter itersub;
      int untag_count = 0;
      BM_ITER_ELEM (e, &itersub, v, BM_EDGES_OF_VERT) {
        if (!BMO_edge_flag_test(bm, e, EDGE_TAG)) {
          untag_count++;
        }
      }

      /* check that we have 2 edges remaining after dissolve */
      if (untag_count <= 2) {
        BMO_vert_flag_enable(bm, v, VERT_TAG);
      }
    }

    bm_face_split(bm, VERT_TAG, false);
  }

  if (use_verts) {

    /* Mark all verts that are candidates to be dissolved. */
    BMO_ITER (e, &eiter, op->slots_in, "edges", BM_EDGE) {
      BMO_vert_flag_enable(bm, e->v1, VERT_MARK);
      BMO_vert_flag_enable(bm, e->v2, VERT_MARK);
    }
  }

  /* tag all verts/edges connected to faces */
  /* Any element tagged with xxx_ISGC is an edge or vert of a face that borders an edge to be
   * dissolved, and it could end up being cleaned up after a face merge has made it irrelevant. */
  BMO_ITER (e, &eiter, op->slots_in, "edges", BM_EDGE) {
    BMFace *f_pair[2];
    if (BM_edge_face_pair(e, &f_pair[0], &f_pair[1])) {
      uint j;
      for (j = 0; j < 2; j++) {
        BMLoop *l_first, *l_iter;
        l_iter = l_first = BM_FACE_FIRST_LOOP(f_pair[j]);
        do {
          BMO_vert_flag_enable(bm, l_iter->v, VERT_ISGC);
          BMO_edge_flag_enable(bm, l_iter->e, EDGE_ISGC);
        } while ((l_iter = l_iter->next) != l_first);
      }
    }
  }

  /* Merge any face pairs that straddle a selected edge. */
  BMO_ITER (e, &eiter, op->slots_in, "edges", BM_EDGE) {
    BMLoop *l_a, *l_b;
    if (BM_edge_loop_pair(e, &l_a, &l_b)) {
      BM_faces_join_pair(bm, l_a, l_b, false, nullptr);
    }
  }

  /* Cleanup geometry. Remove any edges that are garbage collectible and that have became
   * irrelevant (no loops) because of face merges. */
  BM_ITER_MESH_MUTABLE (e, e_next, &iter, bm, BM_EDGES_OF_MESH) {
    if ((e->l == nullptr) && BMO_edge_flag_test(bm, e, EDGE_ISGC)) {
      BM_edge_kill(bm, e);
    }
  }

  /* Cleanup geometry. Remove any verts that are garbage collectible and that have became
   * isolated verts (no edges) because of edge dissolves. */
  BM_ITER_MESH_MUTABLE (v, v_next, &iter, bm, BM_VERTS_OF_MESH) {
    if ((v->e == nullptr) && BMO_vert_flag_test(bm, v, VERT_ISGC)) {
      BM_vert_kill(bm, v);
    }
  }

  /* If dissolving verts, then evaluate each VERT_MARK vert. */
  if (use_verts) {
    BM_ITER_MESH_MUTABLE (v, v_next, &iter, bm, BM_VERTS_OF_MESH) {
      if (BMO_vert_flag_test(bm, v, VERT_MARK)) {
        if (BM_vert_is_edge_pair(v)) {
          BM_vert_collapse_edge(bm, v->e, v, true, true, true);
        }
      }
    }
  }
}

void bmo_dissolve_verts_exec(BMesh *bm, BMOperator *op)
{
  BMOIter oiter;
  BMIter iter;
  BMVert *v, *v_next;
  BMEdge *e, *e_next;

  const bool use_face_split = BMO_slot_bool_get(op->slots_in, "use_face_split");
  const bool use_boundary_tear = BMO_slot_bool_get(op->slots_in, "use_boundary_tear");

  BMO_ITER (v, &oiter, op->slots_in, "verts", BM_VERT) {
    BMO_vert_flag_enable(bm, v, VERT_MARK | VERT_ISGC);
  }

  if (use_face_split) {
    bm_face_split(bm, VERT_MARK, false);
  }

  if (use_boundary_tear) {
    BMO_ITER (v, &oiter, op->slots_in, "verts", BM_VERT) {
      if (!BM_vert_is_edge_pair(v)) {
        BM_ITER_ELEM (e, &iter, v, BM_EDGES_OF_VERT) {
          if (BM_edge_is_boundary(e)) {
            BMO_vert_flag_enable(bm, v, VERT_MARK_TEAR);
            break;
          }
        }
      }
    }

    bm_face_split(bm, VERT_MARK_TEAR, true);
  }

  BMO_ITER (v, &oiter, op->slots_in, "verts", BM_VERT) {
    BMIter itersub;
    BMLoop *l_first;
    BMEdge *e_first = nullptr;
    BM_ITER_ELEM (l_first, &itersub, v, BM_LOOPS_OF_VERT) {
      BMLoop *l_iter;
      l_iter = l_first;
      do {
        BMO_vert_flag_enable(bm, l_iter->v, VERT_ISGC);
        BMO_edge_flag_enable(bm, l_iter->e, EDGE_ISGC);
      } while ((l_iter = l_iter->next) != l_first);

      e_first = l_first->e;
    }

    /* important e_first won't be deleted */
    if (e_first) {
      e = e_first;
      do {
        e_next = BM_DISK_EDGE_NEXT(e, v);
        if (BM_edge_is_wire(e)) {
          BM_edge_kill(bm, e);
        }
      } while ((e = e_next) != e_first);
    }
  }

  BMO_ITER (v, &oiter, op->slots_in, "verts", BM_VERT) {
    /* tag here so we avoid feedback loop (checking topology as we edit) */
    if (BM_vert_is_edge_pair(v)) {
      BMO_vert_flag_enable(bm, v, VERT_MARK_PAIR);
    }
  }

  BMO_ITER (v, &oiter, op->slots_in, "verts", BM_VERT) {
    BMIter itersub;

    /* Merge across every edge that touches `v`. This does a `BM_faces_join_pair()` for each edge.
     * There may be a possible performance improvement available here, for high valence verts.
     * Collecting a list of 20 faces and performing a single `BM_faces_join` would almost certainly
     * more performant than doing 19 separate `BM_faces_join_pair()` of 2 faces each in sequence.
     * Low valence verts would need benchmarking, to check that such a change isn't harmful. */
    if (!BMO_vert_flag_test(bm, v, VERT_MARK_PAIR)) {
      BM_ITER_ELEM (e, &itersub, v, BM_EDGES_OF_VERT) {
        BMLoop *l_a, *l_b;
        if (BM_edge_loop_pair(e, &l_a, &l_b)) {
          BM_faces_join_pair(bm, l_a, l_b, false, nullptr);
        }
      }
    }
  }

  /* Cleanup geometry (#BM_faces_join_pair, but it removes geometry we're looping on)
   * so do this in a separate pass instead. */
  BM_ITER_MESH_MUTABLE (e, e_next, &iter, bm, BM_EDGES_OF_MESH) {
    if ((e->l == nullptr) && BMO_edge_flag_test(bm, e, EDGE_ISGC)) {
      BM_edge_kill(bm, e);
    }
  }

  /* final cleanup */
  BMO_ITER (v, &oiter, op->slots_in, "verts", BM_VERT) {
    if (BM_vert_is_edge_pair(v)) {
      BM_vert_collapse_edge(bm, v->e, v, false, true, true);
    }
  }

  BM_ITER_MESH_MUTABLE (v, v_next, &iter, bm, BM_VERTS_OF_MESH) {
    if ((v->e == nullptr) && BMO_vert_flag_test(bm, v, VERT_ISGC)) {
      BM_vert_kill(bm, v);
    }
  }
  /* done with cleanup */
}

void bmo_dissolve_limit_exec(BMesh *bm, BMOperator *op)
{
  BMOpSlot *einput = BMO_slot_get(op->slots_in, "edges");
  BMOpSlot *vinput = BMO_slot_get(op->slots_in, "verts");
  const float angle_max = M_PI_2;
  const float angle_limit = min_ff(angle_max, BMO_slot_float_get(op->slots_in, "angle_limit"));
  const bool do_dissolve_boundaries = BMO_slot_bool_get(op->slots_in, "use_dissolve_boundaries");
  const BMO_Delimit delimit = BMO_Delimit(BMO_slot_int_get(op->slots_in, "delimit"));

  BM_mesh_decimate_dissolve_ex(bm,
                               angle_limit,
                               do_dissolve_boundaries,
                               delimit,
                               (BMVert **)BMO_SLOT_AS_BUFFER(vinput),
                               vinput->len,
                               (BMEdge **)BMO_SLOT_AS_BUFFER(einput),
                               einput->len,
                               FACE_NEW);

  BMO_slot_buffer_from_enabled_flag(bm, op, op->slots_out, "region.out", BM_FACE, FACE_NEW);
}

#define EDGE_MARK 1
#define EDGE_COLLAPSE 2

static void bm_mesh_edge_collapse_flagged(BMesh *bm, const int flag, const short oflag)
{
  BMO_op_callf(bm, flag, "collapse edges=%fe uvs=%b", oflag, true);
}

void bmo_dissolve_degenerate_exec(BMesh *bm, BMOperator *op)
{
  const float dist = BMO_slot_float_get(op->slots_in, "dist");
  const float dist_sq = dist * dist;

  bool found;
  BMIter eiter;
  BMEdge *e;

  BMO_slot_buffer_flag_enable(bm, op->slots_in, "edges", BM_EDGE, EDGE_MARK);

  /* collapse zero length edges, this accounts for zero area faces too */
  found = false;
  BM_ITER_MESH (e, &eiter, bm, BM_EDGES_OF_MESH) {
    if (BMO_edge_flag_test(bm, e, EDGE_MARK)) {
      if (BM_edge_calc_length_squared(e) < dist_sq) {
        BMO_edge_flag_enable(bm, e, EDGE_COLLAPSE);
        found = true;
      }
    }

    /* clear all loop tags (checked later) */
    if (e->l) {
      BMLoop *l_iter, *l_first;
      l_iter = l_first = e->l;
      do {
        BM_elem_flag_disable(l_iter, BM_ELEM_TAG);
      } while ((l_iter = l_iter->radial_next) != l_first);
    }
  }

  if (found) {
    bm_mesh_edge_collapse_flagged(bm, op->flag, EDGE_COLLAPSE);
  }

  /* clip degenerate ears from the face */
  found = false;
  BM_ITER_MESH (e, &eiter, bm, BM_EDGES_OF_MESH) {
    if (e->l && BMO_edge_flag_test(bm, e, EDGE_MARK)) {
      BMLoop *l_iter, *l_first;
      l_iter = l_first = e->l;
      do {
        if (
            /* check the loop hasn't already been tested (and flag not to test again) */
            !BM_elem_flag_test(l_iter, BM_ELEM_TAG) &&
            ((void)BM_elem_flag_enable(l_iter, BM_ELEM_TAG),

             /* check we're marked to tested (radial edge already tested) */
             BMO_edge_flag_test(bm, l_iter->prev->e, EDGE_MARK) &&

                 /* check edges are not already going to be collapsed */
                 !BMO_edge_flag_test(bm, l_iter->e, EDGE_COLLAPSE) &&
                 !BMO_edge_flag_test(bm, l_iter->prev->e, EDGE_COLLAPSE)))
        {
          /* test if the faces loop (ear) is degenerate */
          float dir_prev[3], len_prev;
          float dir_next[3], len_next;

          sub_v3_v3v3(dir_prev, l_iter->prev->v->co, l_iter->v->co);
          sub_v3_v3v3(dir_next, l_iter->next->v->co, l_iter->v->co);

          len_prev = normalize_v3(dir_prev);
          len_next = normalize_v3(dir_next);

          if ((len_v3v3(dir_prev, dir_next) * min_ff(len_prev, len_next)) <= dist) {
            bool reset = false;

            if (fabsf(len_prev - len_next) <= dist) {
              /* both edges the same length */
              if (l_iter->f->len == 3) {
                /* ideally this would have been discovered with short edge test above */
                BMO_edge_flag_enable(bm, l_iter->next->e, EDGE_COLLAPSE);
                found = true;
              }
              else {
                /* add a joining edge and tag for removal */
                BMLoop *l_split;
                if (BM_face_split(
                        bm, l_iter->f, l_iter->prev, l_iter->next, &l_split, nullptr, true))
                {
                  BMO_edge_flag_enable(bm, l_split->e, EDGE_COLLAPSE);
                  found = true;
                  reset = true;
                }
              }
            }
            else if (len_prev < len_next) {
              /* split 'l_iter->e', then join the vert with next */
              BMVert *v_new;
              BMEdge *e_new;
              BMLoop *l_split;
              v_new = BM_edge_split(bm, l_iter->e, l_iter->v, &e_new, len_prev / len_next);
              BLI_assert(v_new == l_iter->next->v);
              (void)v_new;
              if (BM_face_split(
                      bm, l_iter->f, l_iter->prev, l_iter->next, &l_split, nullptr, true))
              {
                BMO_edge_flag_enable(bm, l_split->e, EDGE_COLLAPSE);
                found = true;
              }
              reset = true;
            }
            else if (len_next < len_prev) {
              /* split 'l_iter->prev->e', then join the vert with next */
              BMVert *v_new;
              BMEdge *e_new;
              BMLoop *l_split;
              v_new = BM_edge_split(bm, l_iter->prev->e, l_iter->v, &e_new, len_next / len_prev);
              BLI_assert(v_new == l_iter->prev->v);
              (void)v_new;
              if (BM_face_split(
                      bm, l_iter->f, l_iter->prev, l_iter->next, &l_split, nullptr, true))
              {
                BMO_edge_flag_enable(bm, l_split->e, EDGE_COLLAPSE);
                found = true;
              }
              reset = true;
            }

            if (reset) {
              /* we can't easily track where we are on the radial edge, reset! */
              l_first = l_iter;
            }
          }
        }
      } while ((l_iter = l_iter->radial_next) != l_first);
    }
  }

  if (found) {
    bm_mesh_edge_collapse_flagged(bm, op->flag, EDGE_COLLAPSE);
  }
}
