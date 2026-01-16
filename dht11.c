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
#include <wiringPi.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>

void sleep_millis(uint32_t millis) {
  struct timespec sleep;
  sleep.tv_sec = millis / 1000;
  sleep.tv_nsec = (millis % 1000) * 1000000L;
  while (clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep, &sleep) && errno == EINTR);
}

//#define DEBUG

/* The pin you have the sensor hanging off. */
#define DHT11_PIN 7

#define DHT11_OK              0
#define DHT11_ERROR_ARG       -1
#define DHT11_ERROR_CHECKSUM  -2
#define DHT11_ERROR_TIMEOUT   -3

/** 
 * How long to spin, waiting for input.
 */
#define DHT11_MAXCOUNT 32000

/**
 * Number of bit pulses to expect from the DHT.  Note that this is 41 because
 * the first pulse is a constant 50 microsecond pulse, with 40 pulses to
 * represent the data afterwards.
 */
#define DHT11_PULSES	41

int dht11_read(int pin, float *humidity, float *temperature)
{
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
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  sleep_millis(500);
  digitalWrite(pin, LOW);
  sleep_millis(20);

  /* Time the pulses coming in */
  pinMode(pin, INPUT);
  /* Tiny delay to let pin stabilise as input pin and let voltage come up */
  for( volatile int i=0; i<50; i++);

  /* Wait for HIGH->LOW edge */
  uint32_t count = 0;
  while (digitalRead(pin)) {
    if (++count > DHT11_MAXCOUNT) {
      return DHT11_ERROR_TIMEOUT;
    }
  }
  
  /* Record pulse widths */
  int pulse=0;
  while (pulse < DHT11_PULSES*2) {
    /* Time low */
    while (!digitalRead(pin)) {
      if (++pulseWidths[pulse] > DHT11_MAXCOUNT) {
        return DHT11_ERROR_TIMEOUT;
      }
    }
    ++pulse;
    /* Time high */
    while (digitalRead(pin)) {
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

  return DHT11_OK;
}

void main()
{
  float humidity, temperature;

  wiringPiSetup();

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
