#include "led.h"

int led()
{
    const char *chipname = "gpiochip0";
    const char *linename = "GPIO17";
    const int lineoffset = 17;
    struct gpiod_chip *chip;
    struct gpiod_line *gpioPin17;
    int i, val;
 
    // Open GPIO chip
    chip = gpiod_chip_open_by_name(chipname);
    if (!chip) {
        perror("Open chip failed");
        return -1;
    }
 
    // Open GPIO line
    gpioPin17 = gpiod_chip_get_line(chip, lineoffset);
    if (!gpioPin17) {
        fprintf(stderr, "Cannot find line with name: %s\n", linename);
        gpiod_chip_close(chip);
        return -1;
    }
 
    // Open GPIO line for output
    if (gpiod_line_request_output(gpioPin17, "example1", 0) < 0) {
        perror("Request line as output failed");
        gpiod_chip_close(chip);
        return -1;
    }
 
    // Blink LED in a binary pattern
    for (int i=0; i<100; i++) {
        gpiod_line_set_value(gpioPin17, (i & 1) != 0);
        usleep(100000);
    }
    gpiod_line_set_value(gpioPin17,0);

    
    // Release lines and chip
    gpiod_line_release(gpioPin17);
    gpiod_chip_close(chip);
    return 0;
}


