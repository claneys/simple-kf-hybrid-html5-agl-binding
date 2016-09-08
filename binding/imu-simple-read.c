#include <stdio.h>
#include <stdint.h>

#include <linux/input.h>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>

#define test_bit(bit, array) (array[bit/8] & (1<<(bit%8)))

int main(int argc, char *argv[])
{
	int fd;
	int yalv;
	uint8_t evtype_b[(EV_MAX + 7)/8];
    struct input_absinfo absinfo;


	if ((fd = open(argv[1], O_RDONLY)) < 0) {
	    perror("evdev open");
	    exit(1);
	}

	memset(evtype_b, 0, sizeof(evtype_b));
	if (ioctl(fd, EVIOCGBIT(0, EV_MAX), evtype_b) < 0) {
	    perror("evdev ioctl");
	}

	printf("Supported event types:\n");

	for (yalv = 0; yalv < EV_MAX; yalv++) {
	    if (test_bit(yalv, evtype_b)) {
	        /* the bit is set in the event types list */
	        printf("  Event type 0x%02x ", yalv);
	        switch ( yalv)
	        {
	            case EV_SYN :
	                printf(" (Synch Events)\n");
	                break;
	            case EV_KEY :
	                printf(" (Keys or Buttons)\n");
	                break;
	            case EV_REL :
	                printf(" (Relative Axes)\n");
	                break;
	            case EV_ABS :
	                printf(" (Absolute Axes)\n");
	                break;
	            case EV_MSC :
	                printf(" (Miscellaneous)\n");
	                break;
	            case EV_LED :
	                printf(" (LEDs)\n");
	                break;
	            case EV_SND :
	                printf(" (Sounds)\n");
	                break;
	            case EV_REP :
	                printf(" (Repeat)\n");
	                break;
	            case EV_FF :
	            case EV_FF_STATUS:
	                printf(" (Force Feedback)\n");
	                break;
	            case EV_PWR:
	                printf(" (Power Management)\n");
	                break;
	            default:
	                printf(" (Unknown: 0x%04hx)\n",
	             yalv);
	        }
	    }
	}

    for (yalv = 0; yalv < ABS_MAX; yalv++) {
        if (ioctl(fd, EVIOCGABS(yalv), &absinfo) < 0) {
            perror("evdev ioctl");
        }
        printf("ABS_CODE %d : val = %d; min = %d; max = %d; fuzz = %d ; flat = %d ; resolution = %d\n",
               yalv,
               absinfo.value,
               absinfo.minimum,
               absinfo.maximum,
               absinfo.fuzz,
               absinfo.flat,
               absinfo.resolution);
    }

	close(fd);
}
