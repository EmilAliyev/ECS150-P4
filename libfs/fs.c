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

#define FIRST_FAT_BLOCK_INDEX 1

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
    
} __attribute__((packed)) Superblock;

//One entry in root directory
typedef struct Rootentry
{
    int8_t filename[ROOT_FILENAME_SIZE]; //Filename (including NULL character)
    int32_t filesize; //Size of the file (in bytes)
    int16_t firstdatablockindex; //Index of first data block
    int8_t padding [ROOT_ENTRY_UNUSED_BYTES]; //Unused/padding
    
} __attribute__((packed)) Rootentry;

//Root directory - contains 128 32-byte entries, 1 entry per file
typedef struct Rootdirecotry
{
    Rootentry entries [ROOT_ENTRIES];
} __attribute__((packed)) Rootdirectory;

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
    for(int i = FIRST_FAT_BLOCK_INDEX; i < mounteddisk->superblock->numFATBlocks + FIRST_FAT_BLOCK_INDEX; i++){
        block_read(i, &mounteddisk->fat[(i-1) * (BLOCK_SIZE/2)]);
    }

}

//Get the number of free data blocks from the fat
static int numFreeDataBlocks()
{
    int numFatEntries = BLOCK_SIZE/2 * mounteddisk->superblock->numFATBlocks;
    int numFreeBlocks = 0;

    //A FAT entry of 0 corresponds to a free data block
    for(int i = 0; i < numFatEntries; i++)
    {
        if(mounteddisk->fat[i] == 0)
            numFreeBlocks++;
    }
    

    return numFreeBlocks;
}

//Get the number of empty entries in root directory
static int numEmptyEntriesRootDir()
{
    return 0;
}

//Create a new disk
static void createNewDisk(const char *diskname)
{
    int namelength = strlen(diskname) + 1;

    mounteddisk = malloc(sizeof(disk));
    
    mounteddisk->diskname = malloc(namelength * sizeof(char));

    //Allocate blocks
    mounteddisk->superblock = malloc(sizeof(Superblock));
    
    //Copy superblock
    block_read(SUPERBLOCK_INDEX, mounteddisk->superblock);
    
    //Number of entries in FAT is 2048 per block as each entry is 16 bits
    mounteddisk->fat = malloc(BLOCK_SIZE/2 * mounteddisk->superblock->numFATBlocks * sizeof(uint16_t));
    
    //Copy the FAT
    copyFAT();

    //Copy root directory
    mounteddisk->root = malloc(BLOCK_SIZE);
    block_read(mounteddisk->superblock->rootindex, mounteddisk->root);

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
    //Attempt to open disk.
     
    if(block_disk_open(diskname) != SUCCESS)
        return FAILURE;

    //Create new disk
    createNewDisk(diskname);

    return SUCCESS;
}

int fs_umount(void)
{
    //Make sure disk is mounted
    if(mounteddisk == NULL)
        return FAILURE;
    
    //Close the disk
    block_disk_close();

    //Free the disk
    freeDisk();

    return SUCCESS;
}

int fs_info(void)
{
    //Print info
    printf("FS info:\n");
    printf("total_blk_count=%d\n", mounteddisk->superblock->numBlocks);
    printf("fat_blk_count=%d\n", mounteddisk->superblock->numFATBlocks);
    printf("rdir_blk=%d\n", mounteddisk->superblock->numFATBlocks + 1);
    printf("data_blk=%d\n", mounteddisk->superblock->numFATBlocks + 2);
    printf("data_blk_count=%d\n", mounteddisk->superblock->numDataBlocks);
    printf("fat_free_ratio=%d/%d\n", numFreeDataBlocks(), mounteddisk->superblock->numDataBlocks);
    printf("rdir_free_ratio=%d/%d\n", numEmptyEntriesRootDir(), ROOT_ENTRIES);
    
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

