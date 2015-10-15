#include "fs.h"
#include "../common/common.h"

#define DECL(res, name, ...) \
	extern res name(__VA_ARGS__); \
	res (* real_ ## name)(__VA_ARGS__)  __attribute__((section(".magicptr"))); \
	res my_ ## name(__VA_ARGS__)

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
    bss.sd_mount[client] = 0;
}
static int client_num(void *pClient) {
    int i;
    for (i = 0; i < MAX_CLIENT; i++)
        if (bss.pClient_fs[i] == pClient)
            return i;
    return -1;
}

static int strlen(char* path) {
    int i = 0;
    while (path[i++])
        ;
    return i;
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

    /* Note : it's ugly but there is no memory left in payload code, had to comment some */
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

        // set mount base
		int i = 0;
        bss.mount_base[i++]  = '/';
        bss.mount_base[i++]  = 'v';
        bss.mount_base[i++]  = 'o';
        bss.mount_base[i++]  = 'l';
        bss.mount_base[i++]  = '/';
        bss.mount_base[i++]  = 'e';
        bss.mount_base[i++]  = 'x';
        bss.mount_base[i++]  = 't';
        bss.mount_base[i++]  = 'e';
        bss.mount_base[i++]  = 'r';
        bss.mount_base[i++] = 'n';
        bss.mount_base[i++] = 'a';
        bss.mount_base[i++] = 'l';
        bss.mount_base[i++] = '0';
        bss.mount_base[i++] = '1';
        bss.mount_base[i++] = '/';
        bss.mount_base[i++] = 'w';
        bss.mount_base[i++] = 'i';
        bss.mount_base[i++] = 'i';
        bss.mount_base[i++] = 'u';
        bss.mount_base[i++] = '/';
        bss.mount_base[i++] = 'g';
        bss.mount_base[i++] = 'a';
        bss.mount_base[i++] = 'm';
        bss.mount_base[i++] = 'e';
        bss.mount_base[i++] = 's';
        bss.mount_base[i++] = '/';

		// TODO: check length of mount_base, but in case its too long it will hang anyway
		char *game_name_ptr = (char *)GAME_DIR_NAME;
		while(*game_name_ptr) {
			bss.mount_base[i++] = *game_name_ptr++;
		}
        bss.mount_base[i++] = '/';
        bss.mount_base[i++] = 'c';
        bss.mount_base[i++] = 'o';
        bss.mount_base[i++] = 'n';
        bss.mount_base[i++] = 't';
        bss.mount_base[i++] = 'e';
        bss.mount_base[i++] = 'n';
        bss.mount_base[i++] = 't';
		bss.mount_base[i++] = '\0';

        // set save base
        for (i = 0; i < 20; i++)
            bss.save_base[i] = bss.mount_base[i];

        bss.save_base[i++] = '/';
        bss.save_base[i++] = 's';
        bss.save_base[i++] = 'a';
        bss.save_base[i++] = 'v';
        bss.save_base[i++] = 'e';
        bss.save_base[i++] = 's';
        bss.save_base[i++] = '/';

		// TODO: check length of save_base, but in case its too long it will hang anyway
		game_name_ptr = (char *)GAME_DIR_NAME;
		while(*game_name_ptr) {
			bss.save_base[i++] = *game_name_ptr++;
		}
		bss.save_base[i++] = '\0';

        // in case there is no game selected
//        if (bss.mount_base[16] == 0) { bss.mount_base[16] = '0'; bss.save_base[21] = '0'; }
//        if (bss.mount_base[17] == 0) { bss.mount_base[17] = '0'; bss.save_base[22] = '0'; }
//        if (bss.mount_base[18] == 0) { bss.mount_base[18] = '0'; bss.save_base[23] = '0'; }
//        if (bss.mount_base[19] == 0) { bss.mount_base[19] = '0'; bss.save_base[24] = '0'; }
    }
    return real_FSInit();
}
DECL(int, FSShutdown, void) {
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
 * Creates function pointer array
 * ****************************************************************************/
#define MAKE_MAGIC(x) { x, my_ ## x, &real_ ## x }

struct magic_t {
    const void *real;
    const void *replacement;
    const void *call;
} methods[] __attribute__((section(".magic"))) = {
    MAKE_MAGIC(FSInit),
    MAKE_MAGIC(FSShutdown),
    MAKE_MAGIC(FSAddClientEx),
    MAKE_MAGIC(FSDelClient),

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
};
