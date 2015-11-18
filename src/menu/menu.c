#include "menu.h"
#include "../fs/fs.h"
#include "../utils/strings.h"
#include "../utils/rpx_sections.h"
#include "../utils/logger.h"
#include "../utils/utils.h"
#include "../utils/xml.h"
#include "../common/common.h"

/* Utils for the display */
#define PRINT_TEXT1(x, y, str) { OSScreenPutFontEx(1, x, y, str); }
#define PRINT_TEXT2(x, y, _fmt, ...) { __os_snprintf(msg, 80, _fmt, __VA_ARGS__); OSScreenPutFontEx(1, x, y, msg); }
#define BTN_PRESSED (BUTTON_LEFT | BUTTON_RIGHT | BUTTON_UP | BUTTON_DOWN | BUTTON_A | BUTTON_B | BUTTON_X)

/* Static function definitions */
static int IsRpxOrRpl(FSDirEntry *dir_entry);
static int Copy_RPX_RPL(FSClient *pClient, FSCmdBlock *pCmd, FSDirEntry *dir_entry, char *path, int is_rpx, int entry_index, char **rpl_imports, int rpl_import_count);
static void CreateGameSaveDir(FSClient *pClient, FSCmdBlock *pCmd, const char *dir_entry, char* mount_path);
static void GenerateMemoryAreasTable();
static void AddMemoryArea(int start, int end, int cur_index);
static int game_name_cmp(const void *game1, const void *game2);

/* global variable for CosAppXml struct that is forced to bss section */
ReducedCosAppXmlInfo cosAppXmlInfoStruct __attribute__((section(".bss")));

/* Entry point */
int Menu_Main(int argc, char *argv[]) {
    /* ****************************************************************** */
    /*                              MENU CHECK                            */
    /* ****************************************************************** */

    // mii maker is our menu :), if it's another app, continue coreinit process
    if (title_id != 0x000500101004A200 && // mii maker eur
        title_id != 0x000500101004A100 && // mii maker usa
        title_id != 0x000500101004A000)   // mii maker jpn
        return main(argc, argv);

    // check if game is launched, if yes continue coreinit process
    if ((GAME_LAUNCHED == 1) && (LOADIINE_MODE == LOADIINE_MODE_MII_MAKER))
    {
        return main(argc, argv);
    }

    /* ****************************************************************** */
    /*           Get _SYSLaunchTitleByPathFromLauncher pointer            */
    /* ****************************************************************** */
    uint sysapp_handle;
    OSDynLoad_Acquire("sysapp.rpl", &sysapp_handle);

    void(*_SYSLaunchTitleByPathFromLauncher)(const char* path, int len, int zero) = 0;
    OSDynLoad_FindExport(sysapp_handle, 0, "_SYSLaunchTitleByPathFromLauncher", &_SYSLaunchTitleByPathFromLauncher);

    int(*SYSRelaunchTitle)(uint argc, char* argv) = 0;
    OSDynLoad_FindExport(sysapp_handle, 0, "SYSRelaunchTitle", &SYSRelaunchTitle);

    int(*SYSLaunchMenu)() = 0;
    OSDynLoad_FindExport(sysapp_handle, 0, "SYSLaunchMenu", &SYSLaunchMenu);

    /* ****************************************************************** */
    /*                 Get SYSRelaunchTitle pointer                       */
    /* ****************************************************************** */
    GenerateMemoryAreasTable();

    /* ****************************************************************** */
    /*                              Init Memory                           */
    /* ****************************************************************** */

    char* msg           = (char*) malloc(80);
    char* mount_path    = (char*) malloc(FS_MAX_MOUNTPATH_SIZE);
    char* path          = (char*) malloc(FS_MAX_MOUNTPATH_SIZE);
    char **game_dir     = (char**)malloc(MAX_GAME_COUNT * sizeof(char *));
    // initialize all string pointers of the game list with 0
    int j;
    for(j = 0; j < MAX_GAME_COUNT; j++)
        game_dir[j]     = NULL;

    /* ****************************************************************** */
    /*                         Patch FS Functions                         */
    /* ****************************************************************** */
    PatchFsMethods();

    /* ****************************************************************** */
    /*                              Init Screen                           */
    /* ****************************************************************** */

    // Prepare screen
    int screen_buf0_size = 0;
    int screen_buf1_size = 0;
    uint screen_color = 0; // (r << 24) | (g << 16) | (b << 8) | a;

    // Init screen and screen buffers
    OSScreenInit();
    screen_buf0_size = OSScreenGetBufferSizeEx(0);
    screen_buf1_size = OSScreenGetBufferSizeEx(1);
    OSScreenSetBufferEx(0, (void *)0xF4000000);
    OSScreenSetBufferEx(1, (void *)0xF4000000 + screen_buf0_size);

    // Clear screens
    OSScreenClearBufferEx(0, screen_color);
    OSScreenClearBufferEx(1, screen_color);

    // Flush the cache
    DCFlushRange((void *)0xF4000000, screen_buf0_size);
    DCFlushRange((void *)0xF4000000 + screen_buf0_size, screen_buf1_size);

    // Flip buffers
    OSScreenFlipBuffersEx(0);
    OSScreenFlipBuffersEx(1);

    /* ****************************************************************** */
    /*                              Init SDCARD                           */
    /* ****************************************************************** */

    int sd_status = 0;
    FSMountSource mountSrc;
    int game_dh = 0;
    int game_count = 0;
    int path_index = 0;

    // Create client and cmd block
    FSClient* pClient = (FSClient*)malloc(sizeof(FSClient));
    FSCmdBlock* pCmd = (FSCmdBlock*)malloc(sizeof(FSCmdBlock));

    if (pClient && pCmd)
    {
        // Do an FSInit first
        FSInit();
        // Add client to FS.
        FSAddClient(pClient, FS_RET_NO_ERROR);

        // Init command block.
        FSInitCmdBlock(pCmd);

        // Mount sdcard
        if (FSGetMountSource(pClient, pCmd, FS_SOURCETYPE_EXTERNAL, &mountSrc, FS_RET_NO_ERROR) == FS_STATUS_OK)
        {
            if (FSMount(pClient, pCmd, &mountSrc, path, FS_MAX_MOUNTPATH_SIZE, FS_RET_UNSUPPORTED_CMD) == FS_STATUS_OK)
            {
                // Set SD as mounted
                sd_status = 1;

                // Copy mount path
                memcpy(mount_path, path, FS_MAX_MOUNTPATH_SIZE);

                // Open game dir
                while (path[path_index])
                    path_index++;

                path_index += __os_snprintf(&path[path_index], FS_MAX_MOUNTPATH_SIZE - path_index, "%s", SD_GAMES_PATH);

                if (FSOpenDir(pClient, pCmd, path, &game_dh, FS_RET_ALL_ERROR) == FS_STATUS_OK)
                {
                    FSDirEntry dir_entry;
                    while (FSReadDir(pClient, pCmd, game_dh, &dir_entry, FS_RET_ALL_ERROR) == FS_STATUS_OK && game_count < MAX_GAME_COUNT)
                    {
                        // allocate string length + 0 termination bytes
                        int name_len = strnlen(dir_entry.name, 0x1000);
                        game_dir[game_count] = (char *)malloc(name_len + 1);
                        if(!game_dir[game_count])
                            continue;
                        // copy game name
                        memcpy(game_dir[game_count], dir_entry.name, name_len + 1);
                        if(game_count < MAX_GAME_COUNT)
                            game_count++;
                    }
                    FSCloseDir(pClient, pCmd, game_dh, FS_RET_NO_ERROR);
                }
            }
        }
    }

    /* ****************************************************************** */
    /*                              Display MENU                          */
    /* ****************************************************************** */
    uint8_t game_index = 0;
    uint8_t game_sel = 0;
    uint8_t button_pressed = 1;
    uint8_t first_pass = 1;
    uint8_t launching = 0;
    int     mii_maker_mode = 0;
    int     return_to_home = 0;
    uint8_t ready = 0;
    int     error;

    //*(volatile unsigned int *)0xBD900000 = 0xDEADBEAF;
    //*(volatile unsigned int *)0xBD900004 = 0xDEADBABE;

    // sort game name pointers by name case insensitive
    qsort(game_dir, game_count, sizeof(char*), game_name_cmp);

    VPADData vpad_data;
    VPADRead(0, &vpad_data, 1, &error); //Read initial vpad status

    while (1)
    {
        // Refresh screen if needed
        if (button_pressed) { OSScreenFlipBuffersEx(1); OSScreenClearBufferEx(1, 0); }

        // Read vpad
        VPADRead(0, &vpad_data, 1, &error);

        // Title : "-- LOADINE --"
        PRINT_TEXT2(20, 1, "-- LOADIINE %s --", LOADIINE_VERSION);

        if (!sd_status)
        {
            // Print sd card status : "sd card not found or cannot be mounted"
            PRINT_TEXT1(0, 5, "sd card not found or cannot be mounted");
        }
        else
        {
            // Compute first game index
            uint8_t mid_page = MAX_GAME_ON_PAGE / 2;
            if (game_sel > mid_page)
                game_index = game_sel - mid_page;
            else
                game_index = 0;
            if (game_count > mid_page)
            {
                if (game_index > (game_count - mid_page))
                    game_index = (game_count - mid_page);
            }


            // Print game titles
            for (int i = 0; (i < MAX_GAME_ON_PAGE) && ((game_index + i) < game_count); i++)
            {
                PRINT_TEXT1(3, 3 + i, &(game_dir[game_index + i])[0]);
            }

            // Print selector
            PRINT_TEXT1(0, 3 + game_sel - game_index, "=>");

            // Nb games : "%d games :"
            PRINT_TEXT2(0, 15, "%d games", game_count);

            // Print buttons mapping
            PRINT_TEXT1(0, 16, "Press A for Smash Bros U mode (or Y to return to Home Menu)");
            PRINT_TEXT1(0, 17, "Press X for Mii Maker mode");

            // Check buttons
            if (!button_pressed)
            {
                if (vpad_data.btn_hold & BUTTON_UP  ) game_sel = (game_sel <= 0) ? game_count - 1 : game_sel - 1;
                if (vpad_data.btn_hold & BUTTON_DOWN) game_sel = ((game_sel + 1) % game_count);

                // Launch game
                if ((vpad_data.btn_hold & BUTTON_A) ||
                    (return_to_home = (vpad_data.btn_hold & BUTTON_Y)) ||
                    (mii_maker_mode = (vpad_data.btn_hold & BUTTON_X)))
                {
                    launching = 1;

                    // Create game folder path
                    char *cur_game_dir = game_dir[game_sel];
                    int len = strnlen(cur_game_dir, 0x1000);

                    // initialize the RPL/RPX table first entry to zero + 1 byte for name zero termination
                    // just in case no RPL/RPX are found, though it wont boot then anyway
                    memset(RPX_RPL_ARRAY, 0, sizeof(s_rpx_rpl) + 1);

                    // Create base folder string
                    path_index += __os_snprintf(&path[path_index], FS_MAX_MOUNTPATH_SIZE - path_index, "/%s%s", game_dir[game_sel], RPX_RPL_PATH);

                    // Open game folder
                    if (FSOpenDir(pClient, pCmd, path, &game_dh, FS_RET_ALL_ERROR) == FS_STATUS_OK) // /vol/external01/wiiu/[title]/code
                    {
                        // Look for rpx/rpl in the folder
                        FSDirEntry dir_entry;
                        int is_okay = 0;
                        int cur_entry = 0;
                        int rpx_found = 0;
                        char **rpl_imports = NULL;
                        int rpl_import_count = 0;
                        char rpx_name[FS_MAX_MOUNTPATH_SIZE];
                        memset(rpx_name, 0, sizeof(rpx_name));

                        // first find and pre-load RPX into memory
                        while (FSReadDir(pClient, pCmd, game_dh, &dir_entry, FS_RET_ALL_ERROR) == FS_STATUS_OK)
                        {
                            int is_rpx = IsRpxOrRpl(&dir_entry);
                            if (is_rpx != 1)
                                continue;

                            is_okay = Copy_RPX_RPL(pClient, pCmd, &dir_entry, path, 1, cur_entry, rpl_imports, rpl_import_count);
                            cur_entry++;

                            CreateGameSaveDir(pClient, pCmd, game_dir[game_sel], mount_path);
                            rpx_found = 1;
                            break;
                        }

                        // if the RPX is found rewind the whole DIR handler and start over loading all RPLs
                        if(rpx_found && is_okay)
                        {
                            // copy the RPX name
                            strlcpy(rpx_name, dir_entry.name, sizeof(rpx_name));
                            // rewind dir handler
                            FSRewindDir(pClient, pCmd, game_dh, FS_RET_NO_ERROR);
                            // read out all RPL imports on the first entry which is the pre-loaded RPX
                            rpl_import_count = GetRpxImports((s_rpx_rpl *)(RPX_RPL_ARRAY), &rpl_imports);

                            while (FSReadDir(pClient, pCmd, game_dh, &dir_entry, FS_RET_ALL_ERROR) == FS_STATUS_OK)
                            {
                                // Check if it is an rpx or rpl and copy it
                                int is_rpx = IsRpxOrRpl(&dir_entry);
                                if (is_rpx != 0)
                                    continue;

                                is_okay = Copy_RPX_RPL(pClient, pCmd, &dir_entry, path, 0, cur_entry, rpl_imports, rpl_import_count);
                                if (is_okay == 0)
                                    break;

                                cur_entry++;
                            }
                        }

                        // free memory of RPL imports
                        if(rpl_imports) {
                            int i;
                            for(i = 0; i < rpl_import_count; i++) {
                                if(rpl_imports[i])
                                    free(rpl_imports[i]);
                            }
                            free(rpl_imports);
                        }

                        // copy the game name to a place we know for later
                        memcpy(GAME_DIR_NAME, game_dir[game_sel], len+1);
                        DCFlushRange(GAME_DIR_NAME, len+1);

                        // Set ready to quit menu
                        ready = is_okay;

                        // Close dir
                        FSCloseDir(pClient, pCmd, game_dh, FS_RET_NO_ERROR);

                        if(ready){
                            LoadXmlParameters(&cosAppXmlInfoStruct, pClient, pCmd, rpx_name, path, path_index);

                            char acBuffer[200];
                            __os_snprintf(acBuffer, sizeof(acBuffer), "XML Launching Parameters");
                            log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
                            __os_snprintf(acBuffer, sizeof(acBuffer), "rpx_name:        %s", cosAppXmlInfoStruct.rpx_name);
                            log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
                            __os_snprintf(acBuffer, sizeof(acBuffer), "version_cos_xml: %i", cosAppXmlInfoStruct.version_cos_xml);
                            log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
                            __os_snprintf(acBuffer, sizeof(acBuffer), "os_version:      %08X%08X", (unsigned int)(cosAppXmlInfoStruct.os_version >> 32), (unsigned int)(cosAppXmlInfoStruct.os_version & 0xFFFFFFFF));
                            log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
                            __os_snprintf(acBuffer, sizeof(acBuffer), "title_id:        %08X%08X", (unsigned int)(cosAppXmlInfoStruct.title_id >> 32), (unsigned int)(cosAppXmlInfoStruct.title_id & 0xFFFFFFFF));
                            log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
                            __os_snprintf(acBuffer, sizeof(acBuffer), "app_type:        %08X", cosAppXmlInfoStruct.app_type);
                            log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
                            __os_snprintf(acBuffer, sizeof(acBuffer), "cmdFlags:        %08X", cosAppXmlInfoStruct.cmdFlags);
                            log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
                            __os_snprintf(acBuffer, sizeof(acBuffer), "max_size:        %08X", cosAppXmlInfoStruct.max_size);
                            log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
                            __os_snprintf(acBuffer, sizeof(acBuffer), "avail_size:      %08X", cosAppXmlInfoStruct.avail_size);
                            log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
                            __os_snprintf(acBuffer, sizeof(acBuffer), "codegen_size:    %08X", cosAppXmlInfoStruct.codegen_size);
                            log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
                            __os_snprintf(acBuffer, sizeof(acBuffer), "codegen_core:    %08X", cosAppXmlInfoStruct.codegen_core);
                            log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
                            __os_snprintf(acBuffer, sizeof(acBuffer), "max_codesize:    %08X", cosAppXmlInfoStruct.max_codesize);
                            log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
                            __os_snprintf(acBuffer, sizeof(acBuffer), "overlay_arena:   %08X", cosAppXmlInfoStruct.overlay_arena);
                            log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
                            __os_snprintf(acBuffer, sizeof(acBuffer), "sdk_version:     %i", cosAppXmlInfoStruct.sdk_version);
                            log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
                            __os_snprintf(acBuffer, sizeof(acBuffer), "title_version:   %08X", cosAppXmlInfoStruct.title_version);
                            log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
                        }
                    }
                }
            }
        }

        // Check launch
        if (launching)
        {
            if (ready)
            {
                // Set game launched
                GAME_LAUNCHED = 1;
                GAME_RPX_LOADED = 0;
                LOADIINE_MODE = mii_maker_mode ? LOADIINE_MODE_MII_MAKER : LOADIINE_MODE_SMASH_BROS;

                // check launcher mode
                if (LOADIINE_MODE == LOADIINE_MODE_SMASH_BROS) {
                    if (return_to_home == 0) {
                        // Launch smash bros disk without exiting to menu
                        char buf_vol_odd[20] = "/vol/storage_odd03";
                        _SYSLaunchTitleByPathFromLauncher(buf_vol_odd, 18, 0);
                    } else {
                        // Go to home menu
                        SYSLaunchMenu();
                    }
                }
                else {
                    // Restart mii maker
                    SYSRelaunchTitle(0, NULL);
                }
                break;
            }
            PRINT_TEXT1(45, 17, "Can't launch game!");
        }

        // Update screen
        if (first_pass) { OSScreenFlipBuffersEx(1); OSScreenClearBufferEx(1, 0); first_pass = 0; }

        // Check buttons
        if (vpad_data.btn_hold & BUTTON_HOME)
            break;

        // Button pressed ?
        button_pressed = (vpad_data.btn_hold & BTN_PRESSED) ? 1 : 0;
    }

    /* ****************************************************************** */
    /*                            Free Memory                             */
    /* ****************************************************************** */
    // free each game name pointer
    for(j = 0; j < MAX_GAME_COUNT; j++) {
        if(game_dir[j])
            free(game_dir[j]);
    }
    // free game list pointer array
    free(game_dir);
    // free path strings
    free(mount_path);
    free(path);
    free(msg);

    /* ****************************************************************** */
    /*                            Unmount SD Card                         */
    /* ****************************************************************** */
//    if (sd_status == 1)
//    {
//        FSUnmount(pClient, pCmd, mount_path, FS_RET_NO_ERROR);
//    }

    /* ****************************************************************** */
    /*                           Return to app start                      */
    /* ****************************************************************** */

    return main(argc, argv);
}

/* IsRpxOrRpl */
static int IsRpxOrRpl(FSDirEntry *dir_entry)
{
	char *cPtr = dir_entry->name;
	int len = 0;
	while(*cPtr != 0) {
		cPtr++;
		len++;
	}
	if(len < 4) {
		return -1;
	}
	int val = *(int*)(dir_entry->name + len - 4);
	if (val == 0x2e727078) // ".rpx"
		return 1;
	if (val == 0x2e72706c) // ".rpl"
		return 0;

    return -1;
}

static void Add_RPX_RPL_Entry(const char *name, int offset, int size, int is_rpx, int entry_index, s_mem_area* area){
    // fill rpx/rpl entry
    s_rpx_rpl * rpx_rpl_data = (s_rpx_rpl *)(RPX_RPL_ARRAY);
    // get to last entry
    while(rpx_rpl_data->next) {
        rpx_rpl_data = rpx_rpl_data->next;
    }

    // setup next entry on the previous one (only if it is not the first entry)
    if(entry_index > 0) {
        rpx_rpl_data->next = (s_rpx_rpl *)( ((unsigned int)rpx_rpl_data) + sizeof(s_rpx_rpl) + strnlen(rpx_rpl_data->name, 0x1000) + 1 );
        rpx_rpl_data = rpx_rpl_data->next;
    }

    // setup current entry
    rpx_rpl_data->area = area;
    rpx_rpl_data->size = size;
    rpx_rpl_data->offset = offset;
    rpx_rpl_data->is_rpx = is_rpx;
    rpx_rpl_data->next = 0;
    // copy string length + 0 termination
    memcpy(rpx_rpl_data->name, name, strnlen(name, 0x1000) + 1);

    char acBuffer[200];
    __os_snprintf(acBuffer, sizeof(acBuffer), "%s: loaded into 0x%08X, offset: 0x%08X, size: 0x%08X", name, area->address, offset, size);
    log_string(bss.global_sock, acBuffer, BYTE_LOG_STR);
}

/* Copy_RPX_RPL */
static int Copy_RPX_RPL(FSClient *pClient, FSCmdBlock *pCmd, FSDirEntry *dir_entry, char *path, int is_rpx, int entry_index, char **rpl_imports, int rpl_import_count)
{
    // Open rpl file
    int fd = 0;
    char* path_game = (char*)malloc(FS_MAX_MOUNTPATH_SIZE);
    if (!path_game)
        return 0;

    // Concatenate rpl filename
    __os_snprintf(path_game, FS_MAX_MOUNTPATH_SIZE, "%s/%s", path, dir_entry->name);

    // For RPLs :
    if(!is_rpx)
    {
        int preload = 0;
        int i;
        for(i = 0; i < rpl_import_count; i++)
        {
            if(strncasecmp(dir_entry->name, rpl_imports[i], strlen(dir_entry->name) - 4) == 0)
            {
                // file is in the fimports section and therefore needs to be preloaded
                preload = 1;
                break;
            }
        }
        // if we dont need to preload, just add it to the array
        if(!preload)
        {
            // fill rpl entry
            Add_RPX_RPL_Entry(dir_entry->name, 0, 0, is_rpx, entry_index, (s_mem_area*)(MEM_AREA_ARRAY));

            // free path
            free(path_game);
            return 1;
        }
    }

    // For RPX : load file from sdcard and fill memory with the rpx data
    if (FSOpenFile(pClient, pCmd, path_game, "r", &fd, FS_RET_ALL_ERROR) == FS_STATUS_OK)
    {
        int cur_size = 0;
        int ret = 0;

        // malloc mem for read file
        char* data = (char*)malloc(0x1000);

        // this is the initial area
        s_mem_area* mem_area    = (s_mem_area*)(MEM_AREA_ARRAY);
        int mem_area_addr_start = mem_area->address;
        int mem_area_addr_end   = mem_area->address + mem_area->size;
        int mem_area_offset     = 0;

        // on RPLs we need to find the free area we can store data to (at least RPX was already loaded by this point)
        if(!is_rpx)
        {
            s_rpx_rpl *rpl_struct = (s_rpx_rpl*)(RPX_RPL_ARRAY);
            while(rpl_struct != 0)
            {
                // check if this entry was loaded into memory
                if(rpl_struct->size == 0) {
                    // see if we find entries behind this one that was pre-loaded
                    rpl_struct = rpl_struct->next;
                    // entry was not loaded into memory -> skip it
                    continue;
                }

                // this entry has been loaded to memory, remember it's area
                mem_area = rpl_struct->area;

                int rpl_size = rpl_struct->size;
                int rpl_offset = rpl_struct->offset;
                // find the end of the entry and switch between areas if needed
                while((rpl_offset + rpl_size) >= mem_area->size)
                {
                    rpl_size -= mem_area->size - rpl_offset;
                    rpl_offset = 0;
                    mem_area = mem_area->next;
                }

                // set new start, end and memory area offset
                mem_area_addr_start = mem_area->address;
                mem_area_addr_end   = mem_area->address + mem_area->size;
                mem_area_offset     = rpl_offset + rpl_size;

                // see if we find entries behind this one that was pre-loaded
                rpl_struct = rpl_struct->next;
            }
        }

        // Get current memory area limits
        s_mem_area* mem_area_rpl_start = mem_area;
        int mem_area_offset_rpl_start = mem_area_offset;

        // Copy rpl in memory : 22 MB max
        while ((ret = FSReadFile(pClient, pCmd, data, 0x1, 0x1000, fd, 0, FS_RET_ALL_ERROR)) > 0)
        {
            // Copy in memory and save offset
            for (int j = 0; j < ret; j++)
            {
                if ((mem_area_addr_start + mem_area_offset) >= mem_area_addr_end)
                {
                    // Set next memory area
                    mem_area            = mem_area->next;
                    mem_area_addr_start = mem_area->address;
                    mem_area_addr_end   = mem_area->address + mem_area->size;
                    mem_area_offset     = 0;
                }
                *(volatile unsigned char*)(mem_area_addr_start + mem_area_offset) = data[j];
                mem_area_offset += 1;
            }
            cur_size += ret;
        }

        // remember which RPX has to be checked for on loader for allowing the list compare
        if(is_rpx) {
            RPX_CHECK_NAME = *(unsigned int*)dir_entry->name;
        }

        // fill rpx entry
        Add_RPX_RPL_Entry(dir_entry->name, mem_area_offset_rpl_start, cur_size, is_rpx, entry_index, mem_area_rpl_start);

        // close file and free memory
        FSCloseFile(pClient, pCmd, fd, FS_RET_NO_ERROR);
        free(data);
        free(path_game);

        // return okay
        return 1;
    }

    // free path
    free(path_game);

    return 0;
}

/* CreateGameSaveDir - TODO: FSMakeDir and FSGetStat seem to crash if path exists/not exists, investigate. For now open and close to detect existing path */
static void CreateGameSaveDir(FSClient *pClient, FSCmdBlock *pCmd, const char *game_entry, char* mount_path)
{
   char* path_save = (char*)malloc(FS_MAX_MOUNTPATH_SIZE + 255);

   if (!path_save)
       return;

   // Copy path
   memcpy(path_save, mount_path, FS_MAX_MOUNTPATH_SIZE);

   // Create "_SAV" folder
   int path_index = 0;
   while (path_save[path_index])
       path_index++;
   path_save[path_index++] = '/';
   path_save[path_index++] = 'w';
   path_save[path_index++] = 'i';
   path_save[path_index++] = 'i';
   path_save[path_index++] = 'u';
   path_save[path_index++] = '/';
   path_save[path_index++] = 's';
   path_save[path_index++] = 'a';
   path_save[path_index++] = 'v';
   path_save[path_index++] = 'e';
   path_save[path_index++] = 's';
   path_save[path_index] = '\0';

    int dir_handler = 0;

   if (FSOpenDir(pClient, pCmd, path_save, &dir_handler, FS_RET_ALL_ERROR) == FS_STATUS_OK) {
       FSCloseDir(pClient, pCmd, dir_handler, FS_RET_NO_ERROR);
   }
   else {
	   FSMakeDir(pClient, pCmd, path_save, FS_RET_ALL_ERROR);
   }
   // Create "_SAV/[rpxx]" folder
   path_save[path_index++] = '/';
   while(*game_entry) {
	   path_save[path_index++] = *game_entry++;
   }
   path_save[path_index] = '\0';

   if (FSOpenDir(pClient, pCmd, path_save, &dir_handler, FS_RET_ALL_ERROR) == FS_STATUS_OK) {
       FSCloseDir(pClient, pCmd, dir_handler, FS_RET_NO_ERROR);
   }
   else {
	   FSMakeDir(pClient, pCmd, path_save, FS_RET_ALL_ERROR);
   }

   // Create "_SAV/[rpx]/u" and "_SAV/[rpx]/c" folder
   path_save[path_index++] = '/';
   path_save[path_index] = 'u';
   path_save[path_index + 1] = '\0';

   if (FSOpenDir(pClient, pCmd, path_save, &dir_handler, FS_RET_ALL_ERROR) == FS_STATUS_OK) {
       FSCloseDir(pClient, pCmd, dir_handler, FS_RET_NO_ERROR);
   }
   else {
	   FSMakeDir(pClient, pCmd, path_save, FS_RET_ALL_ERROR);
   }

   path_save[path_index] = 'c';

   if (FSOpenDir(pClient, pCmd, path_save, &dir_handler, FS_RET_ALL_ERROR) == FS_STATUS_OK) {
       FSCloseDir(pClient, pCmd, dir_handler, FS_RET_NO_ERROR);
   }
   else {
	   FSMakeDir(pClient, pCmd, path_save, FS_RET_ALL_ERROR);
   }

   free(path_save);
}

/* Create memory areas arrays */
static void GenerateMemoryAreasTable()
{
    static const int mem_vals[][2] =
    {
        // TODO: Check which of those areas are usable
//        {0xB8000000 + 0x000DCC9C, 0xB8000000 + 0x00174F80}, // 608 kB
//        {0xB8000000 + 0x00180B60, 0xB8000000 + 0x001C0A00}, // 255 kB
//        {0xB8000000 + 0x001ECE9C, 0xB8000000 + 0x00208CC0}, // 111 kB
//        {0xB8000000 + 0x00234180, 0xB8000000 + 0x0024B444}, // 92 kB
//        {0xB8000000 + 0x0024D8C0, 0xB8000000 + 0x0028D884}, // 255 kB
//        {0xB8000000 + 0x003A745C, 0xB8000000 + 0x004D2B68}, // 1197 kB
//        {0xB8000000 + 0x004D77B0, 0xB8000000 + 0x00502200}, // 170 kB
//        {0xB8000000 + 0x005B3A88, 0xB8000000 + 0x005C6870}, // 75 kB
//        {0xB8000000 + 0x0061F3E4, 0xB8000000 + 0x00632B04}, // 77 kB
//        {0xB8000000 + 0x00639790, 0xB8000000 + 0x00649BC4}, // 65 kB
//        {0xB8000000 + 0x00691490, 0xB8000000 + 0x006B3CA4}, // 138 kB
//        {0xB8000000 + 0x006D7BCC, 0xB8000000 + 0x006EEB84}, // 91 kB
//        {0xB8000000 + 0x00704E44, 0xB8000000 + 0x0071E3C4}, // 101 kB
//        {0xB8000000 + 0x0073B684, 0xB8000000 + 0x0074C184}, // 66 kB
//        {0xB8000000 + 0x00751354, 0xB8000000 + 0x00769784}, // 97 kB
//        {0xB8000000 + 0x008627DC, 0xB8000000 + 0x00872904}, // 64 kB
//        {0xB8000000 + 0x008C1E98, 0xB8000000 + 0x008EB0A0}, // 164 kB
//        {0xB8000000 + 0x008EEC30, 0xB8000000 + 0x00B06E98}, // 2144 kB
//        {0xB8000000 + 0x00B06EC4, 0xB8000000 + 0x00B930C4}, // 560 kB
//        {0xB8000000 + 0x00BA1868, 0xB8000000 + 0x00BC22A4}, // 130 kB
//        {0xB8000000 + 0x00BC48F8, 0xB8000000 + 0x00BDEC84}, // 104 kB
//        {0xB8000000 + 0x00BE3DC0, 0xB8000000 + 0x00C02284}, // 121 kB
//        {0xB8000000 + 0x00C02FC8, 0xB8000000 + 0x00C19924}, // 90 kB
//        {0xB8000000 + 0x00C2D35C, 0xB8000000 + 0x00C3DDC4}, // 66 kB
//        {0xB8000000 + 0x00C48654, 0xB8000000 + 0x00C6E2E4}, // 151 kB
//        {0xB8000000 + 0x00D04E04, 0xB8000000 + 0x00D36938}, // 198 kB
//        {0xB8000000 + 0x00DC88AC, 0xB8000000 + 0x00E14288}, // 302 kB
//        {0xB8000000 + 0x00E21ED4, 0xB8000000 + 0x00EC8298}, // 664 kB
//        {0xB8000000 + 0x00EDDC7C, 0xB8000000 + 0x00F7C2A8}, // 633 kB
//        {0xB8000000 + 0x00F89EF4, 0xB8000000 + 0x010302B8}, // 664 kB
//        {0xB8000000 + 0x01030800, 0xB8000000 + 0x013F69A0}, // 3864 kB
//        {0xB8000000 + 0x016CE000, 0xB8000000 + 0x016E0AA0}, // 74 kB
//        {0xB8000000 + 0x0170200C, 0xB8000000 + 0x018B9C58}, // 1759 kB
//        {0xB8000000 + 0x01F17658, 0xB8000000 + 0x01F6765C}, // 320 kB
//        {0xB8000000 + 0x01F6779C, 0xB8000000 + 0x01FB77A0}, // 320 kB
//        {0xB8000000 + 0x01FB78E0, 0xB8000000 + 0x020078E4}, // 320 kB
//        {0xB8000000 + 0x02007A24, 0xB8000000 + 0x02057A28}, // 320 kB
//        {0xB8000000 + 0x02057B68, 0xB8000000 + 0x021B957C}, // 1414 kB
//        {0xB8000000 + 0x02891528, 0xB8000000 + 0x028C8A28}, // 221 kB
//        {0xB8000000 + 0x02BBCC4C, 0xB8000000 + 0x02CB958C}, // 1010 kB
//        {0xB8000000 + 0x0378D45C, 0xB8000000 + 0x03855464}, // 800 kB
//        {0xB8000000 + 0x0387800C, 0xB8000000 + 0x03944938}, // 818 kB
//        {0xB8000000 + 0x03944A08, 0xB8000000 + 0x03956E0C}, // 73 kB
//        {0xB8000000 + 0x04A944A4, 0xB8000000 + 0x04ABAAC0}, // 153 kB
//        {0xB8000000 + 0x04ADE370, 0xB8000000 + 0x0520EAB8}, // 7361 kB      // ok
//        {0xB8000000 + 0x053B966C, 0xB8000000 + 0x058943C4}, // 4971 kB      // ok
//        {0xB8000000 + 0x058AD3D8, 0xB8000000 + 0x06000000}, // 7499 kB
//        {0xB8000000 + 0x0638D320, 0xB8000000 + 0x063B0280}, // 139 kB
//        {0xB8000000 + 0x063C39E0, 0xB8000000 + 0x063E62C0}, // 138 kB
//        {0xB8000000 + 0x063F52A0, 0xB8000000 + 0x06414A80}, // 125 kB
//        {0xB8000000 + 0x06422810, 0xB8000000 + 0x0644B2C0}, // 162 kB
//        {0xB8000000 + 0x064E48D0, 0xB8000000 + 0x06503EC0}, // 125 kB
//        {0xB8000000 + 0x0650E360, 0xB8000000 + 0x06537080}, // 163 kB
//        {0xB8000000 + 0x0653A460, 0xB8000000 + 0x0655C300}, // 135 kB
//        {0xB8000000 + 0x0658AA40, 0xB8000000 + 0x065BC4C0}, // 198 kB       // ok
//        {0xB8000000 + 0x065E51A0, 0xB8000000 + 0x06608E80}, // 143 kB       // ok
//        {0xB8000000 + 0x06609ABC, 0xB8000000 + 0x07F82C00}, // 26084 kB     // ok

//        {0xC0000000 + 0x000DCC9C, 0xC0000000 + 0x00180A00}, // 655 kB
//        {0xC0000000 + 0x00180B60, 0xC0000000 + 0x001C0A00}, // 255 kB
//        {0xC0000000 + 0x001F5EF0, 0xC0000000 + 0x00208CC0}, // 75 kB
//        {0xC0000000 + 0x00234180, 0xC0000000 + 0x0024B444}, // 92 kB
//        {0xC0000000 + 0x0024D8C0, 0xC0000000 + 0x0028D884}, // 255 kB
//        {0xC0000000 + 0x003A745C, 0xC0000000 + 0x004D2B68}, // 1197 kB
//        {0xC0000000 + 0x006D3334, 0xC0000000 + 0x00772204}, // 635 kB
//        {0xC0000000 + 0x00789C60, 0xC0000000 + 0x007C6000}, // 240 kB
//        {0xC0000000 + 0x00800000, 0xC0000000 + 0x01E20000}, // 22876 kB     // ok


        {0xC0000000 + 0x00800000, 0xC0000000 + 0x01E20000}, // 22876 kB     // ok
        {0xB8000000 + 0x06609ABC, 0xB8000000 + 0x07F82C00}, // 26084 kB     // ok
        {0xB8000000 + 0x01030800, 0xB8000000 + 0x013F69A0}, // 3864 kB
        {0xB8000000 + 0x008EEC30, 0xB8000000 + 0x00B06E98}, // 2144 kB
        {0xB8000000 + 0x053B966C, 0xB8000000 + 0x058943C4}, // 4971 kB
        {0xB8000000 + 0x04ADE370, 0xB8000000 + 0x0520EAB8}, // 7361 kB

        {0, 0}
    }; // total : 66mB + 25mB

    // Fill entries
    int i = 0;
    while (mem_vals[i][0])
    {
        AddMemoryArea(mem_vals[i][0], mem_vals[i][1], i);
        i++;
    }
}

static void AddMemoryArea(int start, int end, int cur_index)
{
    // Create and copy new memory area
    s_mem_area *mem_area = (s_mem_area *) (MEM_AREA_ARRAY);
    mem_area[cur_index].address = start;
    mem_area[cur_index].size    = end - start;
    mem_area[cur_index].next    = 0;

    // Fill pointer to this area in the previous area
    if (cur_index > 0)
    {
        mem_area[cur_index - 1].next = &mem_area[cur_index];
    }
}

static int game_name_cmp(const void *game1, const void *game2)
{
    // the pointers passed here are pointers to the string pointer so dereference them
    const char *game_name_1 = *((const char **)game1);
    const char *game_name_2 = *((const char **)game2);
    // compare strings case insensitive
    return strncasecmp(game_name_1, game_name_2, FS_MAX_FULLPATH_SIZE);
}

// this macro generates the the magic entry
#define MAKE_MAGIC(x, y, call_type) { x, y, call_type }
// A kind of magic used to store :
// - replace instruction address
// - replace instruction
const struct magic_t {
    const void * repl_func;           // our replacement function which is called
    const unsigned int repl_addr;     // address where to place the jump to the our function
    const unsigned int call_type;     // call type, e.g. 0x48000000 for branch and 0x48000001 for branch link
} menu_methods[] __attribute__((section(".menu_magic"))) = {
    /* Patch coreinit - on 5.3.2 coreinit.rpl starts at 0x101c400 */
    { Menu_Main, 0x0101c55c, 0x48000003 },           // bla Branch Link Adress
};
