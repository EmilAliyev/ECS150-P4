#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

/* TODO: Phase 1 */
#define SUPERBLOCK_UNUSED_BYTES 4079

#define ROOT_ENTRIES 128
#define ROOT_FILENAME_SIZE 16
#define ROOT_ENTRY_UNUSED_BYTES 10

#define SUCCESS 0

//Superblock
typedef struct superblock
{
    int64_t signature; //Signature (must be equal to "ECS150FS")
    int16_t numBlocks; //Total amount of blocks of virtual disk
    int16_t rootindex; //Root directory block index
    int16_t datastartindex; //Data block start index
    int16_t numDataBlocks; //Amount of data blocks
    int8_t numFATBlocks; //Number of blocks for FAT (File Allocation Table)
    int8_t padding [SUPERBLOCK_UNUSED_BYTES]; //Unused/padding
    
} superblock;

//One entry in root directory
typedef struct rootentry
{
    int8_t filename[ROOT_FILENAME_SIZE]; //Filename (including NULL character)
    int32_t filesize; //Size of the file (in bytes)
    int16_t firstdatablockindex; //Index of first data block
    int8_t padding [ROOT_ENTRY_UNUSED_BYTES]; //Unused/padding
    
} rootentry;

//Root directory - contains 128 32-byte entries, 1 entry per file
typedef struct rootdirecotry
{
    rootentry entries [ROOT_ENTRIES];
} rootdirectory;

typedef uint16_t* FAT;

int fs_mount(const char *diskname)
{
    /* TODO: Phase 1 */

    return SUCCESS;
}

int fs_umount(void)
{
    /* TODO: Phase 1 */

    return SUCCESS;
}

int fs_info(void)
{
    /* TODO: Phase 1 */

    return SUCCESS;
}

int fs_create(const char *filename)
{
    /* TODO: Phase 2 */

    return SUCCESS;
}

int fs_delete(const char *filename)
{
    /* TODO: Phase 2 */

    return SUCCESS;
}

int fs_ls(void)
{
    /* TODO: Phase 2 */

    return SUCCESS;
}

int fs_open(const char *filename)
{
    /* TODO: Phase 3 */

    return SUCCESS;
}

int fs_close(int fd)
{
    /* TODO: Phase 3 */
    return SUCCESS;
}

int fs_stat(int fd)
{
    /* TODO: Phase 3 */
    return SUCCESS;
}

int fs_lseek(int fd, size_t offset)
{
    /* TODO: Phase 3 */
    return SUCCESS;
}

int fs_write(int fd, void *buf, size_t count)
{
    /* TODO: Phase 4 */
    return SUCCESS;
}

int fs_read(int fd, void *buf, size_t count)
{
    /* TODO: Phase 4 */
    return SUCCESS;
}

