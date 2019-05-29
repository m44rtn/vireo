#include "multitasking.h"

#define TASK_DONE "_VIREO-DONE"

//so this should be a priority based round robin scheduling system...

//...With only a max of 15 processes at a time! very efficient.

bool isMultitasking = true;
uint8_t CurrentTasks = 0;       //amount of tasks in the queue
static uint8_t ExecTask = 0; //current task

//request a task by using int 3, eax = 0x01 (that's the plan)

//flags:
//      - bit 0:    set if v86 task
//      - bit 1:    set if kernel task
//      - bit 3-15: reserved for future use

uint32_t Task_Save_State(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, 
                    uint32_t ecx, uint32_t eax,  uint32_t eip, uint32_t cs /*uint32_t EFLAGS, uint32_t esp, uint32_t ss*/)
{
    //save all registers
    tasks[ExecTask].entry_ptr = eip;
   
    tasks[ExecTask].registers.eax = eax;
    tasks[ExecTask].registers.ecx = ecx;
    tasks[ExecTask].registers.edx = edx;
    tasks[ExecTask].registers.ebx = ebx;

    tasks[ExecTask].registers.ebp = ebp;
    tasks[ExecTask].registers.esp = esp;

    tasks[ExecTask].registers.esi = esi;
    tasks[ExecTask].registers.edi = edi;

    //tasks[ExecTask].registers.EFLAGS = EFLAGS;

    

    //return the registers in 'reverse order' (so that we can safely return from the interrupt)
    //return ss, esp, EFLAGS, cs, eip, eax, ecx, edx, ebx, esp, ebp, esi, edi;   
}

void task_timeguard()
{    
    //gets run every PIT tick
    
    //return if we don't have tasks in the queue
    //tREGISTERS *regs = &tasks[ExecTask].registers;
   
    if(!isMultitasking)
    {
        task_findnew();
        trace("Exectask = %i\t", ExecTask);
        trace("entryptr = %i\n", tasks[ExecTask].entry_ptr);
        trace("registers = %i\n", &tasks[ExecTask].registers);
        isMultitasking = true;
        
        outb(PIC1, 0x20);
        jmp_user_mode(tasks[ExecTask].entry_ptr, &tasks[ExecTask].registers);  
        return;
    }

    //check if the task has used up it's time slice, otherwise give it more time.
    if(tasks[ExecTask].quantum != 0)
    {
        tasks[ExecTask].quantum--;   
        trace("quantum=%i\n", tasks[ExecTask].quantum);

        trace("TASK1 -eax=%i\n", tasks[0].registers.eax);
        //printline(hexstr(), 13, (ExecTask) * 2);
        //print("\n");
        uint16_t i = 0;
        while(i < 0xFFFF) i++;
        
        return;
    } 
    
    //If the task isn't done, save it's state before we delete it.
    //uint32_t *ebp = (uint32_t *) tasks[ExecTask].registers.ebp;
    //if(!eqlstr( (char *) ebp, TASK_DONE)) 

    
    //task_push(TASK_HIGH, (uint32_t) kernel_thing, TASK_FLAG_KERNEL);
    task_save();
    task_findnew();
    outb(PIC1, 0x20);
    jmp_user_mode(tasks[ExecTask].entry_ptr, (uint32_t *) &tasks[ExecTask].registers); 

}

void task_push(uint8_t priority, uint32_t entry_point, uint16_t flags)
{
    //adds a new task to the queue

    if(ExecTask == 0xFF) ExecTask = 0;

    //if the queue is full, ignore the request -> could end badly
    if(CurrentTasks == 256) return;

    //prepare the task
    tTask task;
    task.entry_ptr = entry_point;
    task.quantum = 8;              //every task has 8 ticks (that's 1/32 of a second)
    task.priority = priority;
    task.flags = flags;

    //reserve 4 KiB stack size
    //task.registers.ebp = malloc(4096);        
    //task.registers.esp = task.registers.ebp + 4096;

    //add the task to the queue
    tasks[CurrentTasks] = task;
    CurrentTasks++;

}

void task_pop(uint8_t tasknum)
{
    //move everything over by one so we don't have an empty gap
    for(uint8_t i = tasknum; i < 256; i++)
    {
        tasks[i].entry_ptr = tasks[i + 1].entry_ptr;
        tasks[i].quantum = tasks[i + 1].quantum;
        tasks[i].priority = tasks[i + 1].priority;
        tasks[i].flags = tasks[i + 1].flags;

        tasks[i].registers.eax = tasks[i + 1].registers.eax;
        tasks[i].registers.ecx = tasks[i + 1].registers.ecx;
        tasks[i].registers.edx = tasks[i + 1].registers.edx;
        tasks[i].registers.ebx = tasks[i + 1].registers.ebx;

        tasks[i].registers.ebp = tasks[i + 1].registers.ebp;
        tasks[i].registers.esp = tasks[i + 1].registers.esp;

        tasks[i].registers.esi = tasks[i + 1].registers.esi;
        tasks[i].registers.edi = tasks[i + 1].registers.edi;        
    }

    //clear the last entry if it isn't empty yet
    if(tasks[255].entry_ptr > 0)
        kmemset(&tasks[256], 0, sizeof(tTask));

    CurrentTasks--;
}

void task_save()
{
    tTask CurrentState;

    //save current flags and priority
    CurrentState.flags = tasks[ExecTask].flags;
    CurrentState.priority = tasks[ExecTask].priority;

    //save registers
    CurrentState.entry_ptr = tasks[ExecTask].entry_ptr;
    CurrentState.registers = tasks[ExecTask].registers;

    //reset the task's quantum
    CurrentState.quantum = 8;

    //save it to the queue
    tasks[CurrentTasks] = CurrentState;
    CurrentTasks++;

}


void task_findnew()
{
    uint8_t highest_prior = 0;
    uint8_t task = 0;

    //check if we have nothing in the queue and if we have ran tasks before
    //if so, our queue is empty while it shouldn't. Re-init it.
    //if(tasks[0].priority == 0 && ExecTask != 0xFF) InitTasking(/*BlueBird*//*);

    //if(isMultitasking)
    //    task = ExecTask + 1;
    //else task = 0;

    if(ExecTask < 254) ExecTask++;
    else
    {
        kernel_panic_dump("MULTITASK_REQUEST_HELL");
    }
    
    //SwitchTask(tasks[ExecTask]);
    
}

void task_internal_ret_from_interrupt()
{
    outb(PIC1, 0x20);
    jmp_back_kernel(tasks[ExecTask].entry_ptr, (uint32_t *) &tasks[ExecTask].registers);
}

