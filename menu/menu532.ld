OUTPUT(build/menu532.elf);

SECTIONS {
	.text 0x011dd000 : {
		*(.text._start);
		*(.text*);
	}
	/DISCARD/ : {
		*(*);
	}
}

/* Main */
PROVIDE(entry_point = 0x1005d180);

/* System */
PROVIDE(_Exit = 0x0101cd70);
PROVIDE(OSFatal = 0x1031368);
PROVIDE(DCFlushRange = 0x1023ee8);
PROVIDE(memset = 0x1035a54);
PROVIDE(memcpy = 0x1035a68);
PROVIDE(__gh_errno_ptr = 0x1040308);
PROVIDE(GX2WaitForVsync = 0x1151964);
PROVIDE(__os_snprintf = 0x102f09c);
PROVIDE(title_id = 0x100136D0);

/* Alloc */
PROVIDE(MEMAllocFromDefaultHeapEx = 0x1004e9c0);
PROVIDE(MEMAllocFromDefaultHeap = 0x100b4878);
PROVIDE(MEMFreeToDefaultHeap = 0x100b487c);

/* Libs */
PROVIDE(OSDynLoad_FindExport = 0x102b790);
PROVIDE(OSDynLoad_Acquire = 0x102a31c);

/* Screen */
PROVIDE(OSScreenInit = 0x0103a880);
PROVIDE(OSScreenGetBufferSizeEx = 0x0103a91c);
PROVIDE(OSScreenSetBufferEx = 0x0103a934);
PROVIDE(OSScreenClearBufferEx = 0x0103aa90);
PROVIDE(OSScreenFlipBuffersEx = 0x0103a9d0);
PROVIDE(OSScreenPutFontEx = 0x0103af14);

/* VPAD */
PROVIDE(VPADRead = 0x011293d0);

/* FS Functions */
PROVIDE(FSAddClient = 0x010689fc);
PROVIDE(FSInitCmdBlock = 0x01068c54);
PROVIDE(FSGetMountSource = 0x0106ec24);
PROVIDE(FSMount = 0x0106ed14);
PROVIDE(FSUnmount = 0x0106ed8c);
PROVIDE(FSOpenDir = 0x0106f690);
PROVIDE(FSReadDir = 0x0106f780);
PROVIDE(FSOpenFile = 0x106ef7c);
PROVIDE(FSReadFile = 0x106f108);
PROVIDE(FSChangeDir = 0x0106eefc);
PROVIDE(FSMakeDir = 0x0106f8e0);
PROVIDE(FSCloseDir = 0x0106f700);
PROVIDE(FSCloseFile = 0x106f088);
PROVIDE(FSGetStat = 0x0106fdc8);
PROVIDE(FSMakeDir = 0x0106f8e0);
