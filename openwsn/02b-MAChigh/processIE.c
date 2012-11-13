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
#include "scheduler.h"
#include "packetfunctions.h"

//=========================== variables =======================================
IEHeader_t                IEHeader_vars;
subIE_t                   syncIE_vars;
subIE_t                   frameAndLinkIE_vars;
subIE_t                   timeslotIE_vars;
subIE_t                   channelHoppingIE_vars;
syncIEcontent_t           syncIEcontent_vars;
frameAndLinkIEcontent_t   frameAndLinkIEcontent_vars;
slotframeIEcontent_t      slotframeIEcontent_vars;
channelHoppingIEcontent_t channelHoppingIEcontent_vars;
uResLinkTypeIEcontent_t   uResLinkTypeIEcontent_vars;
uResCommandIEcontent_t    uResCommandIEcontent_vars;
uResBandwidthIEcontent_t  uResBandwidthIEcontent_vars;
uResScheduleIEcontent_t   uResScheduleIEcontent_vars;
//========================== private ==========================================

//=========================== public ==========================================
//admin
void processIE_init() {
     // initialize variables
    memset(&IEHeader_vars,0,sizeof(IEHeader_t));
    memset(&syncIE_vars,0,sizeof(subIE_t));
    memset(&frameAndLinkIE_vars,0,sizeof(subIE_t));
    memset(&timeslotIE_vars,0,sizeof(subIE_t));
    memset(&channelHoppingIE_vars,0,sizeof(subIE_t));
    memset(&syncIEcontent_vars,0,sizeof(syncIEcontent_t));
    memset(&frameAndLinkIEcontent_vars,0,sizeof(frameAndLinkIEcontent_t));
    memset(&slotframeIEcontent_vars,0,sizeof(slotframeIEcontent_t));
    memset(&channelHoppingIEcontent_vars,0,sizeof(channelHoppingIEcontent_t));
    memset(&uResLinkTypeIEcontent_vars,0,sizeof(uResLinkTypeIEcontent_t));
    memset(&uResCommandIEcontent_vars,0,sizeof(uResCommandIEcontent_t));
    memset(&uResBandwidthIEcontent_vars,0,sizeof(uResBandwidthIEcontent_t));
    memset(&uResScheduleIEcontent_vars,0,sizeof(uResScheduleIEcontent_t));
}

//==================set========================
void processIE_setMLME_IE (){
  //set IE length,groupID and type fields
  IEHeader_vars.Length  = 0;
  IEHeader_vars.GroupID = IE_MLME;
  IEHeader_vars.Type    = IE_TYPE_PAYLOA;
  IEHeader_vars.Length  += 2;
  IEHeader_vars.Length  += syncIE_vars.length;
  IEHeader_vars.Length  += frameAndLinkIE_vars.length;
  //IEHeader_vars.Length  += timeslotIE_vars.length;
  //IEHeader_vars.Length  += channelHoppingIE_vars.length;
}

void processIE_setSubSyncIE(){
  //set subIE length,subID and type fields
  uint8_t length = 0;
  syncIE_vars.SubID = 26;
  syncIE_vars.type = 0;
  length += 2;
  //set asn(asn will be added in IEEE802154e, res_getADVasn() return 0)
  syncIEcontent_vars.asn = res_getADVasn();
  length += 5;
  syncIEcontent_vars.joinPriority = res_getJoinPriority();
  length += 1;
  syncIE_vars.length = length;
}	

void processIE_setSubFrameAndLinkIE(){
  //set subIE length,subID and type fields
  uint8_t length = 0;
  frameAndLinkIE_vars.SubID                             = 0x1b;
  frameAndLinkIE_vars.type                              = 0;
  length += 2;
  frameAndLinkIEcontent_vars.numOfSlotframes            = schedule_getNumSlotframe();
  length += 1;

  for(uint8_t i=0;i<frameAndLinkIEcontent_vars.numOfSlotframes;i++)
  {
    //set SlotframeInfo
    frameAndLinkIEcontent_vars.slotframeInfo[i].slotframeID               = i;
    length += 1;
    frameAndLinkIEcontent_vars.slotframeInfo[i].slotframeSize             = schedule_getSlotframeSize(i);
    length += 2;
    schedule_generateLinkList(i);
    frameAndLinkIEcontent_vars.slotframeInfo[i].numOfLink                 = schedule_getLinksNumber(i);
    length += 1;
    frameAndLinkIEcontent_vars.slotframeInfo[i].links                     = schedule_getLinksList(i);
    length += 5 * frameAndLinkIEcontent_vars.slotframeInfo[i].numOfLink;
  }
  frameAndLinkIE_vars.length = length;
}

void processIE_setSubTimeslotIE(){
  timeslotIE_vars.SubID     = 0x1c;
  timeslotIE_vars.type      = 0;
}

void processIE_setSubChannelHoppingIE(){
  channelHoppingIE_vars.SubID     = 0x9;
  channelHoppingIE_vars.type      = 1;
}

void processIE_setSubuResLinkTypeIE(){
}

void processIE_setSubuResCommandIE(){
}

void processIE_setSubuResBandWidthIE(){
}

void processIE_setSubuResGeneralSheduleIE(){
}

//=====================get============================= 
void processIE_getMLME_IE(){
}

subIE_t* processIE_getSubSyncIE(){
    return      &syncIE_vars;
}	

subIE_t* processIE_getSubFrameAndLinkIE(){
    return      &frameAndLinkIE_vars;
} 

subIE_t* processIE_getSubChannelHoppingIE(){
    return      &channelHoppingIE_vars;
} 

subIE_t* processIE_getSubTimeslotIE(){
    return      &timeslotIE_vars;
} 

subIE_t* processIE_getSubLinkTypeIE(){
  
} 

subIE_t* processIE_getSubuResCommandIE(){
  
} 

subIE_t* processIE_getSubuResBandWidthIE(){
  
} 

subIE_t* processIE_getSubuResGeneralSheduleIE(){
  
}

syncIEcontent_t*  processIE_getSyncIEcontent(){
  return        &syncIEcontent_vars;
}

frameAndLinkIEcontent_t*        processIE_getFrameAndLinkIEcontent(){
  return        &frameAndLinkIEcontent_vars;
}

slotframeIEcontent_t*     processIE_getSlotframeIEcontent(){
  return        &slotframeIEcontent_vars;
}

channelHoppingIEcontent_t*  processIE_getChannelHoppingIEcontent(){
  return        &channelHoppingIEcontent_vars;
}

uResCommandIEcontent_t*         processIE_getuResCommandIEcontent(){
  return        &uResCommandIEcontent_vars;
}

IEHeader_t*       processIE_getIEHeader(){
  return        &IEHeader_vars;
}
