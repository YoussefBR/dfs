#ifndef FS3_DRIVER_INCLUDED
#define FS3_DRIVER_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_driver.h
//  Description    : This is the header file for the standardized IO functions
//                   for used to access the FS3 storage system.
//
//  Author         : Patrick McDaniel
//  Last Modified  : Sun 19 Sep 2021 08:12:43 AM PDT
//

// Include files
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "fs3_cache.h"
#include "fs3_controller.h"

// Defines
#define FS3_MAX_TOTAL_FILES 1024 // Maximum number of files ever
#define FS3_MAX_PATH_LENGTH 128 // Maximum length of filename length
#define FS3_STARTING_HANDLE 5 // Starting file handle
#define FS3_FILE_ARR_STEPSIZE 64 // Step size for the created files arr
#define FS3_OPENFILE_ARR_STEPSIZE 8 // Step size for open files arr
#define POS_ENDOF_FILE 1023 // 1 - FS3_SECTOR SIZE

// Struct storing important file information
typedef struct Fle{ 
	// File data
	char path[FS3_MAX_PATH_LENGTH];
	int32_t length;
		// pointers are hard so I malloced an array of locations and realloced to add more locations
	int32_t sectorCount;
	// Open info
	int8_t isOpen;
	int32_t handle;
	uint64_t pos;
	int16_t readCount;
} File;


//
// CmdBlk Functions
FS3CmdBlk construct_fs3_cmdblk(uint8_t opcode, uint16_t sec, uint_fast32_t track, uint8_t ret);
	// Constructs a Command Block that is ready to be sent to the disk controller
int16_t deconstruct_fs3_cmdblk(FS3CmdBlk cmdblk, uint8_t *op, uint16_t *sec, uint32_t *track, uint8_t *ret);
	// Deconstructs the command block into its respective pieces

//
// Structure Functions
int16_t setFileInfo(File *file, char *path, int32_t length);
	// Takes in a pointer to a file and fills it with the given parameters
int16_t setOpenInfo(File *file, int8_t isOpen, int32_t handle, uint64_t pos);
	// Takes in a pointer to a open file and fills it with the given parameters
int16_t init();
	// Sets up the structures for use	
int8_t findLoc(uint64_t pos, uint32_t fd, int32_t *track, int32_t *sector);

// Outdated index removing function
//int16_t arrRemoveAt(int32_t index, int32_t *arrLength, int32_t elementSize, void *arrStart);
// Linked List Functions
int8_t isFileOpen(int32_t handle);

//
// Interface functions

int32_t fs3_mount_disk(void);
	// FS3 interface, mount/initialize filesystem

int32_t fs3_unmount_disk(void);
	// FS3 interface, unmount the disk, close all files

int16_t fs3_open(char *path);
	// This function opens a file and returns a file handle

int16_t fs3_close(int16_t fd);
	// This function closes a file

int32_t fs3_read(int16_t fd, void *buf, int32_t count);
	// Reads "count" bytes from the file handle "fh" into the buffer  "buf"

int32_t fs3_write(int16_t fd, void *buf, int32_t count);
	// Writes "count" bytes to the file handle "fh" from the buffer  "buf"

int32_t fs3_seek(int16_t fd, uint32_t loc);
	// Seek to specific point in the file

#endif
