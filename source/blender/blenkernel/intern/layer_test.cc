/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2020 by Blender Foundation.
 */
#include "testing/testing.h"

#include "MEM_guardedalloc.h"

#include "BKE_idtype.h"
#include "BKE_layer.h"

#include "BLI_string.h"

#include "RE_engine.h"

#include "IMB_imbuf.h"

namespace blender::bke::tests {

TEST(view_layer, aov_name_conflicts)
{
  /* Set Up */
  IMB_init();
  RE_engines_init();

  Scene scene = {{NULL}};
  IDType_ID_SCE.init_data(&scene.id);
  ViewLayer *view_layer = static_cast<ViewLayer *>(scene.view_layers.first);

  RenderEngineType *engine_type = RE_engines_find(scene.r.engine);
  RenderEngine *engine = RE_engine_create(engine_type);

  EXPECT_FALSE(BKE_view_layer_has_valid_aov(view_layer));
  EXPECT_EQ(view_layer->active_aov, nullptr);

  /* Add an AOV */
  ViewLayerAOV *aov1 = BKE_view_layer_add_aov(view_layer);
  BKE_view_layer_verify_aov(engine, &scene, view_layer);
  EXPECT_EQ(view_layer->active_aov, aov1);
  EXPECT_TRUE(BKE_view_layer_has_valid_aov(view_layer));
  EXPECT_FALSE((aov1->flag & AOV_CONFLICT) != 0);

  /* Add a second AOV */
  ViewLayerAOV *aov2 = BKE_view_layer_add_aov(view_layer);
  BKE_view_layer_verify_aov(engine, &scene, view_layer);
  EXPECT_EQ(view_layer->active_aov, aov2);
  EXPECT_FALSE(BKE_view_layer_has_valid_aov(view_layer));
  EXPECT_TRUE((aov1->flag & AOV_CONFLICT) != 0);
  EXPECT_TRUE((aov2->flag & AOV_CONFLICT) != 0);

  /* Resolve by renaming an AOV */
  BLI_strncpy(aov1->name, "AOV_1", MAX_NAME);
  BKE_view_layer_verify_aov(engine, &scene, view_layer);
  EXPECT_TRUE(BKE_view_layer_has_valid_aov(view_layer));
  EXPECT_FALSE((aov1->flag & AOV_CONFLICT) != 0);
  EXPECT_FALSE((aov2->flag & AOV_CONFLICT) != 0);

  /* Revert previous resolution */
  BLI_strncpy(aov1->name, "AOV", MAX_NAME);
  BKE_view_layer_verify_aov(engine, &scene, view_layer);
  EXPECT_FALSE(BKE_view_layer_has_valid_aov(view_layer));
  EXPECT_TRUE((aov1->flag & AOV_CONFLICT) != 0);
  EXPECT_TRUE((aov2->flag & AOV_CONFLICT) != 0);

  /* Resolve by removing AOV resolution */
  BKE_view_layer_remove_aov(view_layer, aov2);
  aov2 = NULL;
  BKE_view_layer_verify_aov(engine, &scene, view_layer);
  EXPECT_TRUE(BKE_view_layer_has_valid_aov(view_layer));
  EXPECT_FALSE((aov1->flag & AOV_CONFLICT) != 0);

  /* Tear down */
  RE_engine_free(engine);
  RE_engines_exit();
  IDType_ID_SCE.free_data(&scene.id);
  IMB_exit();
}

}  // namespace blender::bke::tests
