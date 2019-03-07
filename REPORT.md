#ECS 150 Project 4 Report

Emil Aliyev
Noah White

##Introduction

##Phase 1

To start, we created data structures to represent the block structures used by the file system to contain the metadata: the superblock, the File Allocation Table (FAT) and the root directory.

###Data Structures

####The Superblock

We implemented the superblock as a simple data structure Superblock consisting of int8_t and int16_t fields corresponding to the entries in the superblock, as shown below.


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

We used an array of 8 int8_ts, as opposed to a single int64_t to represent the signature so we could more easily convert the signature into a string for string comparisons (such as checking that the signature is indeed the required signature).

####The FAT

We implemented the FAT as a simple uint16_t pointer. We used uint16_t as each entry in the FAT is 16 bits. We had the correct number of entries allocated in the FAT with malloc when the disk is mounted. 

####The root directory
We implemented the root directory as a data structure containing 128 of another data structure: the root entry. As with the superblock, the root entry data structure consisted of int8_t, int16_t and int32_t fields corresponding to the data contained within a root directory entry. The definitions of the rootentry and rootdirectory data structures are shown below.

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

##Phase 2
	

##Phase 3

####File Information Structs
We have a file array, containing all open files. Each open file is represented
by a file info struct. Each file info struct contains the following fields:

	open: an integer value that indicates if the file has been closed
	block_offset: Where the cursor is located in relation to the start of
		the current block
	total_offset: Where the cursor is located in relation to the start of
		the file
	block: The current block the offset is at
	first_block: The first block of the current file

##Phase 4
