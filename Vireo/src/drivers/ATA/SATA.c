#include "SATA.h"


#define	SATA_SIG_ATA	0x00000101	
#define	SATA_SIG_ATAPI	0xEB140101
#define	SATA_SIG_SEMB	0xC33C0101
#define	SATA_SIG_PM	0x96690101	

#define AHCI_DEV_NULL 0
#define AHCI_DEV_SATA 1
#define AHCI_DEV_SEMB 2
#define AHCI_DEV_PM 3
#define AHCI_DEV_SATAPI 4

#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3

#define HBA_PxCMD_ST    0x0001
#define HBA_PxCMD_FRE   0x0010
#define HBA_PxCMD_FR    0x4000
#define HBA_PxCMD_CR    0x8000

#define	AHCI_BASED	0x400000

/* ===================================================================
 *                               AHCI
 * ===================================================================
 */

void AHCI_init(){
    if(!getBAR5(true)) return;

    //Get ABAR and the AHCI controller
	systeminfo.AHCI_BASE = getBAR5(false);
	abar = (HBA_MEM *) systeminfo.AHCI_BASE;
	//trace("BAR5: %i\n", (int) hexstr(systeminfo.AHCI_BASE));

    //search for available drives
    search_drives((HBA_MEM *) systeminfo.AHCI_BASE);
    print("\n");
}


uint32_t getBAR5(bool ignoremsg){
    uint32_t SataDevice = GetPCIDevice(0x01, 0x06, ignoremsg, true); //find IDE device
    uint32_t BAR5;

    // decode the BDF values from the return value
    uint8_t bus = getBYTE((uint32_t) SataDevice, 2);
    uint8_t device = getBYTE((uint32_t) SataDevice, 1);
    uint8_t func = getBYTE((uint32_t) SataDevice, 0);

    // get BAR5
    BAR5 = pciConfigRead(bus, device, func, 0x09) & 0xFFFFFFF0; 
    
    return BAR5;
}

void AHCI_setup_port(HBA_PORT *port){
    port->cmd |= 1; //set the Start bit
    port->cmd |= (1 << 4); //set FIS receive enable
}

AHCI_Devices *search_drives(HBA_MEM *abar){
    devices[0] = (AHCI_Devices *) malloc(sizeof(AHCI_Devices) * 32);
    uint32_t pi = abar->pi;
    int i = 0; 
    while(i < 32){
            if(pi & 1){
                int dt = AHCI_check_type(&abar->ports[i]);
                if(dt == AHCI_DEV_SATA){
                    print("SATA drive at device: "); 
                    print(hexstr(i));
                    print("\n");
                    port_rebase(abar->ports, i);
                    
                }
                else if(dt == AHCI_DEV_SATAPI){
                    print("SATAPI drive at device: "); 
                    print(hexstr(i));
                    print("\n");
                    port_rebase(abar->ports, i);
                }
                 else if(dt == AHCI_DEV_SEMB){
                    print("SEMB drive at device: "); 
                    print(hexstr(i));
                    print("\n");
                    port_rebase(abar->ports, i);
                }
                 else if(dt == AHCI_DEV_PM){
                    print("PM drive at device: "); 
                    print(hexstr(i));
                    print("\n");
                    port_rebase(abar->ports, i);
                    
                }
            }
            pi >>= 1;
            i++;
    }
}

static int AHCI_check_type(HBA_PORT *port){
    uint32_t tssts = port->ssts;

    uint8_t ipm = (tssts >> 8) & 0x0F;
    uint8_t det = tssts & 0x0F;

    if(det != HBA_PORT_DET_PRESENT) return AHCI_DEV_NULL;
    if(ipm != HBA_PORT_IPM_ACTIVE) return AHCI_DEV_NULL;

    switch(port->sig){
        case SATA_SIG_ATAPI:
            return AHCI_DEV_SATAPI;
            //break?
        case SATA_SIG_SEMB:
            return AHCI_DEV_SEMB;
        case SATA_SIG_PM:
            return AHCI_DEV_PM;
        default:
            return AHCI_DEV_SATA; //maybe even replace this with NULL and just putting the ATA part in as a case
    } 
}

void port_rebase(HBA_PORT *port, int portno){
    stop_cmd(port);

    port->clb = AHCI_BASED + (portno << 10); //todo AHCI BASE
    port->clbu = 0;
    kmemset((void *) (port->clb), 0, 1024);

    port->fb = AHCI_BASED + (32 << 10) + (portno << 8);
    port->fbu = 0;
    kmemset((void *) (port->fb), 0, 256);

    port->serr = 0; //or 0?
    port->is = 0;
    port->ie = 1;

    HBA_CMD_HEADER *cmdhdr = (HBA_CMD_HEADER *) (port->clb);

    for(int i = 0; i < 32; i++){
        cmdhdr[i].prdtlen = 8;
        cmdhdr[i].ctba = AHCI_BASED + (40 << 10)/* + (portno << 13) */+ (i << 8);
        cmdhdr[i].ctbau = 0;
        kmemset((void*) cmdhdr[i].ctba, 0, 256);
    }
    stop_cmd(port);
}

void start_cmd(HBA_PORT *port){
    while(port->cmd & HBA_PxCMD_CR);
    port->cmd |= HBA_PxCMD_FRE;
    port->cmd |= HBA_PxCMD_ST;
}

void stop_cmd(HBA_PORT *port){
    port->cmd &= ~HBA_PxCMD_ST;
    port->cmd &= ~HBA_PxCMD_FRE;

    while(1){
        if(port->cmd & HBA_PxCMD_FR) continue;
        if(port->cmd & HBA_PxCMD_CR) continue;
        break;
    }
}

void Read_SATA(HBA_PORT *port, uint32_t startl, uint32_t starth, uint32_t *buf, uint32_t count){
    port->is = (uint32_t) -1; //clear all interrupts

    //find slot
    int slot = find_cmdslot(port);
    int spin = 0; //spin timeout timer
    if(slot == -1) return;

    //setup cmd header and prdtable
    HBA_CMD_HEADER *cmdhdr = (HBA_CMD_HEADER *) port->clb;

    
    cmdhdr += slot; //set the cmdheader to the right slot
    //cmdhdr->a = (uint8_t) 0x0A;
    cmdhdr->cfl = sizeof(FIS_REG_H2D)/sizeof(uint32_t);
    //cmdhdr->a = 1;
    cmdhdr->w = 0;
    cmdhdr->c = 1;
    //cmdhdr->p = 1;
    cmdhdr->prdtlen = 1;//(uint16_t) ((count - 1) >> 4) + 1;
    //we don't care about queue, command and cfislength

    
    HBA_CMD_TBL *cmdtable = (HBA_CMD_TBL *) (cmdhdr->ctba);
    kmemset(cmdtable, 0, sizeof(HBA_CMD_TBL) /*+ (cmdhdr->prdtlen-1) * sizeof(HBA_PRDT_ENTRY)*/);
   /* int i;
    for(i = 0; i < cmdhdr->prdtlen - 1; i++ ){
        cmdtable->prdt[i].dba = (uint32_t) buf;
        cmdtable->prdt[i].dbc = 4096 - 1;//8 * 1024 - 1;
        cmdtable->prdt[i].i = 0; //1
        buf += 4 * 1024;
        count -= 16;
    }
    

    cmdtable->prdt[cmdhdr->prdtlen - 1].dba = (uint32_t) buf;
    cmdtable->prdt[cmdhdr->prdtlen - 1].dbc = (count << 9) - 1; //512 bytes sir!
    cmdtable->prdt[cmdhdr->prdtlen - 1].i = 0;//1;*/

    cmdtable->prdt[0].dba = (uint32_t) buf;
    cmdtable->prdt[0].dbc = 4096-1; //512 bytes sir!
    cmdtable->prdt[0].i = 0;

    FIS_REG_H2D *fis = (FIS_REG_H2D *) (&cmdtable->cfis);
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = ATA_CMD_READ_DMA_EX; //Please spec, give me the types of commands
    
    fis->lba0 = (uint8_t) startl;
    fis->lba1 = (uint8_t) (startl >> 8);
    fis->lba2 = (uint8_t) (startl >> 16);
    fis->device = 1 << 6;
    
    fis->lba3 = (uint8_t) (startl >> 24);
    fis->lba4 = (uint8_t) starth;
    fis->lba5 = (uint8_t) (starth >> 8);

    fis->countl = 1;//count & 0xFF;
    fis->counth = 0;//(count >> 8) & 0xFF;
    
  // while((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) spin++;
   //if(spin == 1000000) {print("port = hung\n");error(0); return;}
    port->ci = 1 << slot;

    bool wait = true;
    while(wait){
        if((port->ci & (1 << slot)) == 0) break;
        if(port->is & (1 << 30)){print("READ ERROR\n"); error(0); return;}
    }
    if(port->is & (1 << 30)){print("READ ERROR\n"); error(0); return;}

    print("Worked!\n");
    return;
    //should be working?
    //TODO cleanup and make useful
}

int find_cmdslot(HBA_PORT *port){
    uint32_t slots = (port->sact | port->ci);
    
    int num_slots = (abar->cap & 0x0f00)>>8;

    for(int i = 0; i < num_slots; i++){
        if((slots & 1) == 0) return i;
        slots >>= 1;
    }

    print("couldn't find free command list slot\n");
    error(0);
    return -1;
}

