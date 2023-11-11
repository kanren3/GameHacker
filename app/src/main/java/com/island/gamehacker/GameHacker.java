package com.island.gamehacker;

import android.os.Build;
import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.XposedHelpers;
import de.robv.android.xposed.callbacks.XC_LoadPackage.LoadPackageParam;
import de.robv.android.xposed.XC_MethodHook;

public class GameHacker implements IXposedHookLoadPackage{
    public void handleLoadPackage(final LoadPackageParam lpparam) throws Throwable {
        if (lpparam.packageName.equals("com.tencent.game.rhythmmaster")) {
            System.loadLibrary("rhythmmasterhacker");
        }
        else if (lpparam.packageName.equals("com.shining.nikki4.tw")) {
            System.loadLibrary("nikkihacker");
        }
    }
}