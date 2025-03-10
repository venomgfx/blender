/* SPDX-FileCopyrightText: 2001-2002 NaN Holding BV. All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 */

/**
 * Copyright (C) 2001 NaN Technologies B.V.
 */

#include "GHOST_DisplayManager.hh"
#include "GHOST_Debug.hh"

GHOST_DisplayManager::GHOST_DisplayManager() : m_settingsInitialized(false) {}

GHOST_DisplayManager::~GHOST_DisplayManager() = default;

GHOST_TSuccess GHOST_DisplayManager::initialize()
{
  GHOST_TSuccess success;
  if (!m_settingsInitialized) {
    success = initializeSettings();
    m_settingsInitialized = true;
  }
  else {
    success = GHOST_kSuccess;
  }
  return success;
}

GHOST_TSuccess GHOST_DisplayManager::getNumDisplays(uint8_t & /*numDisplays*/) const
{
  /* Don't know if we have a display. */
  return GHOST_kFailure;
}

GHOST_TSuccess GHOST_DisplayManager::getNumDisplaySettings(uint8_t display,
                                                           int32_t &numSettings) const
{
  GHOST_TSuccess success;

  GHOST_ASSERT(m_settingsInitialized,
               "GHOST_DisplayManager::getNumDisplaySettings(): m_settingsInitialized=false");
  uint8_t numDisplays;
  success = getNumDisplays(numDisplays);
  if (success == GHOST_kSuccess) {
    if (display < numDisplays) {
      numSettings = m_settings[display].size();
    }
    else {
      success = GHOST_kFailure;
    }
  }
  return success;
}

GHOST_TSuccess GHOST_DisplayManager::getDisplaySetting(uint8_t display,
                                                       int32_t index,
                                                       GHOST_DisplaySetting &setting) const
{
  GHOST_TSuccess success;

  GHOST_ASSERT(m_settingsInitialized,
               "GHOST_DisplayManager::getNumDisplaySettings(): m_settingsInitialized=false");
  uint8_t numDisplays;
  success = getNumDisplays(numDisplays);
  if (success == GHOST_kSuccess) {
    if (display < numDisplays && (uint8_t(index) < m_settings[display].size())) {
      setting = m_settings[display][index];
    }
    else {
      success = GHOST_kFailure;
    }
  }
  return success;
}

GHOST_TSuccess GHOST_DisplayManager::getCurrentDisplaySetting(
    uint8_t /*display*/, GHOST_DisplaySetting & /*setting*/) const
{
  return GHOST_kFailure;
}

GHOST_TSuccess GHOST_DisplayManager::setCurrentDisplaySetting(
    uint8_t /*display*/, const GHOST_DisplaySetting & /*setting*/)
{
  return GHOST_kFailure;
}

GHOST_TSuccess GHOST_DisplayManager::findMatch(uint8_t display,
                                               const GHOST_DisplaySetting &setting,
                                               GHOST_DisplaySetting &match) const
{
  GHOST_TSuccess success = GHOST_kSuccess;
  GHOST_ASSERT(m_settingsInitialized,
               "GHOST_DisplayManager::findMatch(): m_settingsInitialized=false");

  const int criteria[4] = {
      int(setting.xPixels),
      int(setting.yPixels),
      int(setting.bpp),
      int(setting.frequency),
  };
  int capabilities[4];
  double field, score;
  double best = 1e12; /* A big number. */
  int found = 0;

  /* Look at all the display modes. */
  for (int i = 0; (i < int(m_settings[display].size())); i++) {
    /* Store the capabilities of the display device. */
    capabilities[0] = m_settings[display][i].xPixels;
    capabilities[1] = m_settings[display][i].yPixels;
    capabilities[2] = m_settings[display][i].bpp;
    capabilities[3] = m_settings[display][i].frequency;

    /* Match against all the fields of the display settings. */
    score = 0;
    for (int j = 0; j < 4; j++) {
      field = capabilities[j] - criteria[j];
      score += field * field;
    }

    if (score < best) {
      found = i;
      best = score;
    }
  }

  match = m_settings[display][found];

  GHOST_PRINT("GHOST_DisplayManager::findMatch(): settings of match:\n");
  GHOST_PRINT("  setting.xPixels=" << match.xPixels << "\n");
  GHOST_PRINT("  setting.yPixels=" << match.yPixels << "\n");
  GHOST_PRINT("  setting.bpp=" << match.bpp << "\n");
  GHOST_PRINT("  setting.frequency=" << match.frequency << "\n");

  return success;
}

GHOST_TSuccess GHOST_DisplayManager::initializeSettings()
{
  uint8_t numDisplays;
  GHOST_TSuccess success = getNumDisplays(numDisplays);
  if (success == GHOST_kSuccess) {
    for (uint8_t display = 0; (display < numDisplays) && (success == GHOST_kSuccess); display++) {
      GHOST_DisplaySettings displaySettings;
      m_settings.push_back(displaySettings);
      int32_t numSettings;
      success = getNumDisplaySettings(display, numSettings);
      if (success == GHOST_kSuccess) {
        int32_t index;
        GHOST_DisplaySetting setting;
        for (index = 0; (index < numSettings) && (success == GHOST_kSuccess); index++) {
          success = getDisplaySetting(display, index, setting);
          m_settings[display].push_back(setting);
        }
      }
      else {
        break;
      }
    }
  }
  return success;
}
