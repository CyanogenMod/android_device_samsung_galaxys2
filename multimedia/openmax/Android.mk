ifeq ($(BOARD_USE_EXYNOS_OMX), true)
omx_dir := exynos_omx
else
omx_dir := sec_omx
endif

include $(call all-named-subdir-makefiles, $(omx_dir))
