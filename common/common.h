#ifndef COMMON_H
#define	COMMON_H

#define IS_USING_MII_MAKER      1

/* Loadiine common paths */
#define CAFE_OS_SD_PATH         "/vol/external01"
#define SD_LOADIINE_PATH        "/wiiu"
#define GAMES_PATH               "/games"
#define SAVES_PATH               "/saves"
#define SD_GAMES_PATH            SD_LOADIINE_PATH GAMES_PATH
#define SD_SAVES_PATH            SD_LOADIINE_PATH SAVES_PATH
#define CONTENT_PATH            "/content"
#define RPX_RPL_PATH            "/code"

/* DATA_ADDRESS : address where flags start */
#define DATA_ADDR               ((void *)0x011e3800)
#define BOUNCE_FLAG_ADDR        (DATA_ADDR - 0x04)  // bounce flag
#define IS_ACTIVE_ADDR          (DATA_ADDR - 0x08)  // is replacement active
#define RPL_REPLACE_ADDR        (DATA_ADDR - 0x0C)  // is it a new rpl to add
#define RPL_ENTRY_INDEX_ADDR    (DATA_ADDR - 0x10)  // entry index of the rpx in our table
#define IS_LOADING_RPX_ADDR     (DATA_ADDR - 0x14)  // used to know if we are currently loading a rpx or a rpl

/* RPX Address : where the rpx is copied or retrieve, depends if we dump or replace */
/* Note : from phys 0x30789C5D to 0x31E20000, memory seems empty (space reserved for root.rpx) which let us approximatly 22.5mB of memory free to put the rpx and additional rpls */
#define MEM_BASE                ((void*)0xC0800000)
#define MEM_SIZE                ((void*)(MEM_BASE - 0x04))
#define MEM_OFFSET              ((void*)(MEM_BASE - 0x08))
#define MEM_AREA                ((void*)(MEM_BASE - 0x0C))
#define MEM_PART                ((void*)(MEM_BASE - 0x10))
#define RPX_NAME                ((void*)(MEM_BASE - 0x14))
#define RPX_NAME_PENDING        ((void*)(MEM_BASE - 0x18))
#define GAME_LAUNCHED           ((void*)(MEM_BASE - 0x1C))
#define GAME_DIR_NAME           ((void*)(MEM_BASE - 0x200))

/* RPX_RPL_ARRAY contains an array of multiple rpl/rpl structures: */
/* Note : The first entry is always the one referencing the rpx (cf. struct s_rpx_rpl) */
#define RPX_RPL_ARRAY           ((void*)0xC07A0000)

/* MEM_AREA_ARRAY contains empty memory areas address - linked */
#define MEM_AREA_ARRAY          ((void*)0xC0790000)

/* RPX Name : from which app/game, our rpx is launched */
#if (IS_USING_MII_MAKER == 0)
    #define RPX_CHECK_NAME          0x63726F73  // 0xEFE00000 contains the rpx name, 0x63726F73 => cros (for smash brox : cross_f.rpx)
#else
    #define RPX_CHECK_NAME          0x66666C5F  // 0xEFE00000 contains the rpx name, 0x66666C5F => ffl_ (for mii maker : ffl_app.rpx)
#endif

/* Union for rpx name */
typedef union uRpxName {
    int name_full;
    char name[4];
} uRpxName;

/* Struct used to organize empty memory areas */
typedef struct _s_mem_area
{
    unsigned int        address;
    unsigned int        size;
    struct _s_mem_area* next;
} s_mem_area;

/* Struct used to organize rpx/rpl data in memory */
typedef struct _s_rpx_rpl
{
    struct _s_rpx_rpl* next;
    s_mem_area*        area;
    unsigned int       offset;
    unsigned int       size;
    unsigned char      is_rpx;
    char               name[0];
} s_rpx_rpl;

/* Log struct */
//typedef struct _loader_debug_t
//{
//    unsigned int tag;
//    unsigned int data;
//} loader_debug_t;

#endif	/* COMMON_H */

