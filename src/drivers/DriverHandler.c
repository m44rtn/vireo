//Driver handler, handles the driver allocation and detection
#include "DriverHandler.h"

#define DRIVER_Signature "_VIREO_SYS"

typedef struct
{
    uint32_t ss;
    uint32_t esp;
    //uint32_t eflags;
    uint32_t cs;
    uint32_t eip;
    uint32_t command;
    uint32_t edi;
} __attribute__ ((packed)) V86_Task;

char* UNKOWN    = "UNKOWN";
char* EMPTY     = " ";
char* VESA      = "VESA";

char *driver_type(uint16_t type)
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

    File *file = FindFile(filename, DIRLOC, 0);

    uint16_t *ReadFileDriver = (uint16_t *) 0x0600;
    uint32_t lba = FAT_cluster_LBA(file->FileLoc);

    PIO_READ_ATA(0, lba, ((file->size / 512) + 1), (uint16_t *) ReadFileDriver);

    header = (DRVR_HEADER *) ReadFileDriver;

    if(hasStr(header->Signature, DRIVER_Signature)) 
    {
        setcolor(0x02); //green on black
        print("[OK] ");
        setcolor(0x07); //default (light grey on black)
        uint32_t type = header->type;
        trace("Found %s driver module", (uint32_t) driver_type(type));
        trace(" --> loaded @ %i\n", (uint32_t) (ReadFileDriver));

    }

    bool isv86 = (header->type == 3) ? TASK_FLAG_v86 : 0;
    uint32_t file_size = ((uint32_t) file - header->size);
    //tTask task = Prepare_Internal_Task(header, (uint32_t) file, file_size, isv86);
    //Switch_Internal_Task(task, TASK_HIGH);   

    run_v86_driver( (uint32_t *) ReadFileDriver, file_size, NULL);
}


void run_v86_driver(uint32_t *file_start, uint32_t file_size, uint16_t flags)
{

    V86_Task task;
    uint16_t offset = (uint16_t) v86_linear_to_sgoff(0x0600);

    task.cs = 0x1b;
    task.eip = 0x450;//offset; //0x0600;
    task.esp = 0xFFFF;
    task.ss = 0x23;

    kmemset(&tasks, 0, sizeof(tTask));
    v86_enter( (uint32_t *) &task, (uint32_t *) &tasks[0].registers);

    //task_findnew();
}




