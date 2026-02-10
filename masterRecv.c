#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main()
{
    int filep;
    char filename[20];
    snprintf(filename, 19, "/dev/i2c-0");

    filep = open(filename, O_RDWR);
    if (filep < 0) {
        printf("Couldnt open file\n");
        exit(1);
    }

    int slave_addr = 0x20;
    if (ioctl(filep, I2C_SLAVE, slave_addr) < 0) {
        printf("Couldnt ioctl\n");
        exit(1);
    }

    //char c = 0;
    while (1) {
        int val = i2c_smbus_read_byte(filep);
        if (val < 0) {
            printf("Couldnt read byte\n");
            exit(1);
        }
        printf("\rValue of button: %d", val);
        fflush(stdout);
        usleep(100000);
    }
    close(filep);
    return 0;
}
