////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_driver.c
//  Description    : This is the implementation of the standardized IO functions
//                   for used to access the CART storage system.
//
//  Author         : [**Hao Qin**]
//  PSU email      : [**hjq5024@psu.edu**]
//

// Includes
#include <stdlib.h>
#include <string.h>
// Project Includes
#include "cart_driver.h"
#include "cart_controller.h"
#include "cmpsc311_log.h"

//
// Implementation
uint64_t ky1, ky2, rt1, ct1, fm1;

//strcut of a file
struct File
{
  char *fileName;
  int fileLength;
  int isOpen;
  int16_t fileHandle;
  uint32_t pos;
  int fileCart[CART_CARTRIDGE_SIZE];
  int fileFrame[CART_FRAME_SIZE];
};

//an array of all files
struct File Files[CART_MAX_TOTAL_FILES];

int fileCount = 0; //number of files
int currentFrame = -1;//initialize framecout
int currentCart = 0;//initialize cartcount

//utility function to generate opcode from 5 segments as input and opcode as output
CartXferRegister create_cart_opcode(CartXferRegister temp_ky1, CartXferRegister temp_ky2, CartXferRegister temp_rt1, CartXferRegister temp_ct1, CartXferRegister temp_fm1)
{
  CartXferRegister opCode;
  temp_ky1 = (temp_ky1 & 0xff) << 56;
  temp_ky2 = (temp_ky2 & 0xff) << 48;
  temp_rt1 = (temp_rt1 & 1) << 47;
  temp_ct1 = (temp_ct1 & 0xff) << 31;
  temp_fm1 = (temp_fm1 & 0xff) << 15;
  opCode = temp_ky1 | temp_ky2 | temp_rt1 | temp_ct1 | temp_fm1;
  return opCode;
}

//utility function to extract 5 segments in opcode, input is opcode and 5 CartXferRegister segments which will be modified  
int32_t extract_cart_opcode(CartXferRegister temp, CartXferRegister temp_ky1, CartXferRegister temp_ky2, CartXferRegister temp_rt1, CartXferRegister temp_ct1, CartXferRegister temp_fm1)
{
  temp_ky1 = (temp & 0xff00000000000000) >> 56;
  temp_ky2 = (temp & 0x00ff000000000000) >> 48;
  temp_rt1 = (temp & 0x0000800000000000) >> 47;
  temp_ct1 = (temp & 0x00007fff80000000) >> 31;
  temp_fm1 = (temp & 0x000000007fff8000) >> 15;
  return 0;
}
////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_poweron
// Description  : Startup up the CART interface, initialize filesystem
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t cart_poweron(void)
{
  CartXferRegister initop;
  CartXferRegister loadcart;
  CartXferRegister bezero;
  //initialize 
  initop = cart_io_bus(create_cart_opcode(CART_OP_INITMS, 0, 0, 0, 0), NULL);
  extract_cart_opcode(initop, ky1, ky2, rt1, ct1, fm1);
  if (rt1 == -1)
  {
    return -1;
  }
  int i;
  for (i = 0; i < CART_MAX_CARTRIDGES; i++)
  {
    //load all cart
    loadcart = cart_io_bus(create_cart_opcode(CART_OP_LDCART, 0, 0, i, 0), NULL);
    extract_cart_opcode(loadcart, ky1, ky2, rt1, ct1, fm1);
    if (rt1 == -1)
    {
      return -1;
    }
    //be zero for all carts
    bezero = cart_io_bus(create_cart_opcode(CART_OP_BZERO, 0, 0, 0, 0), NULL);
    extract_cart_opcode(bezero, ky1, ky2, rt1, ct1, fm1);
    if (rt1 == -1)
    {
      return -1;
    }
  }
  // Return successfully
  return (0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_poweroff
// Description  : Shut down the CART interface, close all files
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t cart_poweroff(void)
{
  CartXferRegister loadcart;
  CartXferRegister bezero;
  CartXferRegister poweroff;
  int i;
  //loading and zeroing all cartridges
  for (i = 0; i < CART_MAX_CARTRIDGES; i++)
  {
    loadcart = cart_io_bus(create_cart_opcode(CART_OP_LDCART, 0, 0, i, 0), NULL);
    extract_cart_opcode(loadcart, ky1, ky2, rt1, ct1, fm1);
    if (rt1 == -1)
    {
      return -1;
    }
    //set all carts to zero
    bezero = cart_io_bus(create_cart_opcode(CART_OP_BZERO, 0, 0, 0, 0), NULL);
    extract_cart_opcode(bezero, ky1, ky2, rt1, ct1, fm1);
    if (rt1 == -1)
    {
      return -1;
    }
  }
  //close all the files
  for (i = 0; i < fileCount; i++)
  {
    if (Files[i].isOpen == 1)
    {
      Files[i].isOpen = 0;
    }
  }
  //poweroff
  poweroff=cart_io_bus(create_cart_opcode(CART_OP_POWOFF, 0, 0, 0, 0), NULL);
  extract_cart_opcode(poweroff, ky1, ky2, rt1, ct1, fm1);
  if (rt1 == -1)
  {
    return -1;
  }

  // Return successfully
  return (0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_open
// Description  : This function opens the file and returns a file handle
//
// Inputs       : path - filename of the file to open
// Outputs      : file handle if successful, -1 if failure

int16_t cart_open(char *path)
{
  int i;
  for (i = 0; i < fileCount; i++)
  {
    //if the file exists
    if (strcmp(path, Files[i].fileName) == 0)
    {
      if (Files[i].isOpen == 1)
      { //if is already open
        return -1;
      }
      Files[i].isOpen = 1;
      Files[i].pos = 0;
      return Files[i].fileHandle;
    }
  }
  //if file is not found
  Files[fileCount].fileName = path;
  Files[fileCount].pos = 0;  
  Files[fileCount].isOpen = 1;  
  Files[fileCount].fileHandle = fileCount;
  Files[fileCount].fileLength = 0;
  fileCount++;
  currentFrame++;
  //if reached the end of the cart
  if (currentFrame == 1024)
  {
    currentCart++;
    currentFrame=0;
  }
  return Files[fileCount-1].fileHandle;

  // THIS SHOULD RETURN A FILE HANDLE
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_close
// Description  : This function closes the file
//
// Inputs       : fd - the file descriptor
// Outputs      : 0 if successful, -1 if failure

int16_t cart_close(int16_t fd)
{
  //if file is not open
  if (Files[fd].isOpen == 0)
  {
    return -1;
  }
  //if path not valid
  if (fd > fileCount || fd < 0)
  {
    return -1;
  }
  Files[fd].isOpen = 0;
  // Return successfully
  return (0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_read
// Description  : Reads "count" bytes from the file handle "fh" into the
//                buffer "buf"
//
// Inputs       : fd - filename of the file to read from
//                buf - pointer to buffer to read into
//                count - number of bytes to read
// Outputs      : bytes read if successful, -1 if failure

int32_t cart_read(int16_t fd, void *buf, int32_t count)
{
  CartXferRegister loadcart;
  CartXferRegister readframe;

  int i;
  int frameCount=0;
  char frame[CART_FRAME_SIZE];
  int originIndex = Files[fd].pos;//original position
  int frameIndex = originIndex/CART_FRAME_SIZE;//starting index origin in the first frame 
  int finalFrame = (originIndex+count)/CART_FRAME_SIZE;//final frame to read from  
  int lengthFrameIndex = Files[fd].fileLength/CART_FRAME_SIZE; //record which frame the length is in
  int frameoffset = CART_FRAME_SIZE-originIndex%CART_FRAME_SIZE; //remaining data index form the frame origin pos in

  for (i = frameIndex; i <= lengthFrameIndex; i++)
  {
    //loadcart
    loadcart = cart_io_bus(create_cart_opcode(CART_OP_LDCART, 0, 0, Files[fd].fileCart[i], 0), NULL);
    extract_cart_opcode(loadcart, ky1, ky2, rt1, ct1, fm1);
    if (rt1 == -1)
    {
      return -1;
    }
    //read current frame to frame[]
    readframe = cart_io_bus(create_cart_opcode(CART_OP_RDFRME, 0, 0, 0, Files[fd].fileFrame[i]), frame);
    extract_cart_opcode(readframe, ky1, ky2, rt1, ct1, fm1);
    if (rt1 == -1)
    {
      return -1;
    }

    if (i < lengthFrameIndex) //have not reached last frame yet
    {
      if (i == frameIndex && i == finalFrame)
      { //need to read only from the first frame
        memcpy(buf,frame+originIndex%CART_FRAME_SIZE,count);
        Files[fd].pos += count;
        return count;
      }
      if (i == frameIndex && i != finalFrame)
      { //if not the final frame to read from, copy the remaing data from the first frame
        memcpy(buf,frame+originIndex%CART_FRAME_SIZE,frameoffset);
        Files[fd].pos += frameoffset;
        frameCount--;
      }
      if (i != frameIndex && i == finalFrame)
      { //it is the last frame to read from
        memcpy(buf+frameoffset+CART_FRAME_SIZE*frameCount, frame, (originIndex + count) % CART_FRAME_SIZE);
        Files[fd].pos += (count + originIndex) % CART_FRAME_SIZE;
        return count;
      }
      if (i != frameIndex && i != finalFrame)
      { //in the middle of reading
        memcpy(buf+frameoffset+CART_FRAME_SIZE*frameCount, frame, CART_FRAME_SIZE);
        Files[fd].pos += CART_FRAME_SIZE;
      }
    }
    else//reached the final frame to read
    {
      if (i == frameIndex && (originIndex + count > Files[fd].fileLength))
      { //the first frame but need to read til EOF
        memcpy(buf, frame + originIndex % CART_FRAME_SIZE, Files[fd].fileLength - Files[fd].pos);
        Files[fd].pos += Files[fd].fileLength - originIndex;
        return Files[fd].fileLength - originIndex;
      }
      if (i == frameIndex && (originIndex + count <= Files[fd].fileLength))
      { //no need to read to EOF
        memcpy(buf, frame + originIndex % CART_FRAME_SIZE, count);
        Files[fd].pos += count;
        return count;
      }
      if (i != frameIndex && (originIndex + count > Files[fd].fileLength))
      { //not the first frame and need to read til EOF
        memcpy(buf + frameoffset + CART_FRAME_SIZE * frameCount, frame, Files[fd].fileLength % CART_FRAME_SIZE);
        Files[fd].pos += Files[fd].fileLength % CART_FRAME_SIZE;
        return Files[fd].fileLength - originIndex;
      }
      if (i != frameIndex && (originIndex + count <= Files[fd].fileLength))
      { //not the first frame but not need to read til EOF
        memcpy(buf + frameoffset + CART_FRAME_SIZE * frameCount, frame, (originIndex + count) % CART_FRAME_SIZE);
        Files[fd].pos += (originIndex + count) % CART_FRAME_SIZE;
        return count;
      }
    }
    frameCount++;
  }
  // Return successfully
  return (count);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_write
// Description  : Writes "count" bytes to the file handle "fh" from the
//                buffer  "buf"
//
// Inputs       : fd - filename of the file to write to
//                buf - pointer to buffer to write from
//                count - number of bytes to write
// Outputs      : bytes written if successful, -1 if failure

int32_t cart_write(int16_t fd, void *buf, int32_t count)
{
  CartXferRegister readframe;
  CartXferRegister loadcart;
  CartXferRegister writeframe;

  char frame[CART_FRAME_SIZE];
  int i;
  int frameCount=0;
  int originIndex = Files[fd].pos;//original position
  int frameIndex = originIndex/CART_FRAME_SIZE;//starting index origin in the first frame 
  int finalFrame = (originIndex+count)/CART_FRAME_SIZE;//final frame to read from  
  int lengthFrameIndex = Files[fd].fileLength/CART_FRAME_SIZE; //record which frame the length is in
  int frameoffset = CART_FRAME_SIZE-originIndex%CART_FRAME_SIZE; //remaining data index form the frame origin pos in

  //initialize for first write of blank file
  if (Files[fd].pos==0 && Files[fd].fileLength==0)
  {
    Files[fd].fileFrame[0] = currentFrame;
    Files[fd].fileCart[0] = currentCart;
  }
  for (i=frameIndex;i<=finalFrame;i++)
  {
    //load cart
    loadcart = cart_io_bus(create_cart_opcode(CART_OP_LDCART, 0, 0, Files[fd].fileCart[i], 0), NULL);
    extract_cart_opcode(loadcart, ky1, ky2, rt1, ct1, fm1);
    if (rt1 == -1)
    {
      return -1;
    }
    //read frame
    readframe = cart_io_bus(create_cart_opcode(CART_OP_RDFRME, 0, 0, 0, Files[fd].fileFrame[i]), frame);
    extract_cart_opcode(readframe, ky1, ky2, rt1, ct1, fm1);
    if (rt1 == -1)
    {
      return -1;
    }
    if (i <= lengthFrameIndex)
    { //not reach length frame yet

      if (i == frameIndex && i == finalFrame)
      { //need to read only from the first frame
        memcpy(frame+originIndex%CART_FRAME_SIZE,buf,count);
        Files[fd].pos += count;
        if (originIndex+count > Files[fd].fileLength)
        { //if need to write more than file length
          Files[fd].fileLength = originIndex + count;
        }
      }
      if (i == frameIndex && i != finalFrame)
      {
        //if not the last frame, write the remaing data for the first frame
        memcpy(frame+originIndex%CART_FRAME_SIZE,buf,frameoffset);
        Files[fd].pos+=frameoffset;
        frameCount--;
      }
      if (i != frameIndex && i == finalFrame)
      { //the last frame but not the first frame
        memcpy(frame,buf+frameoffset+frameCount*CART_FRAME_SIZE,(count+originIndex) % CART_FRAME_SIZE);
        Files[fd].pos+=(count+originIndex)%CART_FRAME_SIZE;
        if (originIndex+count>Files[fd].fileLength)
        { //if need to write more than file length
          Files[fd].fileLength=count+originIndex;
        }
      }
      if (i != frameIndex && i!=finalFrame)
      { //if not the last frame, write to that frame until it is full
        memcpy(frame,buf+frameoffset+frameCount*CART_FRAME_SIZE, CART_FRAME_SIZE);
        Files[fd].pos+=CART_FRAME_SIZE;
      }
    }
    else
    { //reach length frame, create a new frame
      currentFrame++;
      if (currentFrame == 1024)
      {
        currentCart++;
        currentFrame = 0;
      }
      Files[fd].fileFrame[i] = currentFrame; //update frame
      Files[fd].fileCart[i] = currentCart;   //update cart

      if (i == finalFrame)
      {//i is the last frame to write 
        
        Files[fd].pos += (originIndex+count)%CART_FRAME_SIZE;
        Files[fd].fileLength = originIndex+count;
        memcpy(frame,buf+frameoffset+frameCount*CART_FRAME_SIZE,(originIndex+count)%CART_FRAME_SIZE);
      }
      else
      {//if i didn't reach the last frame to write
        memcpy(frame,buf+frameoffset+frameCount*CART_FRAME_SIZE,CART_FRAME_SIZE);
        Files[fd].pos+=CART_FRAME_SIZE;
      }
    }
    //writeframe
    writeframe = cart_io_bus(create_cart_opcode(CART_OP_WRFRME, 0, 0, 0, Files[fd].fileFrame[i]), frame);
    extract_cart_opcode(writeframe, ky1, ky2, rt1, ct1, fm1);
    if (rt1 == -1)
    {
      return -1;
    }
    frameCount++;
  }
  // Return successfully
  return (count);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_read
// Description  : Seek to specific point in the file
//
// Inputs       : fd - filename of the file to write to
//                loc - offfset of file in relation to beginning of file
// Outputs      : 0 if successful, -1 if failure

int32_t cart_seek(int16_t fd, uint32_t loc)
{
  //if file is not opened
  if (!Files[fd].isOpen)
  {
    return -1;
  }  
  //if path invalid
  if ((fd>fileCount)||(fd < 0))
  {
    return -1;
  }  
  //if loc beyond length of file
  if (loc>Files[fd].fileLength)
  {
    return -1;
  }
  //update location
  Files[fd].pos=loc;
  // Return successfully
  return (0);
}
