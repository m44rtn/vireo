#include "api_kernel.h"

#define API_PROC_ACTIVE 1

typedef struct
{
    uint32_t *shared_memory;
    uint8_t ACTIVE;

} tAPI_PROCS;

typedef struct
{
    char signature[8]; // VIREOSYS
    uint16_t proc_ID;
    uint16_t resv0;
    uint32_t *shared_memory;
    uint32_t *program_end;
    uint32_t *allocation_space_end;

    uint8_t resv[488];
} tAPI_INIT;

typedef struct
{
    char signature[8]; // VIREOSYS
    uint16_t proc_ID;
    uint16_t type;

    uint8_t *resv[500];
} tAPI_DEFAULT;

//FIXME: could easily overflow
tAPI_PROCS api_processes[256];
uint32_t next_procID = 0;

tAPI_INIT *response_init;
tAPI_DEFAULT *response_default;

static inline uint16_t api_get_procid()
{
    uint16_t PROCID = 0;
    while(api_processes[PROCID].ACTIVE != API_PROC_ACTIVE)
        PROCID++;

    return PROCID;
}

void api_init(uint32_t *api_struct)
{
    response_init = (tAPI_INIT *) api_struct;

    response_init->signature[0] = 'V';
    response_init->signature[1] = 'I';
    response_init->signature[2] = 'R';
    response_init->signature[3] = 'E';
    response_init->signature[4] = 'O';
    response_init->signature[5] = 'S';
    response_init->signature[6] = 'Y';
    response_init->signature[7] = 'S';

    response_init->shared_memory = malloc(512);
    
    /* todo */
    response_init->program_end          = 4096;
    response_init->allocation_space_end = 8096;  
    
    api_processes[next_procID].shared_memory = response_init->shared_memory;
    api_processes[next_procID].ACTIVE        = API_PROC_ACTIVE;
    next_procID++;

    demalloc((void *) response_init);
}

void api_error()
{
    uint16_t PROCID = api_get_procid();
    response_default = (tAPI_DEFAULT *) malloc(512);

    response_default->signature[0] = 'V';
    response_default->signature[1] = 'I';
    response_default->signature[2] = 'R';
    response_default->signature[3] = 'E';
    response_default->signature[4] = 'O';
    response_default->signature[5] = 'S';
    response_default->signature[6] = 'Y';
    response_default->signature[7] = 'S';

    response_default->proc_ID   = PROCID;
    response_default->type      = 0xFFFF;

    api_processes[PROCID].shared_memory[0] = (uint32_t *) response_default;

    demalloc((void *) response_default);
}