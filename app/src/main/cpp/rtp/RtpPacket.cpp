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
            0), m_pBuffer(nullptr), m_nBufferLen(0), m_pPayloadData(nullptr), m_nPayloadDataLen(0),
            m_bForceFu(
                    false) {
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

        if ((pBuffer[0] & 0x02) == 0x02) {  // 占位标志P，则在该报文的尾部填充一个或多个额外的八位组

        }

        if ((m_nPayloadType == 98 || (m_nPayloadType == 96 && m_bForceFu)) && ((pBuffer[12] & 0x1F) == 28)) { // FU Type
            if (pBuffer[13] & 0x80) {  // 是否是起始帧
                m_pBuffer[9] = 0x00;
                m_pBuffer[10] = 0x00;
                m_pBuffer[11] = 0x00;
                m_pBuffer[12] = 0x01;
                m_pPayloadData = m_pBuffer + 9;
                m_nPayloadDataLen = bufferSize - 9;
            } else {
                m_pPayloadData = m_pBuffer + 14;
                m_nPayloadDataLen = bufferSize - 14;
            }
            m_pBuffer[13] = (char) ((pBuffer[12] & 0xE0) + (pBuffer[13] & 0x1F));
        } else if ((m_nPayloadType == 98 || (m_nPayloadType == 96 && m_bForceFu)) &&
                   (((pBuffer[12] & 0x1F) == 0x07) || ((pBuffer[12] & 0x1F) == 0x08) ||
                    ((pBuffer[12] & 0x1F) == 0x06) || ((pBuffer[12] & 0x1F) == 0x01))) { // sps或pps
            m_pBuffer[8] = 0x00;
            m_pBuffer[9] = 0x00;
            m_pBuffer[10] = 0x00;
            m_pBuffer[11] = 0x01;
            m_pPayloadData = m_pBuffer + 8;
            m_nPayloadDataLen = bufferSize - 8;
        } else {
            m_pPayloadData = m_pBuffer + 12;
            m_nPayloadDataLen = bufferSize - 12;
        }

        return 0;
    }

    void RtpPacket::CreateRtpPacket(unsigned short seq, char *pData,
                                    int dataSize, char nPayloadType, unsigned timeStamp, int nSSRC,
                                    bool marker) {
        m_nBufferLen = 12 + dataSize;
        m_pBuffer = new char[m_nBufferLen];

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
        if (pRtpHeader[0] & 0x80 != 0x80)  // rtp第二版
            return false;
        if (0x80 & pRtpHeader[1]) {  // mark标记
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
