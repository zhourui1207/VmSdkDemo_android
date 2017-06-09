package com.joyware.vmsdk.util;

import android.util.Log;

import java.io.File;

/**
 * Created by zhourui on 16/12/27.
 * email: 332867864@qq.com
 * phone: 17006421542
 */

public class FileUtil {

    private static final String TAG = "FileUtil";

    /**
     * 创建路径
     *
     * @param path 文件的全路径
     * @return
     */
    public static boolean cratePath(String path) {
        if (isExist(path)) {
            return true;
        }

        String[] dirs = path.split("/");
        // 分割后一层层创建，有些手机无法一次性创建多层目录
        StringBuilder fullPath = new StringBuilder("");

        boolean createSuccess = true;
        try {
            // 创建文件夹
            for (String dir1 : dirs) {
                String dir = dir1.trim();
                if (dir.contains(".")) {  // 如果包含后缀名，那么就认为是文件名，不进行创建
                    break;
                }
                if (!dir.equalsIgnoreCase("")) {
                    fullPath.append("/");
                    fullPath.append(dir);
                    File fileDir = new File(fullPath.toString());
                    if (!fileDir.exists()) {  // 文件夹不存在的话
                        if (!fileDir.mkdir()) {
                            createSuccess = false;
                            break;
                        }
                    }
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "cratePath failed！" + e.getMessage());
            return false;
        }

        return createSuccess;
    }

    /**
     * 判断文件或文件夹是否存在
     *
     * @param file 文件或路径的全路径
     * @return
     */
    public static boolean isExist(String file) {
        File fileFile = new File(file);
        return fileFile.exists();
    }

}
