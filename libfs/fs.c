#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

/* TODO: Phase 1 */
#define FS_SIGNATURE "ECS150FS"
#define BLOCK_SIZE 4096

#define SIGNATURE_BYTES 8

#define SUPERBLOCK_INDEX 0
#define SUPERBLOCK_UNUSED_BYTES 4079

#define FAT_EOC 0xFFFF
#define FIRST_FAT_BLOCK_INDEX 1

#define ROOT_ENTRIES 128
#define ROOT_FILENAME_SIZE 16
#define ROOT_ENTRY_UNUSED_BYTES 10

#define SUCCESS 0
#define FAILURE -1

//Superblock
typedef struct Superblock
{
    int8_t signature[SIGNATURE_BYTES]; //Signature (must be equal to "ECS150FS")
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

//Check if char ptr is string (null-terminated)
static int isString(const char *ptr)
{
    while(*ptr)
        ptr++;

    if(*ptr != '\0')
        return FAILURE;

    else
        return SUCCESS;

}

//Check if file name is valid
static int validFilename(const char *filename)
{

    //Check for invalid filename errors

    //Case 1: Filename not null-terminated
    if(isString(filename) != SUCCESS)
        return FAILURE;

    //Case 2: Filename length exceeds maximum
    if(strlen(filename) > FS_FILENAME_LEN)
        return FAILURE;
    
    

    return SUCCESS;
}

//Write the FAT back out to disk
static void writeFAT()
{
    for(int i = FIRST_FAT_BLOCK_INDEX; i < mounteddisk->superblock->numFATBlocks + FIRST_FAT_BLOCK_INDEX; i++)
    {
        block_write(i, &mounteddisk->fat[(i-1) * (BLOCK_SIZE/2)]);
    }
}

//Write blocks back out to disk
static void writeBlocks()
{
    //Write the superblock
    block_write(SUPERBLOCK_INDEX, mounteddisk->superblock);

    //Write the FAT
    writeFAT();

    //Write the root directory
    block_write(mounteddisk->superblock->rootindex, mounteddisk->root);
}

//Copy the FAT of the mounted disk
static void copyFAT()
{
    for(int i = FIRST_FAT_BLOCK_INDEX; i < mounteddisk->superblock->numFATBlocks + FIRST_FAT_BLOCK_INDEX; i++){
        block_read(i, &mounteddisk->fat[(i-1) * (BLOCK_SIZE/2)]);
    }

}

//Make sure the disk's signature is valid
static int validSignature()
{
    char signature[SIGNATURE_BYTES + 1];

    //Turn the signature into a string
    for(int i = 0; i < SIGNATURE_BYTES; i++)
    {
        signature[i] = mounteddisk->superblock->signature[i];
    }

    signature[SIGNATURE_BYTES] = '\0';
    
    //Check the signature
    if(strcmp(signature, FS_SIGNATURE) != 0)
        return FAILURE;

    return SUCCESS;
}

//Check to ensure the mounted disk has the correct block count
static int checkBlockCount()
{
    if(mounteddisk->superblock->numBlocks != block_disk_count())
        return FAILURE;

    return SUCCESS;
}

//Make sure disk has a valid format
static int validFormat()
{
    //Check the signature
    if(validSignature() != SUCCESS)
        return FAILURE;

    //Check the block count
    if(checkBlockCount() != SUCCESS)
        return FAILURE;

    return SUCCESS;
}


//Get the number of free data blocks from the fat
static int numFreeDataBlocks()
{
    int numFreeBlocks = 0;

    //A FAT entry of 0 corresponds to a free data block
    for(int i = 0; i < mounteddisk->superblock->numDataBlocks; i++)
    {
        if(mounteddisk->fat[i] == 0)
            numFreeBlocks++;
    }
    

    return numFreeBlocks;
}

//Get the number of empty entries in root directory
static int numEmptyEntriesRootDir()
{
    int numFreeEntries = 0;

    //An empty entry is defined by the first character of the entry's filename being the NULL character
    for(int i = 0; i < ROOT_ENTRIES; i++)
    {
        if(mounteddisk->root->entries[i].filename[0] == '\0')
            numFreeEntries++;
    }    

    return numFreeEntries;
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
    //Make sure no disk is mounted
    if(mounteddisk != NULL)
        return FAILURE;

    //Attempt to open disk.
     
    if(block_disk_open(diskname) != SUCCESS)
        return FAILURE;

    //Create new disk
    createNewDisk(diskname);

    //Check the format

    if(validFormat() != SUCCESS)
        return FAILURE;

    return SUCCESS;
}

int fs_umount(void)
{
    //Make sure disk is mounted
    if(mounteddisk == NULL)
        return FAILURE;
    
    //Write blocks back out to disk
    writeBlocks();

    //Close the disk
    block_disk_close();

    //Free the disk
    freeDisk();

    return SUCCESS;
}

int fs_info(void)
{
    //Print info
    printf("FS Info:\n");
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
    
    //Error cases

    //Case 1: Invalid filename
    if(validFilename(filename) != SUCCESS)
        return FAILURE;

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

