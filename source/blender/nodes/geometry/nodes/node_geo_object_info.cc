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
 */

#include "node_geometry_util.hh"

static bNodeSocketTemplate geo_node_object_info_in[] = {
    {SOCK_OBJECT, N_("Object")},
    {-1, ""},
};

static bNodeSocketTemplate geo_node_object_info_out[] = {
    {SOCK_VECTOR, N_("Location")},
    {-1, ""},
};

namespace blender::nodes {
static void geo_object_info_exec(bNode *UNUSED(node), GeoNodeInputs inputs, GeoNodeOutputs outputs)
{
  bke::PersistentObjectHandle object_handle = inputs.extract<bke::PersistentObjectHandle>(
      "Object");
  Object *object = inputs.handle_map().lookup(object_handle);

  float3 location = {0, 0, 0};

  if (object != nullptr) {
    location = object->obmat[3];
  }

  outputs.set("Location", location);
}
}  // namespace blender::nodes

void register_node_type_geo_object_info()
{
  static bNodeType ntype;

  geo_node_type_base(&ntype, GEO_NODE_OBJECT_INFO, "Object Info", 0, 0);
  node_type_socket_templates(&ntype, geo_node_object_info_in, geo_node_object_info_out);
  ntype.geometry_node_execute = blender::nodes::geo_object_info_exec;
  nodeRegisterType(&ntype);
}
