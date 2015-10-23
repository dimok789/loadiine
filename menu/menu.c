#include "menu.h"
#include "../common/common.h"

/* Utils for the display */
#define PRINT_TEXT1(x, y, str) { OSScreenPutFontEx(1, x, y, str); }
#define PRINT_TEXT2(x, y, _fmt, ...) { __os_snprintf(msg, 80, _fmt, __VA_ARGS__); OSScreenPutFontEx(1, x, y, msg); }
#define BTN_PRESSED (BUTTON_LEFT | BUTTON_RIGHT | BUTTON_UP | BUTTON_DOWN | BUTTON_A | BUTTON_B)

/* Static function definitions */
static int IsRPX(FSDirEntry *dir_entry);
static int Copy_RPX_RPL(FSClient *pClient, FSCmdBlock *pCmd, FSDirEntry *dir_entry, char *path, int path_index, int is_rpx, int entry_index, int *cur_mem_address);
static void CreateGameSaveDir(FSClient *pClient, FSCmdBlock *pCmd, const char *dir_entry, char* mount_path);

/* Entry point */
int _start(int argc, char *argv[]) {
    /* ****************************************************************** */
    /*                              MENU CHECK                            */
    /* ****************************************************************** */

    // mii maker is our menu :), if it's another app, continue coreinit process
    if (title_id != 0x000500101004A200 && // mii maker eur
        title_id != 0x000500101004A100 && // mii maker usa
        title_id != 0x000500101004A000)   // mii maker jpn
        return main(argc, argv);

    /* ****************************************************************** */
    /*                              Init Memory                           */
    /* ****************************************************************** */

    char* msg           = (char*)malloc(80);
    char* mount_path    = (char*)malloc(FS_MAX_MOUNTPATH_SIZE);
    char* path          = (char*)malloc(FS_MAX_MOUNTPATH_SIZE);

    char game_dir[MAX_GAME_COUNT][FS_MAX_ENTNAME_SIZE];

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
                        memcpy(game_dir[game_count], dir_entry.name, FS_MAX_ENTNAME_SIZE);
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
    uint8_t ready = 0;
    int     error;

    VPADData vpad_data;
    VPADRead(0, &vpad_data, 1, &error); //Read initial vpad status
    while (1)
    {
        // Refresh screen if needed
        if (button_pressed) { OSScreenFlipBuffersEx(1); OSScreenClearBufferEx(1, 0); }

        // Read vpad
        VPADRead(0, &vpad_data, 1, &error);

        // Title : "-- LOADINE --"
        PRINT_TEXT1(24, 1, "-- LOADIINE --");

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

            // Nb games : "%d games :"
            PRINT_TEXT2(0, 17, "%d games", game_count);

            // Print game titles
            for (int i = 0; (i < MAX_GAME_ON_PAGE) && ((game_index + i) < game_count); i++)
            {
                PRINT_TEXT1(3, 3 + i, &(game_dir[game_index + i])[0]);
            }

            // Print selector
            PRINT_TEXT1(0, 3 + game_sel - game_index, "=>");

            // Check buttons
            if (!button_pressed)
            {
                if (vpad_data.btn_hold & BUTTON_UP  ) game_sel = (game_sel <= 0) ? game_count - 1 : game_sel - 1;
                if (vpad_data.btn_hold & BUTTON_DOWN) game_sel = ((game_sel + 1) % game_count);

                // Launch game
                if (vpad_data.btn_hold & BUTTON_A)
                {
                    launching = 1;

                    // Create game folder path
                    char *cur_game_dir = game_dir[game_sel];
                    int len = 0;
                    while (cur_game_dir[len])
                        len++;

                    // Create base folder string
                    path_index += __os_snprintf(&path[path_index], FS_MAX_MOUNTPATH_SIZE - path_index, "/%s%s", game_dir[game_sel], RPX_RPL_PATH);

                    // Open game folder
                    if (FSOpenDir(pClient, pCmd, path, &game_dh, FS_RET_ALL_ERROR) == FS_STATUS_OK) // /vol/external01/_RPC/[title]/
                    {
                        // Look for rpx/rpl in the folder
                        FSDirEntry dir_entry;
                        int is_okay = 0;
                        int cur_rpl = 0;
                        int cur_mem_address = (int)MEM_BASE;
                        while (FSReadDir(pClient, pCmd, game_dh, &dir_entry, FS_RET_ALL_ERROR) == FS_STATUS_OK)
                        {
                            // Check if it is an rpx or rpl and copy it
                            int is_rpx = IsRPX(&dir_entry);
                            if (is_rpx == -1)
                                continue;

                            if (is_rpx)
                            {
                                is_okay = Copy_RPX_RPL(pClient, pCmd, &dir_entry, path, path_index, 1, 0, &cur_mem_address);
                                CreateGameSaveDir(pClient, pCmd, game_dir[game_sel], mount_path);
                            }
                            else
                            {
                                cur_rpl++;
                                is_okay = Copy_RPX_RPL(pClient, pCmd, &dir_entry, path, path_index, 0, cur_rpl, &cur_mem_address);
                            }
                            if (is_okay == 0)
                                break;
                        }

                        // Save the number of rpx/rpl
                        *(volatile unsigned int*)(RPX_RPL_ENTRY_COUNT) = 1 + cur_rpl;

						// copy the game name to a place we know for later
						memcpy(GAME_DIR_NAME, game_dir[game_sel], len+1);
						DCFlushRange(GAME_DIR_NAME, len+1);

                        // Set ready to quit menu
                        ready = is_okay;

                        // Close dir
                        FSCloseDir(pClient, pCmd, game_dh, FS_RET_NO_ERROR);
                    }
                }
            }
        }

        // Check launch
        if (launching)
        {
            if (ready) {
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

/* IsRPX */
static int IsRPX(FSDirEntry *dir_entry)
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

/* Copy_RPX_RPL */
static int Copy_RPX_RPL(FSClient *pClient, FSCmdBlock *pCmd, FSDirEntry *dir_entry, char *path, int path_index, int is_rpx, int entry_index, int *cur_mem_address)
{
    // Open rpl file
    int fd = 0;
    char buf_mode[3] = {'r', '\0' };
    char* path_game = (char*)malloc(FS_MAX_MOUNTPATH_SIZE);
    if (!path_game)
        return 0;

    // Copy path
    memcpy(path_game, path, FS_MAX_MOUNTPATH_SIZE);

    // Get rpx/rpl filename length
    int len = 0;
    while (dir_entry->name[len])
        len++;

    // Concatenate rpl filename
    path_game[path_index++] = '/';
    memcpy(&(path_game[path_index]), dir_entry->name, len);
    path_index += len;
    path_game[path_index++] = '\0';

    if(!is_rpx)
    {
        // fill rpx/rpl entry
        s_rpx_rpl rpx_rpl_data;
		if(len > sizeof(rpx_rpl_data.name)-1) {
			len = sizeof(rpx_rpl_data.name)-1;
		}

        memset(&rpx_rpl_data, 0, sizeof(s_rpx_rpl));
        rpx_rpl_data.address = (int)MEM_BASE;
        rpx_rpl_data.size = 0;
        memcpy(rpx_rpl_data.name, dir_entry->name, len);

        // copy rpx/rpl entry
        memcpy(RPX_RPL_ARRAY + entry_index * sizeof(s_rpx_rpl), &rpx_rpl_data, sizeof(s_rpx_rpl));

        // free path
        free(path_game);
        return 1;
    }

    if (FSOpenFile(pClient, pCmd, path_game, buf_mode, &fd, FS_RET_ALL_ERROR) == FS_STATUS_OK)
    {
        int cur_size = 0;
        int ret = 0;

        // malloc mem for read file
        char* data = (char*)malloc(0x1000);

        // Copy rpl in memory : 22 MB max
        while ((ret = FSReadFile(pClient, pCmd, data, 0x1, 0x1000, fd, 0, FS_RET_ALL_ERROR)) > 0)
        {
            // Copy in memory and save offset
            for (int j = 0; j < ret; j++)
                *(volatile unsigned char*)(*cur_mem_address + cur_size + j) = data[j];
            cur_size += ret;
        }

        // fill rpx/rpl entry
        s_rpx_rpl rpx_rpl_data;
		if(len > sizeof(rpx_rpl_data.name)-1) {
			len = sizeof(rpx_rpl_data.name)-1;
		}

        memset(&rpx_rpl_data, 0, sizeof(s_rpx_rpl));
        rpx_rpl_data.address = (int)MEM_BASE;
        rpx_rpl_data.size = cur_size;
        memcpy(rpx_rpl_data.name, dir_entry->name, len);

        // copy rpx/rpl entry
        memcpy(RPX_RPL_ARRAY + entry_index * sizeof(s_rpx_rpl), &rpx_rpl_data, sizeof(s_rpx_rpl));

        // update current mem address
        *(volatile unsigned int *)cur_mem_address = *cur_mem_address + cur_size + 4;

        // if rpx, set pending rpx name
        if (is_rpx)
        {
            // Get rpx name, always 4 characters !!!
            uRpxName rpx;
            rpx.name[0] = dir_entry->name[0];
            rpx.name[1] = dir_entry->name[1];
            rpx.name[2] = dir_entry->name[2];
            rpx.name[3] = dir_entry->name[3];
            rpx.name[4] = 0;

            *(volatile unsigned int*)(RPX_NAME_PENDING) = rpx.name_full;
            *(volatile unsigned int*)(RPX_NAME) = 0;
        }

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
