gcc -shared led.c -c -fPIC -lgpiod
gcc main.c led.o -lgpiod -I. -L. -o run-led
