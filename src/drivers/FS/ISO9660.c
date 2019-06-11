#include "ISO9660.h"

#define ISO_START_VOL_DESC  0x10

typedef struct
{
    char fill[34];
} tISO_DIRECTORY;

typedef struct
{
    char year[4];
    char month[2];
    char day[2];
    char hour[2];
    char minute[2];
    char second[2];
    char hundrethofasecond[2];
    uint8_t timezone;
} tISO_DATETIME;


typedef struct
{
    uint8_t type_code;
    char identifier[5];
    uint8_t version;
} tVOL_DESC_GLOBAL;

typedef struct
{
    tVOL_DESC_GLOBAL vol_desc;
    uint8_t zero;

    char systemid[32];
    char volid[32];

    uint8_t zeros[8];

    uint32_t VolSpaceSize;
    uint32_t VolSpaceSize_ignore;

    uint8_t allzeros[32];

    uint16_t volsetsize;
    uint16_t volsetsize_ignore;

    uint16_t volseqnum;
    uint16_t volseqnum_ignore;

    uint16_t logicalsize;
    uint16_t logicalsize_ignore;

    uint32_t PathTableSize;
    uint32_t PathTableSize_ignore;

    uint32_t typeLPathTable;
    uint32_t typeLOptPathTable;

    uint32_t typeMPathTable_ignore;
    uint32_t typeMOptPathTable_ignore;

    tISO_DIRECTORY root_dir;

    char volsetID[128];
    char PublisherID[128];
    char DataPrepID[128];
    char AppID[128];
    char CopyrightFileID[38];
    char AbstractFileID[36];

    tISO_DATETIME creation;
    tISO_DATETIME modified;
    tISO_DATETIME expiration;
    tISO_DATETIME effective;

    uint8_t FileStructureVer;

    uint8_t zeroed;

    char undefined[512];
    char reserved[653];
} tVOL_DESC_PRIMARY;

tVOL_DESC_PRIMARY primary;

/* void iso_init(uint8_t drive)
{
    /* init the filesystem */ 
   /* PIO_READ_ATAPI(x ,0x10, 1, )
} */