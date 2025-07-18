/* SPDX-FileCopyrightText: 2021 Tangent Animation. All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include "usd.hh"
#include "usd_reader_xform.hh"

#include <pxr/usd/usdVol/volume.h>

namespace blender::io::usd {

class USDVolumeReader : public USDXformReader {
 private:
  pxr::UsdVolVolume volume_;

 public:
  USDVolumeReader(const pxr::UsdPrim &prim,
                  const USDImportParams &import_params,
                  const ImportSettings &settings)
      : USDXformReader(prim, import_params, settings), volume_(prim)
  {
  }

  bool valid() const override
  {
    return bool(volume_);
  }

  void create_object(Main *bmain) override;
  void read_object_data(Main *bmain, pxr::UsdTimeCode time) override;
};

}  // namespace blender::io::usd
