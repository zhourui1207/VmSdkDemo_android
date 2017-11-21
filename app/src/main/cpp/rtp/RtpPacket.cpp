/*
 * RtpPacket.cpp
 *
 *  Created on: 2016年10月19日
 *      Author: zhourui
 */

#include <cstring>

#include "RtpPacket.h"
#include "../util/public/platform.h"

namespace Dream {

    RtpPacket::RtpPacket() :
            m_nSequenceNumber(0), m_nPayloadType(0), m_bMarker(false), m_nTimeStamp(0), m_nSsrc(
            0), m_pBuffer(nullptr), m_nBufferLen(0), m_pPayloadData(nullptr), m_nPayloadDataLen(0),
            m_bForceFu(false), m_extLength(0), m_defineProfile(0), m_bJWExtHeader(false), m_magic(0),
            m_defVersion(0), m_res(0), m_isFirstFrame(0), m_isLastFrame(0), m_utcTimeStamp(0) {
    }

    RtpPacket::~RtpPacket() {
        delete[] m_pBuffer;
    }

    int RtpPacket::Parse(const char *pBuffer, size_t bufferSize) {
//        LOGE("!!!", "Parse len[%d] data[%s]", bufferSize, getHexString(pBuffer, 0, bufferSize).c_str());
//        LOGE("!!!", "Parse len[%d]]\n", bufferSize);

        if (bufferSize < 12) {
            return -1;
        }

        if (!ParseRtpHeader((unsigned char *) pBuffer, &m_bMarker, &m_nPayloadType,
                            &m_nSequenceNumber, &m_nTimeStamp, &m_nSsrc))
            return -1;

//        LOGE("!!!", "seqnumber [%d]", m_nSequenceNumber);
        m_pBuffer = new char[bufferSize];

        memcpy(m_pBuffer, pBuffer, bufferSize);

        m_defineProfile = 0;
        m_extLength = 0;
        m_bJWExtHeader = false;
        m_magic = 0;
        m_defVersion = 0;
        m_res = 0;
        m_isFirstFrame = 0;
        m_isLastFrame = 0;
        m_utcTimeStamp = 0;

        if ((pBuffer[0] & 0x10) == 0x10) {  // 扩展标示
//            LOGE("rtp", "have ext header\n");
            if (bufferSize < 16) {  // 后面有还有4个字节的扩展头
                return -1;
            }
            int pos = 0;
            DECODE_INT16(pBuffer + 12, m_defineProfile, pos);
            int16_t tmpLen = 0;
            DECODE_INT16(pBuffer + 14, tmpLen, pos);
//            LOGE("rtp", "jwheader, profile [%d], extLength[%d]\n", m_defineProfile, tmpLen);
            m_extLength = (4 + 4 * tmpLen);  // 加上扩展头字节

            if (bufferSize < 12 + m_extLength) {
                return -1;
            }

            // 判断中威rtp扩展头
            if (m_defineProfile == 0x1081 && m_extLength == 20) {
                JW_CLOUD_RTP_EXT_DATA extData;
                memset(&extData, 0, sizeof(extData));
                memcpy(&extData, pBuffer + 16, sizeof(extData));
                m_bJWExtHeader = true;
                m_magic = extData.magic;
                m_defVersion = extData.defVersion;
                m_res = extData.res;
                m_isFirstFrame = extData.isFirstFrame;
                m_isLastFrame = extData.isLastFrame;
                m_utcTimeStamp = extData.utcTimeStamp;
//                LOGE("rtp", "jwheader, extData size[%d], utcTimeStamp[%lld]\n", sizeof(extData), m_utcTimeStamp);
            }
        }

        if ((m_nPayloadType == 98 || (m_nPayloadType == 96 && m_bForceFu)) && ((pBuffer[12 + m_extLength] & 0x1F) == 28)) { // FU Type
            if (pBuffer[13 + m_extLength] & 0x80) {  // 是否是起始帧
                m_pBuffer[9 + m_extLength] = 0x00;
                m_pBuffer[10 + m_extLength] = 0x00;
                m_pBuffer[11 + m_extLength] = 0x00;
                m_pBuffer[12 + m_extLength] = 0x01;
                m_pPayloadData = m_pBuffer + m_extLength + 9;
                m_nPayloadDataLen = bufferSize - m_extLength - 9;
            } else {
                m_pPayloadData = m_pBuffer + m_extLength + 14;
                m_nPayloadDataLen = bufferSize - m_extLength - 14;
            }
            m_pBuffer[13 + m_extLength] = (char) ((pBuffer[12 + m_extLength] & 0xE0) + (pBuffer[13 + m_extLength] & 0x1F));
        } else if ((m_nPayloadType == 98 || (m_nPayloadType == 96 && m_bForceFu)) &&
                   (((pBuffer[12 + m_extLength] & 0x1F) == 0x07) || ((pBuffer[12 + m_extLength] & 0x1F) == 0x08) ||
                    ((pBuffer[12 + m_extLength] & 0x1F) == 0x06) || ((pBuffer[12 + m_extLength] & 0x1F) == 0x01))) { // sps或pps
            m_pBuffer[8 + m_extLength] = 0x00;
            m_pBuffer[9 + m_extLength] = 0x00;
            m_pBuffer[10 + m_extLength] = 0x00;
            m_pBuffer[11 + m_extLength] = 0x01;
            m_pPayloadData = m_pBuffer + m_extLength + 8;
            m_nPayloadDataLen = bufferSize - m_extLength - 8;
        } else {
            m_pPayloadData = m_pBuffer + m_extLength + 12;
            m_nPayloadDataLen = bufferSize - m_extLength - 12;
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
