#ifndef _VIDEO_H_
#define _VIDEO_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __HDMI_VIDEO_VIDEOFORMAT__
#define __HDMI_VIDEO_VIDEOFORMAT__
/**
 * @enum VideoFormat
 * Video format
 */
enum VideoFormat {
    /** 640x480p\@60Hz */
    v640x480p_60Hz = 0,
    /** 720x480p\@60Hz */
    v720x480p_60Hz,
    /** 1280x700p\@60Hz */
    v1280x720p_60Hz,
    /** 1920x1080i\@60Hz */
    v1920x1080i_60Hz,
    /** 720x480i\@60Hz */
    v720x480i_60Hz,
    /** 720x240p\@60Hz */
    v720x240p_60Hz,
    /** 2880x480i\@60Hz */
    v2880x480i_60Hz,
    /** 2880x240p\@60Hz */
    v2880x240p_60Hz,
    /** 1440x480p\@60Hz */
    v1440x480p_60Hz,
    /** 1920x1080p\@60Hz */
    v1920x1080p_60Hz,
    /** 720x576p\@60Hz */
    v720x576p_50Hz,
    /** 1280x720p\@50Hz */
    v1280x720p_50Hz,
    /** 1920x1080i\@50Hz (V total = 1125) */
    v1920x1080i_50Hz,
    /** 720x576i\@50Hz */
    v720x576i_50Hz,
    /** 720x288p\@50Hz */
    v720x288p_50Hz,
    /** 2880x576i\@50Hz */
    v2880x576i_50Hz,
    /** 2880x288p\@50Hz */
    v2880x288p_50Hz,
    /** 1440x576p\@50Hz */
    v1440x576p_50Hz,
    /** 1920x1080p\@50Hz */
    v1920x1080p_50Hz,
    /** 1920x1080p\@24Hz */
    v1920x1080p_24Hz,
    /** 1920x1080p\@25Hz */
    v1920x1080p_25Hz,
    /** 1920x1080p\@30Hz */
    v1920x1080p_30Hz,
    /** 2880x480p\@60Hz */
    v2880x480p_60Hz,
    /** 2880x576p\@60Hz */
    v2880x576p_50Hz,
    /** 1920x1080i\@50Hz (V total = 1250) */
    v1920x1080i_50Hz_1250,
    /** 1920x1080i\@100Hz */
    v1920x1080i_100Hz,
    /** 1280x720p\@100Hz */
    v1280x720p_100Hz,
    /** 720x576p\@100Hz */
    v720x576p_100Hz,
    /** 720x576i\@100Hz */
    v720x576i_100Hz,
    /** 1920x1080i\@120Hz */
    v1920x1080i_120Hz,
    /** 1280x720p\@120Hz */
    v1280x720p_120Hz,
    /** 720x480p\@120Hz */
    v720x480p_120Hz,
    /** 720x480i\@120Hz */
    v720x480i_120Hz,
    /** 720x576p\@200Hz */
    v720x576p_200Hz,
    /** 720x576i\@200Hz */
    v720x576i_200Hz,
    /** 720x480p\@240Hz */
    v720x480p_240Hz,
    /** 720x480i\@240Hz */
    v720x480i_240Hz,
    /** 1280x720p\@24Hz */
    v1280x720p_24Hz,
    /** 1280x720p\@25Hz */
    v1280x720p_25Hz,
    /** 1280x720p\@30Hz */
    v1280x720p_30Hz,
    /** 1920x1080p\@120Hz */
    v1920x1080p_120Hz,
    /** 1920x1080p\@100Hz */
    v1920x1080p_100Hz,
    /** 4Kx2K\@30Hz     */
    v4Kx2K_30Hz,
};
#endif /* __HDMI_VIDEO_VIDEOFORMAT__ */
#ifndef __HDMI_VIDEO_COLORSPACE__
#define __HDMI_VIDEO_COLORSPACE__
/**
 * @enum ColorSpace
 * Color space of video stream.
 */
enum ColorSpace {
    /** RGB color space */
    HDMI_CS_RGB,
    /** YCbCr 4:4:4 color space */
    HDMI_CS_YCBCR444,
    /** YCbCr 4:2:2 color space */
    HDMI_CS_YCBCR422
};
#endif /* __HDMI_VIDEO_COLORSPACE__ */

#ifndef __HDMI_VIDEO_COLORDEPTH__
#define __HDMI_VIDEO_COLORDEPTH__
/**
 * @enum ColorDepth
 * Color depth per pixel of video stream
 */
enum ColorDepth {
    /** 36 bit color depth per pixel */
    HDMI_CD_36,
    /** 30 bit color depth per pixel */
    HDMI_CD_30,
    /** 24 bit color depth per pixel */
    HDMI_CD_24
};
#endif /* __HDMI_VIDEO_COLORDEPTH__ */

#ifndef __HDMI_VIDEO_HDMIMODE__
#define __HDMI_VIDEO_HDMIMODE__
/**
 * @enum HDMIMode
 * System mode
 */
enum HDMIMode {
    /** DVI mode */
    DVI = 0,
    /** HDMI mode */
    HDMI
};
#endif /* __HDMI_VIDEO_HDMIMODE__ */

#ifndef __HDMI_VIDEO_PIXELLIMIT__
#define __HDMI_VIDEO_PIXELLIMIT__
/**
 * @enum PixelLimit
 * Pixel limitation of video stream
 */
enum PixelLimit {
    /** Full range */
    HDMI_FULL_RANGE,
    /** Limit range for RGB color space */
    HDMI_RGB_LIMIT_RANGE,
    /** Limit range for YCbCr color space */
    HDMI_YCBCR_LIMIT_RANGE
};
#endif /* __HDMI_VIDEO_PIXELLIMIT__ */

#ifndef __HDMI_VIDEO_COLORIMETRY__
#define __HDMI_VIDEO_COLORIMETRY__
/**
 * @enum HDMIColorimetry
 * Colorimetry of video stream
 */
enum HDMIColorimetry {
    /** Colorimetry is not defined */
    HDMI_COLORIMETRY_NO_DATA,
    /** ITU601 colorimetry */
    HDMI_COLORIMETRY_ITU601,
    /** ITU709 colorimetry */
    HDMI_COLORIMETRY_ITU709,
    /** Extended ITU601 colorimetry */
    HDMI_COLORIMETRY_EXTENDED_xvYCC601,
    /** Extended ITU709 colorimetry */
    HDMI_COLORIMETRY_EXTENDED_xvYCC709
};
#endif /* __HDMI_VIDEO_COLORIMETRY__ */

#ifndef __HDMI_VIDEO_PIXELASPECTRATIO__
#define __HDMI_VIDEO_PIXELASPECTRATIO__
/**
 * @enum PixelAspectRatio
 * Pixel aspect ratio of video stream
 */
enum PixelAspectRatio {
    /** as picutre pixel ratio */
    HDMI_PIXEL_RATIO_AS_PICTURE,
    /** 4:3 pixel ratio */
    HDMI_PIXEL_RATIO_4_3,
    /** 16:9 pixel ratio */
    HDMI_PIXEL_RATIO_16_9
};
#endif /* __HDMI_VIDEO_PIXELASPECTRATIO__ */

#ifndef __HDMI_VIDEO_PIXELFREQUENCY__
#define __HDMI_VIDEO_PIXELFREQUENCY__
/**
 * @enum PixelFreq
 * Pixel Frequency
 */
enum PixelFreq {
    /** 25.2 MHz pixel frequency */
    PIXEL_FREQ_25_200 = 2520,
    /** 25.175 MHz pixel frequency */
    PIXEL_FREQ_25_175 = 2517,
    /** 27 MHz pixel frequency */
    PIXEL_FREQ_27 = 2700,
    /** 27.027 MHz pixel frequency */
    PIXEL_FREQ_27_027 = 2702,
    /** 54 MHz pixel frequency */
    PIXEL_FREQ_54 = 5400,
    /** 54.054 MHz pixel frequency */
    PIXEL_FREQ_54_054 = 5405,
    /** 74.25 MHz pixel frequency */
    PIXEL_FREQ_74_250 = 7425,
    /** 74.176 MHz pixel frequency */
    PIXEL_FREQ_74_176 = 7417,
    /** 148.5 MHz pixel frequency */
    PIXEL_FREQ_148_500 = 14850,
    /** 148.352 MHz pixel frequency */
    PIXEL_FREQ_148_352 = 14835,
    /** 108.108 MHz pixel frequency */
    PIXEL_FREQ_108_108 = 10810,
    /** 72 MHz pixel frequency */
    PIXEL_FREQ_72 = 7200,
    /** 25 MHz pixel frequency */
    PIXEL_FREQ_25 = 2500,
    /** 65 MHz pixel frequency */
    PIXEL_FREQ_65 = 6500,
    /** 108 MHz pixel frequency */
    PIXEL_FREQ_108 = 10800,
    /** 162 MHz pixel frequency */
    PIXEL_FREQ_162 = 16200,
    /** 59.4 MHz pixel frequency */
    PIXEL_FREQ_59_400 = 5940,
};
#endif /* __HDMI_VIDEO_PIXELFREQUENCY__ */

#ifndef __HDMI_PHY_PIXELFREQUENCY__
#define __HDMI_PHY_PIXELFREQUENCY__

/**
 * @enum PHYFreq
 * PHY Frequency
 */
enum PHYFreq {
    /** Not supported */
    PHY_FREQ_NOT_SUPPORTED = -1,
    /** 25.2 MHz pixel frequency */
    PHY_FREQ_25_200 = 0,
    /** 25.175 MHz pixel frequency */
    PHY_FREQ_25_175,
    /** 27 MHz pixel frequency */
    PHY_FREQ_27,
    /** 27.027 MHz pixel frequency */
    PHY_FREQ_27_027,
    /** 54 MHz pixel frequency */
    PHY_FREQ_54,
    /** 54.054 MHz pixel frequency */
    PHY_FREQ_54_054,
    /** 74.25 MHz pixel frequency */
    PHY_FREQ_74_250,
    /** 74.176 MHz pixel frequency */
    PHY_FREQ_74_176,
    /** 148.5 MHz pixel frequency */
    PHY_FREQ_148_500,
    /** 148.352 MHz pixel frequency */
    PHY_FREQ_148_352,
    /** 108.108 MHz pixel frequency */
    PHY_FREQ_108_108,
    /** 72 MHz pixel frequency */
    PHY_FREQ_72,
    /** 25 MHz pixel frequency */
    PHY_FREQ_25,
    /** 65 MHz pixel frequency */
    PHY_FREQ_65,
    /** 108 MHz pixel frequency */
    PHY_FREQ_108,
    /** 162 MHz pixel frequency */
    PHY_FREQ_162,
    /** 59.4 MHz pixel frequency */
    PHY_FREQ_59_400,
};

#endif /* __HDMI_PHY_PIXELFREQUENCY__ */

#ifndef __HDMI_VIDEO_SOURCE__
#define __HDMI_VIDEO_SOURCE__
/**
 * @enum HDMIVideoSource
 * Type of video source.
 */
enum HDMIVideoSource {
    /** Internal Video Source */
    HDMI_SOURCE_INTERNAL,
    /** External Video Source */
    HDMI_SOURCE_EXTERNAL,
};
#endif /* __HDMI_VIDEO_SOURCE__ */

#ifndef __HDMI_3D_VIDEO_STRUCTURE__
#define __HDMI_3D_VIDEO_STRUCTURE__
/**
 * @enum HDMI3DVideoStructure
 * Type of 3D Video Structure
 */
enum HDMI3DVideoStructure {
    /** 2D Video Format  */
    HDMI_2D_VIDEO_FORMAT = -1,
    /** 3D Frame Packing Structure */
    HDMI_3D_FP_FORMAT = 0,
    /** 3D Field Alternative Structure  */
    HDMI_3D_FA_FORMAT,
    /** 3D Line Alternative Structure */
    HDMI_3D_LA_FORMAT,
    /** Side-by-Side(Full)Structure */
    HDMI_3D_SSF_FORMAT,
    /** 3D L+Depth Structure */
    HDMI_3D_LD_FORMAT,
    /** 3D L+Depth+Graphics Structure */
    HDMI_3D_LDGFX_FORMAT,
    /** 3D Top-and-Bottom Structure */
    HDMI_3D_TB_FORMAT,
    /** HDMI VIC Structure (ex. 4Kx2K) */
    HDMI_VIC_FORMAT,
    /** Side-by-Side(Half)Structure */
    HDMI_3D_SSH_FORMAT,
};
#endif /* __HDMI_3D_VIDEO_STRUCTURE__ */

#ifndef __HDMI_VIDEO_PARAMETER__
#define __HDMI_VIDEO_PARAMETER__
//! Structure for HDMI video
struct HDMIVideoParameter {
    /** Video interface */
    enum HDMIMode mode;
    /** Video format */
    enum VideoFormat resolution;
    /** Color space */
    enum ColorSpace colorSpace;
    /** Color depth */
    enum ColorDepth colorDepth;
    /** Colorimetry */
    enum HDMIColorimetry colorimetry;
    /** Pixel aspect ratio */
    enum PixelAspectRatio pixelAspectRatio;
    /** Video Source */
    enum HDMIVideoSource videoSrc;
    /** 3D Video Structure */
    enum HDMI3DVideoStructure hdmi_3d_format;
};
#endif /* __HDMI_VIDEO_PARAMETER__*/

#ifdef __cplusplus
}
#endif
#endif /* _VIDEO_H_ */
