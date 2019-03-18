//Driver handler, handles the driver allocation and detection
#include "DriverHandler.h"

#define DRIVER_Signature "_VIREO_SYS"

typedef struct
{
    uint32_t ss;
    uint32_t esp;
    uint32_t cs;
    uint32_t eip;
} __attribute__ ((packed)) V86_Task;

char* UNKOWN    = "UNKOWN";
char* EMPTY     = " ";
char* VESA      = "VESA";

uint32_t *driver_type(uint16_t type)
{
    switch(type)
    {
        case 3:
            return VESA;
            break;
        default:
            return UNKOWN;
            break;
    }
}

void *FindDriver(char *filename/*, uint8_t priority*/)
{

    DRVR_HEADER *header;

    uint32_t DIRLOC = FAT_Traverse("HD0/BIRDOS/");
    trace("DIRLOC: %i\n", DIRLOC);

    File *file = FindFile(filename, DIRLOC, 0);

    uint16_t *ReadFileDriver = 0x0600;
    uint32_t lba = FAT_cluster_LBA(file->FileLoc);

     PIO_READ_ATA(0, lba, ((file->size / 512) + 1), (uint16_t *) ReadFileDriver);

    header = (DRVR_HEADER *) ReadFileDriver;

    if(hasStr(header->Signature, DRIVER_Signature)) 
    {
        setcolor(0x02); //green on black
        print("[OK] ");
        setcolor(0x07); //default (light grey on black)
        uint32_t type = header->type;
        trace("driver type: %i\n", type);
        trace("Found %s driver module", driver_type(type));
        trace(" --> loaded @ %i\n", (uint32_t) (ReadFileDriver));

    }

    bool isv86 = (header->type == 3) ? TASK_FLAG_v86 : 0;
    uint32_t file_size = ((uint32_t) file - header->size);
    //tTask task = Prepare_Internal_Task(header, (uint32_t) file, file_size, isv86);
    //Switch_Internal_Task(task, TASK_HIGH);   

    run_v86_driver(ReadFileDriver, file_size, NULL);
}
/*
tTask Prepare_Internal_Task(DRVR_HEADER *header, uint32_t file_start, uint32_t file_size,
                             bool isv86)
{
    if(isv86) setup_v86(file_start, file_size, TASK_FLAG_KERNEL | TASK_FLAG_v86);
    else task_push(TASK_HIGH, file_start, TASK_FLAG_KERNEL);
    tTask task;
    task.registers.ebp = malloc(0xFFFF);
    task.registers.esp = task.registers.ebp + 0xFFFF;
    task.entry_ptr = (header->offset + file_start);
    return task;
}*/

void run_v86_driver(uint32_t *file_start, uint32_t file_size, uint16_t flags)
{
    
    //copy the file to 0x0600 (the first 3KiB)
    //MemCopy((uint32_t *) file_start/*(file_start + sizeof(DRVR_HEADER))*/, 0x0600 , (file_size));
    //sleep(1);

    V86_Task task;
    uint16_t offset = (uint16_t) v86_linear_to_sgoff(0x0600);

    task.cs = 0;
    task.eip = 0x450;//offset; //0x0600;
    task.esp = 0xFFFF;
    task.ss = 0x0000;
    
    /*move the driverheader to free up memory (at this moment
      the entire file is allocated at it's old place too)*/
    //DRVR_HEADER *DriverHeader = malloc(sizeof(DRVR_HEADER));
    //MemCopy(file_start, DriverHeader, sizeof(DRVR_HEADER));
    //demalloc(file_start);
   
    //now it isn't anymore   

    //anyway, let's execute it. (this won't be able to return back, so we need int 3)
    v86_enter(&task);

    //task_findnew();
}




