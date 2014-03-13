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

#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "c_gpio.h"
#include "bcm2835.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#define BCM2708_PERI_BASE   0x20000000
#define GPIO_BASE           (BCM2708_PERI_BASE + 0x200000)
#define FSEL_OFFSET         0   // 0x0000
#define SET_OFFSET          7   // 0x001c / 4
#define CLR_OFFSET          10  // 0x0028 / 4
#define PINLEVEL_OFFSET     13  // 0x0034 / 4
#define EVENT_DETECT_OFFSET 16  // 0x0040 / 4
#define RISING_ED_OFFSET    19  // 0x004c / 4
#define FALLING_ED_OFFSET   22  // 0x0058 / 4
#define HIGH_DETECT_OFFSET  25  // 0x0064 / 4
#define LOW_DETECT_OFFSET   28  // 0x0070 / 4
#define PULLUPDN_OFFSET     37  // 0x0094 / 4
#define PULLUPDNCLK_OFFSET  38  // 0x0098 / 4

#define PAGE_SIZE  (4*1024)
#define BLOCK_SIZE (4*1024)

static volatile uint32_t *gpio_map;

int BMC2835_IsInit = 0;

void short_wait(void)
{
    int i;
    
    for (i=0; i<150; i++)     // wait 150 cycles
    {
		asm volatile("nop");
    }
}

int setup(void)
{
    int mem_fd;
    uint8_t *gpio_mem;

    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0)
    {
        return SETUP_DEVMEM_FAIL;
    }

    if ((gpio_mem = malloc(BLOCK_SIZE + (PAGE_SIZE-1))) == NULL)
        return SETUP_MALLOC_FAIL;

    if ((uint32_t)gpio_mem % PAGE_SIZE)
        gpio_mem += PAGE_SIZE - ((uint32_t)gpio_mem % PAGE_SIZE);

    gpio_map = (uint32_t *)mmap( (caddr_t)gpio_mem, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, mem_fd, GPIO_BASE);

    if ((uint32_t)gpio_map < 0)
        return SETUP_MMAP_FAIL;

    return SETUP_OK;
}

void clear_event_detect(int gpio)
{
	int offset = EVENT_DETECT_OFFSET + (gpio/32);
    int shift = (gpio%32);

    *(gpio_map+offset) |= (1 << shift);
    short_wait();
    *(gpio_map+offset) = 0;
}

int eventdetected(int gpio)
{
	int offset, value, bit;
   
    offset = EVENT_DETECT_OFFSET + (gpio/32);
    bit = (1 << (gpio%32));
    value = *(gpio_map+offset) & bit;
    if (value)
    {
        clear_event_detect(gpio);
	}
    return value;
}

void set_rising_event(int gpio, int enable)
{
	int offset = RISING_ED_OFFSET + (gpio/32);
    int shift = (gpio%32);

	if (enable)
	    *(gpio_map+offset) |= 1 << shift;
	else
	    *(gpio_map+offset) &= ~(1 << shift);
    clear_event_detect(gpio);
}

void set_falling_event(int gpio, int enable)
{
	int offset = FALLING_ED_OFFSET + (gpio/32);
    int shift = (gpio%32);

	if (enable)
	{
	    *(gpio_map+offset) |= (1 << shift);
	    *(gpio_map+offset) = (1 << shift);
	} else {
	    *(gpio_map+offset) &= ~(1 << shift);
	}
    clear_event_detect(gpio);
}

void set_high_event(int gpio, int enable)
{
	int offset = HIGH_DETECT_OFFSET + (gpio/32);
    int shift = (gpio%32);

	if (enable)
	{
	    *(gpio_map+offset) |= (1 << shift);
	} else {
	    *(gpio_map+offset) &= ~(1 << shift);
	}
    clear_event_detect(gpio);
}

void set_low_event(int gpio, int enable)
{
	int offset = LOW_DETECT_OFFSET + (gpio/32);
    int shift = (gpio%32);

	if (enable)
	    *(gpio_map+offset) |= 1 << shift;
	else
	    *(gpio_map+offset) &= ~(1 << shift);
    clear_event_detect(gpio);
}

void set_pullupdn(int gpio, int pud)
{
    int clk_offset = PULLUPDNCLK_OFFSET + (gpio/32);
    int shift = (gpio%32);
    
    if (pud == PUD_DOWN)
       *(gpio_map+PULLUPDN_OFFSET) = (*(gpio_map+PULLUPDN_OFFSET) & ~3) | PUD_DOWN;
    else if (pud == PUD_UP)
       *(gpio_map+PULLUPDN_OFFSET) = (*(gpio_map+PULLUPDN_OFFSET) & ~3) | PUD_UP;
    else  // pud == PUD_OFF
       *(gpio_map+PULLUPDN_OFFSET) &= ~3;
    
    short_wait();
    *(gpio_map+clk_offset) = 1 << shift;
    short_wait();
    *(gpio_map+PULLUPDN_OFFSET) &= ~3;
    *(gpio_map+clk_offset) = 0;
}

void setup_gpio(int gpio, int direction, int pud)
{
    int offset = FSEL_OFFSET + (gpio/10);
    int shift = (gpio%10)*3;

    set_pullupdn(gpio, pud);
    if (direction == OUTPUT)
        *(gpio_map+offset) = (*(gpio_map+offset) & ~(7<<shift)) | (1<<shift);
    else  // direction == INPUT
        *(gpio_map+offset) = (*(gpio_map+offset) & ~(7<<shift));
}

// Contribution by Eric Ptak <trouch@trouch.com>
int gpio_function(int gpio)
{
   int offset = FSEL_OFFSET + (gpio/10);
   int shift = (gpio%10)*3;
   int value = *(gpio_map+offset);
   value >>= shift;
   value &= 7;
   return value; // 0=input, 1=output, 4=alt0
}

void output_gpio(int gpio, int value)
{
    int offset, shift;
    
    if (value) // value == HIGH
        offset = SET_OFFSET + (gpio/32);
    else       // value == LOW
        offset = CLR_OFFSET + (gpio/32);
    
    shift = (gpio%32);

    *(gpio_map+offset) = 1 << shift;
}

int input_gpio(int gpio)
{
   int offset, value, mask;
   
   offset = PINLEVEL_OFFSET + (gpio/32);
   mask = (1 << gpio%32);
   value = *(gpio_map+offset) & mask;
   return value;
}

void cleanup(void)
{
    // fixme - set all gpios back to input
    munmap((caddr_t)gpio_map, BLOCK_SIZE);
}

int init_bcm2835(void) 
{   
    if (!BMC2835_IsInit) {
        if (!bcm2835_init()) {
            printf("bcm2835_init fail\n");
            BMC2835_IsInit = 0;
            return 0;
        }
        printf("bcm2835_init OK\n");
        BMC2835_IsInit = 1;
    } else {
        printf("BCM2835 already init.");
    }
    return 1;
}

void close_bcm2835(void)
{      
        BMC2835_IsInit = 0;
        bcm2835_close();
}

void init_pwm(int gpio, int pwm_channel, int divider, int range)
{
      // Set the output pin to Alt Fun 5, to allow PWM channel 0 to be output there
    bcm2835_gpio_fsel(gpio, BCM2835_GPIO_FSEL_ALT5);
    // Clock divider is set to 16.
    // With a divider of 16 and a RANGE of 1024, in MARKSPACE mode,
    // the pulse repetition frequency will be
    // 1.2MHz/1024 = 1171.875Hz, suitable for driving a DC motor with PWM  -  BCM2835_PWM_CLOCK_DIVIDER_16
    bcm2835_pwm_set_clock(divider);
    bcm2835_pwm_set_mode(pwm_channel, 1, 1);
    bcm2835_pwm_set_range(pwm_channel, range);
}

int pwm_setclock(unsigned int divider)
{
    bcm2835_pwm_set_clock(divider);
    return 0;
}

int pwm_setrange(unsigned int pwm_channel, unsigned int range)
{
    bcm2835_pwm_set_range(pwm_channel, range);
    return 0;
}

int pwm_setlevel(unsigned int pwm_channel, unsigned int range)
{
    bcm2835_pwm_set_data(pwm_channel, range);
    return 0;
}

// Hardware pwm on gpio pin with BCM2538 lib
int pwm_pulsepause(int pwm_channel, long tpulse, long tpause, int range, PulsePair *pair)
{
    struct timeval tStart, tPulse, tPause;
    
    gettimeofday (&tStart, NULL);
    bcm2835_pwm_set_data(pwm_channel, range);
    bcm2835_delayMicroseconds((uint64_t)tpulse);
    
    gettimeofday (&tPulse, NULL);
    pair->pulse = delta_time_in_microseconds(&tPulse, &tStart);
    bcm2835_pwm_set_data(pwm_channel, 0);
    bcm2835_delayMicroseconds((uint64_t)tpause);
    
    gettimeofday (&tPause, NULL);
    pair->pause = delta_time_in_microseconds(&tPause, &tPulse);
    return 0;
}

// Software pwm on gpio pin with BCM2538 lib
int gpio_pulsepause(int gpio, long tpulse, long tpause, PulsePair *pair)
{
    struct timeval tStart, tPulse, tPause;
    
    gettimeofday (&tStart, NULL);
    while (tpulse > 0) {
        bcm2835_gpio_write(gpio, 1);
        bcm2835_delayMicroseconds(13);
        bcm2835_gpio_write(gpio, 0);
        bcm2835_delayMicroseconds(12);
        tpulse -= 26;
    }; 
    gettimeofday (&tPulse, NULL);
    pair->pulse = delta_time_in_microseconds(&tPulse, &tStart);
    bcm2835_delayMicroseconds((uint64_t)tpause);
    
    gettimeofday (&tPause, NULL);
    pair->pause = delta_time_in_microseconds(&tPause, &tPulse);
    return 0;
}

// Look on possible pulse/pause pairs received on imput gpio, used BCM2538 lib. Return 0 in not, 1 if potential code
// Use full CPU capability, call it after an interrupt event on gpio (GPIO.add_event_callback, for example)
// Don't forget to free the memory pulsepairs tab  after call by using free_plusepairs()
int gpio_watchpulsepairs(int gpio, PulsePairs *pulsepairs)
{   
    int value = 0, vread = 0;
    int size = 0, finish = 0;
    long pulse = 0, pause =0, tStage = 0;
    struct timeval tStart, tPulse;
    
//    Allocate memory for pulsepairs tab pointeur result
    pulsepairs->pairs = malloc(sizeof(int *) * 1);
    pulsepairs->size = 1;
    pulsepairs->pairs[0] = malloc(sizeof(long *) * 2);
    gettimeofday (&tStart, NULL);
    while (!finish) {    //
        tStage = 0;
        while ((vread == value) & (tStage < PULSEPAIR_TIMEOUTSTAGE)) { // look on gpio state change or state no change to long.
            vread =  bcm2835_gpio_lev(gpio);
            gettimeofday (&tPulse, NULL);
            tStage = delta_time_in_microseconds(&tPulse, &tStart);
        };
        gettimeofday (&tStart, NULL);
        if (tStage >= PULSEPAIR_TIMEOUTSTAGE) {   // Set flag termitate watch if time-out, end pulsepairs or no pulspairs
            finish = 1;
        };
        if (value == 0) {
            pulse = tStage;
        } else {
            if (pulse) { // check if a pluse is set
                pause = tStage;
                if (size != 0) { 
                    int **new_pairs = realloc(pulsepairs->pairs, sizeof(int *) * (size +1));
                    if (new_pairs != NULL) {
                        pulsepairs->pairs = new_pairs;
                        pulsepairs->size = size + 1;
                        pulsepairs->pairs[size] = malloc(sizeof(long *) * 2);
                    } else {
                        fprintf(stderr, "Mem realloc error: %d\n", errno);
                        finish = 1;
                        break;
                    };
                };
                pulsepairs->pairs[size][0] = pulse;
                pulsepairs->pairs[size][1] = pause;
//                printf( "add Pulse/Pause IR : %ld / %ld\n", pulsepairs->pairs[size][0], pulsepairs->pairs[size][1]);
                size += 1;
                pulse = pause =0;
            };
        };
        value = vread;
    };
    if (size < PULSEPAIR_MINPAIRS) {
//        printf("No valide pulse/pause pairs detected");
        return 0;
    }
//    printf("codeIR : %d pairs, pulse %ld, pause %ld, value %d, nbpulse %ld\n", pulsepairs->size, pulse, pause, value, nbPulse);
    return 1;
}

void free_plusepairs(PulsePairs *pulsepairs)
{
    int i;
    int size = num_pulsepairs(pulsepairs);
    printf("Free plusepairs size : %d\n", size);
    if (pulsepairs->pairs != NULL) {
        for (i = 0; i < size; i++) {
            free(pulsepairs->pairs[i]);
        };
        free(pulsepairs->pairs);
        pulsepairs->pairs = NULL;
        free(pulsepairs);
    };
}

int num_pulsepairs(PulsePairs *pulsepairs)
{
    if (pulsepairs !=NULL) {
        if (pulsepairs->pairs != NULL) {
            int size = pulsepairs->size;
            printf("num_pulsepairs  = %d\n", size);
            return size;
        };
    };
    return 0;
}

/* The time difference in microseconds */
long delta_time_in_microseconds (struct timeval * t2, struct timeval * t1)
{
  /* Compute delta in second, 1/10's and 1/1000's second units */
  time_t delta_seconds      = t2 -> tv_sec - t1 -> tv_sec;
  time_t delta_microseconds = t2 -> tv_usec - t1 -> tv_usec;

  if (delta_microseconds < 0)
    { /* manually carry a one from the seconds field */
      delta_microseconds += 1000000;                            /* 1e6 */
      -- delta_seconds;
    }
  return (long)((delta_seconds * 1000000) + delta_microseconds);
}
