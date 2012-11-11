#ifndef __PROCESSIE_H
#define __PROCESSIE_H

#define         MAXLINKNEIGHBORS    10

#include "openwsn.h"                                // needed for uin8_t, uint16_t

//=========================== typedef =========================================
typedef enum {
   ADV                             = 0,
   ASSOCIATE_REQ                   = 1,
   ASSOCIATE_RESP                  = 2,
   RES_LINK_REQ                    = 3,
   RES_LINK_RESP                   = 4,
   REMOVE_LINK_REQ                 = 5,
   SCHEDULE_REQ                    = 6,
   SCHEDULE_RESP                   = 7,
} resPacket_ID_t;

enum IE_groupID_enums {
   IE_ESDU                      = 0,
   IE_MLME                      = 1,
};

enum IE_type_enums {
   IE_TYPE_HEADER               = 0,
   IE_TYPE_PAYLOA               = 1,
};

typedef	struct{
	uint16_t	channelOffset;
	uint16_t	slotOffset;
	uint8_t	        linktype;
        struct Link*    nextLinks;
}Link;


typedef struct{
        uint8_t         slotframeID;
        uint16_t        size;
        uint8_t         numOfLink;
        Link*           links;
        struct slotframeInfo*  nextSlotframeInfo;
}slotframeInfo;

typedef	struct{
	uint16_t	length;
	uint8_t	        SubID;
	uint8_t	        type;
}subIE;

typedef	struct{
	uint16_t	Length;
	uint8_t	        GroupID;
	uint8_t	        Type;
}IEHeader;

typedef struct{
  asn_t asn;
  uint8_t joinPriority;
}syncIEcontent;

typedef struct{
	uint8_t	        numSlotframes;
        slotframeInfo*  nextSlotframeInfo;
}FrameandLinkIEcontent;

typedef	struct{
	uint8_t	        slotfameTemplt;
	void*	        otherField;
}slotframeIEcontent;

typedef struct{
  union {
	uint8_t	hoppingSequence_1Byte;
        uint8_t hoppingSequence_8Byte;
  };
}channelHoppingIEcontent;

typedef	struct{
	uint8_t		numSlotframes;
	uint8_t		slotframeID;
	uint8_t		size;
	uint8_t		numLinks;
	Link*	links;
}uResLinkTypeIEcontent;

typedef	struct{
	uint8_t	uResCommandID;
}uResCommandIEcontent;

typedef	struct{
	uint8_t	slotframeID;
	uint8_t	numLinks;
}uResBandwidthIEcontent;

typedef	struct{
	uint8_t	compressType;
	uint8_t	otherFields;
}uResScheduleIEcontent;

//=========================== variables =======================================

//=========================== prototypes ======================================
//admin
void processIE_init();

void setMLME_IE ();
void setSubSyncIE();	
void setSubFrameandLinkIE();
void setSubTimeslotIE();
void setSubChannelHoppingIE();
void setSubuResLinkTypeIE();
void setSubuResCommandIE();
void setSubuResBandWidthIE();
void setSubuResGeneralSheduleIE();
        
void getMLME_IE();
subIE* getSubSyncIE();	 
subIE* getSubFrameandLinkIE(); 
subIE* getSubChannelHoppingIE(); 
subIE* getSubTimeslotIE(); 
subIE* getSubLinkTypeIE(); 
subIE* getSubuResCommandIE(); 
subIE* getSubuResBandWidthIE(); 
subIE* getSubuResGeneralSheduleIE();

subIE*                  getSyncIE();
syncIEcontent*          getSyncIEcontent();
IEHeader*               getIEHeader();

void Notify_Reservation();




#endif
