/*
 * Copyright@ Samsung Electronics Co. LTD
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

#ifndef _AUDIO_H_
#define _AUDIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __HDMI_AUDIO_HDMIAUDIOPORT__
#define __HDMI_AUDIO_HDMIAUDIOPORT__
/**
 * @enum HDMIAudioPort
 * Available audio inputs on HDMI HW module.
 */
enum HDMIAudioPort {
    /** I2S input port */
    I2S_PORT,
    /** SPDIF input port */
    SPDIF_PORT,
    /** DSD input port */
    DSD_PORT,
};
#endif /* __HDMI_AUDIO_HDMIAUDIOPORT__ */

#ifndef __HDMI_AUDIO_AUDIOFORMAT__
#define __HDMI_AUDIO_AUDIOFORMAT__
/**
 * @enum AudioFormat
 * The encoding format of audio stream
 */
enum AudioFormat {
    /** LPCM encoding format */
    LPCM_FORMAT = 1,
    /** AC3 encoding format */
    AC3_FORMAT,
    /** MPEG1 encoding format */
    MPEG1_FORMAT,
    /** MP3 encoding format */
    MP3_FORMAT,
    /** MPEG2 encoding format */
    MPEG2_FORMAT,
    /** AAC encoding format */
    AAC_FORMAT,
    /** DTS encoding format */
    DTS_FORMAT,
    /** ATRAC encoding format */
    ATRAC_FORMAT,
    /** DSD encoding format */
    DSD_FORMAT,
    /** Dolby Digital+ encoding format */
    Dolby_Digital_Plus_FORMAT,
    /** DTS HD encoding format */
    DTS_HD_FORMAT,
    /** MAT encoding format */
    MAT_FORMAT,
    /** DST encoding format */
    DST_FORMAT,
    /** WAM_Pro encoding format */
    WAM_Pro_FORMAT
};
#endif /* __HDMI_AUDIO_AUDIOFORMAT__ */

#ifndef __HDMI_AUDIO_LPCMWORDLENGTH__
#define __HDMI_AUDIO_LPCMWORDLENGTH__
/**
 * @enum LPCM_WordLen
 * Word length of LPCM audio stream.
 */
enum LPCM_WordLen {
    /** 16bit word length */
    WORD_16 = 0,
    /** 17bit word length */
    WORD_17,
    /** 18bit word length */
    WORD_18,
    /** 19bit word length */
    WORD_19,
    /** 20bit word length */
    WORD_20,
    /** 21bit word length */
    WORD_21,
    /** 22bit word length */
    WORD_22,
    /** 23bit word length */
    WORD_23,
    /** 24bit word length */
    WORD_24
};
#endif /* __HDMI_AUDIO_LPCMWORDLENGTH__ */

#ifndef __HDMI_AUDIO_SAMPLINGFREQUENCY__
#define __HDMI_AUDIO_SAMPLINGFREQUENCY__
/**
 * @enum SamplingFreq
 * Sampling frequency of audio stream.
 */
enum SamplingFreq {
    /** 32KHz sampling frequency */
    SF_32KHZ = 0,
    /** 44.1KHz sampling frequency */
    SF_44KHZ,
    /** 48KHz sampling frequency */
    SF_48KHZ,
    /** 88.2KHz sampling frequency */
    SF_88KHZ,
    /** 96KHz sampling frequency */
    SF_96KHZ,
    /** 176.4KHz sampling frequency */
    SF_176KHZ,
    /** 192KHz sampling frequency */
    SF_192KHZ
};
#endif /* __HDMI_AUDIO_SAMPLINGFREQUENCY__ */

#ifndef __HDMI_AUDIO_CHANNELNUMBER__
#define __HDMI_AUDIO_CHANNELNUMBER__
/**
 * @enum ChannelNum
 * Channel number of audio stream.
 */
enum ChannelNum {
    /** 2 channel audio stream */
    CH_2 = 2,
    /** 3 channel audio stream */
    CH_3,
    /** 4 channel audio stream */
    CH_4,
    /** 5 channel audio stream */
    CH_5,
    /** 6 channel audio stream */
    CH_6,
    /** 7 channel audio stream */
    CH_7,
    /** 8 channel audio stream */
    CH_8,
};
#endif /* __HDMI_AUDIO_CHANNELNUMBER__ */

#ifndef __HDMI_AUDIO_AUDIOSAMPLEPACKETTYPE__
#define __HDMI_AUDIO_AUDIOSAMPLEPACKETTYPE__
/**
 * @enum HDMIASPType
 * Type of HDMI audio sample packet
 */
enum HDMIASPType {
    /** Audio Sample Packet Type */
    HDMI_ASP,
    /** One Bit Audio Packet Type */
    HDMI_DSD,
    /** High Bit Rate Packet Type */
    HDMI_HBR,
    /** DST Packet Type */
    HDMI_DST
};
#endif /* __HDMI_AUDID_AUDIOSAMPLEPACKETTYPE__ */

#ifndef __HDMI_AUDIO_I2S_CUV_AUDIO_CODING_TYPE__
#define __HDMI_AUDIO_I2S_CUV_AUDIO_CODING_TYPE__
/**
 * @enum CUVAudioCoding
 * Audio coding type information for CUV fields.
 */
enum CUVAudioCoding {
    /** Linear PCM coding type */
    CUV_LPCM,
    /** Non-linear PCM coding type */
    CUV_NLPCM
};
#endif /* __HDMI_AUDIO_I2S_CUV_AUDIO_CODING_TYPE__ */

#ifndef __HDMI_AUDIO_SPDIF_AUDIO_CODING_TYPE__
#define __HDMI_AUDIO_SPDIF_AUDIO_CODING_TYPE__
/**
 * @enum SPDIFAudioCoding
 * Audio coding type information for SPDIF input port.
 */
enum SPDIFAudioCoding {
    /** Linear PCM coding type */
    SPDIF_LPCM,
    /** Non-linear PCM coding type */
    SPDIF_NLPCM
};
#endif /* __HDMI_AUDIO_SPDIF_AUDIO_CODING_TYPE__ */

#ifndef __HDMI_AUDIO_I2S_CUV_CHANNEL_NUMBER__
#define __HDMI_AUDIO_I2S_CUV_CHANNEL_NUMBER__
/**
 * @enum CUVChannelNumber
 * Channel number information for CUV fields.
 */
enum CUVChannelNumber {
    /** Unknown channel audio stream */
    CUV_CH_UNDEFINED = 0,
    /** 1 channel audio stream */
    CUV_CH_01,
    /** 2 channel audio stream */
    CUV_CH_02,
    /** 3 channel audio stream */
    CUV_CH_03,
    /** 4 channel audio stream */
    CUV_CH_04,
    /** 5 channel audio stream */
    CUV_CH_05,
    /** 6 channel audio stream */
    CUV_CH_06,
    /** 7 channel audio stream */
    CUV_CH_07,
    /** 8 channel audio stream */
    CUV_CH_08,
    /** 9 channel audio stream */
    CUV_CH_09,
    /** 10 channel audio stream */
    CUV_CH_10,
    /** 11 channel audio stream */
    CUV_CH_11,
    /** 12 channel audio stream */
    CUV_CH_12,
    /** 13 channel audio stream */
    CUV_CH_13,
    /** 14 channel audio stream */
    CUV_CH_14,
    /** 15 channel audio stream */
    CUV_CH_15,
};
#endif /* __HDMI_AUDIO_I2S_CUV_CHANNEL_NUMBER__ */

#ifndef __HDMI_AUDIO_I2S_CUV_WORD_LENGTH__
#define __HDMI_AUDIO_I2S_CUV_WORD_LENGTH__
/**
 * @enum CUVWordLength
 * Word length information of LPCM audio stream for CUV fields.
 */
enum CUVWordLength {
    /** Max word length is 20 bits, number of valid bits is not defined */
    CUV_WL_20_NOT_DEFINED,
    /** Max word length is 20 bits, 16 bits are valid */
    CUV_WL_20_16,
    /** Max word length is 20 bits, 18 bits are valid */
    CUV_WL_20_18,
    /** Max word length is 20 bits, 19 bits are valid */
    CUV_WL_20_19,
    /** Max word length is 20 bits, 20 bits are valid */
    CUV_WL_20_20,
    /** Max word length is 20 bits, 17 bits are valid */
    CUV_WL_20_17,
    /** Max word length is 24 bits, number of valid bits is not defined */
    CUV_WL_24_NOT_DEFINED,
    /** Max word length is 24 bits, 20 bits are valid */
    CUV_WL_24_20,
    /** Max word length is 24 bits, 22 bits are valid */
    CUV_WL_24_22,
    /** Max word length is 24 bits, 23 bits are valid */
    CUV_WL_24_23,
    /** Max word length is 24 bits, 24 bits are valid */
    CUV_WL_24_24,
    /** Max word length is 24 bits, 21 bits are valid */
    CUV_WL_24_21,
};
#endif /* __HDMI_AUDIO_I2S_CUV_WORD_LENGTH__ */

#ifndef __HDMI_AUDIO_I2S_BITS_PER_CHANNEL__
#define __HDMI_AUDIO_I2S_BITS_PER_CHANNEL__

/**
 * @enum I2SBitsPerChannel
 * Serial data bit per channel in I2S audio stream.
 */
enum I2SBitsPerChannel {
    /** 16 bits per channel */
    I2S_BPC_16,
    /** 20 bits per channel */
    I2S_BPC_20,
    /** 24 bits per channel */
    I2S_BPC_24
};

#endif /* __HDMI_AUDIO_I2S_BITS_PER_CHANNEL__ */

#ifndef __HDMI_AUDIO_I2S_DATA_FORMAT__
#define __HDMI_AUDIO_I2S_DATA_FORMAT__

/**
 * @enum I2SDataFormat
 * Foramt of data in I2S audio stream.
 */
enum I2SDataFormat {
    /** Basic format */
    I2S_BASIC,
    /** Left justified format */
    I2S_LEFT_JUSTIFIED,
    /** Right justified format */
    I2S_RIGHT_JUSTIFIED
};

#endif /* __HDMI_AUDIO_I2S_DATA_FORMAT__ */

#ifndef __HDMI_AUDIO_I2S_CLOCK_PER_FRAME__
#define __HDMI_AUDIO_I2S_CLOCK_PER_FRAME__

/**
 * @enum I2SClockPerFrame
 * Bit clock per Frame in I2S audio stream.
 */
enum I2SClockPerFrame {
    /** 32 clock per Frame */
    I2S_32FS,
    /** 48 clock per Frame */
    I2S_48FS,
    /** 64 clock per Frame */
    I2S_64FS
};

#endif /* __HDMI_AUDIO_I2S_CLOCK_PER_FRAME__ */

#ifndef __HDMI_AUDIO_I2S_PARAMETER__
#define __HDMI_AUDIO_I2S_PARAMETER__

//! Structure for I2S audio stream
struct I2SParameter {
    enum I2SBitsPerChannel bpc;
    enum I2SDataFormat format;
    enum I2SClockPerFrame clk;
};
#endif /* __HDMI_AUDIO_I2S_PARAMETER__ */

//! Structure for HDMI audio input
struct HDMIAudioParameter {
    /** Input audio port to HDMI HW */
    enum HDMIAudioPort inputPort;
    /** Output Packet type **/
    enum HDMIASPType outPacket;
    /** Encoding format */
    enum AudioFormat formatCode;
    /** Channel number */
    enum ChannelNum channelNum;
    /** Sampling frequency */
    enum SamplingFreq sampleFreq;
    /** Word length. This is avaliable only if LPCM encoding format */
    enum LPCM_WordLen wordLength;
    /** structure for I2S audio stream */
    struct I2SParameter i2sParam;
};

#ifdef __cplusplus
}
#endif

#endif // _AUDIO_H_
