OUTPUT(build/loader532.elf);

SECTIONS {
	.text 0x011de400 : {
            *(.text._start);
            *(.text*);
	}
        .magic : {
            *(.magic*);
        }
	/DISCARD/ : {
            *(*);
	}
}

/* Instructions addresses to replace by "bl 0x????????   - instr addr    -  original instruction              	    => comment */
/* This are the real functions that are used for our purpose */ 
PROVIDE(LiWaitIopComplete                                = 0x0100FFA4);
PROVIDE(LiWaitIopCompleteWithInterrupts                  = 0x0100FE90);
PROVIDE(LiCheckAndHandleInterrupts                       = 0x010046B0);
PROVIDE(Loader_SysCallGetProcessIndex                    = 0x010000A8);
PROVIDE(LiLoadAsync                                      = 0x0101005C);
PROVIDE(strncpy                                          = 0x01010690);
PROVIDE(strnlen                                          = 0x010106E0);

/* This are just addresses to the real functions which we only need as reference */
PROVIDE(addr_LiWaitOneChunk                              = 0x010007EC);
PROVIDE(addr_LiBounceOneChunk                            = 0x010003B0);