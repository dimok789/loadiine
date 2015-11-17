#include "../common/common.h"
#include "../fs/fs.h"
#include "socket.h"
#include "logger.h"
#include "utils.h"

int fs_mount_sd(int sock, void* pClient, void* pCmd) {
    int is_mounted = 0;
    char buffer[1];

    if (sock != -1) {
        buffer[0] = BYTE_MOUNT_SD;
        sendwait(sock, buffer, 1);
    }

    // mount sdcard
    FSMountSource mountSrc;
    char mountPath[FS_MAX_MOUNTPATH_SIZE];
    int status = FSGetMountSource(pClient, pCmd, FS_SOURCETYPE_EXTERNAL, &mountSrc, FS_RET_NO_ERROR);
    if (status == FS_STATUS_OK)
    {
        status = FSMount(pClient, pCmd, &mountSrc, mountPath, sizeof(mountPath), FS_RET_UNSUPPORTED_CMD);
        if (status == FS_STATUS_OK)
        {
            // set as mounted
            is_mounted = 1;
        }
    }

    if (sock != -1) {
        buffer[0] = is_mounted ? BYTE_MOUNT_SD_OK : BYTE_MOUNT_SD_BAD;
        sendwait(sock, buffer, 1);
    }

    return is_mounted;
}


static void swap(void *e1, void *e2, unsigned int length)
{
    unsigned char tmp;
    unsigned char *a = (unsigned char *)e1;
    unsigned char *b = (unsigned char *)e2;

    while(length--)
    {
        tmp = *a;
        *a = *b;
        *b = tmp;
        a++;
        b++;
    }
}

void qsort(void *ptr, unsigned int count, unsigned int size, int (*compare)(const void*,const void*))
{
    if((count <= 1) || (size == 0)) {
        return;
    }

    unsigned char *begin = (unsigned char*)ptr;
    unsigned char *end = begin + size * count;
     // the pivot is improvable but for now it's ok, we don't need fastest sort algorithm as speed is not an issue ;-)
    unsigned char *pivot = begin;

    unsigned char *left = pivot + size;
    unsigned char *right = end;

    while ( left < right )
    {
        if (compare(left, pivot) <= 0 )  {
            left += size;
        }
        else {
            right -= size;

            if(left != right)
                swap(left, right, size);
        }
    }

    left -= size;

    if(begin != left)
        swap(begin, left, size);

    int left_num = (left - begin) / size;
    if(left_num > 1)
        qsort(begin, left_num, size, compare);

    int right_num = (end - right) / size;
    if(right_num > 1)
        qsort(right, right_num, size, compare);
}
