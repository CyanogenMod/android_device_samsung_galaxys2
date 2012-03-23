ifeq ($(BOARD_USE_EXYNOS_OMX), true)
codec_dir := exynos_codecs
else
codec_dir := sec_codecs
endif

include $(call all-named-subdir-makefiles, $(codec_dir))
