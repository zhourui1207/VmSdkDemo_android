/*
 * ControlType.h
 *
 *  Created on: 2016年10月11日
 *      Author: zhourui
 */

#ifndef CONTROLTYPE_H_
#define CONTROLTYPE_H_

namespace Dream {

    enum CONTROL_TYPE {
        CONTROL_TYPE_PTZ_STOP = 0,  // 云台停止，参数1、2保留
        CONTROL_TYPE_PTZ_UP = 1,  // 云台上。参数1为速度值，速度值范围：从0 到10；参数2为自动停止时间，单位为秒 注:现在服务端自动停止还没实现！！
        CONTROL_TYPE_PTZ_DOWN = 2,  // 云台下
        CONTROL_TYPE_PTZ_LEFT = 3,  // 云台左
        CONTROL_TYPE_PTZ_RIGHT = 4,  // 云台右
        CONTROL_TYPE_PTZ_LEFT_UP = 5,  // 云台左上
        CONTROL_TYPE_PTZ_RIGHT_UP = 6,  // 云台右上
        CONTROL_TYPE_PTZ_LEFT_DOWN = 7,  // 云台左下
        CONTROL_TYPE_PTZ__RIGHT_DOWN = 8,  // 云台右下
        CONTROL_TYPE_IRIS_OPEN = 20,  // 增大光圈
        CONTROL_TYPE_IRIS_CLOSE = 21,  // 减小光圈
        CONTROL_TYPE_ZOOM_WIDE = 30,  // 收缩镜头
        CONTROL_TYPE_ZOOM_IN = 31,  // 伸长镜头
        CONTROL_TYPE_FOCUS_FAR = 40,  // 增大焦距
        CONTROL_TYPE_FOCUS_NEAR = 41,  // 减小焦距
        CONTROL_TYPE_PRE_POINT_ADD = 50,  // 增加预置点。参数1为预置点号值；从1开始编号
        CONTROL_TYPE_PRE_POINT_DEL = 51,  // 删除预置点
        CONTROL_TYPE_PRE_POINT_GOTO = 52,  // 转到预置点
        CONTROL_TYPE_SYN_TIME = 0x0200,  // 同步时间。参数1为时间值，参数2保留
        CONTROL_TYPE_IMAGE_BRIGHTNESS = 0x0100,  // 图像亮度。参数1为亮度值，参数2保留
        CONTROL_TYPE_IMAGE_SATURATION = 0x0101,  // 图像饱和度。参数1为饱和度值，参数2保留
        CONTROL_TYPE_IMAGE_CONTRAST = 0x0102,  // 图像对比度。参数1为对比度值，参数2保留
        CONTROL_TYPE_IMAGE_HUE = 0x0103  // 图像灰度。参数1为灰度值，参数2保留
    };

} /* namespace Dream */

#endif /* CONTROLTYPE_H_ */
