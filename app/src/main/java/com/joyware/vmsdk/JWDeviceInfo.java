package com.joyware.vmsdk;

/**
 * Created by zhourui on 2017/11/10.
 * email: 332867864@qq.com
 * phone: 17130045945
 */

//typedef struct JW_DEVICE_INFO {
//        uint8_t product[4];  // 产品标识
//        char macId[8];  // 前两字节为0，后6个mac地址
//        char obcpVersion;  // 支持的obcp版本号，为10
//        char obcpCrypt;  // 支持的obcp密钥编号，缺省为1
//        uint16_t obcpPort;  // obcp监听端口
//        uint32_t obcpAddr;  // ip地址
//        uint32_t netMask;  // 子网掩码
//        uint32_t gateWay;  // 缺省网关
//        char module[32];  // 设备型号
//        char version[32];  // 设备版本
//        char serial[32];  // 产品序列号
//        char hardware[32];  // 硬件版本
//        // 上方有156字节
//
//        // 2017-10-26新增
//        char dspVersion[32];  // dsp版本号
//        uint32_t bootTime;  // 系统启动时间
//        uint8_t support[8];  // 能力集  bit1: 是否支持应答机制; bit2: 是否支持密码重置; bit3: 是否支持http端口; bit4: 是否支持激活
//        uint16_t httpPort;  // http端口
//        uint8_t activated;  // 是否激活 0-无效, 1-已激活, 2-未激活
//        uint8_t resv1;  // 预留
//        uint8_t resv2[52];  // 预留
//        } JWDeviceInfo;

public class JWDeviceInfo {
    public byte[] product = new byte[4];
    public byte[] macId = new byte[8];
    public byte obcpVersion;
    public byte obcpCrypt;
    public short obcpPort;
    public int obcpAddr;
    public int netMask;
    public int gateWay;
    public byte[] module = new byte[32];
    public byte[] version = new byte[32];
    public byte[] serial = new byte[32];
    public byte[] hardware = new byte[32];
    public byte[] dspVersion = new byte[32];
    public int bootTime;
    public byte[] support = new byte[8];
    public short httpPort;
    public byte activated;
    public byte resv1;
    public byte[] resv2 = new byte[52];

    public JWDeviceInfo(byte[] product, byte[] macId, byte obcpVersion, byte obcpCrypt, short
            obcpPort, int obcpAddr, int netMask, int gateWay, byte[] module, byte[] version,
                        byte[] serial, byte[] hardware, byte[] dspVersion, int bootTime, byte[]
                                support, short httpPort, byte activated, byte resv1, byte[] resv2) {
        this.product = product;
        this.macId = macId;
        this.obcpVersion = obcpVersion;
        this.obcpCrypt = obcpCrypt;
        this.obcpPort = obcpPort;
        this.obcpAddr = obcpAddr;
        this.netMask = netMask;
        this.gateWay = gateWay;
        this.module = module;
        this.version = version;
        this.serial = serial;
        this.hardware = hardware;
        this.dspVersion = dspVersion;
        this.bootTime = bootTime;
        this.support = support;
        this.httpPort = httpPort;
        this.activated = activated;
        this.resv1 = resv1;
        this.resv2 = resv2;
    }
}
