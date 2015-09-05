# Synopsis
The aim of this work is to familiarize us with system programming in Unix, namely the creation of processes and communication between them with pipelining. We are implementing a very basic debugger using ptrace system call, which will inform the user of the system calls executed by the monitored program. We are also implementing a bash script which automates the entire process of using the picodb targeted program, extracting measurements from the exit.

# Background
The ptrace is a system call that allows a process-father to monitor and intervene in the execution of a child-process. Used mainly by debuggers and system calls monitoring programs (see. strace). When one
```long ptrace (enum ptrace_request request, pid_t pid, void * addr, void * data);```process is monitored at each signal it receives, stops execution and process-father will be notified in the next execution of the wait. In turn, it can make any actions offered by ptrace and then notify the child-process to continue execution. The signature of the function is in sys/ ptrace.h header file is the following:

The value of the first argument defines which action we want to realize. For example, for process-child to indicate that it wants to be monitored, the request will have a value **PTRACE_TRACEME**, or for the process-father to indicate that it wants to be notified of the system calls performed by the child, the request must have the **PTRACE_SYSCALL** price. You can find all the values that can take the request, and the description of what they do, the man page of ptrace. The second argument is the pid of the passive process. Argument address is used for indexing an address that a ptrace request can use. Finally, the data argument is used when a request requires data from the process that calls the ptrace. When addr and data arguments are indifferent to a request, we pass them NULL values. The ptrace, returns a value of type long, which varies with the request that executed.

In order to execute a system call, you should perform the assembly instruction int $0x80. This command will execute the interrupt 80, which shows the code that handles the system calls. This code expects to find in the registers of the CPU, the number that corresponds to the system call to be executed and the arguments with which to get the system call. For example, the i386 architecture, the number of system call must be in register eax (the x64 architecture, the RAX) and arguments in the registers ebx, ecx, edx, esi and edi. When the system call executed, will place the return value in register eax. Also, the operating system, is keeping the values ​​of registers in a memory area called the User Area, to enable the ptrace to see prices register a child process. So if for example we want to see the contents of register eax child-a process that wants to execute a system call, we would do this:

``` long eax_value = ptrace(PTRACE_PEEKUSER, child_pid, EAX * 4, NULL); ```

and return the value of register eax. The EAX statement in argument address, is a value specified in the header *sys/reg.h*, which has the value of the position within the User Area that corresponds to the EAX register. This value, multiply by four to be address within the structure, because in a 32-bit system every word has a length of four bytes. In a 64-bit system, the corresponding value (RAX) should be multiplied by eight.

Also, notice (at */usr/include/sys/reg.h*) and the existence of **ORIG_EAX** (ORIG_RAX respectively for 64 bit architecture) which gives us the value of the requested system call both on entry (first event) and during exit (second event).

A solution to support both 32 and 64 bit architectures is the use of predefined macro "x86_64". If it is active, then we are in 64 bit architecture, otherwise 32.

For more information see the man page of ptrace (man ptrace).
A presentation and examples can be found in Article Playing with ptrace, Part 1, the Linux journal
<http://www.linuxjournal.com/article/6100>

# Running the project

### picodb.c

picodb will accept as argument the path of the compiled executable, which will be the executable that will follow. Also it provides a Command Line Interface (CLI), with the following commands:

| Command                | Description                  |
|:-----------------------------------|:----------------------------|
|**trace\<category\>**| Applying monitoring system calls the class <category>.The parameter <category> can be: 1."process-control": Specific class syscalls defined below. 2. "file-management": Specific class syscalls defined below. 3. "all": Monitors all the above categories of system calls. Practically it's like he gave orders "trace process-control" \| and "trace file-management" by the user. |
|**redirect\<stream\>\<filename\>**| Requesting the redirection of one of the three standard input / output streams of the program will be executed from / to a file. Parameter \<filename\> is the file name. The <stream> can be: 1. stdin: requesting the redirection of input program to be implemented by the file \<filename\>. The file must exist, otherwise an error message appears. 2. stdout: applying the redirect output stdout program to be implemented in the file \<filename\>. This file is created if it does not exist. If it exists, the old content to be deleted with the start of execution. 3. stderr: requesting the redirected output stderr program to be implemented in the file \<filename\>. This file is created if it does not exist. If it exists, the old content to be deleted with the start of execution.|
|**blocking-mode\<mode\>**|Sets the blocking-mode parameter to the state \<mode\> The parameter \<mode\> can be: 1. on: to freeze the execution of the monitored program whenever one of the monitored system calls called and ask the user if he wants to continue the monitored execution. To resume execution, the user responds with y and n to stop. 2. off: the program runs without interruption |
|**limit-trace\<number\>**|It sets a limit on how many system calls each category will be monitored. If you set the limit-trace, after \<number\> calls system calls in each category, will be ignored other calls of that class. If you do not set the limit-trace, the picodb will monitor all calls system calls the categories set via the CLI.|
|**go**|Runs the program given to picodb as argument and monitor according to the commands given by the user to the CLI. When you finish the monitored program will print the information the user requested via the CLI, before giving the command go.|
|**quit**|Ends the picodb.|
|**help**|Prints all CLI commands supported by picodb the user, along with a brief description.|

Note** All the above commands can be curtailed only in their first letter (which is unique). For example, the command trace all can be given as **t all**.

The types of system calls are:

**Process Control**
* execve (SYS_execve)
* fork (SYS_fork)
*	wait4 (SYS_wait4)
*	kill (SYS_kill)

**File Management**
*	open (SYS_open)
*	close (SYS_close)
*	read (SYS_read)
*	write (SYS_write)

Overall, picodb starts with the name of an executable as parameter and waits for commands from the input. Once we give the command go, it regulates all the necessary functions and starts the desired target program. While the latter is executed, the output prints all syscalls requested monitored.

### readfile.c

The readfile program accepts input from the terminal and implements two methods of reading files, one with **read** function and the other with **fread**.
myscript.sh,can start picodb with readfile program, so it can read both ways the file  */etc/services*.
At readfile we will be having: 
* *read/etc/services*
* *fread/etc/services quit me*

The script will record the output of readfile and confirm that read the correct number of characters. The script will also record all the output of picodb and end will measure syscalls that required to read the file with each of the two read modes. Eventually it will giving some statistics.

The program was developed and tested in a linux environment. It is compiled with the command make, otherwise gcc –o picodb picodb.c
