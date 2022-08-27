////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_driver.c
//  Description    : This is the implementation of the standardized IO functions
//                   for used to access the FS3 storage system.
//
//   Author        : *** INSERT YOUR NAME ***
//   Last Modified : *** DATE ***
//

// 
// Includes
#include <string.h>
#include <assert.h>

// Project File Includes
#include <fs3_driver.h>
#include <cmpsc311_log.h>

//
// Defines
#define SECTOR_INDEX_NUMBER(x) ((int)(x/POS_ENDOF_FILE))

//
// Global Vars

// Counters
int32_t lastAssignedHandle;
int32_t createdFilesSize;
uint64_t assignedSectors;

// CmdBlk Vars
const int OPCODE_POS = 60;
const int SEC_NUM_POS = 44;
const int TRACK_NUM_POS = 12;
const int RETURN = 11;

//
// Data Structures
File *createdFiles;
uint32_t fileAt[FS3_MAX_TRACKS][FS3_TRACK_SIZE];

// IMPLEMENTATION

//
// CmdBlk Functions
FS3CmdBlk construct_fs3_cmdblk(uint8_t opcode, uint16_t sec, uint_fast32_t track, uint8_t ret){
	// Hard coding 0 for sector and track until I figure out memory issues with current structure
	// Pushes the value into its place on the command block then ORs it with the final command block
	FS3CmdBlk command = ((FS3CmdBlk)opcode << OPCODE_POS);
	command = (command | (FS3CmdBlk)sec << SEC_NUM_POS);
	command = (command | (FS3CmdBlk)track << TRACK_NUM_POS);
	command = (command | (FS3CmdBlk)ret << RETURN);
	return command;
}
int16_t deconstruct_fs3_cmdblk(FS3CmdBlk cmdblk, uint8_t *op, uint16_t *sec, uint32_t *track, uint8_t *ret){
	// Brings the edge of the section wanted to rightmost bit then ANDs with a val of all 1s the size of the section
	*op = (uint8_t)((cmdblk >> OPCODE_POS) & 7);
	*sec = (uint16_t)((cmdblk >> SEC_NUM_POS) & UINT16_MAX);
	*track = (uint32_t)((cmdblk >> TRACK_NUM_POS) & UINT32_MAX);
	*ret = (uint8_t)((cmdblk >> RETURN) & 1);
	return (*ret);
}

//
// Data Structure Functions
int16_t setFileInfo(File *file, char *path, int32_t length){
	file->length = length;
	strcpy(file->path, path);
	file->readCount = 0;
	file->sectorCount = 1;
	return(0);
}
int16_t setOpenInfo(File *file, int8_t isOpen, int32_t handle, uint64_t pos){
	file->isOpen = isOpen;
	file->handle = handle;
	file->pos = pos;
	return(0);
}

int16_t init(){
	int16_t i, j;
	// Initial variable declaration
	lastAssignedHandle = FS3_STARTING_HANDLE - 1;
	createdFilesSize = 0;
	assignedSectors = 0;
	// Mallocing arrays for the structures
	createdFiles = ((malloc(sizeof(File) * FS3_FILE_ARR_STEPSIZE)));
	// Makes sure all elements of fileAt are initially zero;
	for(i = 0; i < FS3_MAX_TRACKS; i++){
		for(j = 0; j < FS3_TRACK_SIZE; j++){
			fileAt[i][j] = -1;
		}
	}
	return(0);
}

int8_t isFileOpen(int32_t handle){
	int8_t ret = -1;
	if(createdFiles[handle - FS3_STARTING_HANDLE].isOpen){
		ret = 0;
	}
	return ret;
}

int8_t findLoc(uint64_t pos, uint32_t fd, int32_t *track, int32_t *sector){
	int i, j;
	int ignoreSectors = pos / POS_ENDOF_FILE;
	for(i = 0; i < FS3_MAX_TRACKS; i++){
		for(j = 0; j < FS3_TRACK_SIZE; j++){
			if(fileAt[i][j] == fd){
				if(ignoreSectors == 0){
					*track = i;
					*sector = j;
					break;
				}else if(ignoreSectors > 0){
					ignoreSectors--;
				}
			}
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_mount_disk
// Description  : FS3 interface, mount/initialize filesystem
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t fs3_mount_disk(void) {
	// Initializes data structures
	init();
	// Packs mount command 
	FS3CmdBlk command = construct_fs3_cmdblk(FS3_OP_MOUNT, 0, 0, 0);
	// Sends it to hardware
	fs3_syscall(command, NULL);
	return 0;

}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_unmount_disk
// Description  : FS3 interface, unmount/destroy filesystem
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t fs3_unmount_disk(void){
	// Free malloc-ed data structure
	free(createdFiles);
	// Packs unmount command 
	FS3CmdBlk command = construct_fs3_cmdblk(FS3_OP_UMOUNT, 0, 0, 0);
	// Sends it to hardware
	fs3_syscall(command, NULL);
	return 0;
}

// Description  : This function opens the file and returns a file handle
//
// Inputs       : path - filename of the file to open
// Outputs      : file handle if successful, -1 if failure

int16_t fs3_open(char *path) {
	int32_t i = 0, handle = 0;
	int8_t fileNotFound = 1;
	// Check if path is included in created files
	while(fileNotFound && i < createdFilesSize){
		// If the file does exist checks to see if there is an associated open file
		if(strcmp(createdFiles[i].path, path) == 0){
			fileNotFound = 0;
			handle = i + FS3_STARTING_HANDLE;
			// If there is no open file creates one;
			if(!createdFiles[i].isOpen){
				// Adds the new open file to the open file linked list
				File *file = &createdFiles[i];
				setOpenInfo(file, 1, handle, 0);
			} else{
				logMessage(DEFAULT_LOG_LEVEL, "File is already open.");
			}
		}
		i++;
	}
	// If the file still has not been created, create it;
	if(fileNotFound){
		// Set handle
		handle = lastAssignedHandle + 1;
		lastAssignedHandle++;
		// Set Loc
		fileAt[assignedSectors / FS3_SECTOR_SIZE][assignedSectors % FS3_SECTOR_SIZE] = handle;
		assignedSectors++;
		// Init new file
		File file;
		if((createdFilesSize + 1) % FS3_FILE_ARR_STEPSIZE == 0){
			createdFiles = realloc(createdFiles, (createdFilesSize + 1 + FS3_FILE_ARR_STEPSIZE) * (sizeof(File)));
			assert(createdFiles != NULL);
		}
		memcpy(&createdFiles[handle - FS3_STARTING_HANDLE], &file, sizeof(File));
		// Add in file and open info
		setFileInfo(&createdFiles[handle - FS3_STARTING_HANDLE], path, 0);
		setOpenInfo(&createdFiles[handle - FS3_STARTING_HANDLE], 1, handle, 0);
		createdFilesSize++;
	}
	return handle;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_close
// Description  : This function closes the file
//
// Inputs       : fd - the file handle
// Outputs      : 0 if successful, -1 if failure

int16_t fs3_close(int16_t fd) {
	int8_t ret = -1;
	if(fd <= lastAssignedHandle){
		File *file = &createdFiles[fd - FS3_STARTING_HANDLE];
		if(file->isOpen){
			file->isOpen = 0;
			ret = 0;
		}
	}
	return ret;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_read
// Description  : Reads "count" bytes from the file handle "fh" into the 
//                buffer "buf"
//
// Inputs       : fd - filename of the file to read from
//                buf - pointer to buffer to read into
//                count - number of bytes to read
// Outputs      : bytes read if successful, -1 if failure

int32_t fs3_read(int16_t fd, void *buf, int32_t count) {
	int32_t sect, track, bytesRead = -1;
	uint8_t returnedOp, returnedRet, errorCheck = 0;
	uint16_t returnedSec;
	uint32_t returnedTrack;
	uint64_t pos;
	// Empties buffer
	char *sectBuf[FS3_SECTOR_SIZE];
	void *sectContent = (void *)sectBuf;
	// If the file is open reads
	if(fd <= lastAssignedHandle){
		// Creates pointer to corresponding file
		File *file = &createdFiles[fd - FS3_STARTING_HANDLE];
		// Checks if file is open then proceeds with read if true
		if(file->isOpen){
			pos = file->pos;
			findLoc(pos, fd, &track, &sect);
			// Checks if read will go into another sector;
			if(((count +  pos) / POS_ENDOF_FILE) != ((pos) / POS_ENDOF_FILE)){
				bytesRead = POS_ENDOF_FILE - (pos % POS_ENDOF_FILE);
			}
			// Checks if read will go over the file length
			if((bytesRead + pos) > file->length){
				bytesRead = file->length - pos;
			}
			// Checks if read is limited to just the current sector.
			if(bytesRead == -1){
				bytesRead = count;
			}
			// Checks if pos isn't at the end of file, if not, follows through with read
			if(bytesRead > 0){
				void *cacheBuf = fs3_get_cache(track, sect);
				if(cacheBuf == NULL){
					FS3CmdBlk seekCommand = construct_fs3_cmdblk(FS3_OP_TSEEK, 0, track, 0);
					FS3CmdBlk readCommand = construct_fs3_cmdblk(FS3_OP_RDSECT, sect, 0, 0);
					// Seeks to the correct track
					seekCommand = fs3_syscall(seekCommand, NULL);
					errorCheck += deconstruct_fs3_cmdblk(seekCommand, &returnedOp, &returnedSec, &returnedTrack, &returnedRet);
					// Reads
					readCommand = fs3_syscall(readCommand, sectContent);
					errorCheck += deconstruct_fs3_cmdblk(readCommand, &returnedOp, &returnedSec, &returnedTrack, &returnedRet);
					// Copies the requested bytes into the user buffer
					fs3_put_cache(track, sect, sectContent);
					memcpy(buf, &((char *)sectContent)[pos % POS_ENDOF_FILE], bytesRead);
					// Updates position of open file
					file->pos += bytesRead;
				}else{
					memcpy(buf, &((char *)cacheBuf)[pos % POS_ENDOF_FILE], bytesRead);
					file->pos += bytesRead;
				}
			} else{
				bytesRead = -1;
			}
		}
		if(errorCheck != 0){
			logMessage(FS3DriverLLevel, "Something went wrong!");
		}
		// If there are still bytes to read, go through read again on the next sector
		if((count - bytesRead) != 0){
			if((file->pos + 1) != file->length){
				file->readCount++;
				bytesRead += fs3_read(fd, &((char*)buf)[bytesRead], (count - bytesRead));
			}
		}
		file->readCount = 0;
	}
	return bytesRead;
}
////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_write
// Description  : Writes "count" bytes to the file handle "fh" from the 
//                buffer  "buf"
//
// Inputs       : fd - filename of the file to write to
//                buf - pointer to buffer to write from
//                count - number of bytes to write
// Outputs      : bytes written if successful, -1 if failure

int32_t fs3_write(int16_t fd, void *buf, int32_t count) {
	int32_t sect = -1, track = -1, bytesWritten = -1;
	uint8_t returnedOp, returnedRet, errorCheck = 0, notEnoughSpace = 0;
	uint16_t returnedSec;
	uint32_t returnedTrack;
	uint64_t pos, writeLocPos;
	// Empties buffer
	char *sectBuf[POS_ENDOF_FILE];
	void *sectContent = (void *)sectBuf;
	if(fd <= lastAssignedHandle){
		// Checks to see if the file is open
		File *file = &createdFiles[fd - FS3_STARTING_HANDLE];
		if(file->isOpen){
			// Checks which sector the write will begin at
			writeLocPos = ((file->pos) / POS_ENDOF_FILE);
			// Sets starter vars
			pos = file->pos;
			findLoc(pos, fd, &track, &sect);
			// Checks if there's enough space on the current sector and modifies the bytes written accordingly
			writeLocPos = (((file->pos + count) / POS_ENDOF_FILE) - writeLocPos);
			if(writeLocPos != 0){
				bytesWritten = (POS_ENDOF_FILE - ((file->pos) % POS_ENDOF_FILE));
				notEnoughSpace = 1;
				if((file->sectorCount - 1) != ((file->pos + count) / POS_ENDOF_FILE)){
					// If there's not enough space on the current sector and no next sector, creates one
					fileAt[assignedSectors / FS3_SECTOR_SIZE][assignedSectors % FS3_SECTOR_SIZE] = fd;
					assignedSectors++;
				}
			}else{
				bytesWritten = count;
			}
			// Checks if bytes written will go over the length of the file and file values accordingly
			if((bytesWritten + pos) > file->length){
				file->length = bytesWritten + pos;
			}
			file->pos += bytesWritten;
			// Writes
			FS3CmdBlk seekCommand = construct_fs3_cmdblk(FS3_OP_TSEEK, 0, track, 0);
			FS3CmdBlk readCommand = construct_fs3_cmdblk(FS3_OP_RDSECT, sect, 0, 0);
			// Seeks to proper track
			seekCommand = fs3_syscall(seekCommand, NULL);
			errorCheck += deconstruct_fs3_cmdblk(seekCommand, &returnedOp, &returnedSec, &returnedTrack, &returnedRet);
			// Reads sector info
			readCommand = fs3_syscall(readCommand, sectContent);
			errorCheck += deconstruct_fs3_cmdblk(readCommand, &returnedOp, &returnedSec, &returnedTrack, &returnedRet);
			// Writes over the correct portion of the sector
			memcpy(&((char*)sectContent)[pos % POS_ENDOF_FILE], (char*)buf, bytesWritten);
			void *cacheBuf = fs3_get_cache(track, sect);
			if(cacheBuf == NULL){
				fs3_put_cache(track, sect, sectContent);
			} else{
				memcpy((char *)cacheBuf, (char *)sectContent, POS_ENDOF_FILE);
			}
			// Updates disk with proper sector contents
			FS3CmdBlk writeCommand = construct_fs3_cmdblk(FS3_OP_WRSECT, sect, 0, 0);
			fs3_syscall(writeCommand, sectContent);
			errorCheck += deconstruct_fs3_cmdblk(writeCommand, &returnedOp, &returnedSec, &returnedTrack, &returnedRet);
		}
		if(errorCheck != 0){
			logMessage(FS3DriverLLevel, "Something went wrong!");
		}
		// If there is not enough space to write on the sector, go to the next sector and finish writing there
		if(notEnoughSpace){
			void *writtenUntil = &((char*)buf)[bytesWritten];
			//memcpy(buf, writtenUntil, (count - bytesWritten));
			bytesWritten += fs3_write(fd, writtenUntil, (count - bytesWritten));
		}
	}
	return bytesWritten;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_seek
// Description  : Seek to specific point in the file
//
// Inputs       : fd - filename of the file to write to
//                loc - offfset of file in relation to beginning of file
// Outputs      : 0 if successful, -1 if failure

int32_t fs3_seek(int16_t fd, uint32_t loc) {
	int32_t ret = -1;
	// If file is open and the position is less than the length then it updates position.
	if(fd <= lastAssignedHandle){
		File *file = &createdFiles[fd - FS3_STARTING_HANDLE];
		if(file->isOpen){
			if(file->length > loc){
				file->pos = loc;
				ret = 0;
			}
		}
	}
	// Possible implementation of created file search to return if file exists but is not open
	return ret;
}