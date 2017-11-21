//
// Created by zhou rui on 2017/11/10.
//

#ifndef DREAM_OBCPPACKET_H
#define DREAM_OBCPPACKET_H


#include <cstddef>
#include <cstring>

namespace Dream {

    /////////////////////////////////////
    // OBCP协议报文头

    typedef struct {
        char mark[2] = {'O', 'B'};    /**<前导表识，为 "OB"的ASCII码，方便调试，用于定位部*/
        char version = 10;    /**<协议版本号，当前为十进制10*/
        char encoded = 0;    /**<编码方式，0：明文，1-255：采用的加密方式(密钥编号). RC4加密。若加密，密文从length字段后开始*/
        unsigned int length = 56;        /**<mode、error、product sequence、command、data字段总长度，单位为字节。*/
        unsigned short mode = 1;        /**<操作方式：1：请求, 2：应答, 9:通告与通知，11: 反向命令请求，12:反向命令应答*/
        unsigned short errCode = 0;         /**<错误号，命令请求时该值为0；命令应答时成功为0，其他值详见命令说明*/
        unsigned char product[4] = {
                0};  /**<产品标识，BYTE[3]:设备类型, BYTE[2]:主版本号, BYTE[1]:子版本号, BYTE[0]:分支编号，用于检测和标示命令所属的产品版本，便于客户端区分处理*/
        unsigned int sequence = 0;    /**<流水号，用于标示应答与请求的对应关系，防止重复或乱序。由发起端填写，并递增累进，应答端必须将该字段复制。*/
        unsigned int command = 0;     /**<录像机命令编号*/
        unsigned int checkSum = 0;    /**<校验和 TODO: 在这里添加校验和算法说明*/
        int userID = 0;      /**<用户ID*/
        int status = 0;      /**<返回状态值 TODO: 在这里添加返回状态值列表说明*/
        unsigned char res[28];     /**<保留字节,当前使用28字节,这里再保留28字节以便后续扩展*/
    } ObcpHeader;

    class ObcpPacket {
    public:
        static const unsigned int COMMAND_DEVICE_DISCOVERY = 0x0001dd00;
        static const size_t HEADER_HENGHT = sizeof(ObcpHeader);

    public:
        ObcpPacket(unsigned int command = 0, unsigned int sequence = 0, const char *data = nullptr,
                   size_t dataLen = 0)
                : _data(nullptr), _dataLen(0) {
            _header.command = command;
            _header.sequence = sequence;
            if (data && dataLen > 0) {
                _dataLen = dataLen;
                _data = new char[_dataLen];
                memcpy(_data, data, _dataLen);
            }
        }

        virtual ~ObcpPacket() {
            if (_data) {
                delete[] _data;
                _data = nullptr;
            }
        }

        // 序列化
        virtual int encode(char* buf, size_t bufLen) const {
            size_t obcpHeaderLen = sizeof(ObcpHeader);
            size_t packetLen = obcpHeaderLen + _dataLen;
            if (bufLen < packetLen) {
                return -1;
            }
            memcpy(buf, &_header, obcpHeaderLen);
            if (_data && _dataLen > 0) {
                memcpy(buf + obcpHeaderLen, _data, _dataLen);
            }
            return packetLen;
        }

        // 反序列化
        virtual int decode(const char* buf, size_t bufLen) {
            size_t obcpHeaderLen = sizeof(ObcpHeader);
            if (bufLen < obcpHeaderLen) {
                return -1;
            }
            memcpy(&_header, buf, obcpHeaderLen);
            int leaveLen = bufLen - obcpHeaderLen;
            if (leaveLen > 0) {
                if (_data) {
                    delete [] _data;
                }
                _dataLen = (size_t) leaveLen;
                _data = new char[_dataLen];
                memcpy(_data, buf + obcpHeaderLen, (size_t) leaveLen);
            }
            return bufLen;
        }

        const char* data() const {
            return _data;
        }

        size_t dataLen() const {
            return _dataLen;
        };

    private:
        ObcpHeader _header;
        char *_data;
        size_t _dataLen;
    };

}

#endif //VMSDKDEMO_ANDROID_OBCPPACKET_H
