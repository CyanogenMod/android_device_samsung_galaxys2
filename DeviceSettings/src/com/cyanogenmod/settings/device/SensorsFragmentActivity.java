/*
 * Copyright (C) 2012 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.cyanogenmod.settings.device;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.util.Log;

import com.cyanogenmod.settings.device.R;

public class SensorsFragmentActivity extends PreferenceFragment {

    private static final String PREF_ENABLED = "1";
    private static final String TAG = "GalaxyS2Parts_General";

    private static final String FILE_USE_GYRO_CALIB = "/sys/class/sec/gsensorcal/calibration";
    private static final String FILE_TOUCHKEY_LIGHT = "/data/.disable_touchlight";
    private static final String FILE_TOUCHKEY_TOGGLE = "/sys/class/sec/sec_touchkey/brightness";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.xml.sensors_preferences);

        PreferenceScreen prefSet = getPreferenceScreen();

    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {

        String boxValue;
        String key = preference.getKey();

        Log.w(TAG, "key: " + key);

        if (key.compareTo(DeviceSettings.KEY_USE_GYRO_CALIBRATION) == 0) {
            boxValue = (((CheckBoxPreference)preference).isChecked() ? "1" : "0");
            Utils.writeValue(FILE_USE_GYRO_CALIB, boxValue);
        } else if (key.compareTo(DeviceSettings.KEY_CALIBRATE_GYRO) == 0) {
            // when calibration data utilization is disablen and enabled back,
            // calibration is done at the same time by driver
            Utils.writeValue(FILE_USE_GYRO_CALIB, "0");
            Utils.writeValue(FILE_USE_GYRO_CALIB, "1");
            Utils.showDialog((Context)getActivity(), "Calibration done", "The gyroscope has been successfully calibrated!");
        } else if (key.compareTo(DeviceSettings.KEY_TOUCHKEY_LIGHT) == 0) {
            Utils.writeValue(FILE_TOUCHKEY_LIGHT, ((CheckBoxPreference)preference).isChecked() ? "1" : "0");
            Utils.writeValue(FILE_TOUCHKEY_TOGGLE, ((CheckBoxPreference)preference).isChecked() ? "1" : "2");
        }

        return true;
    }

    public static boolean isSupported(String FILE) {
        return Utils.fileExists(FILE);
    }

    public static void restore(Context context) {
        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        String gyroCalib = sharedPrefs.getString(DeviceSettings.KEY_USE_GYRO_CALIBRATION, "1");

        // When use gyro calibration value is set to 1, calibration is done at the same time, which
        // means it is reset at each boot, providing wrong calibration most of the time at each reboot.
        // So we only set it to "0" if user wants it, as it defaults to 1 at boot
        if (gyroCalib.compareTo("1") != 0)
            Utils.writeValue(FILE_USE_GYRO_CALIB, gyroCalib);

        Utils.writeValue(FILE_TOUCHKEY_LIGHT, sharedPrefs.getString(DeviceSettings.KEY_TOUCHKEY_LIGHT, "1"));
    }
}
