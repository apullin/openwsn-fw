#include "hdlcserial.h"

//variables
//====prototypes=================
char fcs_calc(uint8_t *buffer,uint8_t length,uint16_t crc);
static uint16_t fcs_fcs16(uint16_t fcs, uint8_t data);

//=====function implementations==

uint8_t hdlcify(uint8_t* str, uint8_t len){
  
  uint16_t    fcs = 0;
  uint8_t c,stuff_count,index;
  uint8_t crc1,crc2;
  uint8_t tempbuffer[HDLC_MAX_LEN];
  uint8_t tempbufferlen;

  //compute crc:
  fcs = (uint16_t) 0xffff;
  for( c=0;c <len;c++)
    fcs = fcs_fcs16(fcs, str[c]);
  fcs = ~fcs;//one's complement
  
  //count the number of bytes to stuff
  stuff_count = 0;
  for ( c=0;c<len;c++) if((str[c] == 0x7E) || (str[c] == 0x7D)) stuff_count++;
  
  crc1 = fcs;
  crc2 = fcs>>8;
  if ((crc1 == 0x7E) || (crc1 == 0x7D)) stuff_count++;
  if ((crc2 == 0x7E) || (crc2 == 0x7D)) stuff_count++;
  
  //build the temporary buffer of the hdlc packet
  tempbufferlen = len+stuff_count+4;//to account for the header, footer and crc + stuffing bytes
  tempbuffer[0] = HDLC_HEADER_FLAG;
  tempbuffer[tempbufferlen-1] = HDLC_HEADER_FLAG;//delimiters
  tempbuffer[tempbufferlen-2] = crc2;
  tempbuffer[tempbufferlen-3] = crc1;//fill in crc
  index=1;
  for (c=0;c<len;c++){
    if (str[c]==0x7E){
      tempbuffer[index] = 0x7D;
      tempbuffer[index+1] = 0x5E;
      index+=2;
    } else if(str[c] == 0x7D){
      tempbuffer[index] = 0x7D;
      tempbuffer[index+1] = 0x5D;
      index+=2;
    } else
      tempbuffer[index++] = str[c];
  }
  
  //now copy the tempbuffer back in the original space
  memcpy(str,tempbuffer,tempbufferlen);
  
  //return the number of bytes in the updated buffer
  return tempbufferlen;
}

uint8_t dehdlcify(uint8_t *str,uint8_t len){
  uint16_t    fcs = 0;
  uint8_t c,stuff_count,index;
  uint8_t tempbuffer[HDLC_MAX_LEN];
  uint8_t tempbufferlen;
  
  //first check to see if this is a real hdlc packet
  if(str[0] != HDLC_HEADER_FLAG || str[len-1] != HDLC_HEADER_FLAG)
    return 255;//obviously this value can't be the real length of the original packet
  
  //count the number of bytes to unstuff
  stuff_count = 0;
  for ( c=0;c<len;c++) if(str[c] == 0x7D) stuff_count++;
  
  //build the temporary buffer of the original packet
  tempbufferlen = len-stuff_count-4;//to account for the header and footer + stuffing bytes
  index = 1;
  for(c=0;c<tempbufferlen;c++){
    if(str[index]==0x7D){
      if(str[index+1]==0x5E)
        tempbuffer[c] = 0x7E;
      else if(str[index+1]==0x5D)
        tempbuffer[c] = 0x7D;
      index+=2;
    } else
      tempbuffer[c] = str[index++];
  }
  
  fcs = tempbuffer[tempbufferlen - 2] + (tempbuffer[tempbufferlen-1] <<8);
  
  tempbufferlen-=2;//removing the two crc bytes
  
  if (fcs_calc(tempbuffer,tempbufferlen,fcs)){ //crc checks
    memcpy(str,tempbuffer,tempbufferlen);
    return tempbufferlen;
  }
  else //crc doesn't check
    return 255;
}

//============private===============
char fcs_calc(uint8_t *buffer,uint8_t length,uint16_t crc){//change function to bool?
  uint16_t    fcs;
  uint8_t count;
 
  fcs = (uint16_t) 0xffff;
  for ( count=0;count<length-2;count++)
    fcs = fcs_fcs16(fcs, buffer[count]);
  return ((~fcs) == crc); /* add 1's complement then compare*/
}

static uint16_t fcs_fcs16(uint16_t fcs, uint8_t data){
  return (fcs >> 8) ^ fcstab[(fcs ^ data) & 0xff];
}