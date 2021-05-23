#ifndef FS_HARD_DISK_H
#define FS_HARD_DISK_H

#include "common/common.h"
#include "fs/fs.h"

#define SECTOR_SIZE  512

void init_hard_disk();

void read_hard_disk(char* buffer, uint32 start, uint32 length);


#endif
