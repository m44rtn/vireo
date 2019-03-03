#include "memory.h"

#define MEM_TABLE_SIZE 65536

//TODO: Cleanup this file!!!

uint32_t *MEM_start = 6 * 1024*1024+1; //1 MiB //10*1024*1024; //because other solutions don't work yet, this works fine for now
uint32_t MEM_MAX;

typedef struct{ 
	uint32_t *loc;
	uint32_t size;
	uint32_t next;
	bool alloct;
} MEM_TABLE;


MEM_TABLE *mem_table[MEM_TABLE_SIZE];
uint32_t mem_table_entry = 1;


void GRUB_GetMemInfo(multiboot_info_t* mbh){
	
	systeminfo.memlo = mbh->mem_lower; //Store the by GRUB detected lower memory amount
	systeminfo.memhi = mbh->mem_upper; //Same but for upper
	systeminfo.mem_total = systeminfo.memlo + systeminfo.memhi;
	
	MEM_MAX = systeminfo.mem_total  * 1024 - ((uint32_t) MEM_start); //amount of max available memory, GRUB uses KB's for amount of memory

	mem_table[0]->loc = MEM_start;
	mem_table[0]->size = 0;
	mem_table[0]->next = MEM_start;
	mem_table[0]->alloct = true;

	for(int i = 1; i < MEM_TABLE_SIZE; i++)
	{
		mem_table[i]->alloct = false;
		mem_table[i]->size = NULL;
	}
	
}


void *malloc(size_t size)
{
	uint32_t entry = mem_table_entry;
	
	//check if we have enough memory for the block
	if(MEM_MAX < size)
	{
		error(54); //out of memory
		return 54;
	}

	//search for an empty piece (does not care about the size of a previously allocated block)

	if(mem_table_entry >= MEM_TABLE_SIZE)
	{
		for(int i = 0; i < MEM_TABLE_SIZE; i++)
		{
			if(mem_table[i]->alloct == false && (mem_table[i]->size == 0 || mem_table[i]->size >= size))
			{
				entry = i;
				i = MEM_TABLE_SIZE;
			}
		}
	}

	//check for more errors
	if(entry == MEM_TABLE_SIZE){
		error(57); //couldn't allocate memory
		return 57;
	}

	//allocate, return
	mem_table[entry]->alloct = true;
	mem_table[entry]->loc = mem_table[entry - 1]->next;
	mem_table[entry]->next = mem_table[entry]->loc + size;
	MEM_MAX -= size;

	if(mem_table_entry < MEM_TABLE_SIZE) mem_table_entry++;

	return (void *) mem_table[entry]->loc;
}

void demalloc(void *ptr)
{
	int entry;
	for(entry = 0; entry < MEM_TABLE_SIZE && mem_table[entry]->loc != ptr; entry++);
	trace("entry of the de-allocation: %i\n", entry);
	mem_table[entry]->alloct = false;
}

void *kmemset(void *ptr, uint32_t val, size_t size){
	char* fall = ptr;
	while(size--) fall = (char*) val;
	return ptr;

}

void MemCopy(uint32_t *src, uint32_t *dest, size_t size)
{
	for(uint32_t i = 0; i < size; i++)
		src[i] = dest[i];
}

char *strcopy(char* dest, char* src)
{
	int i;
	for(i = 0; src[i]; i++) dest[i] = src[i];
	dest[i] = 0;
	return dest;
}