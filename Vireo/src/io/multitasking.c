#include "multitasking.h"

#define TASK_DONE "_VIREO-DONE"

//so this should be a priority based round robin scheduling system...

//...With only a max of 15 processes at a time! very efficient.


uint8_t CurrentTasks = 0;       //amount of tasks in the queue
static uint8_t ExecTask = 0x00; //current task

//request a task by using int 3, eax = 0x01 (that's the plan)

//flags:
//      - bit 0:    set if v86 task
//      - bit 1:    set if kernel task
//      - bit 3-15: reserved for future use

uint32_t Task_Save_State(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp_unused, uint32_t ebx, uint32_t edx, 
                    uint32_t ecx, uint32_t eax,  uint32_t eip, uint32_t cs, uint32_t EFLAGS, uint32_t esp, uint32_t ss)
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
    tasks[ExecTask].registers.EFLAGS = EFLAGS;

    //return the registers in 'reverse order' (so that we can safely return from the interrupt)
    //return ss, esp, EFLAGS, cs, eip, eax, ecx, edx, ebx, esp, ebp, esi, edi;   
}
/*
void task_timeguard()
{    
    //gets run every PIT tick
    
    //return if we don't have tasks in the queue
    if(ExecTask == 0xFF) return;

    //check if the task has used up it's time slice, otherwise give it more time.
    if(tasks[ExecTask].quantum != 0)
    {
        tasks[ExecTask].quantum--;   
        return;
    } 
    
    //If the task isn't done, save it's state before we delete it.
    char* ebp = tasks[ExecTask].registers.ebp;
    if(!eqlstr(ebp, TASK_DONE)) task_save();

    //clear the task and get a new one
    task_pop(ExecTask);  
    task_findnew();

    //tell the PIC we're done with the interrupt to (hopefully) avoid 
    //being in an interrupt forever.
    outb(PIC1, 0x20);

    if((tasks[ExecTask].flags & TASK_FLAG_v86)) v86_switch();
    else if(tasks[ExecTask].flags & TASK_FLAG_KERNEL) task_kernel_switch(/*insert eip*//*);
    else jump_user_mode(/*insert eip*//*);
}


void task_push_v86(uint32_t ebp, uint32_t entry_point, uint8_t priority, uint16_t flags)
{
    //if the queue is full, ignore the request -> could end badly
    if(CurrentTasks == 15) return;

    //prepare the task
    tTask task;
    task.entry_ptr = entry_point;
    task.quantum = 8;              //every task has 8 ticks (that's a 32nd of a second)
    task.priority = priority;

    //ensure the v86 bit is set
    task.flags = TASK_FLAG_v86 | flags;

    task.registers.ebp = 0x0000;
    task.registers.esp = 0xFFFF;

    //add the task to the queue
    tasks[CurrentTasks] = task;
    CurrentTasks++;    
}

void task_push(uint8_t priority, uint32_t entry_point, uint16_t flags)
{
    //adds a new task to the queue

    //if the queue is full, ignore the request -> could end badly
    if(CurrentTasks == 15) return;

    //prepare the task
    tTask task;
    task.entry_ptr = entry_point;
    task.quantum = 8;              //every task has 8 ticks (that's 1/32 of a second)
    task.priority = priority;
    task.flags = flags;

    //reserve 4 KiB stack size
    task.registers.ebp = malloc(4096);        
    task.registers.esp = task.registers.ebp + 4096;

    //add the task to the queue
    tasks[CurrentTasks] = task;
    CurrentTasks++;

}

/*static*/ /*void task_pop(uint8_t tasknum)
{
    //move everything over by one so we don't have an empty gap
    for(uint8_t i = tasknum; i < 14; i++)
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
    if(tasks[14].entry_ptr > 0)
        kmemset(&tasks[14], 0, sizeof(tTask));

    CurrentTasks--;
}

/*static*/ /*void task_save()
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

/*static*/ /*void task_findnew()
{
    uint8_t highest_prior = 0;
    uint8_t task = 0;

    //check if we have nothing in the queue and if we have ran tasks before
    //if so, our queue is empty while it shouldn't. Re-init it.
    //if(tasks[0].priority == 0 && ExecTask != 0xFF) InitTasking(/*BlueBird*//*);

    for(uint8_t i = 0; i < 15; i++)
    {
        //if(tasks[i].priority == 0) InitTasking(); //we've reached the end of the queue which is bad, and we just re-init it

        if(highest_prior < tasks[i].priority)
        {
            highest_prior = tasks[i].priority;
            task = i;
        } 

    }

    ExecTask = task;
    
    //SwitchTask(tasks[ExecTask]);
    
}

//is this necessary?
/*static void task_switch(tTask task)
{
    switch((task.flags) & 0x02)
    {
        case 0:
            //task is an external task
            //doesn't do anything else yet
            jmp_user_mode(task.entry_ptr);
            break;

        case 1:
            //task is a kernel task
            Switch_Internal_Task(task);
            break;
       
    }
}

//is this necessary?
static void task_switch_internal(tTask task)
{
    //is not a v86 task
    if(!(task.flags & 0x01)) jmp_user_mode(task.entry_ptr);

    //is a v86 task
    if((task.flags & 0x01)) setup_v86();
}*/

//toolbox, if we even need it (spoiler alert! we don't)
void setup_SYSenter()
{
    getESP();
    cpuSetMSR(0x174, segments.cs, 0);
}