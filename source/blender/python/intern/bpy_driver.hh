/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup pythonintern
 */

#pragma once

#include <Python.h>

/**
 * For faster execution we keep a special dictionary for py-drivers, with
 * the needed modules and aliases.
 */
[[nodiscard]] int bpy_pydriver_create_dict();
/**
 * For PyDrivers
 * (drivers using one-line Python expressions to express relationships between targets).
 */
extern PyObject *bpy_pydriver_Dict;

extern bool BPY_driver_secure_bytecode_test_ex(PyObject *expr_code,
                                               PyObject *py_namespace_array[],
                                               const bool verbose,
                                               const char *error_prefix);
extern bool BPY_driver_secure_bytecode_test(PyObject *expr_code,
                                            PyObject *py_namespace,
                                            const bool verbose);
