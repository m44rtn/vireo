/*
MIT license
Copyright (c) 2019-2021 Maarten Vermeulen

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

/* This 'module' will handle drivers and such */
#include "driver.h"

#include "pci.h"

#include "../include/types.h"
#include "../include/exit_code.h"

#include "../exec/prog.h"
#include "../exec/exec.h"
#include "../exec/task.h"
#include "../exec/elf.h"

#include "../memory/memory.h"
#include "../memory/paging.h"

#include "../api/api.h"
#include "../api/syscalls.h"

#include "../dbg/dbg.h"

#include "../util/util.h"

#include "../drv/COMMANDS.H"

#include "../dsk/fs.h"
#include "../drv/FS_commands.h"

#ifndef NO_DEBUG_INFO
    #include "../screen/screen_basic.h"
#endif

#define DRIVER_STRUCT_HEXSIGN   0xB14D05
#define DRIVER_STRUCT_CHARSIGN  "VIREODRV"

#define DRIVER_INTERNAL_MAX_SUPPORTED    16

#define DRIVER_EXTERNAL_LIST_MAX_PAGES   1
#define DRIVER_EXTERNAL_LIST_SIZE        DRIVER_EXTERNAL_LIST_MAX_PAGES * PAGE_SIZE
#define DRIVER_EXTERNAL_MAX_SUPPORTED    DRIVER_EXTERNAL_LIST_SIZE / sizeof(ext_driver_list_t)

struct DRIVER_SEARCH
{
    unsigned int sign1; // signature
    char sign2[8];      // signature
    unsigned int type;
} __attribute__((packed));

typedef struct
{
    uint32_t device;
    uint32_t type;
    uint32_t *driver;
} __attribute__((packed)) int_driver_list_t;

typedef struct ext_driver_list_t
{
    uint32_t type;
    void *start;  // pointer to the starting function of the binary
    void *binary; // pointer to start of binary
    void *stack;
    char bin_name[FAT_MAX_FILENAME_LEN];
} __attribute__((packed)) ext_driver_list_t;

typedef struct driver_info_t
{
    uint32_t type;
    char bin_name[FAT_MAX_FILENAME_LEN];
} __attribute__((packed)) driver_info_t;

typedef struct driver_call_t
{
    syscall_hdr_t hdr;
    uint32_t type;
    char *path;
} __attribute__((packed)) driver_call_t;

/* with 16 drivers this list is 192 bytes */
int_driver_list_t drv_list[DRIVER_INTERNAL_MAX_SUPPORTED];
ext_driver_list_t *ext_drv_list = NULL;
uint8_t cur_devices = 0;

static void driver_search_pciAll(void);

/* An internal driver command 'packet' is an array of five uint32_t's. 
    The packet is as follows:

    0: command
    1-4: parameter1 - parameter4
    */
uint32_t driver_ext_find_free_index(void);
uint32_t driver_ext_find_index(uint32_t type);

void driver_init(void)
{
    driver_search_pciAll();
    ext_drv_list = evalloc(DRIVER_EXTERNAL_LIST_SIZE, PID_KERNEL);

#ifndef NO_DEBUG_INFO
    print( "\n");
#endif
}

// executes internal drivers
void driver_exec_int(uint32_t type, uint32_t *data)
{
    uint8_t i;

    /* the kernel only supports one type for a driver, so this means we can use it as a key (which is easier to test with since you can just look it up on google). */
    for(i = 0; i < DRIVER_INTERNAL_MAX_SUPPORTED; ++i)
        if(drv_list[i].type == type)
            break;

    if(i >= DRIVER_INTERNAL_MAX_SUPPORTED)     
        return;
    
    EXEC_CALL_FUNC(drv_list[i].driver, (uint32_t *) data);
}

static bool_t driver_check_exists(uint32_t id)
{
    for(uint8_t i = 0; i < DRIVER_INTERNAL_MAX_SUPPORTED; ++i)
        if(drv_list[i].type == id)
            return TRUE;
    
    return FALSE;
}

/* identifier: for example (FS_TYPE_FAT | DRIVER_TYPE_FS) 
 * (upper 8 bits used for driver type, lower 24 bits used for the driver to say what it supports)
 * FS_TYPE_FAT is the lower 24 bits, which tells the system that it's a FAT driver
 * DRIVER_TYPE_FS, the upper 8 bits tells the system it's a filesystem driver
 */ 
void driver_addInternalDriver(uint32_t identifier) 
{
    struct DRIVER_SEARCH drv = {DRIVER_STRUCT_HEXSIGN, "VIREODRV", 0};
    uint32_t *driver_loc;

    if(driver_check_exists(identifier))
        return;
    
    // find the associated driver
    drv.type = identifier;
    driver_loc = memsrch((void *) &drv, sizeof(struct DRIVER_SEARCH), memory_getKernelStart(), memory_getMallocStart());
    
    if(!driver_loc || (cur_devices >= DRIVER_INTERNAL_MAX_SUPPORTED))
        return;
        
    // save the information on our driver
    drv_list[cur_devices].device = 0;
    drv_list[cur_devices].type = identifier;
    drv_list[cur_devices].driver = (uint32_t *) *( (uint32_t*) ((uint32_t)driver_loc + sizeof(struct DRIVER_SEARCH)));

    ++cur_devices;
}

/* FYI: not all, only internal drivers */
static void driver_search_pciAll(void)
{
    uint8_t i;
    uint32_t info, driver_type, *driver_loc;
    struct DRIVER_SEARCH drv = {DRIVER_STRUCT_HEXSIGN, "VIREODRV", 0};
    uint32_t *devicelist = pciGetAllDevices();

    /* search for internal drivers */
    for(i = 0; i < 255; ++i)
    {
        if(devicelist[i] == 0)
            break;

        info = pciGetInfo(devicelist[i]);
        driver_type = info | DRIVER_TYPE_PCI;

        drv.type = driver_type;
        driver_loc = memsrch((void *) &drv, sizeof(struct DRIVER_SEARCH), memory_getKernelStart(), memory_getMallocStart());

        if(driver_loc)
        {
            drv_list[cur_devices].device = devicelist[i];
            drv_list[cur_devices].type = driver_type;
            /* store the address of the driver's handler */
            drv_list[cur_devices].driver = (uint32_t *) *( (uint32_t*) ((uint32_t)driver_loc + sizeof(struct DRIVER_SEARCH))); /* I'm sorry */
            
            // execute initialization routine of the driver
            uint32_t drvcmd[DRIVER_COMMAND_PACKET_LEN];
            drvcmd[0] = DRV_COMMAND_INIT;
            drvcmd[1] = (uint32_t) drv_list[cur_devices].device;

            driver_exec_int(info | DRIVER_TYPE_PCI, drvcmd);

            #ifndef NO_DEBUG_INFO
            print_value( "[DRIVER SUBSYSTEM] Found driver for device %x @ ", pciGetReg0(drv_list[cur_devices].device));
            print_value( "0x%x\n", (uint32_t) drv_list[cur_devices].driver);
            #endif

            ++cur_devices;    
        }

    }
    kfree(devicelist);
}

uint32_t driver_ext_find_free_index(void)
{
    uint32_t i = 0;
    for(; i < DRIVER_EXTERNAL_MAX_SUPPORTED; ++i)
        if(!ext_drv_list[i].type)
            break;

    return i == DRIVER_EXTERNAL_MAX_SUPPORTED ? MAX : i;
}

uint32_t driver_ext_find_index(uint32_t type)
{
    uint32_t i = 0;
    for(; i < DRIVER_EXTERNAL_MAX_SUPPORTED; ++i)
        if(ext_drv_list[i].type == type)
            break;

    return i >= DRIVER_EXTERNAL_MAX_SUPPORTED ? MAX : i;
}

err_t driver_add_external_driver(uint32_t type, char *path)
{
    size_t size = 0;
    uint32_t index = driver_ext_find_free_index();

    if(index == MAX)
        return EXIT_CODE_GLOBAL_GENERAL_FAIL;

    // TODO: move to util.c?
    uint32_t filename_index = 0;
    uint32_t  i;
    while((i = find_in_str(&path[filename_index], "/")) != MAX)
        filename_index += (i + 1);

    size_t filename_len = strlen(&path[filename_index]);
    filename_len = filename_len  > FAT_MAX_FILENAME_LEN ?
                   FAT_MAX_FILENAME_LEN : filename_len; 

    // store binary-/filename
    memcpy(&ext_drv_list[index].bin_name[0], &path[filename_index], filename_len);

    // read binary file
    file_t *f = fs_read_file(path, &size);

    if(!f)
        return EXIT_CODE_GLOBAL_GENERAL_FAIL;

    file_t *elf = f;

    err_t err = 0;
    void *rel_addr = elf_parse_binary(&elf, PID_DRIVER, &err);

    // add information to the driver list based on the type of binary (elf, flat)
    if(!err)
    {
        ext_drv_list[index].start = (void *) ((uint32_t)rel_addr | (uint32_t)(elf));
        ext_drv_list[index].binary = elf;
    }
    else
    { 
        ext_drv_list[index].start = f; // start of file (in case of flat binary)
        ext_drv_list[index].binary = f;
    }

    ext_drv_list[index].type = type;
    ext_drv_list[index].stack = evalloc(PROG_DEFAULT_STACK_SIZE, PID_DRIVER);

    prog_set_status_drv_running();

    // initialize the  driver by calling its main()
    // this is not the right function to use since this function does not change
    // stack pointers, which means that the driver uses the stack of whatever program was
    // running at the time of calling
    // FIXME: change stacks
    EXEC_CALL_FUNC(ext_drv_list[index].start, NULL); // (((uint32_t)ext_drv_list[index].stack) + PROG_DEFAULT_STACK_SIZE)
    prog_set_status_prog_running();

    return EXIT_CODE_GLOBAL_SUCCESS;
}

void driver_remove_external_driver(uint32_t type)
{
    uint32_t index = driver_ext_find_index(type);

    if(index == MAX)
        return;
    
    vfree(ext_drv_list[index].stack);
    vfree(ext_drv_list[index].binary);

    // empty the entry of this driver
    memset(&ext_drv_list[index], sizeof(ext_driver_list_t), 0);
}

void driver_api(void *req)
{
    driver_call_t *call = req;
    
    if(ext_drv_list == NULL)
    { call->hdr.exit_code = EXIT_CODE_GLOBAL_NOT_INITIALIZED; return; }

    switch(call->hdr.system_call)
    {
        case SYSCALL_DRIVER_GET_LIST:
        {
            driver_info_t *list = evalloc(DRIVER_EXTERNAL_LIST_SIZE, PID_DRIVER);
            
            for(uint32_t i = 0; i < DRIVER_EXTERNAL_MAX_SUPPORTED; ++i)
            {
                list[i].type = ext_drv_list[i].type;
                memcpy(&list[i].bin_name[0], &ext_drv_list[i].bin_name[0], FAT_MAX_FILENAME_LEN);
            } 

            call->hdr.exit_code = EXIT_CODE_GLOBAL_SUCCESS;
            call->hdr.response_ptr = list;
            call->hdr.response_size = DRIVER_EXTERNAL_LIST_SIZE;

            break;
        }
        

        case SYSCALL_DRIVER_ADD:
            call->hdr.exit_code = driver_add_external_driver(call->type, call->path);
        break;

        case SYSCALL_DRIVER_REMOVE:
            driver_remove_external_driver(call->type);
        break;

        default:
            call->hdr.exit_code = EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;
        break;
    }
}