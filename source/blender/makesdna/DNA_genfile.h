/* SPDX-FileCopyrightText: 2001-2002 NaN Holding BV. All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup DNA
 * \brief blenloader genfile private function prototypes
 */

#pragma once

#include "intern/dna_utils.h"

struct SDNA;

/**
 * DNAstr contains the prebuilt SDNA structure defining the layouts of the types
 * used by this version of Blender. It is defined in a file dna.c, which is
 * generated by the `makesdna` program during the build process (see `makesdna.cc`).
 */
extern const unsigned char DNAstr[];
/** Length of DNAstr. */
extern const int DNAlen;

/**
 * Primitive (non-struct, non-pointer/function/array) types,
 *
 * NOTE: this is endianness-sensitive.
 * \warning Don't change these values!
 * Currently changes here will work on native endianness, however before 5.0,
 * #DNA_struct_switch_endian used to check these hard-coded values against those from old files.
 */
typedef enum eSDNA_Type {
  SDNA_TYPE_CHAR = 0,
  SDNA_TYPE_UCHAR = 1,
  SDNA_TYPE_SHORT = 2,
  SDNA_TYPE_USHORT = 3,
  SDNA_TYPE_INT = 4,
  /* SDNA_TYPE_LONG     = 5, */ /* deprecated (use as int) */
  /* SDNA_TYPE_ULONG    = 6, */ /* deprecated (use as int) */
  SDNA_TYPE_FLOAT = 7,
  SDNA_TYPE_DOUBLE = 8,
/* ,SDNA_TYPE_VOID = 9 */
/* define so switch statements don't complain */
#define SDNA_TYPE_VOID 9
  SDNA_TYPE_INT64 = 10,
  SDNA_TYPE_UINT64 = 11,
  SDNA_TYPE_INT8 = 12,
  /**
   * Type used for untyped raw bytes buffers (written by #BLO_write_raw and read by
   * #BLO_read_data_address).
   *
   * Technically, it also covers all 'raw data' types above.
   */
  SDNA_TYPE_RAW_DATA = 13,
} eSDNA_Type;

/**
 * For use with #DNA_struct_reconstruct & #DNA_struct_get_compareflags
 */
enum eSDNA_StructCompare {
  /* Struct has disappeared
   * (values of this struct type will not be loaded by the current Blender) */
  SDNA_CMP_REMOVED = 0,
  /* Struct is the same (can be loaded with straight memory copy). */
  SDNA_CMP_EQUAL = 1,
  /* Struct is different in some way
   * (needs to be copied/converted field by field) */
  SDNA_CMP_NOT_EQUAL = 2,
  /* This is only used temporarily by #DNA_struct_get_compareflags. */
  SDNA_CMP_UNKNOWN = 3,
};

/**
 * Constructs and returns a decoded SDNA structure from the given encoded SDNA data block.
 */
struct SDNA *DNA_sdna_from_data(
    const void *data, int data_len, bool data_alloc, bool do_alias, const char **r_error_message);
void DNA_sdna_free(struct SDNA *sdna);

/* Access for current Blender versions SDNA. */
void DNA_sdna_current_init(void);
/* borrowed reference */
const struct SDNA *DNA_sdna_current_get(void);
void DNA_sdna_current_free(void);

struct DNA_ReconstructInfo;
/**
 * Pre-process information about how structs in \a newsdna can be reconstructed from structs in
 * \a oldsdna. This information is then used to speedup #DNA_struct_reconstruct.
 */
struct DNA_ReconstructInfo *DNA_reconstruct_info_create(const struct SDNA *oldsdna,
                                                        const struct SDNA *newsdna,
                                                        const char *compare_flags);
void DNA_reconstruct_info_free(struct DNA_ReconstructInfo *reconstruct_info);

/**
 * \param struct_index_last: Support faster lookups when there is the possibility
 * of the same name being looked up multiple times. Initialize to `UINT_MAX`.
 *
 * \return the index of the struct or -1 on failure.
 */
int DNA_struct_find_index_with_alias_ex(const struct SDNA *sdna,
                                        const char *str,
                                        unsigned int *struct_index_last);
/** \note prefer #DNA_struct_find_with_alias_ex unless there is a good reason not to. */
int DNA_struct_find_index_without_alias_ex(const struct SDNA *sdna,
                                           const char *str,
                                           unsigned int *struct_index_last);
/**
 * \return the index of the struct or -1 on failure.
 */
int DNA_struct_find_with_alias(const struct SDNA *sdna, const char *str);
/** \note prefer #DNA_struct_find_with_alias unless there is a good reason not to. */
int DNA_struct_find_index_without_alias(const struct SDNA *sdna, const char *str);

/**
 * A convenience function, the equivalent of: `DNA_struct_find_with_alias(..) != -1`
 */
bool DNA_struct_exists_with_alias(const struct SDNA *sdna, const char *str);
/** \note prefer #DNA_struct_exists_with_alias unless there is a good reason not to. */
bool DNA_struct_exists_without_alias(const struct SDNA *sdna, const char *stype);
/**
 * A convenience function, the equivalent of: `DNA_struct_member_find_with_alias(..) != -1`
 */
bool DNA_struct_member_exists_with_alias(const struct SDNA *sdna,
                                         const char *stype,
                                         const char *vartype,
                                         const char *name);
/** \note prefer #DNA_struct_exists_with_alias unless there is a good reason not to. */
bool DNA_struct_member_exists_without_alias(const struct SDNA *sdna,
                                            const char *stype,
                                            const char *vartype,
                                            const char *name);

/**
 * Constructs and returns an array of byte flags with one element for each struct in oldsdna,
 * indicating how it compares to newsdna.
 */
const char *DNA_struct_get_compareflags(const struct SDNA *oldsdna, const struct SDNA *newsdna);
/**
 * \param reconstruct_info: Information preprocessed by #DNA_reconstruct_info_create.
 * \param old_struct_index: Index of struct info within oldsdna.
 * \param blocks: The number of array elements.
 * \param old_blocks: Array of struct data.
 * \param alloc_name: String to pass to the allocation calls for reconstructed data.
 * \return An allocated reconstructed struct.
 */
void *DNA_struct_reconstruct(const struct DNA_ReconstructInfo *reconstruct_info,
                             int old_struct_index,
                             int blocks,
                             const void *old_blocks,
                             const char *alloc_name);

/**
 * A version of #DNA_struct_member_offset_by_name_with_alias that uses the non-aliased name.
 * Always prefer aliased names where possible.
 */
int DNA_struct_member_offset_by_name_without_alias(const struct SDNA *sdna,
                                                   const char *stype,
                                                   const char *vartype,
                                                   const char *name);
/**
 * Returns the offset of the field with the specified name and type within the specified
 * struct type in #SDNA, -1 on failure.
 */
int DNA_struct_member_offset_by_name_with_alias(const struct SDNA *sdna,
                                                const char *stype,
                                                const char *vartype,
                                                const char *name);

/**
 * Returns the size of struct fields of the specified type and member_index.
 *
 * \param type: Index into sdna->types/types_size
 * \param member_index: Index into sdna->names, needed to extract possible pointer/array
 * information.
 */
int DNA_struct_member_size(const struct SDNA *sdna, short type, short member_index);

/**
 * Returns the size in bytes of a primitive type.
 */
int DNA_elem_type_size(eSDNA_Type elem_nr);

/**
 * Returns the size of a struct.
 *
 * \param struct_index: Index into the #sdna.structs array (aka #BHead.SDNAnr).
 */
int DNA_struct_size(const struct SDNA *sdna, int struct_index);

/**
 * Get the alignment that should be used when allocating memory for this type.
 */
int DNA_struct_alignment(const struct SDNA *sdna, int struct_index);

/**
 * Return the current (alias) type name of the given struct index.
 */
const char *DNA_struct_identifier(struct SDNA *sdna, int struct_index);

/**
 * Find the struct matching the given `old_type_name`, and rename its type (referenced by its
 * #SDNA_Struct.type_index) to the given `new_type_name`.
 *
 * WARNING: Deprecated, do not use in new code. Only used to version some renaming done during
 * early 2.80 development.
 */
bool DNA_sdna_patch_struct_by_name(struct SDNA *sdna,
                                   const char *old_type_name,
                                   const char *new_type_name);
/**
 * Rename \a old_member_name with \a new_member_name for struct matching \a type_name.
 *
 * Handles search & replace, maintaining surrounding non-identifier characters such as pointer &
 * array size.
 *
 * WARNING: Deprecated, do not use in new code. Only used to version some renaming done during
 * early 2.80 development.
 */
bool DNA_sdna_patch_struct_member_by_name(struct SDNA *sdna,
                                          const char *type_name,
                                          const char *old_member_name,
                                          const char *new_member_name);

void DNA_sdna_alias_data_ensure(struct SDNA *sdna);

/**
 * Separated from #DNA_sdna_alias_data_ensure because it's not needed
 * unless we want to lookup aliased struct names (#DNA_struct_find_with_alias and friends).
 */
void DNA_sdna_alias_data_ensure_structs_map(struct SDNA *sdna);

/* For versioning, avoid verbosity selecting between with/without alias versions of functions. */
#ifdef DNA_GENFILE_VERSIONING_MACROS
#  define DNA_struct_exists(sdna, str) DNA_struct_exists_with_alias(sdna, str)
#  define DNA_struct_member_exists(sdna, stype, vartype, name) \
    DNA_struct_member_exists_with_alias(sdna, stype, vartype, name)
#endif
