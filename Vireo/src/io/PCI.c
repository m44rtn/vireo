#include "PCI.h"

//definition of classes


#define PCI_CLASS_MSC		"Mass storage controller" //Mass storage controller
#define PCI_CLASS_NC			"Network controller" //Network Controller
#define PCI_CLASS_DC			"Display controller" //Display controller

//definition of subclasses


#define PCI_CNFG			0xCF8
#define PCI_DATA			0xCFC



uint16_t getPCIval(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint8_t strt, bool BorW){
	uint32_t address = pciConfigRead(bus, device, func, reg);
	uint32_t result;

	if(!BorW){
		//byte
		result = (uint16_t) getBYTE(address, strt);
	}else{
		//word
		result = getWORD(address, strt);
	}
	return result;
}

uint32_t GetPCIDevice(uint8_t basecl, uint8_t subcl, bool iTrace, bool iErr) // I previously did this with a list, but that didn' t work either
{ 

	//RETURNS:
	// byte 3: none
	// byte 2: bus
	// byte 1: device
	// byte 0: function
	
	uint32_t fndDev;
	uint8_t bus;
	uint8_t device;
	uint8_t func;
	uint8_t class;
	uint8_t subclass;
	bool found = false;
	
	char i = 0;

	for(bus = 0; bus != 255; bus++){ // PCI busses are 8 bits, 256 possible combinations numbered 0 - 255
		for(device = 0; device < 32; device++){ // PCI busses are 5 bits, 32 possible combinations numbered 0 - 31
			for(func = 0; func < 8; func++){ // PCI busses are 3 bits, 8 possible combinations numbered 0 - 7
			uint16_t VendorID = getPCIval(bus, device, func, 0x00, 0, GETWORD);

				if(VendorID != 0xFFFF){ //does device exist?

					class = getPCIval(bus, device, func, 0x02, 3, GETBYTE); //get the class of the current thing
					subclass = getPCIval(bus, device, func, 0x02, 2, GETBYTE); //get the subclass of the current thing

					//Is it the right thing?
					if(class == basecl)
					{
						if(subclass == subcl)
						{
							fndDev = (uint32_t) ( ( (uint32_t) bus << 16) | ( (uint32_t) device << 8) | ( (uint32_t) func));
							found = true;

							if(!iTrace)
							{
								trace("PCI device found at bus %i ", bus);
      							trace(" device %i ", device);
      							trace(" function %i\n", func);
							}
							
						}
					}
				}
			}
		}
	}
	if(!found && !iErr)
	{ 
		error(58);
		return 0;
	}else if(!found && iErr) return 0;

	return fndDev;

}


uint32_t pciConfigRead (uint8_t bus, uint8_t device, uint8_t func, uint8_t reg){
	
	/*just so it looks nice*/
	uint32_t lbus = (uint32_t) bus;
	uint32_t ldevice = (uint32_t) device;
	uint32_t lfunc = (uint32_t) func;
	uint32_t lreg = (uint32_t) reg;
	uint32_t tmmp;
	
	//create config adress
	uint32_t address = (uint32_t) ( (lbus << 16) | (device << 11) | (lfunc << 8)  | (lreg << 2) | (uint32_t) 0x80000000);
	
	outl(0x0cf8, address);
	tmmp = sysinlong(0x0cfc);
	
	return tmmp;
}


void pciConfigWrite(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint32_t val)
{
	/*just so it looks nice*/
	uint32_t lbus = (uint32_t) bus;
	uint32_t ldevice = (uint32_t) device;
	uint32_t lfunc = (uint32_t) func;
	uint32_t lreg = (uint32_t) reg;
	uint32_t tmmp;
	
	//create config adress
	uint32_t address = (uint32_t) ( (lbus << 16) | (device << 11) | (lfunc << 8)  | (lreg << 2) | (uint32_t) 0x80000000);
	
	outl(0x0cf8, address);
	
	outl(0x0CFC, val);

}

uint8_t pciGetInterruptLine(uint8_t bus, uint8_t device, uint8_t func)
{
	uint32_t thing = pciConfigRead(bus, device, func, 0x0F);
	uint8_t interruptline = (uint8_t) thing;
	return interruptline;
}

