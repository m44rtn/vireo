#ifndef INFO_H
#define INFO_H

#define INFO_FLAG_MULTITASKING_ENABLED 1

typedef struct{
	//CPU
	char* CPUVendor;
	int nItterations;
	//PIT
	uint32_t PITcount;

	//memory
	uint32_t memlo;
	uint32_t memhi;
	uint32_t mem_total;
	uint32_t mem_map_addr;
	uint16_t mem_map_length;

	uint32_t *KMEMSET_LOC;
		
	//OS info
	char* release_state;
	char* version_major;
	char* version_minor;
	char* version_build;
	char* version_revision;
	char* version_architecture;

	uint32_t FLAGS;

	//keyboard stuff
	uint8_t keybrdbfrloc;
	char LastKey;
	bool KEYB_OnScreenEnabled;

	//PCI stuff
	uint16_t PCI_number;
	void* LastAlloctSpace;

	//DRIVE STUFF
	uint8_t master;
	uint8_t slave;
	long AHCI_BASE;
	uint8_t ports;


	uint16_t *ATAPIbuf;
	bool ATAPIint;
	bool waitingforATAPI;

	uint32_t mouseX, mouseY;
} SystemInfo;

SystemInfo systeminfo;

typedef struct{
	uint16_t pci_vendor;
	uint16_t pci_device_number;
	uint8_t pci_baseclass;
	uint8_t pci_subclass;
	uint8_t bus;
	uint8_t device;
	uint8_t func;
	uint8_t prog_if;
	char* pci_devicetype;
	char* pci_device;
} PCI_device_info;

PCI_device_info pci_device_info[8192];

typedef struct
{
	uint16_t cs;
	uint16_t ss;
} SEGMENTS;
SEGMENTS segments;

#endif
