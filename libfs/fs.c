#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

/* TODO: Phase 1 */
#define BLOCK_SIZE 4096

#define SUPERBLOCK_INDEX 0
#define SUPERBLOCK_UNUSED_BYTES 4079

#define ROOT_ENTRIES 128
#define ROOT_FILENAME_SIZE 16
#define ROOT_ENTRY_UNUSED_BYTES 10

#define SUCCESS 0
#define FAILURE -1

//Superblock
typedef struct Superblock
{
    int64_t signature; //Signature (must be equal to "ECS150FS")
    int16_t numBlocks; //Total amount of blocks of virtual disk
    int16_t rootindex; //Root directory block index
    int16_t datastartindex; //Data block start index
    int16_t numDataBlocks; //Amount of data blocks
    int8_t numFATBlocks; //Number of blocks for FAT (File Allocation Table)
    int8_t padding [SUPERBLOCK_UNUSED_BYTES]; //Unused/padding
    
} Superblock;

//One entry in root directory
typedef struct Rootentry
{
    int8_t filename[ROOT_FILENAME_SIZE]; //Filename (including NULL character)
    int32_t filesize; //Size of the file (in bytes)
    int16_t firstdatablockindex; //Index of first data block
    int8_t padding [ROOT_ENTRY_UNUSED_BYTES]; //Unused/padding
    
} Rootentry;

//Root directory - contains 128 32-byte entries, 1 entry per file
typedef struct Rootdirecotry
{
    Rootentry entries [ROOT_ENTRIES];
} Rootdirectory;

typedef uint16_t* FAT;

typedef struct disk
{
    char *diskname;
    Superblock *superblock;
    FAT fat;
    Rootdirectory *root;
    
} disk;

disk *mounteddisk = NULL;

//Copy the FAT of the mounted disk
static void copyFAT()
{

}

//Copy the blocks to the mounted disk
static void copyBlocks()
{

    //Copy superblock
    block_read(SUPERBLOCK_INDEX, mounteddisk->superblock);

    //Get the index of the root directory
    int rootindex = (int) mounteddisk->superblock->numFATBlocks + 1;

    //Copy the FAT
    copyFAT();

    //Copy root directory
    block_read(rootindex, mounteddisk->root);
}

//Create a new disk
static void createNewDisk(const char *diskname)
{
    int namelength = strlen(diskname);

    mounteddisk = malloc(sizeof(disk));
    
    mounteddisk->diskname = malloc(namelength * sizeof(char));

    //Allocate blocks
    mounteddisk->superblock = malloc(sizeof(Superblock));

    //Number of entries in FAT is 2048 per block as each entry is 16 bits
    mounteddisk->fat = malloc(BLOCK_SIZE/2 * mounteddisk->superblock->numFATBlocks * sizeof(uint16_t));

    mounteddisk->root = malloc(BLOCK_SIZE);

    strcpy(mounteddisk->diskname, diskname);
}


//Free mounted disk
static void freeDisk()
{
    free(mounteddisk->diskname);
    free(mounteddisk->superblock);
    free(mounteddisk->fat);
    free(mounteddisk->root);
    free(mounteddisk);
}


int fs_mount(const char *diskname)
{
    /* TODO: Phase 1 */

    //Attempt to open disk.
     
    if(block_disk_open(diskname) != SUCCESS)
        return FAILURE;

    //Create new disk
    createNewDisk(diskname);

    //Copy the blocks
    copyBlocks();
    

    return SUCCESS;
}

int fs_umount(void)
{
    /* TODO: Phase 1 */

    //Make sure disk is mounted
    if(mounteddisk == NULL)
        return FAILURE;
    

    //Free the disk
    freeDisk();

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

