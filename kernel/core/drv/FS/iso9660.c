/*
MIT license
Copyright (c) 2019-2022 Maarten Vermeulen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "iso9660.h"

#include "fs_exitcode.h"

#include "../FS_TYPES.H"

#include "../COMMANDS.H"
#include "../FS_commands.h"

#include "../../include/types.h"
#include "../../include/exit_code.h"

#include "../../dbg/dbg.h"

#include "../../hardware/driver.h"

#include "../../screen/screen_basic.h"

#include "../../memory/memory.h"
#include "../../memory/paging.h"

#include "../../exec/task.h"

#include "../../dsk/diskio.h"
#include "../../dsk/diskdefines.h"

#include "../../util/util.h"

// =========================================

#define ISO_SECTOR_SIZE			2048 // bytes
#define CD_INFO_SIZE            512  // bytes
#define CD_INFO_ENTRIES			CD_INFO_SIZE / sizeof(cd_info_t)
#define VOL_IDENT_SIZE          32   // bytes

#define FIRST_DESCRIPTOR_LBA	0x10

// Descriptor types
#define VD_TYPE_PRIMARY         0x01
#define VD_TYPE_TERMINATOR      0xFF // I'll be back

// offsets within the Primary Volume Descriptor
#define PVD_VOL_IDENT           40
#define PVD_VOL_SIZE            80
#define PVD_BLOCK_SIZE          128
#define PVD_PATHTABLE_SIZE      132
#define PVD_PATHTABLE_LBA       140
#define PVD_ROOTDIR_ENTRY       156

// supported file flags
#define FF_HIDDEN               1 << 0
#define FF_DIRECTORY            1 << 1
#define FF_NOT_FINAL_DIR        1 << 7 // TODO: support does not exist yet

// date and time defines
#define MINUTE_OFFSET			8
#define HOUR_OFFSET				16

#define MONTH_OFFSET			8
#define YEAR_OFFSET				16

// for functions
#define RESET					1
#define NO_RESET				0

// path table stuff
#define MINIMUM_NEXT_LBA_PTABLE(a)		((sizeof(pathtable_t) * a) / ISO_SECTOR_SIZE)		// used to compute the minimum lba that an entry may be at

typedef struct 
{
    uint8_t ident_len;  // dir name len
    uint8_t ext_AR_len; // extended attribute record length
    uint32_t lba;
    uint16_t parent;

    // [directory name here (variable length)]
} __attribute__((packed)) pathtable_t;

typedef struct
{
    uint8_t DR_len; 		// length of directory record
    uint8_t EAR_len; 		// extended attribute record length
    uint32_t lba_extend;	// file loc
    uint32_t lba_extendMSB; // unused
    uint32_t size;          // extend size (file size)
    uint32_t sizeMSB;       // same as above, not used
    uint8_t datetime[7];    // date and time thing
    uint8_t file_flags;
    uint8_t interleavedsize; // ????
    uint8_t interleavedgap;
    uint16_t volseqnr;      // volume sequence number
    uint16_t volseqnrMSB;   // unused
    uint8_t ident_len;      // length of file name	
    // [directory name here (variable length)]
} __attribute__((packed)) direntry_t;

typedef struct
{
	uint8_t drive;
	uint32_t path_table_lba;
	uint32_t path_table_size; // in sectors
	uint32_t rootdir_lba;

	char volident[VOL_IDENT_SIZE+1]; // volume name
	uint32_t vol_size;	// in blocks of 2048 bytes
} __attribute__((packed)) cd_info_t;


/* the indentifier for drivers + information about our driver */
struct DRIVER ISO_driver_id = {(uint32_t) 0xB14D05, "VIREODRV", (FS_TYPE_ISO | DRIVER_TYPE_FS), (uint32_t) (iso_handler)};

uint32_t atapi_devices  = 0;
uint8_t n_atapi_devs    = 0;

cd_info_t *cd_info_ptr 	= NULL;

uint8_t gerror;

void iso_handler(uint32_t * drv)
{
	gerror = 0;
	drv[4] = 0;

    switch(drv[0])
    {
        case DRV_COMMAND_INIT:
            iso_init((uint8_t) drv[1]);
        break;

		case FS_COMMAND_READ:
			iso_read((const char *) drv[1], drv);
		break;

		case FS_COMMAND_GET_FILE_INFO:
			drv[2] = (uint32_t) iso_get_file_info((char *) drv[1]);
		break;

		case FS_COMMAND_GET_DIR_CONTENTS:
			drv[2] = (uint32_t) iso_get_dir_contents((char *) drv[1], drv);
		break;

		default:
            gerror = EXIT_CODE_GLOBAL_UNSUPPORTED;
        break;
    }

	drv[4] = gerror;
    
}

static cd_info_t *iso_get_cd_info_entry(uint8_t drive)
{
	for(uint8_t i = 0; i < CD_INFO_ENTRIES; ++i)
		if(cd_info_ptr[i].drive == drive)
			return &cd_info_ptr[i]; 

	VERIFY_NOT_REACHED();
	return NULL;
}

static cd_info_t *iso_find_free_cd_info_entry(void)
{
	for(uint8_t i = 0; i < CD_INFO_ENTRIES; ++i)
		if(cd_info_ptr[i].drive == 0xFF)
			return &cd_info_ptr[i]; 

	return NULL;
}

static void iso_allocate_cd_info(void)
{
	cd_info_ptr = iso_allocate_bfr(CD_INFO_SIZE);
	ASSERT(cd_info_ptr);

	memset(cd_info_ptr, CD_INFO_SIZE, 0xFF);
}

void iso_init(uint8_t drive)
{    
	// do we know this drive already?
	if(atapi_devices >> drive)
		return;

	// allocate a cd info buffer if we don't have one yet
	if(cd_info_ptr == NULL)
		iso_allocate_cd_info();

	uint8_t *buffer = (uint8_t *) iso_allocate_bfr(ISO_SECTOR_SIZE);

	if(gerror)
		// oh no! something went wrong with the allocation of memory (buffer / cd_info)
		return; 

	// search and read the primary vol. desc.
	iso_search_descriptor(drive, buffer, VD_TYPE_PRIMARY);
	ASSERT(buffer[0] == VD_TYPE_PRIMARY);

	cd_info_t *info = iso_find_free_cd_info_entry();
	
	if(!info)
	{ iso_free_bfr(buffer); gerror = EXIT_CODE_GLOBAL_GENERAL_FAIL; return; }

	info->drive = drive;

	// save all interesting data
	iso_save_pvd_data(buffer, info);

	if(n_atapi_devs < IDE_DRIVER_MAX_DRIVES)
		atapi_devices |= (1u << drive);

	// free buffer space with the right function depending on the type
	iso_free_bfr((uint32_t *) buffer);

	#ifndef NO_DEBUG_INFO
		print_value("[ISO9660 DRIVER] Vol. ident.: %s\n", (uint32_t) (info->volident));
		print_value("[ISO9660 DRIVER] Vol. size (in 2048 byte blocks): %i\n", (uint32_t) (info->vol_size));
		print_value("[ISO9660 DRIVER] Path table size (in bytes): %i\n", (uint32_t) (info->path_table_size));
		print_value("[ISO9660 DRIVER] Path tanle lba: %i\n", (uint32_t) (info->path_table_lba));
		print_value("[ISO9660 DRIVER] Rootdir lba: %i\n", (uint32_t) (info->rootdir_lba));
        print("\n");
	#endif
}

void iso_search_descriptor(uint8_t drive, uint8_t * buffer, uint8_t type)
{
	uint32_t lba = FIRST_DESCRIPTOR_LBA;

	while(1)
	{
		// read disk
		uint8_t error = read(drive, lba, 0x01, buffer);

		ASSERT(!error); // asserts when drive is out of range

		if((buffer[0] == type) || (buffer[0] == VD_TYPE_TERMINATOR))
			break;

		++lba;

		ASSERT(lba != 0xFF);	// we don't want to read the whole CD until we 'find' what we
									// are looking for
	}
}

void iso_save_pvd_data(uint8_t * pvd, void *info_ptr)
{
    // since I don't want to use 882 bytes of my precious kernel space
    // for stuff I'm probably never going to use, I'm going to use pointers 
    // and array indexes for searching all the information I need (this is a warning
    // because it may get messy) :)

	cd_info_t *info = info_ptr;
	uint32_t * dword;
	uint16_t * word;

	// save volume identifier
	memcpy((char *) &(info->volident[0]), (char *) &pvd[PVD_VOL_IDENT], VOL_IDENT_SIZE);
	info->volident[VOL_IDENT_SIZE] = '\0';

	// save size of volume (in blocks of 2048)
	dword = (uint32_t *) &pvd[PVD_VOL_SIZE];
	info->vol_size = *(dword);

	// check if sector size is equal to standard sector size (this driver does not
	// support ISO's that deviate from that sector size)
	word = (uint16_t *) &pvd[PVD_BLOCK_SIZE];
	ASSERT(*(word) == ISO_SECTOR_SIZE);

	// store path table size (in sectors) and lba
	dword = (uint32_t *) &pvd[PVD_PATHTABLE_SIZE];
	info->path_table_size = *(dword);

	dword = (uint32_t *) &pvd[PVD_PATHTABLE_LBA];
	info->path_table_lba = *(dword);

	// store root dir lba
	direntry_t *root = (direntry_t *) &pvd[PVD_ROOTDIR_ENTRY];
	info->rootdir_lba = (root->lba_extend);
}

void * iso_allocate_bfr(size_t size)
{
	uint32_t * ptr = NULL;

	if(size < ISO_SECTOR_SIZE)
		ptr = kmalloc(size);

	if(ptr)
		return ptr;

	// // no kernel memory left, so let's use a page
	// PAGE_REQ req;
	// req.attr = PAGE_REQ_ATTR_READ_WRITE | PAGE_REQ_ATTR_SUPERVISOR;
	// req.pid  = PID_KERNEL;
	// req.size = size;

	ptr = evalloc(size, PID_DRIVER);
	ASSERT(ptr);
	if(!ptr)
		gerror = EXIT_CODE_GLOBAL_OUT_OF_MEMORY;
	
	return ptr;
}

void iso_free_bfr(void *ptr)
{
	if((uint32_t *) ptr > (uint32_t *) memory_get_malloc_end())
		vfree(ptr);
	else
		kfree(ptr);
}

static uint32_t iso_search_dir_bfr(uint32_t *bfr, size_t bfr_size, const char *filename, size_t *fsize, direntry_t *out_entry)
{
	uint8_t *b = (uint8_t *) bfr;
	uint32_t len = strlen((char *) filename);
	uint32_t i = 0;

	*(fsize) = 0;
	
	while(i < bfr_size)
	{
		direntry_t *entry = (direntry_t *) &b[i];
		size_t size = (size_t) ((entry->DR_len) + ((entry->DR_len) % 2 != 0));
		char *file = ((char *)&(entry->ident_len) + sizeof(uint8_t));

		if(!size)
			{ size = ISO_SECTOR_SIZE - (i % ISO_SECTOR_SIZE); i += size; continue; }

		i += size;

		if(!size)
			continue;
		
		// A file identifier always contains ';1' at the end of a filename, which counts towards ident_len.
		// a file with no file extension (e.g. a file called 'config') will still always carry the extension seperator (i.e. the dot '.').
		// Therefore, we ignore the last two and three characters of the identifier when checking if a filename is the same length.
		if(entry->ident_len == len || (entry->ident_len - 2u) == len || (entry->ident_len - 3u) == len || filename[0] == '\0')
			if(!strcmp_until(filename, file, len))
			{
				memcpy(out_entry, entry, sizeof(direntry_t));
				*(fsize) = (entry->size);
				return (entry->lba_extend);
			}

		
	}

	return 0;
}

static uint32_t iso_search_dir(uint8_t drive, uint32_t dir_lba, const char *filename, size_t *fsize, direntry_t *out_entry)
{
	uint32_t flba; // file lba
	uint32_t *bfr;

	size_t size = iso_get_dir_size(drive, dir_lba);

	if(size == MAX)
		return MAX;

	size_t bfr_size = iso_alloc_dir_buffer(size, &bfr);

	if(!bfr || !bfr_size)
	{gerror = EXIT_CODE_GLOBAL_OUT_OF_MEMORY; return 0;}
	
	uint32_t nlba = size / ISO_SECTOR_SIZE + ((size % ISO_SECTOR_SIZE) != 0);
	const uint32_t to_read = (bfr_size / ISO_SECTOR_SIZE);

	while(nlba)
	{
		read(drive, dir_lba, to_read, (uint8_t *) bfr);
		flba = iso_search_dir_bfr(bfr, bfr_size, filename, fsize, out_entry);

		if(flba || !(nlba--))
			break;

		iso_free_bfr(bfr);
		dir_lba++;
	}

	iso_free_bfr(bfr);
	return flba;
}

// use this function to convert a path into the lba of the file
static uint32_t iso_traverse(const char *path, size_t *fsize, direntry_t *entry)
{
	// convert drive identifier (e.g. 'CD0') to something useful
	uint8_t drive = (uint8_t) ((drive_convert_drive_id((const char *) path)) >> DISKIO_DISK_NUMBER);

	// save file name
	char * p = create_backup_str(path);
	to_uc(p, strlen(p));

	char *filename = iso_allocate_bfr(ISO_MAX_FILENAME_LEN + 1);
	reverse_path(p, filename);

	cd_info_t *info = iso_get_cd_info_entry(drive);
	uint32_t dir_lba = (p[0] == '\0') ? (info->rootdir_lba) : iso_path_to_dir_lba(drive, p);
	iso_free_bfr(p);

	if(dir_lba == MAX)
		{ iso_free_bfr(filename); return MAX; }

	uint32_t flba = iso_search_dir(drive, dir_lba, (const char *) filename, fsize, entry);

	if(entry->file_flags & (FF_DIRECTORY) == (FF_DIRECTORY))
		*fsize = iso_get_dir_size(drive, flba);

	iso_free_bfr(filename);

	return flba;
}

size_t iso_alloc_dir_buffer(size_t dir_size, uint32_t **ret_addr)
{
	// first try and allocate the entire size of the dir, since that would make our job easier
	// otherwise just try and allocate the size of one lba.
	size_t bytes_more_than_lba_size = dir_size % ISO_SECTOR_SIZE;
	
	if(bytes_more_than_lba_size)
		dir_size += ISO_SECTOR_SIZE;

	*(ret_addr) = iso_allocate_bfr(dir_size);
	
	// succesfully allocated?
	if(*(ret_addr))
		return dir_size;

	*(ret_addr) = iso_allocate_bfr(ISO_SECTOR_SIZE);

	return *(ret_addr) ? ISO_SECTOR_SIZE : 0;
}

size_t iso_get_dir_size(uint8_t drive, uint32_t dir_lba)
{
	direntry_t *dir = (direntry_t *) evalloc(ISO_SECTOR_SIZE, PID_DRIVER);

	if(!dir)
		{ gerror = EXIT_CODE_GLOBAL_OUT_OF_MEMORY; return MAX; }

	read(drive, dir_lba, 1, (uint8_t *) dir);
		
	size_t size = (dir->size);
	iso_free_bfr((uint32_t *) dir);

	return size;
}

static uint32_t iso_read_path_table_buffer(uint8_t *buffer, size_t buffer_size, const char *filename, uint16_t *entry, uint16_t *o_parent_entry)
{
	uint32_t read_bytes = 0;
	uint16_t minimum_entry = *entry;
	*(entry) = 0;
	
	size_t filename_len = strlen(filename);
	
	while(read_bytes < buffer_size)
	{
		pathtable_t *p = (pathtable_t *) &buffer[read_bytes];
		uint16_t ident_len = (uint16_t) (p->ident_len);

		if(!(p->ident_len) && !(p->parent))
		 break;

		const char *str = (const char *) &buffer[read_bytes + sizeof(pathtable_t)];

		// compare string
		if((*(entry) >= minimum_entry) && (filename_len == ident_len) && !strcmp_until(str, filename, ident_len))
		{
			*o_parent_entry = (uint16_t) ((p->parent) - 1);
			return p->lba;
		}
		
		// add amount of bytes read: base entry size, length of the identifier and padding byte 
		read_bytes = (uint32_t) (read_bytes + sizeof(pathtable_t) + ident_len + (ident_len % 2 == 1));

		*entry = (uint16_t) (*(entry) + 1);
	}

	return MAX;
}

static uint32_t iso_find_in_path_table(uint8_t drive, const char *filename, uint16_t *o_entry, uint16_t *o_parent_entry)
{
	void *buffer = iso_allocate_bfr(PAGE_SIZE);
	
	cd_info_t *info = iso_get_cd_info_entry(drive);
	
	size_t path_table_size = info->path_table_size;
	uint32_t path_lba = info->path_table_lba;

	uint16_t minimum_entry = *o_entry;
	uint16_t entry = 0;

	uint32_t dir_lba = 0;

	while(path_table_size)
	{
		read(drive, path_lba, (PAGE_SIZE / ISO_SECTOR_SIZE), buffer);

		size_t s = (path_table_size > PAGE_SIZE) ? PAGE_SIZE : path_table_size;

		// set the minimum entry number of the next directory name in the path table
		uint16_t e = ((uint16_t) (((int32_t)minimum_entry - (int32_t)entry) > 0)) ? (uint16_t) (minimum_entry - entry) : (uint16_t) 0;
		
		dir_lba = iso_read_path_table_buffer(buffer, s, filename, &e, o_parent_entry);
		entry = (uint16_t) (entry + e);
		
		if(dir_lba != MAX)
			break;

		path_table_size = path_table_size - s;
		path_lba = path_lba + (PAGE_SIZE / ISO_SECTOR_SIZE);
	}

	iso_free_bfr(buffer);
	*o_entry = entry;
	return dir_lba;
}

static bool_t iso_check_pathtable_parents(uint8_t drive, const char *parents_path, uint16_t parent_entry)
{
	char *parent_name = iso_allocate_bfr(ISO_MAX_FILENAME_LEN + 1),
		 *next = iso_allocate_bfr(ISO_MAX_FILENAME_LEN + 1);
	
	uint16_t entry = parent_entry;
		
	if(!parent_name || !next)
	{gerror = EXIT_CODE_GLOBAL_OUT_OF_MEMORY; return false; }

	uint32_t part = 0;
	bool_t result = true;

	while(result)
	{
		if(!str_get_part(parent_name, parents_path, "/", &part))
			break;

		if(parent_name[0] == '\0' && parent_entry == 0)
			break;

		uint16_t e = entry, parent = 0;
		uint32_t lba = 0;

		lba = iso_find_in_path_table(drive, parent_name, &e, &parent);

		// check if the parent was found, and the
		// entry is equal to the the parent's entry number
		if((e != entry) || (lba == MAX))
		{result = false; break;}

		entry = parent;
	}

	iso_free_bfr(parent_name);
	iso_free_bfr(next);

	return result;
}

uint32_t iso_path_to_dir_lba(uint8_t drive, const char *path)
{
	char *filename = iso_allocate_bfr(ISO_MAX_FILENAME_LEN + 1);

	ASSERT(filename);

	if(!filename)
	{gerror = EXIT_CODE_GLOBAL_OUT_OF_MEMORY; return 0;}

	uint32_t part = 0;
	str_get_part(filename, path, "/", &part);

	uint32_t start_parent_path = find_in_str(path, "/");
	start_parent_path = (start_parent_path == MAX) ? strlen(path) : start_parent_path + 1;

	uint16_t entry = 0;
	uint32_t lba = 0;
	
	while(1)
	{
		uint16_t parent_entry = 0;
		lba = iso_find_in_path_table(drive, filename, &entry, &parent_entry);

		if(lba == MAX)
			break;
		
		if(iso_check_pathtable_parents(drive, &path[start_parent_path], parent_entry))
			break;

		// add one to the entry (iso_find_in_path_table() returns the last entry number checked or the entry number of
		// the correct directory)
		entry = (uint16_t) (entry + 1);
	}

	iso_free_bfr(filename);

	return lba;
}

static uint16_t iso_count_index(uint16_t *index_start, uint16_t until, uint8_t *buffer)
{
	uint16_t read = 0, index = *(index_start);
	
	while(read < ISO_SECTOR_SIZE)
	{
		pathtable_t *p = (pathtable_t *) &buffer[read];
		uint16_t ident_len = (uint32_t) (p->ident_len);

		if(index == until)
			break;

		// add amount of bytes read: base entry size, length of the identifier and padding byte
		read = (uint16_t) (read + sizeof(pathtable_t) + ident_len + ((ident_len % 2) == 1));
		index++;
	}

	*(index_start) = index;
	return read;
}

uint32_t *iso_find_index(uint8_t drive, uint16_t index)
{
	// there is nothing at index 0
	if(!index)
		return NULL;

	uint32_t lba = 0;
	uint16_t total = 0;
	uint16_t i = 0;

	const cd_info_t *info = iso_get_cd_info_entry(drive);
	uint8_t * b = evalloc(ISO_SECTOR_SIZE, PID_DRIVER);

	if(!b)
		return NULL;

	// do while the current sector does not equal the max sectors of the path table
	while(lba < (info->path_table_size)) // was: (lba * ISO_SECTOR_SIZE) < (info->path_table_size)
	{
		read(drive, (info->path_table_lba) + lba, 1, b);
		uint16_t read = iso_count_index(&i, index, b);

		// if we are at the end of the current buffer, update the total
		// reset i and increase the lba
		if(read >= ISO_SECTOR_SIZE)
		{
			total = (uint16_t) (total + i); 
			i = 0; 
			lba++; 
			continue; 
		}
		
		if((total + i) != index)
		 	continue;
		
		// if we get here we found what we were looking for
		pathtable_t *t = (pathtable_t *) &b[read];
		uint32_t len = (t->ident_len) + sizeof(pathtable_t);
		uint32_t *ret = iso_allocate_bfr(len);
		
		ASSERT(ret);
		
		memcpy((char *) ret, (char *) &b[read], len);
		iso_free_bfr(b);

		return ret;
	}
	
	iso_free_bfr(b);
	return NULL;
}

// !! WARNING !!
// Destroy's original string!
void reverse_path(char *path, char *out_filename)
{
	ASSERT(out_filename);

	char *backup = create_backup_str(path);
	ASSERT(backup);

	memcpy(backup, path, strlen(path));
	
	// remove disk identifier from backup
	size_t s = find_in_str(path, "/");
	remove_from_str(backup, s + 1);

	uint32_t b_index = strlen(backup);

	// the last file in the path is the filename
	for(; b_index > 0; b_index--)
		if(backup[b_index] == '/')
			break;

	// if b_index == 0 then there was no '/', meaning only a filename exists
	// in the path	
	b_index = (b_index) ? b_index + 1 : 0;
	memcpy(out_filename, &backup[b_index], strlen(&backup[b_index]) + 1);

	if(!b_index)
	{
		path[0] = '\0';
		kfree(backup);
		return;
	}

	backup[b_index - 1] = '\0';

	// copy the rest of the path
	uint32_t p_index = 0;
	b_index = strlen(backup);
	for(; b_index > 0; b_index--)
	{
		if(backup[b_index] != '/')
			continue;

		size_t size = strlen(&backup[b_index + 1]);
		memcpy(&path[p_index], &backup[b_index + 1], size);
		backup[b_index] = '\0';

		p_index += size;
		path[p_index] = '/';
		p_index++;
	}

	s = strlen(&backup[b_index]);
	memcpy(&path[p_index], &backup[b_index], s);
	path[s + p_index] = '\0';

	kfree(backup);
}

void iso_read(const char * path, uint32_t *drv)
{
	size_t fsize = 0;
    uint32_t flba = iso_traverse(path, &fsize, NULL);

	if(!flba || flba == MAX)
	{ 
		gerror = EXIT_CODE_FS_FILE_NOT_FOUND; 
		drv[2] = NULL; 
		drv[3] = 0; 
		return; 
	}
	
	uint32_t nlba = (fsize / ISO_SECTOR_SIZE) + ((fsize % ISO_SECTOR_SIZE) != 0);
	uint8_t drive = (uint8_t) (drive_convert_drive_id((const char *) path) >> DISKIO_DISK_NUMBER);

	uint8_t *bfr = evalloc(fsize, PID_DRIVER);

	if(!bfr)
	{
		gerror = EXIT_CODE_GLOBAL_OUT_OF_MEMORY; 
		drv[2] = NULL; 
		drv[3] = 0; 
		return; 
	}

	if(nlba)
		read(drive, flba, nlba, bfr);
	else
		{ vfree(bfr); bfr = NULL; gerror = EXIT_CODE_FS_FILE_NOT_FOUND; }

	drv[2] = (uint32_t) bfr;
	drv[3] = fsize;
}

static uint8_t iso_convert_fileflags_to_fat_filetype(uint8_t file_flags)
{
	uint8_t fat_type = 0;

	if(file_flags & (FF_HIDDEN))
		fat_type |= FAT_FILE_ATTRIB_HIDDEN;
	
	if(file_flags & (FF_DIRECTORY))
		fat_type |= FAT_FILE_ATTRIB_DIR;
	
	// because Vireo doesn't support writing to ATAPI anyway:
	fat_type |= FAT_FILE_ATTRIB_READONLY;

	return file_flags;
}

static uint16_t iso_convert_date_to_fat_date(uint32_t date)
{
	uint8_t day   = (uint8_t) (((date & 0xFF)) & 0x1F);
	uint8_t month = (uint8_t) ((date >> MONTH_OFFSET) & 0xF);

	// ISO9660 years are counted since 1900, while FAT years
	// are counted since 1980. Therefore, the first 80 years must be subtracted
	// from the value
	uint8_t year  = (uint8_t) (((date >> YEAR_OFFSET) - 80) & 0xFF);

	return (uint16_t) (day | (month << FAT_MONTH_OFFSET) | (year << FAT_YEAR_OFFSET));
}

static uint16_t iso_convert_time_to_fat_time(uint32_t time)
{
	uint8_t sec = (uint8_t) (((time & 0xFF) / 2) & 0x1F);
	uint8_t min = (uint8_t) ((time >> MINUTE_OFFSET) & 0x3F);
	uint8_t h   = (uint8_t) ((time >> HOUR_OFFSET) & 0x1F);

	return (uint16_t) (sec | (min << FAT_MINUTE_OFFSET) | (h << FAT_HOUR_OFFSET));
}

fs_file_info_t *iso_get_file_info(const char *path)
{
	size_t fsize;
	direntry_t entry;

	uint32_t flba = iso_traverse(path, &fsize, &entry);

	if(!flba)
	{ gerror = EXIT_CODE_FS_FILE_NOT_FOUND; return NULL; }

	fs_file_info_t *info = evalloc(sizeof(fs_file_info_t), PID_DRIVER);
	
	if(!info)
	{ gerror = EXIT_CODE_GLOBAL_OUT_OF_MEMORY; return NULL; }
	
	info->file_size = fsize;
	info->first_sector = flba;
	info->first_cluster = MAX; // not applicable

	info->file_type = iso_convert_fileflags_to_fat_filetype(entry.file_flags);

	uint32_t date = (uint32_t) (entry.datetime[2] | (entry.datetime[1] << 8) | (entry.datetime[0] << 16));
	info->creation_date = iso_convert_date_to_fat_date(date);
	
	uint32_t time = (uint32_t) (entry.datetime[5] | (entry.datetime[4] << 8) | (entry.datetime[3] << 16));
	info->creation_time = iso_convert_time_to_fat_time(time);

	return info;
}

static void iso_fill_dircontent_entry(char *filename, size_t fname_len, uint8_t file_flags, size_t size, fs_dir_contents_t *entry)
{
	uint32_t x = find_in_str(filename, ";");

	if(filename[0] == '\0' || filename[0] == ' ')
		{fname_len = sizeof("."); memcpy(entry->name, ".", fname_len);}
	else if(filename[0] == '\1')
		{ fname_len = sizeof(".."); memcpy(entry->name, "..", fname_len); }
	else
		memcpy(entry->name, filename, fname_len);

	if(x != MAX)
		fname_len = x;
	
	entry->name[fname_len] = '\0';
	entry->attrib = iso_convert_fileflags_to_fat_filetype(file_flags);
	entry->file_size = size;
}

fs_dir_contents_t *iso_get_dir_contents(const char *path, uint32_t *drv)
{
	iso_read(path, drv);

	ASSERT(!gerror);
	ASSERT(drv[2]);
	ASSERT(drv[3]);

	if(!drv[3] || !drv[2] || gerror)
		return NULL;
	
	uint8_t *dir = (uint8_t *) drv[2];
	size_t fsize = drv[3];
	drv[2] = drv[3] = 0;

	// allocate enough memory for the directory information. However, I don't like this, since this way will always allocate a buffer
	// way bigger than necessary. At some point, this may be improved by
	// first counting how many directory entries there actually are.
	fs_dir_contents_t *c = evalloc(fsize / sizeof(direntry_t) * sizeof(fs_dir_contents_t), PID_DRIVER);
	ASSERT(c);

	if(!c)
		{ gerror = EXIT_CODE_GLOBAL_OUT_OF_MEMORY; return NULL; }
	
	uint32_t i = 0, outi = 0;
	while(i < fsize)
	{
		direntry_t *entry = (direntry_t *) &dir[i];
		size_t size = (size_t) ((entry->DR_len) + ((entry->DR_len) % 2 != 0));
		char *file = ((char *)&(entry->ident_len) + sizeof(uint8_t));

		if(!size)
			{ size = ISO_SECTOR_SIZE - (i % ISO_SECTOR_SIZE); i += size; continue; }

		i += size;
		
		iso_fill_dircontent_entry(file, entry->ident_len, entry->file_flags, entry->size, &c[outi++]);
	}

	vfree(dir);

	drv[3] = outi * sizeof(fs_dir_contents_t);
	ASSERT(drv[3]);

	return c;
}
