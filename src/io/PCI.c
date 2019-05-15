#include "PCI.h"

//definition of classes


#define PCI_CLASS_MSC		"Mass storage controller" //Mass storage controller
#define PCI_CLASS_NC			"Network controller" //Network Controller
#define PCI_CLASS_DC			"Display controller" //Display controller

//definition of subclasses


#define PCI_CNFG			0xCF8
#define PCI_DATA			0xCFC


void pci_init()
{	
	uint8_t list = 0;
	uint8_t bus, device, func;
	uint8_t class;
	uint8_t subclass;
	
	for(bus = 0; bus != 255; bus++){ // PCI busses are 8 bits, 256 possible combinations numbered 0 - 255
		for(device = 0; device < 32; device++){ // PCI busses are 5 bits, 32 possible combinations numbered 0 - 31
			for(func = 0; func < 8; func++){ // PCI busses are 3 bits, 8 possible combinations numbered 0 - 7
			uint16_t VendorID = pci_get_val(bus, device, func, 0x00, 0, GETWORD);

				if(VendorID == 0xFFFF) continue;

				pci_devices[list].bus 			= bus;
				pci_devices[list].device 		= device;
				pci_devices[list].func 			= func;
				pci_devices[list].header_type	= pci_get_val(bus, device, func, 0x03, 3, GETBYTE);
				pci_devices[list].class 		= pci_get_val(bus, device, func, 0x02, 3, GETBYTE); //get the class of the current thing
				pci_devices[list].subclass 		= pci_get_val(bus, device, func, 0x02, 2, GETBYTE); //get the subclass of the current thing

				list++;
			}
		}
	}
}

uint32_t pci_get_device(uint8_t class, uint8_t subclass)
{
	//RETURNS:
	// byte 3: N/A
	// byte 2: bus
	// byte 1: device
	// byte 0: function

	uint32_t dev_found;
	for(uint8_t i = 0; i < 256; i++)
	{
		if(pci_devices[i].class != class) continue;
		if(pci_devices[i].subclass != subclass) continue;

		dev_found = ((uint32_t) pci_devices[i].bus << 16) | ((uint32_t) pci_devices[i].device << 8) | ((uint32_t) pci_devices[i].func);
		break;
	}
	return dev_found;
}

uint16_t pci_get_val(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint8_t strt, bool BorW)
{
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
	//DEPECRATED, but here until pci_get_device(class, subclass) works

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
			uint16_t VendorID = pci_get_val(bus, device, func, 0x00, 0, GETWORD);

				if(VendorID != 0xFFFF){ //does device exist?

					class = pci_get_val(bus, device, func, 0x02, 3, GETBYTE); //get the class of the current thing
					subclass = pci_get_val(bus, device, func, 0x02, 2, GETBYTE); //get the subclass of the current thing

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

