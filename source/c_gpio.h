/*
Copyright (c) 2012-2013 Ben Croston

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

typedef struct PulsePair PulsePair;
struct PulsePair
{
    long pulse;
    long pause;
};

typedef struct PulsePairs PulsePairs;
struct PulsePairs
{ 
    int **pairs;
    unsigned int size;
};

int setup(void);
void setup_gpio(int gpio, int direction, int pud);
int gpio_function(int gpio);
void output_gpio(int gpio, int value);
int input_gpio(int gpio);
void set_rising_event(int gpio, int enable);
void set_falling_event(int gpio, int enable);
void set_high_event(int gpio, int enable);
void set_low_event(int gpio, int enable);
int eventdetected(int gpio);
void cleanup(void);
int init_bcm2835(void);
void close_bcm2835(void);
void init_pwm(int gpio, int pwm_channel, int divider, int range);
int pwm_setclock(unsigned int divider);
int pwm_setrange(unsigned int pwm_channel, unsigned int range);
int pwm_setlevel(unsigned int pwm_channel, unsigned int range);
int pwm_pulsepause(int pwm_channel, long tpulse, long tpause, int range, PulsePair *pair);
int gpio_pulsepause(int gpio, long tpulse, long tpause, PulsePair *pair);
int gpio_watchpulsepairs(int gpio, PulsePairs *pulsepairs);
void free_plusepairs(PulsePairs *pulsepairs);
int num_pulsepairs(PulsePairs *pulsepairs);
long delta_time_in_microseconds (struct timeval * t2, struct timeval * t1);

#define SETUP_OK          0
#define SETUP_DEVMEM_FAIL 1
#define SETUP_MALLOC_FAIL 2
#define SETUP_MMAP_FAIL   3

#define INPUT  1 // is really 0 for control register!
#define OUTPUT 0 // is really 1 for control register!
#define ALT0   4

//#define HIGH 1      // define in bcm2835.h
//#define LOW  0

#define PUD_OFF  0
#define PUD_DOWN 1
#define PUD_UP   2

#define PULSEPAIR_TIMEOUTSTAGE 65000  // time-out in us for report non pulsepair
#define PULSEPAIR_MINPAIRS 5 // minimal pairs number for consider a code
