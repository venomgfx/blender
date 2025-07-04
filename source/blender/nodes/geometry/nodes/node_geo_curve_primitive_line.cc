/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "BKE_curves.hh"

#include "NOD_rna_define.hh"

#include "UI_interface_layout.hh"
#include "UI_resources.hh"

#include "node_geometry_util.hh"

namespace blender::nodes::node_geo_curve_primitive_line_cc {

NODE_STORAGE_FUNCS(NodeGeometryCurvePrimitiveLine)

static void node_declare(NodeDeclarationBuilder &b)
{
  auto enable_direction = [](bNode &node) {
    node_storage(node).mode = GEO_NODE_CURVE_PRIMITIVE_LINE_MODE_DIRECTION;
  };

  b.add_input<decl::Vector>("Start")
      .subtype(PROP_TRANSLATION)
      .description("Position of the first control point");
  auto &end = b.add_input<decl::Vector>("End")
                  .default_value({0.0f, 0.0f, 1.0f})
                  .subtype(PROP_TRANSLATION)
                  .description("Position of the second control point")
                  .make_available([](bNode &node) {
                    node_storage(node).mode = GEO_NODE_CURVE_PRIMITIVE_LINE_MODE_POINTS;
                  });
  auto &direction =
      b.add_input<decl::Vector>("Direction")
          .default_value({0.0f, 0.0f, 1.0f})
          .description("Direction the line is going in. The length of this vector does not matter")
          .make_available(enable_direction);
  auto &length = b.add_input<decl::Float>("Length")
                     .default_value(1.0f)
                     .subtype(PROP_DISTANCE)
                     .description("Distance between the two points")
                     .make_available(enable_direction);
  b.add_output<decl::Geometry>("Curve");

  const bNode *node = b.node_or_null();
  if (node != nullptr) {
    const NodeGeometryCurvePrimitiveLine &storage = node_storage(*node);
    const GeometryNodeCurvePrimitiveLineMode mode = GeometryNodeCurvePrimitiveLineMode(
        storage.mode);

    end.available(mode == GEO_NODE_CURVE_PRIMITIVE_LINE_MODE_POINTS);
    direction.available(mode == GEO_NODE_CURVE_PRIMITIVE_LINE_MODE_DIRECTION);
    length.available(mode == GEO_NODE_CURVE_PRIMITIVE_LINE_MODE_DIRECTION);
  }
}

static void node_layout(uiLayout *layout, bContext * /*C*/, PointerRNA *ptr)
{
  layout->prop(ptr, "mode", UI_ITEM_R_EXPAND, std::nullopt, ICON_NONE);
}

static void node_init(bNodeTree * /*tree*/, bNode *node)
{
  NodeGeometryCurvePrimitiveLine *data = MEM_callocN<NodeGeometryCurvePrimitiveLine>(__func__);

  data->mode = GEO_NODE_CURVE_PRIMITIVE_LINE_MODE_POINTS;
  node->storage = data;
}

static Curves *create_point_line_curve(const float3 start, const float3 end)
{
  Curves *curves_id = bke::curves_new_nomain_single(2, CURVE_TYPE_POLY);
  bke::CurvesGeometry &curves = curves_id->geometry.wrap();

  curves.positions_for_write().first() = start;
  curves.positions_for_write().last() = end;

  return curves_id;
}

static Curves *create_direction_line_curve(const float3 start,
                                           const float3 direction,
                                           const float length)
{
  Curves *curves_id = bke::curves_new_nomain_single(2, CURVE_TYPE_POLY);
  bke::CurvesGeometry &curves = curves_id->geometry.wrap();

  curves.positions_for_write().first() = start;
  curves.positions_for_write().last() = math::normalize(direction) * length + start;

  return curves_id;
}

static void node_geo_exec(GeoNodeExecParams params)
{
  const NodeGeometryCurvePrimitiveLine &storage = node_storage(params.node());
  const GeometryNodeCurvePrimitiveLineMode mode = (GeometryNodeCurvePrimitiveLineMode)storage.mode;

  Curves *curves = nullptr;
  if (mode == GEO_NODE_CURVE_PRIMITIVE_LINE_MODE_POINTS) {
    curves = create_point_line_curve(params.extract_input<float3>("Start"),
                                     params.extract_input<float3>("End"));
  }
  else if (mode == GEO_NODE_CURVE_PRIMITIVE_LINE_MODE_DIRECTION) {
    curves = create_direction_line_curve(params.extract_input<float3>("Start"),
                                         params.extract_input<float3>("Direction"),
                                         params.extract_input<float>("Length"));
  }

  params.set_output("Curve", GeometrySet::from_curves(curves));
}

static void node_rna(StructRNA *srna)
{
  static const EnumPropertyItem mode_items[] = {
      {GEO_NODE_CURVE_PRIMITIVE_LINE_MODE_POINTS,
       "POINTS",
       ICON_NONE,
       "Points",
       "Define the start and end points of the line"},
      {GEO_NODE_CURVE_PRIMITIVE_LINE_MODE_DIRECTION,
       "DIRECTION",
       ICON_NONE,
       "Direction",
       "Define a line with a start point, direction and length"},
      {0, nullptr, 0, nullptr, nullptr},
  };

  RNA_def_node_enum(srna,
                    "mode",
                    "Mode",
                    "Method used to determine radius and placement",
                    mode_items,
                    NOD_storage_enum_accessors(mode),
                    GEO_NODE_CURVE_PRIMITIVE_LINE_MODE_POINTS);
}

static void node_register()
{
  static blender::bke::bNodeType ntype;
  geo_node_type_base(&ntype, "GeometryNodeCurvePrimitiveLine", GEO_NODE_CURVE_PRIMITIVE_LINE);
  ntype.ui_name = "Curve Line";
  ntype.ui_description = "Generate a poly spline line with two points";
  ntype.enum_name_legacy = "CURVE_PRIMITIVE_LINE";
  ntype.nclass = NODE_CLASS_GEOMETRY;
  ntype.initfunc = node_init;
  blender::bke::node_type_storage(ntype,
                                  "NodeGeometryCurvePrimitiveLine",
                                  node_free_standard_storage,
                                  node_copy_standard_storage);
  ntype.declare = node_declare;
  ntype.geometry_node_execute = node_geo_exec;
  ntype.draw_buttons = node_layout;
  blender::bke::node_register_type(ntype);

  node_rna(ntype.rna_ext.srna);
}
NOD_REGISTER_NODE(node_register)

}  // namespace blender::nodes::node_geo_curve_primitive_line_cc
