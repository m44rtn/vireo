#include "PCI.h"

//definition of classes


#define PCI_CLASS_MSC		"Mass storage controller" //Mass storage controller
#define PCI_CLASS_NC			"Network controller" //Network Controller
#define PCI_CLASS_DC			"Display controller" //Display controller

//definition of subclasses


#define PCI_CNFG			0xCF8
#define PCI_DATA			0xCFC

/*DEBUG8 PCI.c*/

uint16_t getPCIval(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint8_t strt, bool BorW){
	uint32_t address = pciConfigRead(bus, device, func, offset);
	uint32_t res;
	if(!BorW){
		//byte
		res = (uint16_t) getBYTE(address, strt);
	}else{
		//word
		res = getWORD(address, strt);
	}
	return res;
}

uint32_t GetPCIDevice(uint8_t basecl, uint8_t subcl){ // I previously did this with a list, but that didn' t work either
	uint32_t fndDev;
	uint8_t bus;
	uint8_t device;
	uint8_t func = 0;

	uint8_t CL;
	uint8_t SCL;


	for(bus = 0; bus < 256; bus++){
		for(device = 0; device < 32; device++){
			uint16_t VendorID = getPCIval(bus, device, func, 0x00, 0, GETWORD);
			if(VendorID != 0xFFFF){ //does device exist?
				uint8_t HeaderType = (uint8_t) getPCIval(bus, device, func, 0x0c, 2, GETBYTE);


					if(HeaderType & 0x80 != 0){ //check if MF
						for(func = 1; func < 8; func++){
							CL = (uint8_t) getPCIval(bus, device, func, 0x08, 3, GETBYTE);
							SCL = (uint8_t) getPCIval(bus, device, func, 0x08, 2, GETBYTE);	
							
							if(CL != basecl){error(58); return 0;}
								if(SCL != subcl){
									error(58); return 0;
								}
							
							fndDev = (uint32_t) ( ( (uint32_t) bus << 16) | ( (uint32_t) device << 8) | ( (uint32_t) func));
							return fndDev;	
						}
					
						func = 0;
					}

					CL = (uint8_t) getPCIval(bus, device, func, /*0x02*/ 0x08, 3, GETBYTE);
					SCL = (uint8_t) getPCIval(bus, device, func, 0x08, 2, GETBYTE);

					if(CL != basecl){ error(58); return 0;}
						if(SCL != subcl){
							error(58);
							//fndDev = (uint32_t) ( ((uint32_t)bus << 16) | ((uint32_t)device << 8) | ((uint32_t)func));
							return 0;
						}
					
				
			}			
		}
	}

	fndDev = (uint32_t) ( ((uint32_t)bus << 16) | ((uint32_t)device << 8) | ((uint32_t)func));
	return fndDev;
}


uint32_t pciConfigRead (uint8_t bus, uint8_t device, uint8_t func, uint8_t reg){
	
	/*just so it looks nice*/
	uint32_t lbus = (uint32_t) bus;
	uint32_t ldevice = (uint32_t) device;
	uint32_t lfunc = (uint32_t) func;
	uint32_t lreg = (uint32_t) reg;
	int tmmp;
	
	//create config adress
	int address = (int) (lbus << 16) | (device << 11) | (lfunc << 8)  | (lreg & 0xfc) | 0x80000000;
	
	x86out((uint16_t) 0x0cf8, (long) address);
	tmmp = (int) sysinlong((uint16_t) 0x0cfc);
	
	return tmmp;
}


/*
 *--------------------------------------------------
 * 				IGNORE BELOW, NOT USED
 * -------------------------------------------------
 */
void PCIsetup(){
	uint16_t bus;
	uint8_t device;
	uint8_t func;
	uint32_t *value;
	int pci_pointer = 0;
	
	for(bus = 0; bus < 256; bus++){
		for(device = 0; device < 32; device++){
			pciCheckDevice(bus, device);			
		}
	}
}

void pciCheckDevice(uint16_t bus, uint8_t device){
	uint8_t function = 0;
	uint16_t vendorID = getPCIval(bus, device, function, 0x00, 0, GETWORD);

	if(vendorID == 0xFFFF) return;

	uint8_t HeaderType = getPCIval(bus, device, function, 0x0c, 2, GETBYTE);

	if(HeaderType & 0x80 != 0){
		
		for(function = 1; function < 8; function++){
			checkfunc(bus, device, function);
		
		}
	}
}

void checkfunc(uint16_t bus, uint8_t device, uint8_t function){
	uint8_t HeaderType = getPCIval(bus, device, function, 0x0c, 2, GETBYTE);
}


