#include "openwsn.h"
#include "processIE.h"
#include "packetfunctions.h"
#include "idmanager.h"
#include "openserial.h"
#include "IEfield.h"
#include "res.h"
#include "schedule.h"
#include "scheduler.h"

//=========================== typedef =========================================

void notif_retrieveIEDone();
//=========================== variables =======================================


//=========================== public ======================================

void IEFiled_prependIE  (OpenQueueEntry_t*      msg){
    subIE_t* tempSubIE;
    uint16_t temp_16b = 0;
    /*
    //add subIE to msg's payload
    temp_16b = 0;
    if(tempSubIE->length != 0)
    {

    }
    
    temp_16b = 0;
    if(tempSubIE->length != 0)
    {

    }
    */
    //add FrameAndLinksIE to msg's paylaod
    temp_16b = 0;
    tempSubIE = processIE_getSubFrameAndLinkIE();
    if(tempSubIE->length != 0)
    {
      temp_16b  |=      tempSubIE->length       <<      SUBIE_SHORT_LENGTH;
      temp_16b  |=      SUBIE_FRAME_AND_LINK    <<      SUBIE_SUBID;
      temp_16b  |=      0                       <<      SUBIE_TYPE;
      //add frameAndLink IE content
      frameAndLinkIEcontent_t*  tempFrameAndLinkIEcontent       = processIE_getFrameAndLinkIEcontent();
      Link_t* tempLink = NULL;
      for(uint8_t i=0;i<tempFrameAndLinkIEcontent->numOfSlotframes;i++)
      {
        //add Links 
        tempLink = tempFrameAndLinkIEcontent->slotframeInfo[i].links;
        // add links
        for(uint8_t j=0;j<tempFrameAndLinkIEcontent->slotframeInfo[i].numOfLink;j++)
        {
          packetfunctions_reserveHeaderSize(msg,sizeof(Link_t));
          packetfunctions_htons(tempLink[j].slotOffset, &(msg->payload[0])); 
          packetfunctions_htons(tempLink[j].channelOffset, &(msg->payload[2]));
          msg->payload[4]     = tempLink[j].linktype;
        }
        //add number of links
        packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
        *((uint8_t*)(msg->payload))     = tempFrameAndLinkIEcontent->slotframeInfo[i].numOfLink;
        //add slotframe length
        packetfunctions_reserveHeaderSize(msg,sizeof(frameLength_t));
        packetfunctions_htons(tempFrameAndLinkIEcontent->slotframeInfo[i].slotframeSize, &(msg->payload[0]));
        //add slotframeID
        packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
        *((uint8_t*)(msg->payload))     = tempFrameAndLinkIEcontent->slotframeInfo[i].slotframeID;
      }
      //reset links
      memset(tempLink,0,MAXACTIVESLOTS*sizeof(Link_t));
      //add number of slotframes
      packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
      *((uint8_t*)(msg->payload))     = tempFrameAndLinkIEcontent->numOfSlotframes;
      //add length subID and subType
      packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
      *((uint8_t*)(msg->payload))               = (uint8_t)(temp_16b>>8);
      packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
      *((uint8_t*)(msg->payload))               = (uint8_t)(temp_16b);
      //reset subIEcontent
      memset(tempFrameAndLinkIEcontent,0,sizeof(frameAndLinkIEcontent_t));
    }
    //reset subIE
    memset(tempSubIE,0,sizeof(subIE_t));
    
    //add syncIE to msg's payload
    temp_16b = 0;
    tempSubIE = processIE_getSubSyncIE();
    if(tempSubIE->length != 0)
    {
      temp_16b  |=      tempSubIE->length       <<      SUBIE_SHORT_LENGTH;
      temp_16b  |=      SUBIE_SYNC              <<      SUBIE_SUBID;
      temp_16b  |=      0                       <<      SUBIE_TYPE;
      //add joinPriority
      syncIEcontent_t*    tempSyncIEcontent       = processIE_getSyncIEcontent();
      packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
      *((uint8_t*)(msg->payload))                 = tempSyncIEcontent->joinPriority;
      //reserve asn field
      packetfunctions_reserveHeaderSize(msg,sizeof(asn_t));
      //packetfunctions_writeASN(msg, tempSyncIEcontent->asn);
      msg->l2_ASN_payload       = msg->payload;
      //add length subID and subType
      packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
      *((uint8_t*)(msg->payload))               = (uint8_t)(temp_16b>>8);
      packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
      *((uint8_t*)(msg->payload))               = (uint8_t)(temp_16b);
      //reset subIEcontent
      memset(tempSyncIEcontent,0,sizeof(syncIEcontent_t));
    }
    //reset subIE
    memset(tempSubIE,0,sizeof(subIE_t));


    //add IE to msg's payload
    temp_16b = 0;
    IEHeader_t* tempIE = processIE_getIEHeader();
    temp_16b    |=      tempIE->Length   <<      IE_LENGTH;
    temp_16b    |=      tempIE->GroupID  <<      IE_GROUPID;
    temp_16b    |=      tempIE->Type     <<      IE_TYPE;
    //add length groupID and Type
    packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
    *((uint8_t*)(msg->payload))               = (uint8_t)(temp_16b>>8);
    packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
    *((uint8_t*)(msg->payload))               = (uint8_t)(temp_16b);
    //reset IEHeader
    memset(tempIE,0,sizeof(IEHeader_t));
    
}

void IEFiled_retrieveIE (OpenQueueEntry_t*      msg){
     uint8_t       i    = 0;
     subIE_t  tempSubIE;
     IEHeader_t* tempIE   = processIE_getIEHeader();
     tempIE->Length     = 0;
     uint16_t temp_16b  = msg->payload[i]+256*msg->payload[i+1];
     i =i + 2;

     tempIE->Length     = (temp_16b               >>      IE_LENGTH)&0x07FF;//11b
     tempIE->GroupID    = (uint8_t)((temp_16b     >>      IE_GROUPID)&0x000F);//4b
     tempIE->Type       = (uint8_t)((temp_16b     >>      IE_TYPE)&0x0001);//1b

     //subIE
     if(i>tempIE->Length)       {return;}
     do{
      temp_16b  = msg->payload[i]+256*msg->payload[i+1];
      i = i + 2;
      tempSubIE.type            = (uint8_t)((temp_16b     >>      SUBIE_TYPE)&0x0001);//1b
      if(tempSubIE.type == SUBIE_TYPE_SHORT)
      {
            tempSubIE.length    = (temp_16b               >>      SUBIE_SHORT_LENGTH)&0x00FF;//8b
            tempSubIE.SubID     = (uint8_t)((temp_16b     >>      SUBIE_SUBID)&0x007F);//7b       
      }
      else
      {
            tempSubIE.length    = (temp_16b               >>      SUBIE_LONG_LENGTH)&0x07FF;//11b
            tempSubIE.SubID     = (uint8_t)((temp_16b     >>      SUBIE_SUBID)&0x000F);//4b   
      }
      switch(tempSubIE.SubID)
      {
        //syncIE(subID = 0x1a)
      case 26:
        subIE_t*   tempSyncIE = processIE_getSubSyncIE();
        tempSyncIE->length        = tempSubIE.length;
        tempSyncIE->SubID         = tempSubIE.SubID;
        tempSyncIE->type          = tempSubIE.type;
        syncIEcontent_t*    tempSyncIEcontent = processIE_getSyncIEcontent();
        //length of asn(asn had been stored by ieee802154e)
        i = i+5;
        //store joinPriority
        tempSyncIEcontent->joinPriority     = *((uint8_t*)(msg->payload)+i);
        i++;
        break;
        //frameAndLinkIE(subID = 0x1b)
      case 27:
        subIE_t*     tempFrameAndLinkIE = processIE_getSubFrameAndLinkIE();
        tempFrameAndLinkIE->length      = tempSubIE.length;
        tempFrameAndLinkIE->SubID       = tempSubIE.SubID;
        tempFrameAndLinkIE->type        = tempSubIE.type;
        frameAndLinkIEcontent_t*  tempFrameAndLinkIEcontent = processIE_getFrameAndLinkIEcontent();
        //store number of slotframes
        tempFrameAndLinkIEcontent->numOfSlotframes = *((uint8_t*)(msg->payload)+i);
        i++;
        for(uint8_t j=0;j<tempFrameAndLinkIEcontent->numOfSlotframes;j++)
        {
          //store slotframeID
          tempFrameAndLinkIEcontent->slotframeInfo[j].slotframeID = *((uint8_t*)(msg->payload)+i);
          i++;
          //store length of slotframe
          tempFrameAndLinkIEcontent->slotframeInfo[j].slotframeSize = packetfunctions_ntohs(&(msg->payload[i]));
          i = i + 2;
          //store number of links
          tempFrameAndLinkIEcontent->slotframeInfo[j].numOfLink = *((uint8_t*)(msg->payload)+i);
          i++;
          //sotre links
          tempFrameAndLinkIEcontent->slotframeInfo[i].links = schedule_getLinksList(j);
          Link_t* tempLink =  tempFrameAndLinkIEcontent->slotframeInfo[i].links;
          for(uint8_t k=0;k<tempFrameAndLinkIEcontent->slotframeInfo[j].numOfLink;k++)
          {
            //store slotoffset
            tempLink[k].slotOffset = packetfunctions_ntohs(&(msg->payload[i]));
            i = i + 2;
            //store channeloffset
            tempLink[k].channelOffset = packetfunctions_ntohs(&(msg->payload[i]));
            i  = i + 2;
            //store link type
            tempLink[k].linktype = *((uint8_t*)(msg->payload)+i);
            i++;
          }
        }
        break;

      default:
        return;
      }
     } while(i<tempIE->Length);

     packetfunctions_tossHeader(msg, tempIE->Length);

     notif_retrieveIEDone();// use task?
}


void notif_retrieveIEDone(){
   // post RES's retrieveIEDone task
   scheduler_push_task(task_resNotifRetrieveIEDone,TASKPRIO_RES);
   // wake up the scheduler
   SCHEDULER_WAKEUP();
}