	.text

/* Switch from current_thread to next_thread. Make next_thread
 * the current_thread, and set next_thread to 0.
 * Use eax as a temporary register; it is caller saved.
 */
	.globl thread_switch
thread_switch:
	/* YOUR CODE HERE */ // not save ip yet
    movl current_thread, %eax // different from C infer asm, this symbol from C is exactly used just like ptr, just deference it
    pushal
    movl %esp, (%eax) // assign this val to esp and to push to save all registers

    leal current_thread, %eax // assign currentthread to val in nextthread
    leal next_thread, %ebx
    movl (%ebx), %ecx
    movl $0, (%ebx) // set next thread to 0
    movl %ecx, (%eax) // now ecx is the addr of new thread struct
    
    movl (%ecx), %esp
    popal //restore general registers

	ret				/* pop return address from stack */
