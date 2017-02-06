/*
 * RtpPacket.cpp
 *
 *  Created on: 2016年10月19日
 *      Author: zhourui
 */

#include <cstring>

#include "RtpPacket.h"

namespace Dream {

    RtpPacket::RtpPacket() :
            m_nSequenceNumber(0), m_nPayloadType(0), m_bMarker(false), m_nTimeStamp(0), m_nSsrc(
            0), m_pBuffer(nullptr), m_pPayloadData(nullptr), m_nPayloadDataLen(0) {
    }

    RtpPacket::~RtpPacket() {
        delete[] m_pBuffer;
    }

    int RtpPacket::Parse(char *pBuffer, int bufferSize) {
        if (bufferSize < 12) {
            return -1;
        }

        if (!ParseRtpHeader((unsigned char *) pBuffer, &m_bMarker, &m_nPayloadType,
                            &m_nSequenceNumber, &m_nTimeStamp, &m_nSsrc))
            return -1;

        m_pBuffer = new char[bufferSize];

        memcpy(m_pBuffer, pBuffer, bufferSize);

        m_pPayloadData = m_pBuffer + 12;
        m_nPayloadDataLen = bufferSize - 12;

        return 0;
    }

    void RtpPacket::CreateRtpPacket(unsigned short seq, char *pData,
                                    int dataSize, char nPayloadType, unsigned timeStamp, int nSSRC,
                                    bool marker) {
        m_pBuffer = new char[12 + dataSize];

        m_pBuffer[0] = 0x80;
        m_pBuffer[1] = (marker ? 0x80 : 0x00) | nPayloadType;

        m_pBuffer[2] = (seq >> 8) & 0xff;
        m_pBuffer[3] = seq & 0xff;

        m_pBuffer[4] = (char) (timeStamp >> 24) & 0xff;
        m_pBuffer[5] = (char) (timeStamp >> 16) & 0xff;
        m_pBuffer[6] = (char) (timeStamp >> 8) & 0xff;
        m_pBuffer[7] = (char) timeStamp & 0xff;

        m_pBuffer[8] = (nSSRC >> 24) & 0xff;
        m_pBuffer[9] = (nSSRC >> 16) & 0xff;
        m_pBuffer[10] = (nSSRC >> 8) & 0xff;
        m_pBuffer[11] = nSSRC & 0xff;
        m_nPayloadDataLen = dataSize;

        memcpy(m_pBuffer + 12, pData, dataSize);
    }

    bool RtpPacket::ParseRtpHeader(unsigned char *pRtpHeader, bool *pMarker,
                                   char *pPayloadType, unsigned short *pSequenceNumber,
                                   unsigned *pTimeStamp, int *pSsrc) {
        if (pRtpHeader[0] != 0x80)
            return false;
        if (0x80 & pRtpHeader[1]) {
            *pMarker = true;
        } else {
            *pMarker = false;
        }

        *pPayloadType = 0x7f & pRtpHeader[1];
        *pSequenceNumber = ((unsigned short) pRtpHeader[2]) << 8 | pRtpHeader[3];
        *pTimeStamp = (unsigned) pRtpHeader[4] << 24 | (unsigned) pRtpHeader[5] << 16
                      | (unsigned) pRtpHeader[6] << 8 | pRtpHeader[7];
        *pSsrc = (int) pRtpHeader[8] << 24 | (int) pRtpHeader[9] << 16
                 | (int) pRtpHeader[10] << 8 | pRtpHeader[11];

        return true;
    }

} /* namespace Dream */
