#include "processIE.h"
#include "openwsn.h"
#include "res.h"
#include "idmanager.h"
#include "openserial.h"
#include "IEEE802154.h"
#include "openqueue.h"
#include "neighbors.h"
#include "IEEE802154E.h"
#include "schedule.h"
#include "packetfunctions.h"

//=========================== variables =======================================
IEHeader                IEHeader_vars;
subIE                   syncIE_vars;
subIE                   frameAndLinkIE_vars;
subIE                   timeslotIE_vars;
subIE                   channelHoppingIE_vars;
syncIEcontent           syncIEcontent_vars;
FrameandLinkIEcontent   FrameandLinkIEcontent_vars;
slotframeIEcontent      slotframeIEcontent_vars;
channelHoppingIEcontent channelHoppingIEcontent_vars;
uResLinkTypeIEcontent   uResLinkTypeIEcontent_vars;
uResCommandIEcontent    uResCommandIEcontent_vars;
uResBandwidthIEcontent  uResBandwidthIEcontent_vars;
uResScheduleIEcontent   uResScheduleIEcontent_vars;
//==========================

//=========================== public ==========================================
//admin
void processIE_init() {
     // initialize variables
    memset(&IEHeader_vars,0,sizeof(IEHeader));
    memset(&syncIE_vars,0,sizeof(subIE));
    memset(&frameAndLinkIE_vars,0,sizeof(subIE));
    memset(&timeslotIE_vars,0,sizeof(subIE));
    memset(&channelHoppingIE_vars,0,sizeof(subIE));
    memset(&syncIEcontent_vars,0,sizeof(syncIEcontent));
    memset(&FrameandLinkIEcontent_vars,0,sizeof(FrameandLinkIEcontent));
    memset(&slotframeIEcontent_vars,0,sizeof(slotframeIEcontent));
    memset(&channelHoppingIEcontent_vars,0,sizeof(channelHoppingIEcontent));
    memset(&uResLinkTypeIEcontent_vars,0,sizeof(uResLinkTypeIEcontent));
    memset(&uResCommandIEcontent_vars,0,sizeof(uResCommandIEcontent));
    memset(&uResBandwidthIEcontent_vars,0,sizeof(uResBandwidthIEcontent));
    memset(&uResScheduleIEcontent_vars,0,sizeof(uResScheduleIEcontent));
}

//==================set========================
void setMLME_IE (){
  //set IE length,groupID and type fields
  IEHeader_vars.Length  = 0;
  IEHeader_vars.GroupID = IE_MLME;
  IEHeader_vars.Type    = IE_TYPE_PAYLOA;
  IEHeader_vars.Length  += 2;
  IEHeader_vars.Length  += syncIE_vars.length;
  IEHeader_vars.Length  += frameAndLinkIE_vars.length;
  IEHeader_vars.Length  += frameAndLinkIE_vars.length;
  IEHeader_vars.Length  += frameAndLinkIE_vars.length;
}

void setSubSyncIE(){
  //set subIE length,subID and type fields
  uint8_t length = 0;
  syncIE_vars.SubID = 26;
  syncIE_vars.type = 0;
  length = length + 2;
  syncIEcontent_vars.asn = getADVasn();
  length = length + 5;
  syncIEcontent_vars.joinPriority = getJoinPriority();
  length = length + 1;
  syncIE_vars.length = length;
}	

void setSubFrameandLinkIE(){
  /*
  frameAndLinkIE_vars.SubID                             = 0x1b;
  frameAndLinkIE_vars.type                              = 0;
  FrameandLinkIEcontent_vars.numSlotframes     = getNumSlotframe();
  slotframeInfo*        tempSlotframeInfo       = NULL;
  FrameandLinkIEcontent_vars.nextslotframeInfo = tempSlotframeInfo;

  Link*                 tempLink;
  for(uint8_t i=0;i<FrameandLinkIEcontent_vars.numSlotframes;i++)
  {
    //add a SlotframeInfo
    tempSlotframeInfo.slotframeID      = i;
    tempSlotframeInfo.numOfLink        = getFrameLength(tempSlotframeInfo.slotframeID);
    //add a Link to SlotframeInfo
    tempSlotframeInfo.links            = getActiveLinks(tempSlotframeInfo.slotframeID);
    tempLink                            = tempSlotframeInfo.links;
    uint8_t j=0;
    do
    {
      tempLink.nextLinks               = getActiveLinks(tempSlotframeInfo.slotframeID);
      tempLink                          = tempLink.nextLinks;
    }while(tempLink.nextLinks)
    tempSlotframeInfo                   = tempSlotframeInfo.nextSlotframeInfo;
  }
  tempSlotframeInfo                     = NULL;

  frameAndLinkIE_vars.nextSubIE = NULL;
  */
}

void setSubTimeslotIE(){
  timeslotIE_vars.SubID     = 0x1c;
  timeslotIE_vars.type      = 0;
}

void setSubChannelHoppingIE(){
  channelHoppingIE_vars.SubID     = 0x9;
  channelHoppingIE_vars.type      = 1;
}

void setSubuResLinkTypeIE(){
}

void setSubuResCommandIE(){
}

void setSubuResBandWidthIE(){
}

void setSubuResGeneralSheduleIE(){
}

//=====================get============================= 
void getMLME_IE(){
}

subIE* getSubSyncIE(){
  
}	

subIE* getSubFrameandLinkIE(){
  
} 

subIE* getSubChannelHoppingIE(){
  
} 

subIE* getSubTimeslotIE(){
  
} 

subIE* getSubLinkTypeIE(){
  
} 

subIE* getSubuResCommandIE(){
  
} 

subIE* getSubuResBandWidthIE(){
  
} 

subIE* getSubuResGeneralSheduleIE(){
  
}

subIE*          getSyncIE(){
  return        &syncIE_vars;
}

syncIEcontent*          getSyncIEcontent(){
  return        &syncIEcontent_vars;
}

IEHeader*       getIEHeader(){
  return        &IEHeader_vars;
}

void Notify_Reservation(){
}