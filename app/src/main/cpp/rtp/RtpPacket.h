/*
 * RtpPacket.h
 *
 *  Created on: 2016年10月19日
 *      Author: zhourui
 */

#ifndef _RTPPACKET_H_
#define _RTPPACKET_H_

#include <stdint.h>

namespace Dream {

#pragma pack(1)
    typedef struct {
        uint32_t magic;
        uint8_t defVersion;
        uint8_t res;
        uint8_t isFirstFrame;
        uint8_t isLastFrame;
        uint64_t utcTimeStamp;
    } JW_CLOUD_RTP_EXT_DATA;
#pragma pack ()

    class RtpPacket {
    public:
        RtpPacket();

        virtual ~RtpPacket();

        int Parse(const char *pBuffer, size_t bufferSize);

        void CreateRtpPacket(unsigned short seq, char *pData, int dataSize, char nPayloadType,
                             unsigned timeStamp, int nSSRC, bool marker = false);

        bool ParseRtpHeader(unsigned char *pRtpHeader, bool *pMarker, char *pPayloadType,
                            unsigned short *pSequenceNumber, unsigned *pTimeStamp, int *pSsrc);

    public:
        int GetPayloadLength() {
            return m_nPayloadDataLen;
        }

        unsigned short GetSequenceNumber() {
            return m_nSequenceNumber;
        }

        char GetPayloadType() {
            return m_nPayloadType;
        }

        unsigned GetTimestamp() {
            return m_nTimeStamp;
        }

        char *GetPayloadData() {
            return m_pPayloadData;
        }

        bool HasMarker() {
            return m_bMarker;
        }

        char* GetBuffer() {
            return m_pBuffer;
        }

        int GetBudderLen() {
            return m_nBufferLen;
        }

        void ForceFu() {
            m_bForceFu = true;
        }

    public:

        unsigned short m_nSequenceNumber;
        char m_nPayloadType;
        bool m_bMarker;
        unsigned m_nTimeStamp;
        int m_nSsrc;

        char *m_pBuffer;
        int m_nBufferLen;
        char *m_pPayloadData;
        int m_nPayloadDataLen;

        bool m_bForceFu;
        int32_t m_extLength;
        int16_t m_defineProfile;

        bool m_bJWExtHeader;
        uint32_t  m_magic;
        uint8_t m_defVersion;
        uint8_t m_res;
        uint8_t m_isFirstFrame;
        uint8_t m_isLastFrame;
        uint64_t m_utcTimeStamp;
    };

} /* namespace Dream */

#endif /* RTPPACKET_H_ */
