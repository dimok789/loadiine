OUTPUT(loadiine.elf);

SECTIONS {
	.kernel_code 0x00AE1000 : {
		*(.kernel_code*);
	}
	. = .;
	.text : {
		*(.text*);
	}
	.menu_magic : {
		*(.menu_magic*);
	}
	.loader_magic : {
		*(.loader_magic*);
	}
	.fs_method_calls : {
		*(.fs_method_calls*);
	}
	. = . + 0xBC000000;
	.rodata : {
		*(.rodata*);
	}
	.bss : {
		*(.bss*);
	}
	.fs_magic : {
		*(.fs_magic*);
	}
	.magicptr 0x011DD000 : {
		*(.magicptr*);
	}
	/DISCARD/ : {
		*(*);
	}
}

/******************************************************** FS ********************************************************/
/* coreinit.rpl difference in addresses 0xFE3C00 */
/* FSA methods */
PROVIDE(FSAInit = 0x10608ac);
PROVIDE(FSAShutdown = 0x1060974);
PROVIDE(FSAAddClient = 0x106546c);
PROVIDE(FSADelClient = 0x1060aa4);
PROVIDE(FSAOpenFile = 0x10621f8);

/* FS base methods */
PROVIDE(FSInit = 0x10683c8);
PROVIDE(FSShutdown = 0x1068538);
PROVIDE(FSAddClient = 0x010689fc);
PROVIDE(FSAddClientEx = 0x10685fc);
PROVIDE(FSDelClient = 0x1068a08);
PROVIDE(FSInitCmdBlock = 0x01068c54);

/* FS methods for path replacement */
PROVIDE(FSOpenFile = 0x106ef7c);
PROVIDE(FSOpenFileAsync = 0x0106a434);
PROVIDE(FSOpenDir = 0x0106f690);
PROVIDE(FSOpenDirAsync = 0x0106afb8);
PROVIDE(FSChangeDir = 0x0106eefc);
PROVIDE(FSChangeDirAsync = 0x0106a1a8);
PROVIDE(FSGetStat = 0x0106fdc8);
PROVIDE(FSGetStatAsync = 0x0106bff4);
PROVIDE(FSMakeDir = 0x0106f8e0);
PROVIDE(FSMakeDirAsync = 0x0106b460);
PROVIDE(FSRename = 0x0106f9e0);
PROVIDE(FSRenameAsync = 0x0106b66c);
PROVIDE(FSRemove = 0x0106f960);
PROVIDE(FSRemoveAsync = 0x0106b568);
PROVIDE(FSFlushQuota = 0x0106fac8);
PROVIDE(FSFlushQuotaAsync = 0x0106ba4c);
PROVIDE(FSGetFreeSpaceSize = 0x0106fe38);
PROVIDE(FSGetFreeSpaceSizeAsync = 0x0106c008);
PROVIDE(FSRollbackQuota = 0x0106fc48);
PROVIDE(FSRollbackQuotaAsync = 0x0106bb50);

/* FS methods - not replaced */
PROVIDE(FSReadDir = 0x0106f780);
PROVIDE(FSRewindDir = 0x0106F7F0);
PROVIDE(FSReadFile = 0x106f108);
PROVIDE(FSCloseFile = 0x106f088);
PROVIDE(FSCloseDir = 0x0106f700);

/* FS methods - log */
PROVIDE(FSCloseFile_log = 0x106f088);
PROVIDE(FSCloseFileAsync_log = 0x0106a46c);
PROVIDE(FSSetPosFile_log = 0x106f530);
PROVIDE(FSGetPosFile_log = 0x106f4c0);
PROVIDE(FSGetStatFile_log = 0x106f5a0);
PROVIDE(FSIsEof_log = 0x106f610);
PROVIDE(FSReadFile_log = 0x106f108);
PROVIDE(FSReadFileWithPos_log = 0x106f194);
PROVIDE(FSReadFileAsync_log = 0x0106a808);
PROVIDE(FSCloseDir_log = 0x0106f700);
PROVIDE(FSGetCwd_log = 0x0106f870);
PROVIDE(FSReadDir_log = 0x0106f780);
PROVIDE(FSFlushFile_log = 0x0106f440);
PROVIDE(FSGetErrorCodeForViewer_log = 0x0106c5d4); 
PROVIDE(FSGetLastError_log = 0x0106c598);
PROVIDE(FSGetLastErrorCodeForViewer_log = 0x0106c9c8);
PROVIDE(FSSetStateChangeNotification_log = 0x01068e54);
PROVIDE(FSSetCmdPriority_log = 0x0106cab4);
PROVIDE(FSTruncateFile_log = 0x0106f3c0);
PROVIDE(FSWriteFile_log = 0x0106f228);                  
PROVIDE(FSWriteFileAsync_log = 0x0106aa08);
PROVIDE(FSWriteFileWithPos_log = 0x0106f2b4);
PROVIDE(FSGetVolumeState_log = 0x01068e20);

/* FS methods for sd card */
PROVIDE(FSGetMountSource = 0x0106ec24);
PROVIDE(FSMount = 0x0106ed14);
PROVIDE(FSUnmount = 0x0106ed8c);

/* GX2 methods */
PROVIDE(GX2WaitForVsync = 0x1151964);

/* Socket methods */
PROVIDE(socket_lib_init = 0x10c02f4);
PROVIDE(socket = 0x10c21c8);
PROVIDE(socketclose = 0x10c2314);
PROVIDE(connect = 0x10c0828);
PROVIDE(send = 0x10c16ac);
PROVIDE(recv = 0x10c0aec);
PROVIDE(setsockopt = 0x10C2E0C);

/* Standard library methods */
PROVIDE(MEMAllocFromDefaultHeapEx = 0x1004e9c0);
PROVIDE(MEMAllocFromDefaultHeap = 0x100b4878);
PROVIDE(MEMFreeToDefaultHeap = 0x100b487c);

/* Screen */
PROVIDE(OSScreenInit = 0x0103a880);
PROVIDE(OSScreenGetBufferSizeEx = 0x0103a91c);
PROVIDE(OSScreenSetBufferEx = 0x0103a934);
PROVIDE(OSScreenClearBufferEx = 0x0103aa90);
PROVIDE(OSScreenFlipBuffersEx = 0x0103a9d0);
PROVIDE(OSScreenPutFontEx = 0x0103af14);

/* OS data */
PROVIDE(title_id = 0x100136D0);

/* OS methods */
PROVIDE(OSDynLoad_Acquire = 0x0102A31C);
PROVIDE(OSDynLoad_FindExport = 0x102b790);
PROVIDE(OSDynLoad_IsModuleLoaded = 0x0102A504);
PROVIDE(OSDynLoad_GetModuleName = 0x0102B960);
PROVIDE(__os_snprintf = 0x102f09c);
PROVIDE(OSFatal = 0x1031368);
PROVIDE(OSSetExceptionCallbackEx = 0x010443f8);
PROVIDE(DCFlushRange = 0x1023ee8);

/* VPAD */
PROVIDE(VPADRead = 0x011293d0);

/******************************************************** Menu ********************************************************/
/* Mii Maker */
PROVIDE(MiiMaker_main = 0x1005d180);

/******************************************************* Loader *******************************************************/
/* This are the real functions that are used for our purpose */ 
PROVIDE(LiWaitIopComplete                                = 0x0100FFA4);
PROVIDE(LiWaitIopCompleteWithInterrupts                  = 0x0100FE90);
PROVIDE(LiCheckAndHandleInterrupts                       = 0x010046B0);
PROVIDE(Loader_SysCallGetProcessIndex                    = 0x010000A8);
PROVIDE(LiLoadAsync                                      = 0x0101005C);

/* This are just addresses to the real functions which we only need as reference */
PROVIDE(addr_LiWaitOneChunk                              = 0x010007EC);
PROVIDE(addr_LiBounceOneChunk                            = 0x010003B0);