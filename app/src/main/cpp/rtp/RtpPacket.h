/*
 * RtpPacket.h
 *
 *  Created on: 2016年10月19日
 *      Author: zhourui
 */

#ifndef RTPPACKET_H_
#define RTPPACKET_H_

namespace Dream {

class RtpPacket
{
public:
  RtpPacket();
  virtual ~RtpPacket();

  int Parse(char* pBuffer, int bufferSize);

  void CreateRtpPacket(unsigned short seq, char* pData, int dataSize, char nPayloadType, unsigned timeStamp, int nSSRC, bool marker = false);
  bool ParseRtpHeader(unsigned char* pRtpHeader, bool *pMarker, char* pPayloadType, unsigned short * pSequenceNumber, unsigned* pTimeStamp, int* pSsrc);

public:
  int GetPayloadLength()
  {
    return m_nPayloadDataLen;
  }
  unsigned short GetSequenceNumber()
  {
    return m_nSequenceNumber;
  }
  char GetPayloadType()
  {
    return m_nPayloadType;
  }
  unsigned GetTimestamp()
  {
    return m_nTimeStamp;
  }
  char* GetPayloadData()
  {
    return m_pPayloadData;
  }
  bool HasMarker()
  {
    return m_bMarker;
  }
public:

  unsigned short m_nSequenceNumber;
  char m_nPayloadType;
  bool m_bMarker;
  unsigned m_nTimeStamp;
  int m_nSsrc;

  char* m_pBuffer;
  char* m_pPayloadData;
  int m_nPayloadDataLen;
};

} /* namespace Dream */

#endif /* RTPPACKET_H_ */
