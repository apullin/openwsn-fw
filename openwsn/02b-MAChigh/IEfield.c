#include "openwsn.h"
#include "processIE.h"
#include "packetfunctions.h"
#include "idmanager.h"
#include "openserial.h"
#include "IEfield.h"

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== public ======================================

void uRes_prependIE  (OpenQueueEntry_t*      msg){
    subIE* tempSubIE;
    //add syncIE to msg's payload
    uint16_t temp_16b = 0;
    tempSubIE = getSyncIE();
    if(tempSubIE->length != 0)
    {
      temp_16b  |=      tempSubIE->length       <<      SUBIE_SHORT_LENGTH;
      temp_16b  |=      SUBIE_SYNC              <<      SUBIE_SUBID;
      temp_16b  |=      0                       <<      SUBIE_TYPE;
      //add joinPriority
      syncIEcontent*    tempSyncIEcontent       = getSyncIEcontent();
      packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
      *((uint8_t*)(msg->payload))               = tempSyncIEcontent->joinPriority;
      //reserve asn field
      packetfunctions_reserveHeaderSize(msg,sizeof(asn_t));
      //packetfunctions_writeASN(msg, tempSyncIEcontent->asn);
      msg->l2_ASN       = msg->payload;
      //add length subID and subType
      packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
      *((uint8_t*)(msg->payload))               = (uint8_t)(temp_16b>>8);
      packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
      *((uint8_t*)(msg->payload))               = (uint8_t)(temp_16b);
    }
    //add another subIE to msg's payload
    temp_16b = 0;
    if(tempSubIE->length != 0)
    {

    }
    if(tempSubIE->length != 0)
    {

    }
    if(tempSubIE->length != 0)
    {

    }
    //add IE to msg's payload
    temp_16b = 0;
    IEHeader* tempIE = getIEHeader();
    temp_16b    |=      tempIE->Length   <<      IE_LENGTH;
    temp_16b    |=      tempIE->GroupID  <<      IE_GROUPID;
    temp_16b    |=      tempIE->Type     <<      IE_TYPE;
    //add length groupID and Type
    packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
    *((uint8_t*)(msg->payload))               = (uint8_t)(temp_16b>>8);
    packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
    *((uint8_t*)(msg->payload))               = (uint8_t)(temp_16b);
}

void uRes_retrieveIE (OpenQueueEntry_t*      msg){
     uint8_t       i    = 0;
     subIE  tempSubIE;
     IEHeader* tempIE   = getIEHeader();
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
        subIE*   tempSyncIE = getSyncIE();
        tempSyncIE->length        = tempSubIE.length;
        tempSyncIE->SubID         = tempSubIE.SubID;
        tempSyncIE->type          = tempSubIE.type;
        syncIEcontent*    tempSyncIEcontent = getSyncIEcontent();
        //length of asn
        i = i+5;
        tempSyncIEcontent->joinPriority     = *((uint8_t*)(msg->payload)+i);
        i++;
        break;
      default:
        return;
      }
     } while(i<tempIE->Length);

     packetfunctions_tossHeader(msg, tempIE->Length);

     Notify_Reservation();
}