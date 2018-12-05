MY_LOCAL_PATH:= $(call my-dir)

RM_CBB_PATH := $(MY_LOCAL_PATH)/../../RMCBB_Release
##include $(RM_CBB_PATH)/Common/Release/Common-android/Android.mk


LOCAL_PATH := $(MY_LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_SRC_FILES += \
	src/ConnectCtrl.cpp \
	src/CCANDataBYDStandard.cpp \
    src/Main.cpp

LOCAL_CFLAGS +=-D__SVN_PATH__="\"$(shell svn info $(LOCAL_PATH)|grep URL|cut -d" " -f2)\""
LOCAL_CFLAGS +=-D__SVN_REVERSION__="\"$(shell svn info $(LOCAL_PATH)|grep "最后修改的版本:"|sed -n '1p'|cut -d" " -f2)\""
LOCAL_CFLAGS +=-D__SVN_LASTDATE__="\"$(shell svn info $(LOCAL_PATH)|grep "最后修改的时间:"|sed -n '1p'|cut -d" " -f2)\""
LOCAL_CFLAGS +=-D__SVN_LASTTIME__="\"$(shell svn info $(LOCAL_PATH)|grep "最后修改的时间:"|sed -n '1p'|cut -d" " -f3)\""

LOCAL_MODULE := CANUploadManager

LOCAL_C_INCLUDES+= . \
	bionic \
	external/stlport/stlport \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/product \
	$(LOCAL_PATH)/../../RMCBB_Release/Common/Release/Common-android/include/systemtime \
	$(LOCAL_PATH)/../../RMCBB_Release/Common/Release/Common-android/include/message \
	$(LOCAL_PATH)/../../RMCBB_Release/Common/Release/Common-android/include/json \
	$(LOCAL_PATH)/../../RMCBB_Release/Common/Release/Common-android/include/general \
	$(LOCAL_PATH)/../../RMCBB_Release/Characters/Release/Characters-android/include \
	$(LOCAL_PATH)/../../RMCBB_Release/SysConfig/Release/SysConfig-android/include \
	$(LOCAL_PATH)/../../RMCBB_Release/Common/Release/Common-android/include/streambuffer \
	$(LOCAL_PATH)/../../RMCBB_Release/DatahubTalker/include \
	$(LOCAL_PATH)/../../RMCBB_Release/DeviceManager/include \
	$(LOCAL_PATH)/../../RMCBB_Release/include \
	$(LOCAL_PATH)/include/thirdparty \
	$(LOCAL_PATH)/include/common

LOCAL_SHARED_LIBRARIES += libcutils   \
                          libutils    \
                          libstlport \
                          libdl \
                          libz    \
                          libhardware \
                          libCommon \
                          libdbgprint \
                          libgdrifts \
                          libhardware_legacy \
                          libiconv \
                          libdevprotoadapter \
						  libSysConfig
							
LOCAL_LDLIBS += -lc -lpthread -ldl -llog -lreadline
LIBS_c += -lreadline

LOCAL_MODULE_PATH := $(LOCAL_PATH)/../../E6/bin

include $(BUILD_EXECUTABLE)
