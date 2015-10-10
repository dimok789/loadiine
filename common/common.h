#ifndef COMMON_H
#define	COMMON_H

/* DATA_ADDRESS : address where flags start */
#define DATA_ADDR               ((void *)0x011e3800)
#define BOUNCE_FLAG_ADDR        (DATA_ADDR - 0x04)  // bounce flag
#define IS_ACTIVE_ADDR          (DATA_ADDR - 0x08)  // is replacement active
#define RPL_REPLACE_ADDR        (DATA_ADDR - 0x0C)  // is it a new rpl to add
#define RPL_ENTRY_INDEX_ADDR    (DATA_ADDR - 0x10)  // entry index of the rpx in our table

/* RPX Address : where the rpx is copied or retrieve, depends if we dump or replace */
/* Note : from phys 0x30789C5D to 0x31E20000, memory seems empty (space reserved for root.rpx) which let us approximatly 22.5mB of memory free to put the rpx and additional rpls */
#define MEM_BASE                ((void*)0xC0800000)
#define MEM_SIZE                ((void*)(MEM_BASE - 0x04))
#define MEM_OFFSET              ((void*)(MEM_BASE - 0x08))
#define MEM_PART                ((void*)(MEM_BASE - 0x0C))
#define RPX_NAME                ((void*)(MEM_BASE - 0x10))
#define RPX_NAME_PENDING        ((void*)(MEM_BASE - 0x14))
#define GAME_DIR_NAME           ((void*)(MEM_BASE - 0x100))

/* RPX_RPL_ARRAY contains an array of multiple rpl/rpl structures: */
/* Note : The first entry is always the one referencing the rpx (cf. struct s_rpx_rpl) */
#define RPX_RPL_ARRAY           ((void*)0xC07A0000)
#define RPX_RPL_ENTRY_COUNT     ((void*)(RPX_RPL_ARRAY - 0x04))

/* RPX Name : from which app/game, our rpx is launched */
#define RPX_CHECK_NAME          0x63726F73  // 0xEFE00000 contains the rpx name, 0x63726F73 => cros (for smash brox : cross_f.rpx)

/* Union for rpx name */
typedef union uRpxName {
    int name_full;
    char name[4];
} uRpxName;

/* Struct used to organize rpx/rpl data in memory */
typedef struct s_rpx_rpl
{
    int  address;
    int  size;
    int  offset;
    char name[64]; // TODO: maybe set the exact same size than dir_entry->name
} s_rpx_rpl;

#endif	/* COMMON_H */

