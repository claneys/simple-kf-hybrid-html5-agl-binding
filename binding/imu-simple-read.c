#include <stdio.h>
#include <stdint.h>

#include <linux/input.h>

#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>

// Check that a bit at a given position is set
#define test_bit(bit, array) (array[bit/8] & (1<<(bit%8)))

#define IMU_DEV "/dev/input/event"
#define IMU_ACC 0
#define IMU_MAG 1
#define IMU_GYR 2
#define NB_AXIS 3
// 180/pi
#define RAD_TO_DEG 57.29578
#define M_PI  3.14159265358979323846

// Association code as IMU use absolute values
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

static int open_dev(int imu_device)
{
    char fname_path[64];
	int fd;
    snprintf(fname_path, sizeof(fname_path), "%s%d", IMU_DEV, imu_device);
	
	if ((fd = open(fname_path, O_RDONLY)) < 0) {
	    perror("evdev open");
	    exit(1);
	}

    return fd;
}

/*
 * Get Accelerometer raw values in g linear acceleration
 */

static void get_Accel_raw(int fd, int *AccelRaw)
{
    struct input_absinfo absinfo;

    int i;

    // Limit scan to X, Y and Z axis as Accel only use those one.
    for (i = 0; i < NB_AXIS; i++)
    {
        if (ioctl(fd, EVIOCGABS(i), &absinfo) < 0)
        {
            perror("IOCTL : EVIOVGABS error");
        }
        AccelRaw[i] = absinfo.value;
    }
}

static void get_Accel_angles(int accRaw[3], int *AccelAngles)
{
	//Convert Accelerometer values to degrees
	AccelAngles[0] = (float) (atan2(accRaw[1],accRaw[2])+M_PI)*RAD_TO_DEG;
    AccelAngles[1] = (float) (atan2(accRaw[2],accRaw[0])+M_PI)*RAD_TO_DEG;
	AccelAngles[2] = (float) (atan2(accRaw[1],accRaw[0])+M_PI)*RAD_TO_DEG;
}

// Give input device path
int main(int argc, char *argv[])
{
	int fd;
	int i;
	int AccelRaw[3];
	int AccelAngles[3];

    struct input_absinfo absinfo;

	fd = open_dev(IMU_ACC);
	
	get_Accel_raw(fd, AccelRaw);
	get_Accel_angles(AccelRaw, AccelAngles);
	for (i=0; i < NB_AXIS; i++)
	{
		printf("%s : %d g\n", absolutes[i], AccelRaw[i]);
		printf("%s : %d d\n", absolutes[i], AccelAngles[i]);
	}
	
	
	close(fd);
}
