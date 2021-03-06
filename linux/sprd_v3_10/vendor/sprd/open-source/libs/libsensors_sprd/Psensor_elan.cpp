/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2011 Sorin P. <sorin@hypermagik.com>
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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>

#include <cutils/log.h>

#include "PSensor.h"
#include "sensors.h"

#define ELAN_PS_DEVICE_NAME               		"/dev/elan_ps"
#if 0
#define ELAN_IOCTL_MAGIC                         0x1C
#define ELAN_EPL2812_IOCTL_GET_PFLAG _IOR(ELAN_IOCTL_MAGIC, 1, int *)
#define ELAN_EPL2812_IOCTL_GET_LFLAG _IOR(ELAN_IOCTL_MAGIC, 2, int *)
#define ELAN_EPL2812_IOCTL_ENABLE_PFLAG _IOW(ELAN_IOCTL_MAGIC, 3, int *)
#define ELAN_EPL2812_IOCTL_ENABLE_LFLAG _IOW(ELAN_IOCTL_MAGIC, 4, int *)
#define ELAN_EPL2812_IOCTL_GETDATA _IOR(ELAN_IOCTL_MAGIC, 5, int *)
#else
#define ELAN_IOCTL_MAGIC 'c'
#define ELAN_EPL8800_IOCTL_GET_PFLAG    _IOR(ELAN_IOCTL_MAGIC, 1, int)
#define ELAN_EPL8800_IOCTL_GET_LFLAG    _IOR(ELAN_IOCTL_MAGIC, 2, int)
#define ELAN_EPL8800_IOCTL_ENABLE_PFLAG _IOW(ELAN_IOCTL_MAGIC, 3, int)
#define ELAN_EPL8800_IOCTL_ENABLE_LFLAG _IOW(ELAN_IOCTL_MAGIC, 4, int)
#define ELAN_EPL8800_IOCTL_GETDATA      _IOW(ELAN_IOCTL_MAGIC, 5, int)
#endif

/*****************************************************************************/
static struct sensor_t sSensorList[] = {
        { "ELAN Proximity sensor",
          "ELAN",
          1, 
		  SENSORS_PROXIMITY_HANDLE,
          SENSOR_TYPE_PROXIMITY, 
		  1.0f,
          1.0f, 
		  0.005f, 
		  0,
		  0,
		  0, 
		  SENSOR_STRING_TYPE_PROXIMITY,
		  "",
		  0,
		  SENSOR_FLAG_ON_CHANGE_MODE,
		  { } 
		  },
};

PSensor::PSensor() :
	SensorBase(ELAN_PS_DEVICE_NAME, "proximity"),
		mEnabled(0), mPendingMask(0), mInputReader(32), mHasPendingEvent(false)
{
	memset(mPendingEvents, 0, sizeof(mPendingEvents));

	mPendingEvents[Proximity].version = sizeof(sensors_event_t);
	mPendingEvents[Proximity].sensor = ID_P;
	mPendingEvents[Proximity].type = SENSOR_TYPE_PROXIMITY;

	for (int i = 0; i < numSensors; i++)
		mDelays[i] = 200000000;	// 200 ms by default
}

PSensor::~PSensor()
{
}

bool PSensor::hasPendingEvents() const
{
	return mHasPendingEvent;
}

int PSensor::setDelay(int32_t handle, int64_t ns)
{
	return 0;
}

int PSensor::setEnable(int32_t handle, int en)
{
	int what = -1;
	switch (handle) {
	case ID_P:
		what = Proximity;
		break;
	}
	if (uint32_t(what) >= numSensors)
		return -EINVAL;

	int newState = en ? 1 : 0;
	int err = 0;

	ALOGD
	    ("PSensor::enable en=%d; newState=%d; what=%d; mEnabled=%d\n",
	     en, newState, what, mEnabled);

	if ((uint32_t(newState) << what) != (mEnabled & (1 << what))) {
		if (!mEnabled)
			open_device();

		int cmd;

		switch (what) {
		case Proximity:
			#if 0
			cmd = ELAN_EPL2812_IOCTL_ENABLE_PFLAG;
			#else
			cmd = ELAN_EPL8800_IOCTL_ENABLE_PFLAG;
			#endif
			break;
		}

		int flags = newState;
		err = ioctl(dev_fd, cmd, &flags);
		err = err < 0 ? -errno : 0;
		ALOGD("PSensor::enable what=%d; flags=%d; err=%d\n",
		      what, flags, err);
		ALOGE_IF(err, "ECS_IOCTL_APP_SET_XXX failed (%s)",
			 strerror(-err));
		if (!err) {
			mEnabled &= ~(1 << what);
			mEnabled |= (uint32_t(flags) << what);
		}
		ALOGD("PSensor::mEnabled=0x%x\n", mEnabled);
		if (!mEnabled)
			close_device();
	}
	return err;
}

int PSensor::getEnable(int32_t handle)
{
	int enable = 0;
	int what = -1;
	switch (handle) {
	case ID_P:
		what = Proximity;
		break;
	}
	if (uint32_t(what) >= numSensors)
		return -EINVAL;
	enable = mEnabled & (1 << what);
	if (enable > 0)
		enable = 1;
	ALOGD("PSensor::mEnabled=0x%x; enable=%d\n", mEnabled, enable);
	return enable;
}

int PSensor::readEvents(sensors_event_t * data, int count)
{
	if (count < 1)
		return -EINVAL;
	ssize_t n = mInputReader.fill(data_fd);
	if (n < 0)
		return n;
	int numEventReceived = 0;
	input_event const *event;
	while (count && mInputReader.readEvent(&event)) {
		int type = event->type;
		if (type == EV_ABS) {
			processEvent(event->code, event->value);
			mInputReader.next();
		} else if (type == EV_SYN) {
			int64_t time = timevalToNano(event->time);
			for (int j = 0;
			     count && mPendingMask && j < numSensors; j++) {
				if (mPendingMask & (1 << j)) {
					mPendingMask &= ~(1 << j);
					mPendingEvents[j].timestamp = time;
					if (mEnabled & (1 << j)) {
						*data++ = mPendingEvents[j];
						count--;
						numEventReceived++;
					}
				}
			}
			if (!mPendingMask) {
				mInputReader.next();
			}
		} else {
			ALOGE
			    ("PSensor: unknown event (type=%d, code=%d)",
			     type, event->code);
			mInputReader.next();
		}
	}
	return numEventReceived;
}

void PSensor::processEvent(int code, int value)
{
	switch (code) {
	case EVENT_TYPE_PROXIMITY:
		mPendingMask |= 1 << Proximity;
		mPendingEvents[Proximity].distance = value;
		ALOGD("PSensor: mPendingEvents[Proximity].distance = %f",
		      mPendingEvents[Proximity].distance);
		break;
	default:
		ALOGD("PSensor: default value = %d", value);
		break;
	}
}

int PSensor::populateSensorList(struct sensor_t *list)
{
	memcpy(list, sSensorList, sizeof(struct sensor_t) * numSensors);
	return numSensors;
}

/*****************************************************************************/
