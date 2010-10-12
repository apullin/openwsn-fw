#ifndef __IEEE802154E_H
#define __IEEE802154E_H

/*----------------------------- IEEE802.15.4E ACK ---------------------------------------*/

typedef struct IEEE802154E_ACK_ht {
   uint8_t     dhrAckNack;
   uint16_t    timeCorrection;
} IEEE802154E_ACK_ht;

enum IEEE802154E_ACK_dhrAckNack_enums {
   IEEE154E_ACK_dhrAckNack_DEFAULT = 0x82,
};

/*----------------------------- IEEE802.15.4E ADV ---------------------------------------*/

typedef struct IEEE802154E_ADV_t {
   uint8_t     commandFrameId;
   uint32_t    timingInformation;   //needs to be 6 bytes long
   uint8_t     securityControlField;
   uint8_t     joinControl;
   uint8_t     timeslotHopping;
   uint8_t     channelPage;
   uint32_t    channelMap;
   uint8_t     numberSlotFrames;
   uint8_t     slotFrameID;
   uint16_t    slotFrameSize;
   uint8_t     numberLinks;
   uint32_t    linkInfo1;
   uint32_t    linkInfo2;
} IEEE802154E_ADV_t;

enum ieee154e_commandFrameId_enums {
   IEEE154E_ADV      = 0x0a,
   IEEE154E_JOIN     = 0x0b,
   IEEE154E_ACTIVATE = 0x0c,
};

enum ieee154e_ADV_defaults_enums {
   IEEE154E_ADV_SEC_DEFAULT           = 0x00,
   IEEE154E_ADV_JOINCONTROL_DEFAULT   = 0x00,
   IEEE154E_ADV_HOPPING_DEFAULT       = 0x00,
   IEEE154E_ADV_CHANNELPAGE_DEFAULT   = 0x04,
   IEEE154E_ADV_CHANNELMAP_DEFAULT    = 0x07FFF800,
   IEEE154E_ADV_NUMSLOTFRAMES_DEFAULT = 0x01,
   IEEE154E_ADV_SLOTFRAMEID_DEFAULT   = 0x00,
   IEEE154E_ADV_SLOTFRAMESIZE_DEFAULT = 0x001F,
   IEEE154E_ADV_NUMLINKS_DEFAULT      = 0x02,
   IEEE154E_ADV_LINKINFO1_DEFAULT     = 0x00000002,
   IEEE154E_ADV_LINKINFO2_DEFAULT     = 0x00010001,
};

#endif
