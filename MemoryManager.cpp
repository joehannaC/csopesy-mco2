/*
DESCRIPTION:

Memory manager: 
It must support a demand paging allocator. For the demand paging allocator, pages are loaded into physical memory frames on demand. 
When a process references a virtual memory page that is not currently in a frame, a page fault occurs, and the required page is brought 
from the backing store into a free frame. If no frames are free, a page replacement algorithm selects a page to be evicted to the backing store.

Memory visualization and backing store access:
The application has some way to debug the memory, such as “vmstat” and “process-smi.” The backing store is represented as a text file that can 
be accessed at any given time. It is saved in a text file “csopesy-backing-store.txt.”

Your system is simulating memory in the background. Thus, it would be limited by the maximum amount of main memory allocated by your original OS. Memory spaces are bound within your running program’s memory address. Memory spaces are pre-allocated and free to use by any processes upon startup.
The memory space will typically be limited to N bytes, and each process will utilize a fraction of the memory.
Your memory manager must support backing store operations when in low memory, context-switching processes in and out of the backing store (writing/reading in a file).



Memory visualization

There must be a mechanism to visualize and debug memory. The user can use either “process-smi,” which provides a high-level overview of available/used memory, or “vmstat,” which provides fine-grained memory details.


The “process-smi” is similar to the nvidia-smi command that prints a summarized view of the memory allocation and utilization of the processor (CPU for your program / GPU for nvidia-smi). A sample mockup is provided below:


----------------------------------------------
PROCESS-SMI V01.00 DRIVER VERSION: 01.00

CPU-Util: some %
Memory Usage: MiB / Total MiB
Memory Util: some %

Running processes and memory usage:
p01 someMib
p02 someMib
p03 someMib
.
.
.
p0n someMib
----------------------------------------------


The “vmstat” command provides a more detailed view:

Total memory | Total main memory in bytes.

Used memory | Total active memory used by processes.

Free memory | Total free memory that can still be used by other processes.

Idle cpu ticks | Number of ticks wherein CPU cores remained idle.

Active cpu ticks | Number of ticks wherein CPU cores are actually executing instructions.

Total cpu ticks | Number of ticks that passed for all CPU cores.

Num paged in | Accumulated number of pages paged in.

Num | paged out Accumulated number of pages paged out.



*/

