#ifndef FS_DEFS_H
#define	FS_DEFS_H

#include "../common/types.h"

/* FS defines and types */
#define FS_MAX_LOCALPATH_SIZE           511
#define FS_MAX_MOUNTPATH_SIZE           128
#define FS_MAX_FULLPATH_SIZE            (FS_MAX_LOCALPATH_SIZE + FS_MAX_MOUNTPATH_SIZE)
#define FS_MAX_ARGPATH_SIZE             FS_MAX_FULLPATH_SIZE

#define FS_STATUS_OK                    0
#define FS_RET_UNSUPPORTED_CMD          0x0400
#define FS_RET_NO_ERROR                 0x0000
#define FS_RET_ALL_ERROR                (uint)(-1)

/* directory entry stat flag */
#define FS_STAT_ATTRIBUTES_SIZE         (48)        /* size of FS-specific attributes field */
#define FS_STAT_FLAG_IS_DIRECTORY       0x80000000  /* entry is directory */

/* max length of file/dir name */
#define FS_MAX_ENTNAME_SIZE             256

/* typedef FSStatus, FSRetFlag*/
typedef int FSStatus;
typedef uint FSRetFlag;
typedef uint FSFlag;

/* FS Mount */
typedef enum
{
    FS_SOURCETYPE_EXTERNAL = 0, // Manual mounted external device
    FS_SOURCETYPE_HFIO,         // Host file IO
    FS_SOURCETYPE_MAX
} FSSourceType;

typedef struct
{
    FSSourceType          type;
    char                  path[FS_MAX_ARGPATH_SIZE];
} FSMountSource;

/* FS Client context buffer */
typedef struct
{
    uint8_t buffer[5888];
} FSClient;

/* FS command block buffer */
typedef struct
{
    uint8_t buffer[2688];
} FSCmdBlock;

/* File/Dir status */
typedef struct
{
    uint        flag;
    uint        permission;
    uint        owner_id;
    uint        group_id;
    uint        size;
    uint        alloc_size;
    uint64_t    quota_size;
    uint        ent_id;
    uint64_t    ctime;
    uint64_t    mtime;
    uint8_t     attributes[FS_STAT_ATTRIBUTES_SIZE];
} __attribute__((packed)) FSStat;

/* Directory entry */
typedef struct
{
    FSStat      stat;
    char        name[FS_MAX_ENTNAME_SIZE];
} FSDirEntry;

#endif	/* FS_DEFS_H */

