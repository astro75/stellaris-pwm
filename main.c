#include "inc/hw_types.h"
#include "inc/hw_memmap.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/pin_map.h"

typedef unsigned long ul;

#define P(port,pin,timer,side) { GPIO_P ## port ## pin ## _T ## timer ## CCP ## side, GPIO_PORT ## port ##_BASE, SYSCTL_PERIPH_TIMER ## timer, TIMER ## timer ## _BASE, 0xff << (side * 2) }

ul d[][5] = {
		     /*{ GPIO_PF0_T0CCP0, GPIO_PORTF_BASE, SYSCTL_PERIPH_TIMER0, TIMER0_BASE, TIMER_A },
		     { GPIO_PF1_T0CCP1, GPIO_PORTF_BASE, SYSCTL_PERIPH_TIMER0, TIMER0_BASE, TIMER_B }...
		     Replaced with equivalent below.*/

			 P(F,0,0,0),
			 P(F,1,0,1),
			 P(F,2,1,0),
			 P(F,3,1,1),
			 P(F,4,2,0),

			 P(B,0,2,0),
			 P(B,1,2,1),
			 P(B,2,3,0),
			 P(B,3,3,1),
			 P(B,4,1,0),
			 P(B,5,1,1),
			 P(B,6,0,0),
			 P(B,7,0,1),

			 P(C,0,4,0),
			 P(C,1,4,1),
			 P(C,2,5,0),
			 P(C,3,5,1)
		   };

#define PORTF 5
#define PORTB 1
#define PORTC 2

int getId(ul port, ul pin) {
	switch(port) {
	case PORTB:
		return pin + 5;
	case PORTC:
		return pin + 5 + 8;
	default:
		return pin;
	}
}

/*
 * Can use PORTF pins 0 - 4
 *         PORTB pins 0 - 7
 *         PORTC pins 0 - 3
 *
 * Cannot use these pins together:
 *   PB6 and PF0
 *   PB7 and PF1
 *   PF2 and PB4
 *   PF3 and PB5
 *   PF4 and PB0
 *
 * You can use up to 12 pins for different pwm output
 */

void timerInit(ul port, ul pin) {
	int id = getId(port, pin);

	SysCtlPeripheralEnable(1 << port | 0x20000000);
	GPIOPinTypeGPIOOutput(d[id][1], 1 << pin);
	GPIOPinWrite(d[id][1], 1 << pin, 0);


	GPIOPinConfigure(d[id][0]);
	GPIOPinTypeTimer(d[id][1], 1 << pin);
	//GPIOPadConfigSet(d[id][1], 1 << pin, GPIO_STRENGTH_8MA_SC, GPIO_PIN_TYPE_STD);

	SysCtlPeripheralEnable(d[id][2]);
	TimerConfigure       (d[id][3], TIMER_CFG_SPLIT_PAIR|TIMER_CFG_A_PWM|TIMER_CFG_B_PWM);
	TimerControlLevel    (d[id][3], d[id][4], true);
	TimerPrescaleSet     (d[id][3], d[id][4], 10); // pwm period = (10+1) * 0xFFFF = 720885 ~ 18 ms on 40 MHz
	TimerPrescaleMatchSet(d[id][3], d[id][4], 0);
	TimerMatchSet        (d[id][3], d[id][4], 0);
	TimerLoadSet         (d[id][3], d[id][4], 0xFFFF);
	TimerEnable          (d[id][3], TIMER_BOTH);
}

void timerSet(unsigned long duty, ul port, ul pin) {
	int id = getId(port, pin);
	TimerPrescaleMatchSet(d[id][3], d[id][4], duty / 0xFFFF);
	TimerMatchSet        (d[id][3], d[id][4], duty % 0xFFFF);
}

int main(void) {
    unsigned long ulPeriod, dutyCycle;

    // 40 MHz system clock
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|
        SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

    ulPeriod  = 0xFFFF;
    dutyCycle =  0xFFFF;

    // use timerInit on port-pin once
    timerInit(PORTF, 1);
    timerInit(PORTF, 2);
    timerInit(PORTF, 0);

    // then timerSet to change duty cycle
    timerSet(40 * 544, PORTF, 1);
    timerSet(40 * 2400, PORTF, 2);

    while(1) {

        dutyCycle += 10000;

        if(dutyCycle >= 40 * 2400) {
            dutyCycle = 40 * 544;
            SysCtlDelay(5000000);
        }

        timerSet(dutyCycle, PORTF, 0);

        if(dutyCycle == 40 * 544)
        	SysCtlDelay(10000000);
        SysCtlDelay(1000000);
    }
}
