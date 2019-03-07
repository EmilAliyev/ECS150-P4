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

#define FILE_NUM 32

//file info 
typedef struct Fileinfo
{
    int8_t open; //Tells if file has been closed
    int32_t block_offset; //offset on the block (bytes), 0 on open
    int32_t total_offset; //total offset
    int16_t block; //current block
    int16_t first_block; //first block
    int16_t size; //The size of the file


} __attribute__((packed)) Fileinfo;

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

struct Fileinfo openfiles[FILE_NUM]; 

//Use FAT to get the next data block in the chain
static int nextBlock(int currentBlock)
{
    return mounteddisk->fat[currentBlock];
}

//Find data block at fd's offset
static int getDataBlock(int fd)
{
    //Get the starting data block
    int currBlock = openfiles[fd].first_block;

    //Get the offset
    int offset = openfiles[fd].total_offset;

    //Go to next block until offset is less than block size
    while(offset > BLOCK_SIZE)
    {
        currBlock = nextBlock(currBlock);
        offset -= BLOCK_SIZE;
    }

    //Set the block offset
    openfiles[fd].block_offset = offset;

    return currBlock;
}

//Calculate the number of blocks that must be read
static int numBlocksToRead(int fd, size_t count)
{
    //If count is 0, no blocks need to be read
    if(count == 0)
        return 0;

    //Otherwise, at least 1 block must be read
    int numBlocks = 1;

    signed int numBytes = (signed int) count;

    //Subtract the bytes that must be read from the first block
    numBytes -= (BLOCK_SIZE - openfiles[fd].block_offset);

    //Every 4096 bytes means another block needs to be read
    while(numBytes > 0)
    {
        numBlocks++;
        numBytes -= BLOCK_SIZE;
    }

    return numBlocks;
}

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

//Check if root directory entry is free
static int rootEntryFree(Rootentry entry)
{
    if(entry.filename[0] != '\0')
        return FAILURE;

    return SUCCESS;
}

//Get the number of empty entries in root directory
static int numEmptyEntriesRootDir()
{
    int numFreeEntries = 0;

    //An empty entry is defined by the first character of the entry's filename being the NULL character
    for(int i = 0; i < ROOT_ENTRIES; i++)
    {
        if(rootEntryFree(mounteddisk->root->entries[i]) == SUCCESS)
            numFreeEntries++;
    }    

    return numFreeEntries;
}

//Get the number of files in the root directory
static int numFilesRootDir()
{
    int numfiles = 0;

    for(int i = 0; i < ROOT_ENTRIES; i++)
    {
        if(rootEntryFree(mounteddisk->root->entries[i]) != SUCCESS)
            numfiles++;
    }

    return numfiles;
}

//return a pointer to the first availible empty root entry
static Rootentry* findNextEmpty()
{
    for(int i = 0; i < ROOT_ENTRIES; i++)
    {
        if(rootEntryFree(mounteddisk->root->entries[i]) == SUCCESS){
	    return &mounteddisk->root->entries[i];
	}
    }

    return NULL;
}

//Search for file in root directory
static Rootentry* findFile(const char *filename)
{
    for(int i = 0; i < ROOT_ENTRIES; i++)
    {
        char *currfile = (char *) mounteddisk->root->entries[i].filename;

        if(strcmp(currfile, filename) == 0)
            return &mounteddisk->root->entries[i];
    }

    return NULL;
}

//Search for file in root directory
static int fileFound(const char *filename)
{
    for(int i = 0; i < ROOT_ENTRIES; i++)
    {
        char *currfile = (char *) mounteddisk->root->entries[i].filename;

        if(strcmp(currfile, filename) == 0)
            return SUCCESS;
    }

    return FAILURE;
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

//Check if file table has space
static int fileTableSpaceAvailable()
{
    for(int i = 0; i < FILE_NUM; i++)
    {
        if(openfiles[i].open == 0)
            return SUCCESS;
    }

    return FAILURE;
}

//Check if fd is open
static int isOpen(int fd)
{
    if(openfiles[fd].open != 1)
        return FAILURE;

    return SUCCESS;
}

//Check if file descriptor is within bounds
static int fd_in_bounds(int fd)
{
    if(fd > FILE_NUM)
        return FAILURE;

    if(fd < 0)
        return FAILURE;

    return SUCCESS;
}

//Check to make sure file descriptor is valid
static int valid_fd(int fd)
{
    //Case 1: fd not in bounds
    if(fd_in_bounds(fd) != SUCCESS)
        return FAILURE;

    //Case 2: fd not open
    if(isOpen(fd) != SUCCESS)
        return FAILURE;

    return SUCCESS;
}

//Check for read errors
static int read_err_check(int fd)
{
    //Case 1: invalid fd
    if(valid_fd(fd) != SUCCESS)
        return FAILURE;

    return SUCCESS;
}

//Check for stat errors
static int stat_err_check(int fd)
{
    //Case 1: invalid fd
    if(valid_fd(fd) != SUCCESS)
        return FAILURE;

    return SUCCESS;
}

//Check for seek errors
static int lseek_err_check(int fd, size_t offset)
{
    //Case 1: invalid fd
    if(valid_fd(fd) != SUCCESS)
        return FAILURE;

    //Case 2: offset out of bounds

    return SUCCESS;
}

//Check for file closing errors
static int close_err_check(int fd)
{
    //Case 1: invalid fd
    if(valid_fd(fd) != SUCCESS)
        return FAILURE;

    return SUCCESS;
}

//Check for file opening errors
static int open_err_check(const char *filename)
{
    //Case 1: Invalid filename
    if(validFilename(filename) != SUCCESS)
        return FAILURE;

    //Case 2: Filename not found
    if(fileFound(filename) != SUCCESS)
        return FAILURE;

    //Case 3: File table full
    if(fileTableSpaceAvailable() != SUCCESS)
        return FAILURE;

    return SUCCESS;
}

//Check for file creation errors
static int delete_err_check(const char *filename)
{
    //Case 1: Invalid filename
    if(validFilename(filename) != SUCCESS)
        return FAILURE;

    //Case 2: File does not exist
    if(fileFound(filename) != SUCCESS)
        return FAILURE;

    //Error check passed
    return SUCCESS;
}

//Check for file creation errors
static int create_err_check(const char *filename)
{
    //Case 1: Invalid filename
    if(validFilename(filename) != SUCCESS)
        return FAILURE;

    //Case 2: No space in root directory
    if(numFilesRootDir() == FS_FILE_MAX_COUNT)
        return FAILURE;

    //Case 3: File already exists
    if(fileFound(filename) == SUCCESS)
        return FAILURE;

    //Error check passed
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

//sets all fat blocks in a chain to FAT EOC
static void clearFATChain(int start_index)
{
    int prev; 
    int index = start_index;

    while(mounteddisk->fat[index] != FAT_EOC){
        prev = index;
        index = mounteddisk->fat[index];
        mounteddisk->fat[prev] = 0;
    }
}

static void clearRootEntry(Rootentry* root_file)
{
    //Save file info to that root entry
    root_file->filename[0] = '\0';
    root_file->filesize = 0;
    root_file->firstdatablockindex = 0;
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

//set up file list
static void setUpFileList()
{
    for(int i = 0; i < FILE_NUM; i++){
        openfiles[i].open = 0;
    }
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

    setUpFileList();

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
    //Check for errors
    if(create_err_check(filename) != SUCCESS)
        return FAILURE;

    //Find next open root entry
    Rootentry* open = findNextEmpty();

    //Save file info to that root entry
    strcpy((char *) open->filename, filename);
    open->filesize = 0;
    open->firstdatablockindex = FAT_EOC;

    return SUCCESS;
}

int fs_delete(const char *filename)
{
    //Check for errors
    if(delete_err_check(filename) != SUCCESS)
        return FAILURE;

    //return index of failure
    Rootentry* root_file = findFile(filename);

    clearFATChain(root_file->firstdatablockindex);

    clearRootEntry(root_file);

    return SUCCESS;
}

int fs_ls(void)
{
    printf("FS Ls:\n");
    
    for(int i = 0; i < ROOT_ENTRIES; i++)
    {
        if(rootEntryFree(mounteddisk->root->entries[i]) != SUCCESS)
        {
            printf("file: %s,", mounteddisk->root->entries[i].filename);
            printf(" size: %d,", mounteddisk->root->entries[i].filesize);
            printf(" data_blk: %hu\n", mounteddisk->root->entries[i].firstdatablockindex);
        }
    }   

    return SUCCESS;
}

int fs_open(const char *filename)
{
    struct Fileinfo new;
    new.total_offset = 0;
    new.block_offset = 0;

    //Check for errors
    if(open_err_check(filename) != SUCCESS)
        return FAILURE;

    Rootentry *fileentry = findFile(filename);

    new.first_block = fileentry->firstdatablockindex;
    new.size = fileentry->filesize;
    new.block = new.first_block;
    new.open = 1;

    //make file info for file and place it in empty table slot
    for(int i = 0; i < FILE_NUM; i++){
        if(openfiles[i].open == 0){
            memcpy(&openfiles[i], &new, sizeof(Fileinfo));
            //return index of file info in table
	    return i;
	}

    }

    return FAILURE;
}

int fs_close(int fd)
{
    if(close_err_check(fd) != SUCCESS)
        return FAILURE;

    openfiles[fd].open = 0;

    return SUCCESS;
}

int fs_stat(int fd)
{
    /* TODO: Phase 3 */

    //Check for errors
    if(stat_err_check(fd) != SUCCESS)
        return FAILURE;

    //Return the file size
    return openfiles[fd].size;
}

int fs_lseek(int fd, size_t offset)
{
    /* TODO: Phase 3 */
    if(lseek_err_check(fd, offset) != SUCCESS)
        return FAILURE;

    //Set offset of file fd
    openfiles[fd].total_offset = offset;


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

    if(read_err_check(fd) != SUCCESS)
        return FAILURE;

    //Find the starting data block (the data block at the offset)
    int dataBlock = getDataBlock(fd);

    //Get the number of blocks that must be read
    int numBlocks = numBlocksToRead(fd, count);

    //Allocate dummy buffer to store all blocks
    uint8_t *tempbuf = malloc(numBlocks * BLOCK_SIZE);

    size_t bytesRead = 0;

    //Read the first block into the dummy buffer
    
    block_read(dataBlock + mounteddisk->superblock->datastartindex, tempbuf);
    bytesRead += BLOCK_SIZE;

    //Read the remaining bytes
    
    
    free(tempbuf);

    return SUCCESS;
}

