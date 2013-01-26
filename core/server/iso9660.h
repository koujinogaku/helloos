
#ifndef ISO9660_H
#define ISO9660_H

#include <stdint.h>

#pragma pack(2)
struct iso_volume_descriptor {
	uint8_t	type;
	int8_t	id[5];
	uint8_t	version;
	int8_t	data[2041];
};

/* volume descriptor types */
#define ISO_VD_TYPE_PRIMARY 1
#define ISO_VD_TYPE_END 255

#define ISO_STANDARD_ID "CD001"

struct iso_primary_descriptor {
	uint8_t type;
	int8_t id[5];
	uint8_t version;
	int8_t unused1[1];
	int8_t system_id[32];
	int8_t volume_id[32];
	int8_t unused2[8];
	uint32_t volume_space_size_le;
	uint32_t volume_space_size_be;
	int8_t unused3[32];
	uint16_t volume_set_size_le;
	uint16_t volume_set_size_be;
	uint16_t volume_sequence_number_le;
	uint16_t volume_sequence_number_be;
	uint16_t logical_block_size_le;
	uint16_t logical_block_size_be;
	uint32_t path_table_size_le;
	uint32_t path_table_size_be;
	uint32_t type_l_path_table_le;
	uint32_t opt_type_l_path_table_le;
	uint32_t type_m_path_table_be;
	uint32_t opt_type_m_path_table_be;
	int8_t root_directory_record[34];
	int8_t volume_set_id[128];
	int8_t publisher_id[128];
	int8_t preparer_id[128];
	int8_t application_id[128];
	int8_t copyright_file_id[37];
	int8_t abstract_file_id[37];
	int8_t bibliographic_file_id[37];
	int8_t creation_date[17];
	int8_t modification_date[17];
	int8_t expiration_date[17];
	int8_t effective_date[17];
	uint8_t file_structure_version;
	int8_t unused4[1];
	int8_t application_data[511];
	int8_t unused5[653];
};


/* We use this to help us look up the parent inode numbers. */

struct iso_path_table {
	uint8_t name_len;
	uint8_t ext_attr_len;
	uint32_t extent;
	uint16_t parent;
	int8_t name[0];
};

/* high sierra is identical to iso, except that the date is only 6 bytes, and
   there is an extra reserved byte after the flags */

struct iso_directory_record {
	uint8_t length;
	uint8_t ext_attr_length;
	uint32_t extent_le;
	uint32_t extent_be;
	uint32_t size_le;
	uint32_t size_be;
	int8_t date[7];
	uint8_t flags;
	uint8_t file_unit_size;
	uint8_t interleave;
	uint16_t volume_sequence_number_le;
	uint16_t volume_sequence_number_be;
	uint8_t name_len;
	int8_t name[0];
};
#pragma pack(0)


#endif
