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
PROVIDE(addr_LOADER_Start                                = 0x01003e60);  /* 0x5586103a : "slwi r6, r12, 2"    	    => called for rpx */
PROVIDE(addr_LOADER_Entry                                = 0x01002938);  /* 0x835e0000 : "lwz r26, 0(r30)"    	    => called for rpl */
PROVIDE(addr_LOADER_Prep                                 = 0x0100A024);  /* 0x3f00fff9 : "lis r24, -7"        	    => called for rpl */
PROVIDE(addr_LiLoadRPLBasics_bef_LiWaitOneChunk          = 0x01009118);  /* 0x809d0010 : "lwz r4, 0x10(r29)"  	    => */
PROVIDE(addr_LiLoadRPLBasics_in_1_load                   = 0x010092E4);  /* 0x7EE3BB78 : "mr r3, r23"         	    => load first chunk of rpx/rpl data, max size 0x400000 */
PROVIDE(addr_LiSetupOneRPL_after                         = 0x0100CDC0);  /* 0x7FC3F378 : "mr r3, r30"        	    => */
PROVIDE(addr_GetNextBounce_bef_LiWaitOneChunk            = 0x0100B6E0);  /* 0x91010008 : "stw r8, 0x18+var_10(r1)"  => instruction is originaly "cmplw r10, r11", it is replaced completely and the store to stack variable is done within function */
PROVIDE(addr_LiRefillBounceBufferForReading_af_getbounce = 0x0100BA28);  /* 0x2C030000 : "cmpwi r3, 0"        	    => replace other rpx/rpl data chunks */
