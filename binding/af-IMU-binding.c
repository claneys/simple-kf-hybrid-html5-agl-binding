/*
 * Copyright (C) 2015, 2016 "IoT.bzh"
 * Author "Romain Forlot"
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <json-c/json.h>

#include <systemd/sd-event.h>

#include <afb/afb-binding.h>
#include <afb/afb-service-itf.h>


/* Some useful math values
 * RAD_TO_DEG = 180 / pi 
 */
#define RAD_TO_DEG 57.295779513
#define M_PI  3.14159265358979323846

#define IMU_DEV "/dev/input/event"
#define IMU_ACC 0
#define IMU_MAG 1
#define IMU_GYR 2

#define NB_AXIS 3

/* Association code as IMU use absolute values */
char *absolutes[ABS_MAX + 1] = {
    [0 ... ABS_MAX] = NULL,
    [ABS_X] = "X",          [ABS_Y] = "Y",
    [ABS_Z] = "Z",          [ABS_RX] = "Rx",
    [ABS_RY] = "Ry",        [ABS_RZ] = "Rz",
    [ABS_THROTTLE] = "Throttle",    [ABS_RUDDER] = "Rudder",
    [ABS_WHEEL] = "Wheel",      [ABS_GAS] = "Gas",
    [ABS_BRAKE] = "Brake",      [ABS_HAT0X] = "Hat0X",
    [ABS_HAT0Y] = "Hat0Y",      [ABS_HAT1X] = "Hat1X",
    [ABS_HAT1Y] = "Hat1Y",      [ABS_HAT2X] = "Hat2X",
    [ABS_HAT2Y] = "Hat2Y",      [ABS_HAT3X] = "Hat3X",
    [ABS_HAT3Y] = "Hat 3Y",     [ABS_PRESSURE] = "Pressure",
    [ABS_DISTANCE] = "Distance",    [ABS_TILT_X] = "XTilt",
    [ABS_TILT_Y] = "YTilt",     [ABS_TOOL_WIDTH] = "Tool Width",
    [ABS_VOLUME] = "Volume",    [ABS_MISC] = "Misc",
};

/***************************************************************************************/
/***************************************************************************************/
/**                                                                                   **/
/**                                                                                   **/
/**       SECTION: HANDLE IMU DEVICE                                                  **/
/**                                                                                   **/
/**                                                                                   **/
/***************************************************************************************/
/***************************************************************************************/

/*
 * @brief Open IMU event device and return file handler
 * 
 * IMU event device are :
 * 	0 : Accelerometer
 *  1 : Magnetometer
 *  2 : gyroscop
 * 
 * @param integer imu_device: indice of imu event device like described above.
 * 
 * @return int fd : file handler of opened device.
 * 
 */
static int open_dev(int imu_device)
{
    char *fname_path[64];
    int fd;

	/* get dev full path */
	snprintf(*fname_path, sizeof(*fname_path), "%s%d", IMU_DEV, imu_device);
	
	if ((fd = open(*fname_path, O_RDONLY)) < 0) {
	    perror("evdev open");
	    exit(1);
	}

    return fd;
}

/*
 * @brief Get Accelerometer 3 Axis Raw values in g and fill AccellRaw array with it
 * 
 * @param integer fd: device file handler
 * @param integer *AccelRaw: pointer to an integer array that will contains returned values
 * 
 */
static void get_AccRaw(int fd, int *AccelRaw)
{
    int i;
	struct input_absinfo acc_absinfo;

    // Limit scan to X, Y and Z axis as Accel only use those one.
    for (i = 0; i < ACC_AXIS; i++)
    {
        if (ioctl(fd, EVIOCGABS(i), &absinfo) < 0)
        {
            perror("IOCTL : EVIOVGABS error");
        }
        AccelRaw[i] = absinfo.value;
    }
}

/*
 * @brief Convert gravity in degrees from Accelerometer raw values
 * 
 * @param integer accRaw: Accelerometer int array of Raw values
 * @param float *AccelAngles: pointer to an integer array that will contains returned values
 * 
 */
static void get_AccAngles(int accRaw[3], float *AccelAngle)
{
	//  TODO : Checks these formula...
	AccelAngle[0] = (float) (atan2(accRaw[1],accRaw[2])+M_PI)*RAD_TO_DEG;
    AccelAngle[1] = (float) (atan2(accRaw[2],accRaw[0])+M_PI)*RAD_TO_DEG;
	AccelAngle[2] = (float) (atan2(accRaw[1],accRaw[0])+M_PI)*RAD_TO_DEG;
}
/***************************************************************************************/
/***************************************************************************************/
/**                                                                                   **/
/**                                                                                   **/
/**       SECTION: BINDING VERBS IMPLEMENTATION                                       **/
/**                                                                                   **/
/**                                                                                   **/
/***************************************************************************************/
/***************************************************************************************/
/*
 * Returns the type corresponding to the given name
 */
static enum type type_of_name(const char *name)
{
	enum type result;
	if (name == NULL)
		return type_DEFAULT;
	for (result = 0 ; result != type_COUNT ; result++)
		if (strcmp(type_NAMES[result], name) == 0)
			return result;
	return type_INVALID;
}/*
 * extract a valid type from the request
 */
static int get_type_for_req(struct afb_req req, enum type *type)
{
	if ((*type = type_of_name(afb_req_value(req, "type"))) != type_INVALID)
		return 1;
	afb_req_fail(req, "unknown-type", NULL);
	return 0;
}

/*
 * Get Gyroscope values reading from lsm9ds0 on board
 *
 * There isn't parameters needed
 *
 * returns the rotating dps about X, Y and Z (degrees per second)
 */

static void get_gyr(struct afb_req req)
{

}

/*
 * Get Accelerometer values reading from lsm9ds0 on board
 *
 * There isn't parameters needed
 *
 * returns the X, Y and Z angles in degrees
 */
static float get_acc(struct afb_req req)
{
}

/*
 * Get Magnetometer values reading from lsm9ds0 on board
 *
 * There isn't parameters needed
 *
 * returns the X, Y angles in degrees
 */
static void get_mag(struct afb_req req)
{

}

/*
 * subscribe to notification of position
 *
 * parameters of the subscription are:
 *
 *    type:   string:  the type of position expected (defaults to WCS84 if not present)
 *                     see the list above (get_gps)
 *    period: integer: the expected period in milliseconds (defaults to 2000 if not present)
 *
 * returns an object with 2 fields:
 *
 *    name:   string:  the name of the event without its prefix
 *    id:     integer: a numeric identifier of the event to be used for unsubscribing
 */
static void subscribe(struct afb_req req)
{
	enum type type;
	const char *period;
	struct event *event;
	struct json_object *json;

	if (get_type_for_req(req, &type)) {
		period = afb_req_value(req, "period");
		event = event_get(type, period == NULL ? DEFAULT_PERIOD : atoi(period));
		if (event == NULL)
			afb_req_fail(req, "out-of-memory", NULL);
		else if (afb_req_subscribe(req, event->event) != 0)
			afb_req_fail_f(req, "failed", "afb_req_subscribe returned an error: %m");
		else {
			json = json_object_new_object();
			json_object_object_add(json, "name", json_object_new_string(event->name));
			json_object_object_add(json, "id", json_object_new_int(event->id));
			afb_req_success(req, json, NULL);
		}
	}
}

/*
 * unsubscribe a previous subscription
 *
 * parameters of the unsubscription are:
 *
 *    id:   integer: the numeric identifier of the event as returned when subscribing
 */
static void unsubscribe(struct afb_req req)
{
	const char *id;
	struct event *event;

	id = afb_req_value(req, "id");
	if (id == NULL)
		afb_req_fail(req, "missing-id", NULL);
	else {
		event = event_of_id(atoi(id));
		if (event == NULL)
			afb_req_fail(req, "bad-id", NULL);
		else {
			afb_req_unsubscribe(req, event->event);
			afb_req_success(req, NULL, NULL);
		}
	}
}

/*
 * array of the verbs exported to afb-daemon
 */
static const struct afb_verb_desc_v1 binding_verbs[] = {
  /* VERB'S NAME            SESSION MANAGEMENT          FUNCTION TO CALL         SHORT DESCRIPTION */
  { .name= "ping"     , .session= AFB_SESSION_NONE, .callback= ping    , "Ping the binder"},
  { .name= "get_gyr"  , .session= AFB_SESSION_NONE, .callback= get_gyr , "Get Gyroscop values"},
  { .name= "get_acc"  , .session= AFB_SESSION_NONE, .callback= get_acc , "Get Accelerometer values"},
  { .name= "get_mag"  , .session= AFB_SESSION_NONE, .callback= get_mag , "Get Magnetometer values"},
/* Kept to may be a future usage.
  { .name= "subscribe",    .session= AFB_SESSION_NONE, .callback= subscribe,    .info= "subscribe to notification of position" },
  { .name= "unsubscribe",  .session= AFB_SESSION_NONE, .callback= unsubscribe,  .info= "unsubscribe a previous subscription" },
*/
  { .name= NULL } /* marker for end of the array */
};

/*
 * description of the binding for afb-daemon
 */
static const struct afb_binding binding_description =
{
  /* description conforms to VERSION 1 */
  .type= AFB_BINDING_VERSION_1,
  .v1= {			/* fills the v1 field of the union when AFB_BINDING_VERSION_1 */
    .info = "Get IMU informations : Gyroscope, Accelerometer and magnetometer",
    .prefix = "IMU",
    .verbs = binding_verbs	/* the array describing the verbs of the API */
  }
};

/*
 * activation function for registering the binding called by afb-daemon
 */
const struct afb_binding *afbBindingV1Register(const struct afb_binding_interface *itf)
{
	afbitf = itf;			/* records the interface for accessing afb-daemon */
	return &binding_description;	/* returns the description of the binding */
}

int afbBindingV1ServiceInit(struct afb_service service)
{
	return connection();
}
const struct afb_binding_interface *interface;

static void ping (struct afb_req request)
{
	static int pingcount = 0;

	json_object *query = afb_req_json(request);
	afb_req_success_f(request, NULL, "Ping Binder Daemon count=%d query=%s", ++pingcount, json_object_to_json_string(query));
}

