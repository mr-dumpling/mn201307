#
# 
#autotest makefile
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(AUTOTST_DEVLP_DBG),true)
LOCAL_CPPFLAGS += -DAUTOTST_DEVLP_DBG
endif

ifeq ($(BOARD_SPRD_WCNBT_MARLIN),true)
LOCAL_CFLAGS  += -DSPRD_WCNBT_MARLIN
endif

ifeq ($(BOARD_SPRD_WCNBT_SR2351),true)
LOCAL_CFLAGS += -DSPRD_WCNBT_SR2351
endif

LOCAL_MODULE_TAGS:= optional

LOCAL_MODULE:= autotest

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES:= \
    atv.cpp      \
    audio.cpp    \
    battery.cpp  \
    bt_fm_mixed.cpp       \
    camera.cpp   \
    channel.cpp  \
    cmmb.cpp     \
    debug.cpp  \
    diag.cpp   \
    driver.cpp \
    fm.cpp     \
    gps.cpp    \
    input.cpp  \
    lcd.cpp    \
    light.cpp  \
    perm.cpp   \
    sensor.cpp \
    sim.cpp    \
    tcard.cpp  \
    tester.cpp \
    tester_main.cpp \
    tester_dbg.cpp  \
    util.cpp        \
    ver.cpp         \
    vibrator.cpp    \
    wifi.cpp        \
    power.cpp       \
    otg.cpp	    \
    main.cpp   


LOCAL_C_INCLUDES:= \
    external/bluetooth/bluez/lib \
    external/bluetooth/bluez/src \
    frameworks/base/include \
    frameworks/av/include \
    system/bluetooth/bluedroid/include \
    system/core/include \
    hardware/libhardware/include \
    hardware/libhardware_legacy/include \
    external/bluetooth/bluedroid/btif/include \
    external/bluetooth/bluedroid/gki/ulinux \
    external/bluetooth/bluedroid/stack/include \
    external/bluetooth/bluedroid/stack/btm \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video

#    libbluedroid \
#    libbluetooth \
#    libbluetoothd\

LOCAL_SHARED_LIBRARIES:= \
    libbinder \
    liblog \
    libcamera_client \
    libcutils    \
    libdl        \
    libgui       \
    libhardware  \
    libhardware_legacy \
    libmedia \
    libui    \
    libutils 

LOCAL_STATIC_LIBRARIES:= \
    libbt-utils\
    libbt-hci


include $(BUILD_EXECUTABLE)
