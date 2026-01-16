#include "led.h"

/* build.sh
gcc -shared led.c -c -fPIC -lgpiod
gcc main.c led.o -lgpiod -I. -L. -o run-led
 */

int main(int argc, char **argv)
{
  return led();
}
