#include "fs.h"
#include "../common/common.h"
#include "../utils/exception_handler.h"
#include "../utils/logger.h"
#include "../utils/strings.h"
#include "../utils/utils.h"

#define USE_EXTRA_LOG_FUNCTIONS   0

#define DECL(res, name, ...) \
        extern res name(__VA_ARGS__); \
        res (* real_ ## name)(__VA_ARGS__)  __attribute__((section(".magicptr"))); \
        res my_ ## name(__VA_ARGS__)

/* Prototypes */
int FSAddClientEx(void *r3, void *r4, void *r5);
int FSDelClient(void *pClient);

/* Client functions */
static int client_num_alloc(void *pClient) {
    int i;

    for (i = 0; i < MAX_CLIENT; i++)
        if (bss.pClient_fs[i] == pClient) {
            return i;
        }

    for (i = 0; i < MAX_CLIENT; i++)
        if (bss.pClient_fs[i] == 0) {
            bss.pClient_fs[i] = pClient;
            return i;
        }
    return -1;
}
static void client_num_free(int client) {
    bss.pClient_fs[client] = 0;
}
static int client_num(void *pClient) {
    int i;
    for (i = 0; i < MAX_CLIENT; i++)
        if (bss.pClient_fs[i] == pClient)
            return i;
    return -1;
}

static int is_gamefile(const char *path) {
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
    if (strncasecmp(new_path, "/vol/content", 12) == 0)
        return 1;

    return 0;
}
static int is_savefile(const char *path) {

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

    if (strncasecmp(new_path, "/vol/save", 9) == 0)
        return 1;

    return 0;
}

static void compute_new_path(char* new_path, const char* path, int len, int is_save) {
    int i, n, path_offset = 0;

    // In case the path starts by "//" and not "/" (some games do that ... ...)
    if (path[0] == '/' && path[1] == '/')
        path = &path[1];

    // In case the path does not start with "/" set an offset for all the accesses
	if(path[0] != '/')
		path_offset = -1;

    // some games are doing /vol/content/./....
    if(path[13 + path_offset] == '.' && path[14 + path_offset] == '/') {
        path_offset += 2;
    }

    if (!is_save) {
        n = strlcpy(new_path, bss.mount_base, sizeof(bss.mount_base));

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
        n = strlcpy(new_path, bss.save_base, sizeof(bss.save_base));
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

static int GetCurClient(void *pClient) {
    if ((int)bss_ptr != 0x0a000000) {
        int client = client_num(pClient);
        if (client >= 0) {
            return client;
        }
    }
    return -1;
}
#include "../common/kernel_defs.h"

DECL(int, FSInit, void) {
    if ((int)bss_ptr == 0x0a000000) {
        // allocate memory for our stuff
        void *mem_ptr = memalign(sizeof(struct bss_t), 0x40);
        if(!mem_ptr)
            return real_FSInit();

        // copy pointer
        bss_ptr = mem_ptr;
        memset(bss_ptr, 0, sizeof(struct bss_t));

        // setup exceptions
        setup_os_exceptions();

        // create game mount path prefix
        __os_snprintf(bss.mount_base, sizeof(bss.mount_base), "%s%s/%s%s", CAFE_OS_SD_PATH, SD_GAMES_PATH, GAME_DIR_NAME, CONTENT_PATH);
        // create game save path prefix
        __os_snprintf(bss.save_base, sizeof(bss.save_base), "%s%s/%s", CAFE_OS_SD_PATH, SD_SAVES_PATH, GAME_DIR_NAME);

        logger_connect(&bss.global_sock);

        // Call real FSInit
        int result = real_FSInit();

        // Mount sdcard
        FSClient *pClient = (FSClient*) MEMAllocFromDefaultHeap(sizeof(FSClient));
        if (!pClient)
            return 0;

        FSCmdBlock *pCmd = (FSCmdBlock*) MEMAllocFromDefaultHeap(sizeof(FSCmdBlock));
        if (!pCmd)
        {
            MEMFreeToDefaultHeap(pClient);
            return 0;
        }

        FSInitCmdBlock(pCmd);
        FSAddClientEx(pClient, 0, FS_RET_NO_ERROR);

        fs_mount_sd(bss.global_sock, pClient, pCmd);

        FSDelClient(pClient);
        MEMFreeToDefaultHeap(pCmd);
        MEMFreeToDefaultHeap(pClient);

        return result;
    }
    return real_FSInit();
}

DECL(int, FSShutdown, void) {
    if ((int)bss_ptr != 0x0a000000) {
        logger_disconnect(bss.global_sock);
        bss.global_sock = -1;
    }
    return real_FSShutdown();
}

DECL(int, FSAddClientEx, void *r3, void *r4, void *r5) {
    int res = real_FSAddClientEx(r3, r4, r5);

    if ((int)bss_ptr != 0x0a000000 && res >= 0) {
        if (GAME_RPX_LOADED != 0) {
            int client = client_num_alloc(r3);
            if (client >= 0) {
                if (logger_connect(&bss.socket_fs[client]) != 0)
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
            logger_disconnect(bss.socket_fs[client]);
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
DECL(int, FSGetStat, FSClient *pClient, FSCmdBlock *pCmd, const char *path, FSStat *stats, FSRetFlag error) {
    int client = GetCurClient(pClient);
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

DECL(int, FSGetStatAsync, void *pClient, void *pCmd, const char *path, void *stats, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient);
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

DECL(FSStatus, FSOpenFile, FSClient *pClient, FSCmdBlock *pCmd, const char *path, const char *mode, int *handle, FSRetFlag error) {
/*
    int client = GetCurClient(pClient);
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
*/
    return real_FSOpenFile(pClient, pCmd, path, mode, handle, error);
}

DECL(int, FSOpenFileAsync, void *pClient, void *pCmd, const char *path, const char *mode, int *handle, int error, const FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient);
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

DECL(int, FSOpenDir, FSClient *pClient, FSCmdBlock* pCmd, const char *path, int *handle, FSRetFlag error) {
    int client = GetCurClient(pClient);
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

DECL(int, FSOpenDirAsync, void *pClient, void* pCmd, const char *path, int *handle, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient);
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

DECL(FSStatus, FSChangeDir, FSClient *pClient, FSCmdBlock *pCmd, const char *path, FSRetFlag error) {
    int client = GetCurClient(pClient);
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

DECL(int, FSChangeDirAsync, void *pClient, void *pCmd, const char *path, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient);
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
DECL(FSStatus, FSMakeDir, FSClient *pClient, FSCmdBlock *pCmd, const char *path, FSRetFlag error) {
    int client = GetCurClient(pClient);
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
DECL(int, FSMakeDirAsync, void *pClient, void *pCmd, const char *path, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient);
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
DECL(int, FSRename, void *pClient, void *pCmd, const char *oldPath, const char *newPath, int error) {
    int client = GetCurClient(pClient);
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
DECL(int, FSRenameAsync, void *pClient, void *pCmd, const char *oldPath, const char *newPath, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient);
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
DECL(int, FSRemove, void *pClient, void *pCmd, const char *path, int error) {
    int client = GetCurClient(pClient);
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
DECL(int, FSRemoveAsync, void *pClient, void *pCmd, const char *path, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient);
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

DECL(int, FSFlushQuota, void *pClient, void *pCmd, const char* path, int error) {
    int client = GetCurClient(pClient);
    if (client != -1) {
        // log
        //log_string(bss.socket_fs[client], path, BYTE_REMOVE);
        char buffer[200];
        __os_snprintf(buffer, sizeof(buffer), "FSFlushQuota: %s", path);
        log_string(bss.global_sock, buffer, BYTE_LOG_STR);

        // change path if it is a save folder
        if (is_savefile(path)) {
            int len = strlen(path);
            int len_base = (strlen(bss.save_base) + 8);
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, 1);

            // log new path
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);

            return real_FSFlushQuota(pClient, pCmd, new_path, error);
        }
    }
    return real_FSFlushQuota(pClient, pCmd, path, error);
}
DECL(int, FSFlushQuotaAsync, void *pClient, void *pCmd, const char *path, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient);
    if (client != -1) {
        // log
        //log_string(bss.socket_fs[client], path, BYTE_REMOVE);
        char buffer[200];
        __os_snprintf(buffer, sizeof(buffer), "FSFlushQuotaAsync: %s", path);
        log_string(bss.global_sock, buffer, BYTE_LOG_STR);

        // change path if it is a save folder
        if (is_savefile(path)) {
            int len = strlen(path);
            int len_base = (strlen(bss.save_base) + 8);
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, 1);

            // log new path
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);

            return real_FSFlushQuotaAsync(pClient, pCmd, new_path, error, asyncParams);
        }
    }
    return real_FSFlushQuotaAsync(pClient, pCmd, path, error, asyncParams);
}

DECL(int, FSGetFreeSpaceSize, void *pClient, void *pCmd, const char *path, uint64_t *returnedFreeSize, int error) {
    int client = GetCurClient(pClient);
    if (client != -1) {
        // log
        //log_string(bss.socket_fs[client], path, BYTE_REMOVE);
        char buffer[200];
        __os_snprintf(buffer, sizeof(buffer), "FSGetFreeSpaceSize: %s", path);
        log_string(bss.global_sock, buffer, BYTE_LOG_STR);

        // change path if it is a save folder
        if (is_savefile(path)) {
            int len = strlen(path);
            int len_base = (strlen(bss.save_base) + 8);
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, 1);

            // log new path
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);

            return real_FSGetFreeSpaceSize(pClient, pCmd, new_path, returnedFreeSize, error);
        }
    }
    return real_FSGetFreeSpaceSize(pClient, pCmd, path, returnedFreeSize, error);
}
DECL(int, FSGetFreeSpaceSizeAsync, void *pClient, void *pCmd, const char *path, uint64_t *returnedFreeSize, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient);
    if (client != -1) {
        // log
        //log_string(bss.socket_fs[client], path, BYTE_REMOVE);
        char buffer[200];
        __os_snprintf(buffer, sizeof(buffer), "FSGetFreeSpaceSizeAsync: %s", path);
        log_string(bss.global_sock, buffer, BYTE_LOG_STR);

        // change path if it is a save folder
        if (is_savefile(path)) {
            int len = strlen(path);
            int len_base = (strlen(bss.save_base) + 8);
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, 1);

            // log new path
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);

            return real_FSGetFreeSpaceSizeAsync(pClient, pCmd, new_path, returnedFreeSize, error, asyncParams);
        }
    }
    return real_FSGetFreeSpaceSizeAsync(pClient, pCmd, path, returnedFreeSize, error, asyncParams);
}

DECL(int, FSRollbackQuota, void *pClient, void *pCmd, const char *path, int error) {
    int client = GetCurClient(pClient);
    if (client != -1) {
        // log
        char buffer[200];
        __os_snprintf(buffer, sizeof(buffer), "FSRollbackQuota: %s", path);
        log_string(bss.global_sock, buffer, BYTE_LOG_STR);

        // change path if it is a save folder
        if (is_savefile(path)) {
            int len = strlen(path);
            int len_base = (strlen(bss.save_base) + 8);
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, 1);

            // log new path
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);

            return real_FSRollbackQuota(pClient, pCmd, new_path, error);
        }
    }
    return real_FSRollbackQuota(pClient, pCmd, path, error);
}
DECL(int, FSRollbackQuotaAsync, void *pClient, void *pCmd, const char *path, int error, FSAsyncParams *asyncParams) {
    int client = GetCurClient(pClient);
    if (client != -1) {
        // log
        char buffer[200];
        __os_snprintf(buffer, sizeof(buffer), "FSRollbackQuotaAsync: %s", path);
        log_string(bss.global_sock, buffer, BYTE_LOG_STR);

        // change path if it is a save folder
        if (is_savefile(path)) {
            int len = strlen(path);
            int len_base = (strlen(bss.save_base) + 8);
            char new_path[len + len_base + 1];
            compute_new_path(new_path, path, len, 1);

            // log new path
            log_string(bss.socket_fs[client], new_path, BYTE_LOG_STR);

            return real_FSRollbackQuotaAsync(pClient, pCmd, new_path, error, asyncParams);
        }
    }
    return real_FSRollbackQuotaAsync(pClient, pCmd, path, error, asyncParams);
}

/* *****************************************************************************
 * Dynamic RPL loading to memory
 * ****************************************************************************/
static int LoadRPLToMemory(s_rpx_rpl *rpl_entry)
{
    if ((int)bss_ptr != 0x0a000000)
    {
        char buffer[200];
        __os_snprintf(buffer, sizeof(buffer), "CheckAndLoadRPL(%s) found and loading", rpl_entry->name);
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
    int path_len = strlen(CAFE_OS_SD_PATH) + strlen(SD_GAMES_PATH) + strlen(GAME_DIR_NAME) + strlen(RPX_RPL_PATH) + strlen(rpl_entry->name) + 3;
    char *path_rpl = MEMAllocFromDefaultHeap(path_len);
    if(!path_rpl) {
        MEMFreeToDefaultHeap(pCmd);
        MEMFreeToDefaultHeap(pClient);
        return 0;
    }
    // create path
    __os_snprintf(path_rpl, path_len, "%s%s/%s%s/%s", CAFE_OS_SD_PATH, SD_GAMES_PATH, GAME_DIR_NAME, RPX_RPL_PATH, rpl_entry->name);

    // malloc mem for read file
    unsigned char* dataBuf = (unsigned char*)MEMAllocFromDefaultHeapEx(0x1000, 0x40);
    if(!dataBuf) {
        MEMFreeToDefaultHeap(pCmd);
        MEMFreeToDefaultHeap(pClient);
        MEMFreeToDefaultHeap(path_rpl);
        return 0;
    }

    // do more initial FS stuff
    FSInitCmdBlock(pCmd);
    FSAddClientEx(pClient, 0, FS_RET_NO_ERROR);

    // set RPL size to 0 to avoid wrong RPL being loaded when opening file fails
    rpl_entry->size = 0;
    rpl_entry->offset = 0;

    int fd = 0;
    if (real_FSOpenFile(pClient, pCmd, path_rpl, "r", &fd, FS_RET_ALL_ERROR) == FS_STATUS_OK)
    {
        int ret;
        int rpl_size = 0;

        // Get current memory area
        s_mem_area* mem_area    = (s_mem_area*)(MEM_AREA_ARRAY);
        int mem_area_addr_start = mem_area->address;
        int mem_area_addr_end   = mem_area->address + mem_area->size;
        int mem_area_offset     = 0;

        // Copy rpl in memory
        while ((ret = FSReadFile(pClient, pCmd, dataBuf, 0x1, 0x1000, fd, 0, FS_RET_ALL_ERROR)) > 0)
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
                *(volatile unsigned char*)(mem_area_addr_start + mem_area_offset) = dataBuf[j];
                mem_area_offset += 1;
            }
            rpl_size += ret;
        }

        // Fill rpl entry
        rpl_entry->area = (s_mem_area*)(MEM_AREA_ARRAY);
        rpl_entry->offset = 0;
        rpl_entry->size = rpl_size;

        // flush memory
        // DCFlushRange((void*)rpl_entry, sizeof(s_rpx_rpl));
        // DCFlushRange((void*)rpl_entry->address, rpl_entry->size);

        if ((int)bss_ptr != 0x0a000000)
        {
            char buffer[200];
            __os_snprintf(buffer, sizeof(buffer), "CheckAndLoadRPL(%s) file loaded 0x%08X %i", rpl_entry->name, rpl_entry->area->address, rpl_entry->size);
            log_string(bss.global_sock, buffer, BYTE_LOG_STR);
        }

        FSCloseFile(pClient, pCmd, fd, FS_RET_NO_ERROR);
    }

    FSDelClient(pClient);
    MEMFreeToDefaultHeap(dataBuf);
    MEMFreeToDefaultHeap(pCmd);
    MEMFreeToDefaultHeap(pClient);
    MEMFreeToDefaultHeap(path_rpl);
    return 1;
}

/* *****************************************************************************
 * Searching for RPL to load
 * ****************************************************************************/
static int CheckAndLoadRPL(const char *rpl) {
    // If we are in Smash Bros app
    if (GAME_LAUNCHED == 0)
        return 0;

    // Look for rpl name in our table
    s_rpx_rpl *rpl_entry = (s_rpx_rpl*)(RPX_RPL_ARRAY);

    do
    {
        if(rpl_entry->is_rpx)
            continue;

        int len = strlen(rpl);
        int len2 = strlen(rpl_entry->name);
        if ((len != len2) && (len != (len2 - 4))) {
            continue;
        }

        // compare name string case insensitive and without ".rpl" extension
        if (strncasecmp(rpl_entry->name, rpl, len) == 0)
            return LoadRPLToMemory(rpl_entry);
    }
    while((rpl_entry = rpl_entry->next) != 0);

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

#if (USE_EXTRA_LOG_FUNCTIONS == 1)
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
#endif

/* *****************************************************************************
 * Log functions
 * ****************************************************************************/
#if (USE_EXTRA_LOG_FUNCTIONS == 1)
static void log_byte_for_client(void *pClient, char byte) {
    int client = GetCurClient(pClient);
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

DECL(int, FSGetVolumeState_log, void *pClient) {
    int result = real_FSGetVolumeState_log(pClient);

    if ((int)bss_ptr != 0x0a000000)
    {
        if (result > 1)
        {
            int error = real_FSGetLastError_log(pClient);

            char buffer[200];
            __os_snprintf(buffer, sizeof(buffer), "FSGetVolumeState: %i, last error = %i", result, error);
            log_string(bss.global_sock, buffer, BYTE_LOG_STR);
        }
    }

    return result;
}

#endif
/* *****************************************************************************
 * Creates function pointer array
 * ****************************************************************************/
#define MAKE_MAGIC(x, orig_instr) { x, my_ ## x, &real_ ## x, orig_instr }

const struct fs_magic_t {
    const void *real;
    const void *replacement;
    const void *call;
    const unsigned int orig_instr;
} fs_methods[] __attribute__((section(".fs_magic"))) = {
    // Common FS functions
    MAKE_MAGIC(FSInit,                      0x7C0802A6),
    MAKE_MAGIC(FSShutdown,                  0x4E800020),
    MAKE_MAGIC(FSAddClientEx,               0x9421FFD8),
    MAKE_MAGIC(FSDelClient,                 0x7C0802A6),

    // Replacement functions
    MAKE_MAGIC(FSGetStat,                   0x9421FFD8),
    MAKE_MAGIC(FSGetStatAsync,              0x7D094378),
    MAKE_MAGIC(FSOpenFile,                  0x9421FFD0),
    MAKE_MAGIC(FSOpenFileAsync,             0x7C0802A6),
    MAKE_MAGIC(FSOpenDir,                   0x9421FFD8),
    MAKE_MAGIC(FSOpenDirAsync,              0x9421FFE0),
    MAKE_MAGIC(FSChangeDir,                 0x7C0802A6),
    MAKE_MAGIC(FSChangeDirAsync,            0x7C0802A6),
    MAKE_MAGIC(FSMakeDir,                   0x7C0802A6),
    MAKE_MAGIC(FSMakeDirAsync,              0x7C0802A6),
    MAKE_MAGIC(FSRename,                    0x9421FFD8),
    MAKE_MAGIC(FSRenameAsync,               0x9421FFE0),
    MAKE_MAGIC(FSRemove,                    0x7C0802A6),
    MAKE_MAGIC(FSRemoveAsync,               0x7C0802A6),
    MAKE_MAGIC(FSFlushQuota,                0x7C0802A6),
    MAKE_MAGIC(FSFlushQuotaAsync,           0x7C0802A6),
    MAKE_MAGIC(FSGetFreeSpaceSize,          0x9421FFD8),
    MAKE_MAGIC(FSGetFreeSpaceSizeAsync,     0x7D094378),
    MAKE_MAGIC(FSRollbackQuota,             0x7C0802A6),
    MAKE_MAGIC(FSRollbackQuotaAsync,        0x7C0802A6),

    // Dynamic RPL loading functions
    MAKE_MAGIC(OSDynLoad_Acquire,           0x38A00000),
#if (USE_EXTRA_LOG_FUNCTIONS == 1)
    MAKE_MAGIC(OSDynLoad_GetModuleName,     0x9421FFD0),
    MAKE_MAGIC(OSDynLoad_IsModuleLoaded,    0x38A00001),

    MAKE_MAGIC(FSCloseFile_log,             0x7C0802A6),
    MAKE_MAGIC(FSCloseDir_log,              0x7C0802A6),
    MAKE_MAGIC(FSFlushFile_log,             0x7C0802A6),
    MAKE_MAGIC(FSGetErrorCodeForViewer_log, 0x9421FEF8),
    MAKE_MAGIC(FSGetLastError_log,          0x7C0802A6),
    MAKE_MAGIC(FSGetPosFile_log,            0x9421FFD8),
    MAKE_MAGIC(FSGetStatFile_log,           0x9421FFD8),
    MAKE_MAGIC(FSIsEof_log,                 0x7C0802A6),
    MAKE_MAGIC(FSReadDir_log,               0x9421FFD8),
    MAKE_MAGIC(FSReadFile_log,              0x9421FFC8),
    MAKE_MAGIC(FSReadFileWithPos_log,       0x9421FFC0),
    MAKE_MAGIC(FSSetPosFile_log,            0x9421FFD8),
    MAKE_MAGIC(FSSetStateChangeNotification_log, 0x7C0802A6),
    MAKE_MAGIC(FSTruncateFile_log,          0x7C0802A6),
    MAKE_MAGIC(FSWriteFile_log,             0x9421FFC8),
    MAKE_MAGIC(FSWriteFileWithPos_log,      0x9421FFC0),
    MAKE_MAGIC(FSGetVolumeState_log,        0x7C0802A6),
#endif
};

/* a buffer to place all the replaced instructions and calls to the real functions */
const unsigned char fs_methods_calls[sizeof(fs_methods)] __attribute__((section(".fs_method_calls")));

void PatchFsMethods(void)
{
    static uint8_t ucFsMethodsPatched = 0;
    if(!ucFsMethodsPatched)
    {
        ucFsMethodsPatched = 1;
        /* Patch branches to it. */
        int len = sizeof(fs_methods) / sizeof(struct fs_magic_t);
        while (len--) {
            unsigned int real_addr = (unsigned int)fs_methods[len].real;
            unsigned int repl_addr = (unsigned int)fs_methods[len].replacement;

            unsigned int replace_instr = 0x48000002 | (repl_addr & 0x03fffffc);

            if(*(volatile unsigned int *)(0xC1000000 + real_addr) != replace_instr) {
                // in the real function, replace the "mflr r0" instruction by a jump to the replacement function
                *(volatile unsigned int *)(0xC1000000 + real_addr) = replace_instr;
                FlushBlock((0xC1000000 + real_addr));
            }
        }
    }
}
