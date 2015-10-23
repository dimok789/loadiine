#include "fs.h"
#include "exception_handler.h"
#include "../common/common.h"

#define USE_EXTRA_LOG_FUNCTIONS   1

#define DECL(res, name, ...) \
        extern res name(__VA_ARGS__); \
        res (* real_ ## name)(__VA_ARGS__)  __attribute__((section(".magicptr"))); \
        res my_ ## name(__VA_ARGS__)

/* Log functions */
//static const struct {
//    const int tag;
//    const char *name;
//} tag_names[] = {
//    { 1, "LOADER_Start" },
//    { 2, "LOADER_Entry" },
//    { 3, "LOADER_Prep" },
//    { 4, "LiLoadRPLBasics_in_1_load" },
//    { 5, "GetNextBounce_1" },
//    { 6, "GetNextBounce_2" },
//    { 7, "After_GetBounce" },
//    { 8, "GetNextBounce result end" },
//};
//
//static const char *get_name_for_tag(int tag) {
//    int i = 0;
//    for(i = 0; i < sizeof(tag_names) / sizeof(tag_names[0]); i++)
//        if(tag == tag_names[i].tag)
//            return tag_names[i].name;
//
//    return "unknown";
//}

/* Common useful functions */
static inline int toupper(int c) {
    return (c >= 'a' && c <= 'z') ? (c - 0x20) : c;
}

static int strlen(const char* path) {
    int i = 0;
    while (path[i])
        i++;
    return i;
}

/* Client functions */
static int client_num_alloc(void *pClient) {
    int i;

    for (i = 0; i < MAX_CLIENT; i++)
        if (bss.pClient_fs[i] == pClient) {
            return i;
        } else if (bss.pClient_fs[i] == 0) {
            bss.pClient_fs[i] = pClient;
            return i;
        }
    return -1;
}
static void client_num_free(int client) {
    bss.pClient_fs[client] = 0;
    bss.sd_mount[client] = 0;
}
static int client_num(void *pClient) {
    int i;
    for (i = 0; i < MAX_CLIENT; i++)
        if (bss.pClient_fs[i] == pClient)
            return i;
    return -1;
}

static int is_gamefile(char *path) {
    // In case the path starts by "//" and not "/" (some games do that ... ...)
    if (path[0] == '/' && path[1] == '/')
        path = &path[1];

    // In case the path does not start with "/" (some games do that too ...)
    int len = 0;
    char new_path[16];
    if(path[0] != '/') {
        new_path[0] = '/';
        len++;
    }

    while(*path && len < sizeof(new_path)) {
        new_path[len++] = *path++;
    }

    /* Note : no need to check everything, it is faster this way */
    if (new_path[0]  != '/') return 0;
    if (new_path[1]  != 'v' && new_path[1] != 'V') return 0;
//    if (new_path[2]  != 'o') return 0;
//    if (new_path[3]  != 'l') return 0;
//    if (new_path[4]  != '/') return 0;
    if (new_path[5]  != 'c' && new_path[5] != 'C') return 0;
//    if (new_path[6]  != 'o') return 0;
//    if (new_path[7]  != 'n') return 0;
//    if (new_path[8]  != 't') return 0;
//    if (new_path[9]  != 'e') return 0;
//    if (new_path[10] != 'n') return 0;
    if (new_path[11] != 't' && new_path[11] != 'T') return 0;

    return 1;
}
static int is_savefile(char *path) {

    // In case the path starts by "//" and not "/" (some games do that ... ...)
    if (path[0] == '/' && path[1] == '/')
        path = &path[1];

    // In case the path does not start with "/" (some games do that too ...)
    int len = 0;
    char new_path[16];
    if(path[0] != '/') {
        new_path[0] = '/';
        len++;
    }

    while(*path && len < sizeof(new_path)) {
        new_path[len++] = *path++;
    }

    /* Note : no need to check everything, it is faster this way */
    if (new_path[0]  != '/') return 0;
//    if (new_path[1]  != 'v' && new_path[1] != 'V') return 0;
//    if (new_path[2]  != 'o') return 0;
//    if (new_path[3]  != 'l') return 0;
    if (new_path[4]  != '/') return 0;
    if (new_path[5]  != 's' && new_path[5] != 'S') return 0;
//    if (new_path[6]  != 'a') return 0;
//    if (new_path[7]  != 'v') return 0;
    if (new_path[8]  != 'e' && new_path[8] != 'E') return 0;

    return 1;
}

static void compute_new_path(char* new_path, char* path, int len, int is_save) {
    int i, n, path_offset = 0;

    // In case the path starts by "//" and not "/" (some games do that ... ...)
    if (path[0] == '/' && path[1] == '/')
        path = &path[1];

    // In case the path does not start with "/" set an offset for all the accesses
	if(path[0] != '/')
		path_offset = -1;

    if (!is_save) {
        for (n = 0; n < sizeof(bss.mount_base) && bss.mount_base[n] != 0; n++) {
            new_path[n] = bss.mount_base[n];
        }

        // copy the content file path with slash at the beginning
        for (i = 0; i < (len - 12 - path_offset); i++) {
            char cChar = path[12 + i + path_offset];
            // skip double slashes
            if((new_path[n-1] == '/') && (cChar == '/')) {
                continue;
            }
            new_path[n++] = cChar;
        }

        new_path[n++] = '\0';
    }
    else {
        for (n = 0; n < sizeof(bss.save_base) && bss.save_base[n] != 0; n++)
            new_path[n] = bss.save_base[n];
        new_path[n++] = '/';

        // Create path for common and user dirs
        if (path[10 + path_offset] == 'c') // common dir ("common")
        {
            new_path[n++] = 'c';

            // copy the save game filename now with the slash at the beginning
            for (i = 0; i < (len - 16 - path_offset); i++) {
                char cChar = path[16 + path_offset + i];
                // skip double slashes
                if((new_path[n-1] == '/') && (cChar == '/')) {
                    continue;
                }
                new_path[n++] = cChar;
            }
        }
        else if (path[10 + path_offset] == '8') // user dir ("800000??") ?? = user permanent id
        {
            new_path[n++] = 'u';

            // copy the save game filename now with the slash at the beginning
            for (i = 0; i < (len - 18 - path_offset); i++) {
               char cChar = path[18 + path_offset + i];
                // skip double slashes
                if((new_path[n-1] == '/') && (cChar == '/')) {
                    continue;
                }
                new_path[n++] = cChar;
            }
        }
        new_path[n++] = '\0';
    }
}

static int GetCurClient(void *pClient, void *pCmd) {
    if ((int)bss_ptr != 0x0a000000) {
        int client = client_num(pClient);
        if (client >= 0) {
            // mount sd card in the same time
            if (!bss.sd_mount[client])
                bss.sd_mount[client] = fs_mount_sd(bss.socket_fs[client], pClient, pCmd);
            return client;
        }
    }
    return -1;
}

static int GetCurClientNoSD(void *pClient) {
    if ((int)bss_ptr != 0x0a000000) {
        int client = client_num(pClient);
        if (client >= 0) {
            return client;
        }
    }
    return -1;
}

// Async
typedef void (*FSAsyncCallback)(void *pClient, void *pCmd, int result, void *context);
typedef struct
{
    FSAsyncCallback userCallback;
    void            *userContext;
    void            *ioMsgQueue;
} FSAsyncParams;

/* *****************************************************************************
 * Base functions
 * ****************************************************************************/
DECL(int, FSInit, void) {
    if ((int)bss_ptr == 0x0a000000) {
        bss_ptr = memalign(sizeof(struct bss_t), 0x40);
        memset(bss_ptr, 0, sizeof(struct bss_t));

        // setup exceptions
        setup_os_exceptions();

        // create game mount path prefix
        __os_snprintf(bss.mount_base, sizeof(bss.mount_base), "%s%s/%s%s", CAFE_OS_SD_PATH, SD_GAMES_PATH, (char *)GAME_DIR_NAME, CONTENT_PATH);
        // create game save path prefix
        __os_snprintf(bss.save_base, sizeof(bss.save_base), "%s%s/%s", CAFE_OS_SD_PATH, SD_SAVES_PATH, (char *)GAME_DIR_NAME);

        fs_connect(&bss.global_sock);
    }
    return real_FSInit();
}
DECL(int, FSShutdown, void) {
    if ((int)bss_ptr != 0x0a000000) {
        fs_disconnect(bss.global_sock);
    }
    return real_FSShutdown();
}

DECL(int, FSAddClientEx, void *r3, void *r4, void *r5) {
    int res = real_FSAddClientEx(r3, r4, r5);

    if ((int)bss_ptr != 0x0a000000 && res >= 0) {
        if (*(int*)(RPX_NAME) != 0) {
            int client = client_num_alloc(r3);
            if (client >= 0) {
                if (fs_connect(&bss.socket_fs[client]) != 0)
                    client_num_free(client);
            }
        }
    }

    return res;
}
DECL(int, FSDelClient, void *pClient) {
    if ((int)bss_ptr != 0x0a000000) {
        int client = client_num(pClient);
        if (client >= 0) {
            fs_disconnect(bss.socket_fs[client]);
            client_num_free(client);
        }
    }

    return real_FSDelClient(pClient);
}

// TODO: make new_path dynamically allocated from heap and not on stack to avoid stack overflow on too long names
// int len = strlen(path) + (is_save ? (strlen(bss.save_base) + 8) : strlen(bss.mount_base));
/* *****************************************************************************
 * Replacement functions
 * ****************************************************************************/
DECL(int, FSGetStat, void *pClient, void *pCmd, char *path, void *stats, int error) {
    int client = GetCurClient(pClient, pCmd);
    if (client != -1) {
        // log
        log_string(bss.socket_fs[client], path, BYTE_STAT);

        // change path if it is a game file
        int is_save = 0;
        if (is_gamefile(path) || (is_save = is_savefile(path))) {
            int len = strlen(path);
            int len_base = (is_save ? (strlen(bss.save_base) + 8) : strlen(bss.mount_base));
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, is_save);
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);

            // return function with new_path if path exists
            return real_FSGetStat(pClient, pCmd, new_path, stats, error);
        }
    }
    return real_FSGetStat(pClient, pCmd, path, stats, error);
}

DECL(int, FSGetStatAsync, void *pClient, void *pCmd, char *path, void *stats, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient, pCmd);
    if (client != -1) {
        // log
        log_string(bss.socket_fs[client], path, BYTE_STAT_ASYNC);

        // change path if it is a game/save file
        int is_save = 0;
        if (is_gamefile(path) || (is_save = is_savefile(path))) {
            int len = strlen(path);
            int len_base = (is_save ? (strlen(bss.save_base) + 8) : strlen(bss.mount_base));
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, is_save);
            // log new path
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);
            return real_FSGetStatAsync(pClient, pCmd, new_path, stats, error, asyncParams);
        }
    }
    return real_FSGetStatAsync(pClient, pCmd, path, stats, error, asyncParams);
}

DECL(int, FSOpenFile, void *pClient, void *pCmd, char *path, char *mode, int *handle, int error) {
    int client = GetCurClient(pClient, pCmd);
    if (client != -1) {
        // log
        log_string(bss.socket_fs[client], path, BYTE_OPEN_FILE);

        // change path if it is a game file
        int is_save = 0;
        if (is_gamefile(path) || (is_save = is_savefile(path))) {
            int len = strlen(path);
            int len_base = (is_save ? (strlen(bss.save_base) + 8) : strlen(bss.mount_base));
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, is_save);
            // log new path
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);
            return real_FSOpenFile(pClient, pCmd, new_path, mode, handle, error);
        }
    }

    return real_FSOpenFile(pClient, pCmd, path, mode, handle, error);
}

DECL(int, FSOpenFileAsync, void *pClient, void *pCmd, char *path, char *mode, int *handle, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient, pCmd);
    if (client != -1) {
        // log
        log_string(bss.socket_fs[client], path, BYTE_OPEN_FILE_ASYNC);

        // change path if it is a game file
        int is_save = 0;
        if (is_gamefile(path) || (is_save = is_savefile(path))) {
            int len = strlen(path);
            int len_base = (is_save ? (strlen(bss.save_base) + 8) : strlen(bss.mount_base));
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, is_save);
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);
            return real_FSOpenFileAsync(pClient, pCmd, new_path, mode, handle, error, asyncParams);
        }
    }
    return real_FSOpenFileAsync(pClient, pCmd, path, mode, handle, error, asyncParams);
}

DECL(int, FSOpenDir, void *pClient, void* pCmd, char *path, int *handle, int error) {
    int client = GetCurClient(pClient, pCmd);
    if (client != -1) {
        // log
        log_string(bss.socket_fs[client], path, BYTE_OPEN_DIR);

        // change path if it is a game folder
        int is_save = 0;
        if (is_gamefile(path) || (is_save = is_savefile(path))) {
            int len = strlen(path);
            int len_base = (is_save ? (strlen(bss.save_base) + 8) : strlen(bss.mount_base));
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, is_save);
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);
            return real_FSOpenDir(pClient, pCmd, new_path, handle, error);
        }
    }
    return real_FSOpenDir(pClient, pCmd, path, handle, error);
}

DECL(int, FSOpenDirAsync, void *pClient, void* pCmd, char *path, int *handle, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient, pCmd);
    if (client != -1) {
        // log
        log_string(bss.socket_fs[client], path, BYTE_OPEN_DIR_ASYNC);

        // change path if it is a game folder
        int is_save = 0;
        if (is_gamefile(path) || (is_save = is_savefile(path))) {
            int len = strlen(path);
            int len_base = (is_save ? (strlen(bss.save_base) + 8) : strlen(bss.mount_base));
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, is_save);
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);
            return real_FSOpenDirAsync(pClient, pCmd, new_path, handle, error, asyncParams);
        }
    }
    return real_FSOpenDirAsync(pClient, pCmd, path, handle, error, asyncParams);
}

DECL(int, FSChangeDir, void *pClient, void *pCmd, char *path, int error) {
    int client = GetCurClient(pClient, pCmd);
    if (client != -1) {
        // log
        log_string(bss.socket_fs[client], path, BYTE_CHANGE_DIR);

        // change path if it is a game folder
        int is_save = 0;
        if (is_gamefile(path) || (is_save = is_savefile(path))) {
            int len = strlen(path);
            int len_base = (is_save ? (strlen(bss.save_base) + 8) : strlen(bss.mount_base));
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, is_save);
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);
            return real_FSChangeDir(pClient, pCmd, new_path, error);
        }
    }
    return real_FSChangeDir(pClient, pCmd, path, error);
}

DECL(int, FSChangeDirAsync, void *pClient, void *pCmd, char *path, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient, pCmd);
    if (client != -1) {
        // log
        log_string(bss.socket_fs[client], path, BYTE_CHANGE_DIR_ASYNC);

        // change path if it is a game folder
        int is_save = 0;
        if (is_gamefile(path) || (is_save = is_savefile(path))) {
            int len = strlen(path);
            int len_base = (is_save ? (strlen(bss.save_base) + 8) : strlen(bss.mount_base));
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, is_save);
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);
            return real_FSChangeDirAsync(pClient, pCmd, new_path, error, asyncParams);
        }
    }
    return real_FSChangeDirAsync(pClient, pCmd, path, error, asyncParams);
}

// only for saves on sdcard
DECL(int, FSMakeDir, void *pClient, void *pCmd, char *path, int error) {
    int client = GetCurClient(pClient, pCmd);
    if (client != -1) {
        // log
        log_string(bss.socket_fs[client], path, BYTE_MAKE_DIR);

        // change path if it is a save folder
        if (is_savefile(path)) {
            int len = strlen(path);
            int len_base = (strlen(bss.save_base) + 8);
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, 1);

            // log new path
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);

            return real_FSMakeDir(pClient, pCmd, new_path, error);
        }
    }
    return real_FSMakeDir(pClient, pCmd, path, error);
}

// only for saves on sdcard
DECL(int, FSMakeDirAsync, void *pClient, void *pCmd, char *path, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient, pCmd);
    if (client != -1) {
        // log
        log_string(bss.socket_fs[client], path, BYTE_MAKE_DIR_ASYNC);

        // change path if it is a save folder
        if (is_savefile(path)) {
            int len = strlen(path);
            int len_base = (strlen(bss.save_base) + 8);
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, 1);

            // log new path
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);

            return real_FSMakeDirAsync(pClient, pCmd, new_path, error, asyncParams);
        }
    }
    return real_FSMakeDirAsync(pClient, pCmd, path, error, asyncParams);
}

// only for saves on sdcard
DECL(int, FSRename, void *pClient, void *pCmd, char *oldPath, char *newPath, int error) {
    int client = GetCurClient(pClient, pCmd);
    if (client != -1) {
        // log
        log_string(bss.socket_fs[client], oldPath, BYTE_RENAME);
        log_string(bss.socket_fs[client], newPath, BYTE_RENAME);

        // change path if it is a save folder
        if (is_savefile(oldPath)) {
            // old path
            int len_base = (strlen(bss.save_base) + 8);
            int len_old = strlen(oldPath);
            char new_old_path[len_old + len_base + 1];
            compute_new_path(new_old_path, oldPath, len_old, 1);

            // new path
            int len_new = strlen(newPath);
            char new_new_path[len_new + len_base + 1];
            compute_new_path(new_new_path, newPath, len_new, 1);

            // log new path
            log_string(bss.socket_fs[client], new_old_path, BYTE_LOG_STR);
            log_string(bss.socket_fs[client], new_new_path, BYTE_LOG_STR);

            return real_FSRename(pClient, pCmd, new_old_path, new_new_path, error);
        }
    }
    return real_FSRename(pClient, pCmd, oldPath, newPath, error);
}

// only for saves on sdcard
DECL(int, FSRenameAsync, void *pClient, void *pCmd, char *oldPath, char *newPath, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient, pCmd);
    if (client != -1) {
        // log
        log_string(bss.socket_fs[client], oldPath, BYTE_RENAME);
        log_string(bss.socket_fs[client], newPath, BYTE_RENAME);

        // change path if it is a save folder
        if (is_savefile(oldPath)) {
            // old path
            int len_base = (strlen(bss.save_base) + 8);
            int len_old = strlen(oldPath);
            char new_old_path[len_old + len_base + 1];
            compute_new_path(new_old_path, oldPath, len_old, 1);

            // new path
            int len_new = strlen(newPath);
            char new_new_path[len_new + len_base + 1];
            compute_new_path(new_new_path, newPath, len_new, 1);

            // log new path
            log_string(bss.socket_fs[client], new_old_path, BYTE_LOG_STR);
            log_string(bss.socket_fs[client], new_new_path, BYTE_LOG_STR);

            return real_FSRenameAsync(pClient, pCmd, new_old_path, new_new_path, error, asyncParams);
        }
    }
    return real_FSRenameAsync(pClient, pCmd, oldPath, newPath, error, asyncParams);
}

// only for saves on sdcard
DECL(int, FSRemove, void *pClient, void *pCmd, char *path, int error) {
    int client = GetCurClient(pClient, pCmd);
    if (client != -1) {
        // log
        log_string(bss.socket_fs[client], path, BYTE_REMOVE);

        // change path if it is a save folder
        if (is_savefile(path)) {
            int len = strlen(path);
            int len_base = (strlen(bss.save_base) + 8);
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, 1);

            // log new path
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);

            return real_FSRemove(pClient, pCmd, new_path, error);
        }
    }
    return real_FSRemove(pClient, pCmd, path, error);
}

// only for saves on sdcard
DECL(int, FSRemoveAsync, void *pClient, void *pCmd, char *path, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient, pCmd);
    if (client != -1) {
        // log
        log_string(bss.socket_fs[client], path, BYTE_REMOVE);

        // change path if it is a save folder
        if (is_savefile(path)) {
            int len = strlen(path);
            int len_base = (strlen(bss.save_base) + 8);
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, 1);

            // log new path
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);

            return real_FSRemoveAsync(pClient, pCmd, new_path, error, asyncParams);
        }
    }
    return real_FSRemoveAsync(pClient, pCmd, path, error, asyncParams);
}

/* *****************************************************************************
 * Dynamic RPL loading
 * ****************************************************************************/
static int CheckAndLoadRPL(const char *rpl) {
    // If we are in Smash Bros app
    if (*(volatile unsigned int *)RPX_NAME_PENDING == 0)
        return 0;

    // Look for rpl name in our table
    for (int k = 1; k < (*(volatile unsigned int *)(RPX_RPL_ENTRY_COUNT)); k++)
    {
        s_rpx_rpl *rpl_struct = (s_rpx_rpl*)(RPX_RPL_ARRAY + k * sizeof(s_rpx_rpl));

        int len = strlen(rpl);
        int len2 = strlen(rpl_struct->name);
        if ((len != len2) && (len != (len2 - 4))) {
            continue;
        }

        int found = 1;
        for (int x = 0; x < len; x++)
        {
            if (toupper(rpl_struct->name[x]) != toupper(rpl[x]))
            {
                found = 0;
                break;
            }
        }

        if (found)
        {
            if ((int)bss_ptr != 0x0a000000)
            {
                char buffer[200];
                __os_snprintf(buffer, sizeof(buffer), "CheckAndLoadRPL(%s) found and loading", rpl);
                log_string(bss.global_sock, buffer, BYTE_LOG_STR);
            }

            // initialize FS
            FSInit();
            FSClient *pClient = (FSClient*) MEMAllocFromDefaultHeap(sizeof(FSClient));
            if (!pClient)
                return 0;

            FSCmdBlock *pCmd = (FSCmdBlock*) MEMAllocFromDefaultHeap(sizeof(FSCmdBlock));
            if (!pCmd)
            {
                MEMFreeToDefaultHeap(pClient);
                return 0;
            }

            // calculate path length for SD access of RPL
            int path_len = strlen(CAFE_OS_SD_PATH) + strlen(SD_GAMES_PATH) + strlen((char *)GAME_DIR_NAME) + strlen(RPX_RPL_PATH) + strlen(rpl_struct->name) + 3;
            char *path_rpl = MEMAllocFromDefaultHeap(path_len);
            if(!path_rpl) {
                MEMFreeToDefaultHeap(pCmd);
                MEMFreeToDefaultHeap(pClient);
                return 0;
            }
            // create path
            __os_snprintf(path_rpl, path_len, "%s%s/%s%s/%s", CAFE_OS_SD_PATH, SD_GAMES_PATH, (char *)GAME_DIR_NAME, RPX_RPL_PATH, rpl_struct->name);

            // do more initial FS stuff
            FSInitCmdBlock(pCmd);
            FSAddClientEx(pClient, 0, FS_RET_NO_ERROR);

            int fd = 0;
            if (real_FSOpenFile(pClient, pCmd, path_rpl, "r", &fd, FS_RET_ALL_ERROR) == FS_STATUS_OK)
            {
                // malloc mem for read file
                unsigned char* dataBuf = (unsigned char*)MEMAllocFromDefaultHeapEx(0x1000, 0x40);
                int ret;
                // Copy rpl in memory : 22 MB max
                while ((ret = FSReadFile(pClient, pCmd, dataBuf, 0x1, 0x1000, fd, 0, FS_RET_ALL_ERROR)) > 0)
                {
                    // Copy in memory and save offset
                    for (int j = 0; j < ret; j++)
                        *(volatile unsigned char*)(((unsigned int)MEM_BASE) + rpl_struct->size + j) = dataBuf[j];
                    rpl_struct->size += ret;
                }
                // reset offset
                rpl_struct->offset = 0;
                rpl_struct->address = (int)MEM_BASE;

                // flush memory
                // DCFlushRange((void*)rpl_struct, sizeof(s_rpx_rpl));
                // DCFlushRange((void*)rpl_struct->address, rpl_struct->size);

                if ((int)bss_ptr != 0x0a000000)
                {
                    char buffer[200];
                    __os_snprintf(buffer, sizeof(buffer), "CheckAndLoadRPL(%s) file loaded 0x%08X %i", rpl, rpl_struct->address, rpl_struct->size);
                    log_string(bss.global_sock, buffer, BYTE_LOG_STR);
                }

                FSCloseFile(pClient, pCmd, fd, FS_RET_NO_ERROR);
                MEMFreeToDefaultHeap(dataBuf);
            }

            FSDelClient(pClient);
            MEMFreeToDefaultHeap(pCmd);
            MEMFreeToDefaultHeap(pClient);
            MEMFreeToDefaultHeap(path_rpl);
            return 1;
        }
    }

    return 0;
}

DECL(int, OSDynLoad_Acquire, char* rpl, unsigned int *handle, int r5 __attribute__((unused))) {
    // Get only the filename (in case there is folders in the module name ... like zombiu)
    char *ptr = rpl;
    while(*ptr) {
        if (*ptr == '/') {
            rpl = ptr + 1;
        }
        ptr++;
    }

    // Look if RPL is in our table and load it from SD Card
    CheckAndLoadRPL(rpl);

    int result = real_OSDynLoad_Acquire(rpl, handle, 0);
    if ((int)bss_ptr != 0x0a000000)
    {
        char buffer[200];
        __os_snprintf(buffer, sizeof(buffer), "OSDynLoad_Acquire: %s result: %i", rpl, result);
        log_string(bss.global_sock, buffer, BYTE_LOG_STR);
    }

    return result;
}

DECL(int, OSDynLoad_GetModuleName, unsigned int *handle, char *name_buffer, int *name_buffer_size) {

    int result = real_OSDynLoad_GetModuleName(handle, name_buffer, name_buffer_size);
    if ((int)bss_ptr != 0x0a000000)
    {
        char buffer[200];
        __os_snprintf(buffer, sizeof(buffer), "OSDynLoad_GetModuleName: %s result %i", (name_buffer && result == 0) ? name_buffer : "NULL", result);
        log_string(bss.global_sock, buffer, BYTE_LOG_STR);
    }

    return result;
}

DECL(int, OSDynLoad_IsModuleLoaded, char* rpl, unsigned int *handle, int r5 __attribute__((unused))) {

    int result = real_OSDynLoad_IsModuleLoaded(rpl, handle, 1);
    if ((int)bss_ptr != 0x0a000000)
    {
        char buffer[200];
        __os_snprintf(buffer, sizeof(buffer), "OSDynLoad_IsModuleLoaded: %s result %i", rpl, result);
        log_string(bss.global_sock, buffer, BYTE_LOG_STR);
    }

    return result;
}

/* *****************************************************************************
 * Log functions
 * ****************************************************************************/
#if (USE_EXTRA_LOG_FUNCTIONS == 1)
static void log_byte_for_client(void *pClient, char byte) {
    int client = GetCurClientNoSD(pClient);
    if (client != -1) {
        log_byte(bss.socket_fs[client], byte);
    }
}

DECL(int, FSCloseFile_log, void *pClient, void *pCmd, int fd, int error) {
    log_byte_for_client(pClient, BYTE_CLOSE_FILE);
    return real_FSCloseFile_log(pClient, pCmd, fd, error);
}
DECL(int, FSCloseDir_log, void *pClient, void *pCmd, int fd, int error) {
    log_byte_for_client(pClient, BYTE_CLOSE_DIR);
    return real_FSCloseDir_log(pClient, pCmd, fd, error);
}
DECL(int, FSFlushFile_log, void *pClient, void *pCmd, int fd, int error) {
    log_byte_for_client(pClient, BYTE_FLUSH_FILE);
    return real_FSFlushFile_log(pClient, pCmd, fd, error);
}
DECL(int, FSGetErrorCodeForViewer_log, void *pClient, void *pCmd) {
    log_byte_for_client(pClient, BYTE_GET_ERROR_CODE_FOR_VIEWER);
    return real_FSGetErrorCodeForViewer_log(pClient, pCmd);
}
DECL(int, FSGetLastError_log, void *pClient) {
    log_byte_for_client(pClient, BYTE_GET_LAST_ERROR);
    return real_FSGetLastError_log(pClient);
}
DECL(int, FSGetPosFile_log, void *pClient, void *pCmd, int fd, int *pos, int error) {
    log_byte_for_client(pClient, BYTE_GET_POS_FILE);
    return real_FSGetPosFile_log(pClient, pCmd, fd, pos, error);
}
DECL(int, FSGetStatFile_log, void *pClient, void *pCmd, int fd, void *buffer, int error) {
    log_byte_for_client(pClient, BYTE_GET_STAT_FILE);
    return real_FSGetStatFile_log(pClient, pCmd, fd, buffer, error);
}
DECL(int, FSIsEof_log, void *pClient, void *pCmd, int fd, int error) {
    log_byte_for_client(pClient, BYTE_EOF);
    return real_FSIsEof_log(pClient, pCmd, fd, error);
}
DECL(int, FSReadDir_log, void *pClient, void *pCmd, int fd, void *dir_entry, int error) {
    log_byte_for_client(pClient, BYTE_READ_DIR);
    return real_FSReadDir_log(pClient, pCmd, fd, dir_entry, error);
}
DECL(int, FSReadFile_log, void *pClient, void *pCmd, void *buffer, int size, int count, int fd, int flag, int error) {
    log_byte_for_client(pClient, BYTE_READ_FILE);
    return real_FSReadFile_log(pClient, pCmd, buffer, size, count, fd, flag, error);
}
DECL(int, FSReadFileWithPos_log, void *pClient, void *pCmd, void *buffer, int size, int count, int pos, int fd, int flag, int error) {
    log_byte_for_client(pClient, BYTE_READ_FILE_WITH_POS);
    return real_FSReadFileWithPos_log(pClient, pCmd, buffer, size, count, pos, fd, flag, error);
}
DECL(int, FSSetPosFile_log, void *pClient, void *pCmd, int fd, int pos, int error) {
    log_byte_for_client(pClient, BYTE_SET_POS_FILE);
    return real_FSSetPosFile_log(pClient, pCmd, fd, pos, error);
}
DECL(void, FSSetStateChangeNotification_log, void *pClient, int r4) {
    log_byte_for_client(pClient, BYTE_SET_STATE_CHG_NOTIF);
    real_FSSetStateChangeNotification_log(pClient, r4);
}
DECL(int, FSTruncateFile_log, void *pClient, void *pCmd, int fd, int error) {
    log_byte_for_client(pClient, BYTE_TRUNCATE_FILE);
    return real_FSTruncateFile_log(pClient, pCmd, fd, error);
}
DECL(int, FSWriteFile_log, void *pClient, void *pCmd, const void *source, int size, int count, int fd, FSFlag flag, int error) {
    log_byte_for_client(pClient, BYTE_WRITE_FILE);
    return real_FSWriteFile_log(pClient, pCmd, source, size, count, fd, flag, error);
}
DECL(int, FSWriteFileWithPos_log, void *pClient, void *pCmd, const void *source, int size, int count, int pos, int fd, FSFlag flag, int error) {
    log_byte_for_client(pClient, BYTE_WRITE_FILE_WITH_POS);
    return real_FSWriteFileWithPos_log(pClient, pCmd, source, size, count, pos, fd, flag, error);
}
#endif

/* *****************************************************************************
 * Creates function pointer array
 * ****************************************************************************/
#define MAKE_MAGIC(x) { x, my_ ## x, &real_ ## x }

struct magic_t {
    const void *real;
    const void *replacement;
    const void *call;
} methods[] __attribute__((section(".magic"))) = {
    // Common FS functions
    MAKE_MAGIC(FSInit),
    MAKE_MAGIC(FSShutdown),
    MAKE_MAGIC(FSAddClientEx),
    MAKE_MAGIC(FSDelClient),

    // Replacement functions
    MAKE_MAGIC(FSGetStat),
    MAKE_MAGIC(FSGetStatAsync),
    MAKE_MAGIC(FSOpenFile),
    MAKE_MAGIC(FSOpenFileAsync),
    MAKE_MAGIC(FSOpenDir),
    MAKE_MAGIC(FSOpenDirAsync),
    MAKE_MAGIC(FSChangeDir),
    MAKE_MAGIC(FSChangeDirAsync),
    MAKE_MAGIC(FSMakeDir),
    MAKE_MAGIC(FSMakeDirAsync),
    MAKE_MAGIC(FSRename),
    MAKE_MAGIC(FSRenameAsync),
    MAKE_MAGIC(FSRemove),
    MAKE_MAGIC(FSRemoveAsync),

    // Dynamic RPL loading functions
    MAKE_MAGIC(OSDynLoad_Acquire),
    MAKE_MAGIC(OSDynLoad_GetModuleName),
    MAKE_MAGIC(OSDynLoad_IsModuleLoaded),

    // Log functions
#if (USE_EXTRA_LOG_FUNCTIONS == 1)
    MAKE_MAGIC(FSCloseFile_log),
    MAKE_MAGIC(FSCloseDir_log),
    MAKE_MAGIC(FSFlushFile_log),
    MAKE_MAGIC(FSGetErrorCodeForViewer_log),
    MAKE_MAGIC(FSGetLastError_log),
    MAKE_MAGIC(FSGetPosFile_log),
    MAKE_MAGIC(FSGetStatFile_log),
    MAKE_MAGIC(FSIsEof_log),
    MAKE_MAGIC(FSReadDir_log),
    MAKE_MAGIC(FSReadFile_log),
    MAKE_MAGIC(FSReadFileWithPos_log),
    MAKE_MAGIC(FSSetPosFile_log),
    MAKE_MAGIC(FSSetStateChangeNotification_log),
    MAKE_MAGIC(FSTruncateFile_log),
    MAKE_MAGIC(FSWriteFile_log),
    MAKE_MAGIC(FSWriteFileWithPos_log),
#endif
};
