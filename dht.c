/**

Written 2021 by Nigel Atkinson because the predominantly found C example for
reading a DHT11 just did not work.
Greatly inspired by the Adafruit python library for DHT11 & DHT22, however
simplified and using the library wiringPi.

Adafruit write good code! :-)

Tested on a Raspberry Pi 2B r1.1

Compile:
gcc dht11.c -o dht11 -lwiringPi

MIT License.
Copyright 2021 Nigel Atkinson

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in the
Software without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
#include "dht.h"
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>


//#define DEBUG

/* The pin you have the sensor hanging off. */
#define DHT11_PIN 17

#define DHT11_OK              0
#define DHT11_ERROR_ARG       -1
#define DHT11_ERROR_CHECKSUM  -2
#define DHT11_ERROR_TIMEOUT   -3

/** 
 * How long to spin, waiting for input.
 */
#define DHT11_MAXCOUNT 64000

/**
 * Number of bit pulses to expect from the DHT.  Note that this is 41 because
 * the first pulse is a constant 50 microsecond pulse, with 40 pulses to
 * represent the data afterwards.
 */
#define DHT11_PULSES	41

int dht(int pin, float *humidity, float *temperature)
{


  const char *chipname = "gpiochip0";
  struct gpiod_chip *chip;
  struct gpiod_line *gpioPin;
  int i, val;

  /* Make sure output pointers are probably ok */
  if (humidity == NULL || temperature == NULL ) {
    return DHT11_ERROR_ARG;
  }

  *humidity = 0.0f;
  *temperature = 0.0f;
  float f=0.f;
  
  /* Array to store length of low and high pulses from the sensor */
  int pulseWidths[DHT11_PULSES*2] = {0};

  /* Signal sensor to output it's data. High for ~500ms then low for ~20ms */
  // Open GPIO chip
  chip = gpiod_chip_open_by_name(chipname);
  if (!chip) {
    fprintf(stdout, "Open chip failed");
    return -1;
  }
 
  // Open GPIO line
  gpioPin = gpiod_chip_get_line(chip, pin);
  if (!gpioPin) {
    fprintf(stdout, "Cannot find line with name: GPIO%d\n", pin);
    gpiod_chip_close(chip);
    return -1;
  }

  // pinMode(pin, OUTPUT);
  // digitalWrite(pin, HIGH);
  gpiod_line_request_output(gpioPin, "dht",1);
  gpiod_line_set_value(gpioPin,1);
  usleep(500000);
  // digitalWrite(pin, LOW);
  gpiod_line_set_value(gpioPin,0);
  usleep(20000);

  /* Time the pulses coming in */
  gpiod_line_set_direction_input(gpioPin);
  /* Tiny delay to let pin stabilise as input pin and let voltage come up */
  for( volatile int i=0; i<500; i++);

  /* Wait for HIGH->LOW edge */
  uint32_t count = 0;
  while (gpiod_line_get_value(gpioPin)) {
    if (++count > DHT11_MAXCOUNT) {
      return DHT11_ERROR_TIMEOUT;
    }
  }

  /* Record pulse widths */
  int pulse=0;
  while (pulse < DHT11_PULSES*2) {
    /* Time low */
    while (!gpiod_line_get_value(gpioPin)) {
      if (++pulseWidths[pulse] > DHT11_MAXCOUNT) {
        return DHT11_ERROR_TIMEOUT;
      }
    }
    ++pulse;
    /* Time high */
    while (gpiod_line_get_value(gpioPin)) {
      if (++pulseWidths[pulse] > DHT11_MAXCOUNT) {
        return DHT11_ERROR_TIMEOUT;
      }
    }
    ++pulse;
  }

  /* Convert pulse widths to bits and bytes */
  uint8_t bytes[5] = {0};
  uint8_t bit = 0;
  pulse = 2; /* Skip over initial bit */
  while (pulse < DHT11_PULSES*2) {
#ifdef DEBUG
    printf( 
           "Bit: %2d Byte: %2d Low: %3d High: %3d -> %1d  = 0x%2x\n", 
           bit,
           bit>>3,
           pulseWidths[pulse],
           pulseWidths[pulse+1],
           pulseWidths[pulse] <
           pulseWidths[pulse+1],
           bytes[bit>>3]
            );
    if (pulse % 16 == 0)
      puts("");
#endif

    bytes[bit>>3] <<= 1;
    if (pulseWidths[pulse] < pulseWidths[++pulse] ) {
      /* High part is longer than the preceding low, so this bit is a 1. */ 
      bytes[bit>>3] |= 1;
    }
    /* Otherwise high part is shorter, this bit is a 0 */

    ++bit;
    ++pulse;
  }

#ifdef DEBUG
  printf( "Data: %02x %02x %02x %02x Checksum: %02x : %02x\n",
          bytes[0],
          bytes[1],
          bytes[2],
          bytes[3],
          bytes[4],
          ((bytes[0] + bytes[1] + bytes[2] + bytes[3]) & 0xff)
          );

  /* If debugging, set outputs regardless of checksum validity */
  *humidity = (float)bytes[0];
  *temperature = (float)bytes[2];
  f = ((int)(bytes[2] & 0x7F)) << 8 | bytes[3];
  f *= 0.1;
  *temperature = f;
#endif

  /* Check the checksum */
  if (bytes[4] != ((bytes[0] + bytes[1] + bytes[2] + bytes[3]) & 0xff)) {
    return DHT11_ERROR_CHECKSUM;
  }

  /* All good, put the data in the vars :-) */
  f = ((int)bytes[0]) << 8 | bytes[1];
  f *= 0.1;
  *humidity = f;
  //*humidity = (float)bytes[0];
  //*temperature = (float)bytes[2];
  f = ((int)(bytes[2] & 0x7F)) << 8 | bytes[3];
  f *= 0.1;
  if (bytes[2] & 0x80 ){
    f *= -1;}
  *temperature = f;

  //  gpiod_line_set_value(gpioPin,0);
  // Release lines and chip
  gpiod_line_release(gpioPin);
  gpiod_chip_close(chip);
  
  return DHT11_OK;
}


/*
void main()
{
  float humidity, temperature;

  //wiringPiSetup();

  int tries;

  for (tries = 3; tries > 0; --tries) {
    int ret = dht11_read( DHT11_PIN, &humidity, &temperature);
    if (ret == DHT11_OK ) {
      printf("Humidity: %2.0f%% RH, Temperature: %2.0f° C\n",
          humidity,
          temperature);
      break;
    }
    else if (ret == DHT11_ERROR_CHECKSUM) {
      puts("Checksum error.");
    }
    else {
      puts("Timeout.");
    }
  }
  if (tries == 0) {
    puts("Ran out of tries.");
  }
}
*/
