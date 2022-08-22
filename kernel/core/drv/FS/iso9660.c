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
#define VOL_IDENT_SIZE          32   // bytes

#define FIRST_DESCRIPTOR_LBA	0x10
#define ISO_MAX_FILENAME_LEN	255 // bytes

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

uint32_t *cd_info_ptr 	= NULL;

uint8_t gerror;

void iso_handler(uint32_t * drv)
{
	gerror = 0;

    switch(drv[0])
    {
        case DRV_COMMAND_INIT:
			// FIXME: does not support multiple CD drives...
            iso_init((uint8_t) drv[1]);
        break;

		case FS_COMMAND_READ:
			iso_read((char *) drv[1], drv);
		break;

		case FS_COMMAND_GET_FILE_INFO:
			drv[2] = (uint32_t) iso_get_file_info((char *) drv[1]);
		break;

		default:
            gerror = EXIT_CODE_GLOBAL_UNSUPPORTED;
        break;
    }

	drv[4] = gerror;
    
}

void iso_init(uint8_t drive)
{    
	// do we know this drive already?
	if(atapi_devices >> drive)
		return;

	// allocate a cd info buffer if we don't have one yet
	if(cd_info_ptr == NULL)
		cd_info_ptr = iso_allocate_bfr(CD_INFO_SIZE);

	uint8_t *buffer = (uint8_t *) iso_allocate_bfr(ISO_SECTOR_SIZE);

	if(gerror)
		// oh no! something went wrong with the allocation of memory (buffer / cd_info)
		return; 

	// search and read the primary vol. desc.
	iso_search_descriptor(drive, buffer, VD_TYPE_PRIMARY);
	dbg_assert(buffer[0] == VD_TYPE_PRIMARY);

	// save all interesting data
	iso_save_pvd_data(buffer);

	if(n_atapi_devs < IDE_DRIVER_MAX_DRIVES)
		atapi_devices |= (1u << drive);

	// free buffer space with the right function depending on the type
	iso_free_bfr((uint32_t *) buffer);

	#ifndef NO_DEBUG_INFO
		cd_info_t * info = (cd_info_t *) cd_info_ptr;
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

		dbg_assert(!error); // asserts when drive is out of range

		if((buffer[0] == type) || (buffer[0] == VD_TYPE_TERMINATOR))
			break;

		++lba;

		dbg_assert(lba != 0xFF);	// we don't want to read the whole CD until we 'find' what we
									// are looking for
	}
}

void iso_save_pvd_data(uint8_t * pvd)
{
    // since I don't want to use 882 bytes of my precious kernel space
    // for stuff I'm probably never going to use, I'm going to use pointers 
    // and array indexes for searching all the information I need (this is a warning
    // because it may get messy) :)

	cd_info_t * info = (cd_info_t *) cd_info_ptr;
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
	dbg_assert(*(word) == ISO_SECTOR_SIZE);

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
	dbg_assert(ptr);
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
		direntry_t *entry = out_entry = (direntry_t *) &b[i];
		size_t size = (size_t) ((entry->DR_len) + ((entry->DR_len) % 2 != 0));
		char *file = ((char *)&(entry->ident_len) + sizeof(uint8_t));

		i += (size) ? size : sizeof(direntry_t);

		if(!size)
			continue;
		
		if(strlen(file) >= len)
			if(!strcmp_until(filename, file, len))
			{
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
	direntry_t entry;

	size_t size = iso_get_dir_size(drive, dir_lba);
	size_t bfr_size = iso_alloc_dir_buffer(size, &bfr);

	if(!bfr || !bfr_size)
	{gerror = EXIT_CODE_GLOBAL_OUT_OF_MEMORY; return 0;}
	
	uint32_t nlba = size / ISO_SECTOR_SIZE + ((size % ISO_SECTOR_SIZE) != 0);
	const uint32_t to_read = (bfr_size / ISO_SECTOR_SIZE);

	// when the buffer is bigger than the sector size,
	// it means we have enough space to read the entire directory in one go
	// instead of multiple small ones
	if(bfr_size > ISO_SECTOR_SIZE)
		nlba = 1;

	while(nlba)
	{
		read(drive, dir_lba, to_read, (uint8_t *) bfr);
		flba = iso_search_dir_bfr(bfr, bfr_size, filename, fsize, &entry);

		if(flba || !(nlba--))
			break;

		iso_free_bfr(bfr);
		dir_lba++;
	}

	if(out_entry)
		memcpy(out_entry, &entry, sizeof(direntry_t));

	iso_free_bfr(bfr);
	return flba;
}

// use this function to convert a path into the lba of the file
static uint32_t iso_traverse(const char *path, size_t *fsize, direntry_t *entry)
{
	// convert drive identifier (e.g. 'CD0') to something useful
	uint8_t drive = (uint8_t) ((drive_convert_drive_id((const char *) path)) >> DISKIO_DISK_NUMBER);
	
	char *p = create_backup_str(path);

	// save file name (TODO: can be seperate function)
	char * a = create_backup_str(path);
	reverse_path(a);

	uint32_t flen = find_in_str(a, "/");
	dbg_assert(flen != MAX);

	char *filename = iso_allocate_bfr(flen + 1);
	memcpy(filename, a, flen);
	filename[flen] = '\0';

	// reverse path and remove everything we don't need anymore
	iso_clean_path_reverse(p);

	cd_info_t *info = (cd_info_t *) cd_info_ptr;
	uint32_t dir_lba = (info->rootdir_lba);

	// if there is only a file in the path, we do not have to search for directories
	if(find_in_str(p, ".") == MAX && find_in_str(p, "/") != MAX)
		dir_lba = iso_path_to_dir_lba(drive, p);
	
	if(dir_lba == MAX)
		dir_lba = (info->rootdir_lba);

	uint32_t flba = iso_search_dir(drive, dir_lba, (const char *) filename, fsize, entry);
	
	iso_free_bfr(p);
	iso_free_bfr(filename);

	return flba;
}

size_t iso_alloc_dir_buffer(size_t dir_size, uint32_t **ret_addr)
{
	// first try and allocate the entire size of the dir, since that would make our job easier
	// otherwise just try and allocate the size of one lba.
	dir_size += ISO_SECTOR_SIZE - (dir_size % ISO_SECTOR_SIZE);

	*(ret_addr) = iso_allocate_bfr(dir_size);
	
	// succesfully allocated?
	if(*(ret_addr))
		return dir_size;

	*(ret_addr) = iso_allocate_bfr(ISO_SECTOR_SIZE);

	return *(ret_addr) ? ISO_SECTOR_SIZE : 0;
}

size_t iso_get_dir_size(uint8_t drive, uint32_t dir_lba)
{
	direntry_t *dir = (direntry_t *) iso_read_drive(drive, dir_lba, 1);
		
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
	
	cd_info_t *info = (cd_info_t *) cd_info_ptr;
	
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

	while(str_get_part(parent_name, parents_path, "/", &part))
	{
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

	dbg_assert(filename);

	if(!filename)
	{gerror = EXIT_CODE_GLOBAL_OUT_OF_MEMORY; return 0;}

	uint32_t part = 0;
	str_get_part(filename, path, "/", &part);

	uint32_t start_parent_path = find_in_str(path, "/") + 1;

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

	const cd_info_t *info = (cd_info_t *) cd_info_ptr;
	uint8_t * b = NULL;

	// do while the current sector does not equal the max sectors of the path table
	while(lba < (info->path_table_size)) // was: (lba * ISO_SECTOR_SIZE) < (info->path_table_size)
	{
		b = (uint8_t *) iso_read_drive(drive, (info->path_table_lba) + lba, 1);
		uint16_t read = iso_count_index(&i, index, b);

		// if we are at the end of the current buffer, update the total
		// reset i and increase the lba
		if(read >= ISO_SECTOR_SIZE)
		{
			total = (uint16_t) (total + i); 
			i = 0; 
			lba++; 
			iso_free_bfr(b);
			continue; 
		}
		
		if((total + i) != index)
		{ iso_free_bfr(b); continue; }
		
		// if we get here we found what we were looking for
		pathtable_t *t = (pathtable_t *) &b[read];
		uint32_t len = (t->ident_len) + sizeof(pathtable_t);
		uint32_t *ret = iso_allocate_bfr(len);
		
		dbg_assert(ret);
		
		memcpy((char *) ret, (char *) &b[read], len);
		iso_free_bfr(b);
		return ret;
	}
	
	return NULL;
}

void iso_clean_path_reverse(char *p)
{	
	if((drive_convert_drive_id(p) >> DISKIO_DISK_NUMBER) != 0xFF)
		remove_from_str(p, strlen(DISKIO_DISKID_CD) + 2); // remove disk id, n = 4 ('CD0/')

	// check if there are slashes in the path (if not it's a file in the root dir)
	uint32_t loc = find_in_str(p, "/");
	if(loc == MAX)
		return;

	reverse_path(p);
	remove_from_str(p, find_in_str(p, "/") + 1);
}

void reverse_path(char *path)
{
	// I bet there is a better way to do this
	const size_t len = strlen(path);
	char *current, *backup = create_backup_str(path);

	char *new = kmalloc(strlen(path) + 1);
	uint32_t index = len - 1;

	current = strtok(backup, "/");

	while(current != NULL)
	{
		// copy current word to the new path var and update the index
		memcpy(&new[index-strlen(current)+1], current, strlen(current));
		index -= strlen(current);
		
		if(!index)
			break;

		// add the forward slash back (delim for strtok when traversing the path)
		new[index] = '/';

		// increase the index once more to account for the slash above
		index--;

		current = strtok(NULL, "/");
	}

	new[len] = '\0';
	memcpy(&path[0], &new[0], strlen(new) + 1);

	kfree(backup);
	kfree(new);
}

uint16_t *iso_read_drive(uint8_t drive, uint32_t lba, uint32_t sctr_read)
{
	// FIXME!!! use read directly for all functions instead of this
	// a function should allocate its own buffer
	uint16_t *buf = evalloc(ISO_SECTOR_SIZE * sctr_read, PID_DRIVER);

	// check if we got a null pointer back
	if(!buf)
	{
		gerror = EXIT_CODE_GLOBAL_OUT_OF_MEMORY;
		return NULL;
	}

	// read the drive
	read(drive, lba, sctr_read, (uint8_t *) buf);

	return buf;
}

void iso_read(char * path, uint32_t *drv)
{
	size_t fsize = 0;
    uint32_t flba = iso_traverse(path, &fsize, NULL);

	if(!flba)
	{ gerror = EXIT_CODE_FS_FILE_NOT_FOUND; return; }
	
	uint32_t nlba = (fsize / ISO_SECTOR_SIZE) + ((fsize % ISO_SECTOR_SIZE) != 0);
	uint8_t drive = (uint8_t) (drive_convert_drive_id((const char *) path) >> DISKIO_DISK_NUMBER);

	uint16_t *bfr = NULL;

	if(nlba)
		bfr = iso_read_drive(drive, flba, nlba);
	else
		gerror = EXIT_CODE_FS_FILE_NOT_FOUND;

	drv[2] = (uint32_t) bfr;
	drv[3] = fsize;
}

static uint8_t iso_convert_fileflags_to_fat_filetype(uint8_t file_flags)
{
	uint8_t fat_type = 0;

	if(file_flags & FF_HIDDEN)
		fat_type |= FAT_FILE_ATTRIB_HIDDEN;
	
	if(file_flags & FF_DIRECTORY)
		fat_type |= FAT_FILE_ATTRIB_DIR;
	
	// because Vireo doesn't support writing to ATAPI anyway:
	file_flags |= FAT_FILE_ATTRIB_READONLY;

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
