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
import android.content.Intent;
import android.content.SharedPreferences;
import android.util.AttributeSet;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceManager;

public class Hspa extends ListPreference implements OnPreferenceChangeListener {

    private static final String FILE = "/system/app/SamsungServiceMode.apk";
    private Context mCtx;

    public Hspa(Context context, AttributeSet attrs) {
        super(context, attrs);
        this.setOnPreferenceChangeListener(this);
        mCtx = context;
    }

    public static boolean isSupported() {
        return Utils.fileExists(FILE);
    }

    /**
     * Restore hspa setting from SharedPreferences. (Write to kernel.)
     * @param context       The context to read the SharedPreferences from
     */
    public static void restore(Context context) {
        if (!isSupported()) {
            return;
        }

        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        sendIntent(context, sharedPrefs.getString(DeviceSettings.KEY_HSPA, "23"));
    }

    public boolean onPreferenceChange(Preference preference, Object newValue) {
        sendIntent(mCtx, (String) newValue);
        return true;
    }

    private static void sendIntent(Context context, String value) {
        Intent i = new Intent("com.cyanogenmod.SamsungServiceMode.EXECUTE");
        i.putExtra("sub_type", 20); // HSPA Setting
        i.putExtra("data", value);
        context.sendBroadcast(i);
    }
}
