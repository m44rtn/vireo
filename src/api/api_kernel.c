#include "api_kernel.h"

typedef struct
{
    uint32_t *shared_memory;
} tAPI_PROCS;

typedef struct
{
    char signature[5]; // VIREO
    uint16_t proc_ID;
    uint32_t *shared_memory;
    uint32_t *program_end;
    uint32_t *allocation_space_end;

    uint8_t resv[497];
} tAPI_INIT;

tAPI_PROCS api_processes[256];
uint32_t procID = 0;


void api_init(uint32_t *api_struct)
{
    tAPI_INIT *response = (tAPI_INIT *) api_struct;

    response->signature[0] = 'V';
    response->signature[1] = 'I';
    response->signature[2] = 'R';
    response->signature[3] = 'E';
    response->signature[4] = 'O';

    response->shared_memory = malloc(512);
    
    /* todo */
    response->program_end = 4096;
    response->allocation_space_end = 8096;  
    
    api_processes[procID].shared_memory = response->shared_memory;
    procID++;
}