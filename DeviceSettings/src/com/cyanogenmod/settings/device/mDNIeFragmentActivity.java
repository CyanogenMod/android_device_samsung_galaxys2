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

import android.os.Bundle;
import android.preference.PreferenceFragment;
import com.cyanogenmod.settings.device.R;

public class mDNIeFragmentActivity extends PreferenceFragment {

    private mDNIeScenario mmDNIeScenario;
    private mDNIeMode mmDNIeMode;
    private mDNIeNegative mmDNIeNegative;
    private mDNIeOutdoor mmDNIeOutdoor;
    private PanelGamma mPanelGamma;
    private TouchscreenSensitivity mTouchscreenSensitivity;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.xml.mdnie_preferences);

        mmDNIeScenario = (mDNIeScenario) findPreference(DeviceSettings.KEY_MDNIE_SCENARIO);
        mmDNIeScenario.setEnabled(mDNIeScenario.isSupported());

        mmDNIeMode = (mDNIeMode) findPreference(DeviceSettings.KEY_MDNIE_MODE);
        mmDNIeMode.setEnabled(mDNIeMode.isSupported());

        mmDNIeNegative = (mDNIeNegative) findPreference(DeviceSettings.KEY_MDNIE_NEGATIVE);
        mmDNIeNegative.setEnabled(mDNIeNegative.isSupported());

        mmDNIeOutdoor = (mDNIeOutdoor) findPreference(DeviceSettings.KEY_MDNIE_OUTDOOR);
        mmDNIeOutdoor.setEnabled(mDNIeOutdoor.isSupported());

        mPanelGamma = (PanelGamma) findPreference(DeviceSettings.KEY_PANEL_GAMMA);
        mPanelGamma.setEnabled(mPanelGamma.isSupported());

        mTouchscreenSensitivity = (TouchscreenSensitivity) findPreference(DeviceSettings.KEY_TOUCHSCREEN_SENSITIVITY);
        mTouchscreenSensitivity.setEnabled(mTouchscreenSensitivity.isSupported());
    }

}
