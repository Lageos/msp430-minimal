#include <msp430.h>
#include "onewire.h" // http://www.smallbulb.net/2012/238-1-wire-and-msp430 , https://github.com/dsiroky/OneWire
#include "delay.h"

#define CONTROL 		BIT0

/*  Global Variables  */
volatile char i = 0;
volatile float temp;
volatile onewire_t ow;
volatile uint8_t scratchpad[9];

int main() {
	WDTCTL = WDTPW + WDTHOLD; //Stop watchdog timer
	BCSCTL1 = CALBC1_8MHZ;
	DCOCTL = CALDCO_8MHZ;

	// set interrupt, timer a
	TA0CCR0 = 50000; // Timmer register 0, a period of 62,500 (max) cycles is 0 to 62,499. 50 ms
	TA0CCR1 = 25000;  // Timmer register 1, start value PWM
	TA0CCTL0 = CCIE;      // Enable interrupts for CCR0.
	TA0CCTL1 = CCIE;      // Enable interrupts for CCR1.
	TA0CTL = TASSEL_2 + ID_3 + MC_1 + TACLR; // SMCLK source, divider 8, up to ccr0, timer a counter clear

	ow.port_out = &P1OUT;
	ow.port_in = &P1IN;
	ow.port_ren = &P1REN;
	ow.port_dir = &P1DIR;
	ow.pin = BIT7;

	// define relais control pin
	P1DIR |= CONTROL;                         // output mode, high
	P1OUT |= CONTROL;	 	                    // select pull-up mode
	P1REN &= ~CONTROL;        // enable internal pull up = high, pull down = low

	_BIS_SR(LPM0_bits + GIE); // low power + enable interrupts
	return 0;
}

/*  Interrupt Service Routines  */
/*
 #pragma vector = TIMER0_A0_VECTOR
 __interrupt void Timer0_A0 (void)  {
 // no flag clearing necessary; CCR0 has only one source, so it's automatic.
 if (i++ == 0) { // start reading
 onewire_reset(&ow);
 onewire_write_byte(&ow, 0xcc); // skip ROM command
 onewire_write_byte(&ow, 0x44); // convert T command
 onewire_line_high(&ow);
 }
 if (i >= 14) { // read values 16
 onewire_reset(&ow);
 onewire_write_byte(&ow, 0xcc); // skip ROM command
 onewire_write_byte(&ow, 0xbe); // read scratchpad command
 for (i = 0; i < 9; i++) scratchpad[i] = onewire_read_byte(&ow);
 temp = ( (scratchpad[1] << 8) + scratchpad[0] )*0.0625;
 // temp control
 if (temp < 32){ // relais on bis 48
 P1OUT &= ~(CONTROL);
 }
 else{ // relais off
 P1OUT |= CONTROL;
 }
 i = 0;
 }
 } // CCR0_ISR */

#pragma vector=TIMER_A0_VECTOR
__interrupt void Timer0_A0(void) {
	switch (__even_in_range(TA0IV, 6)) {
	case TA0IV_NONE: {                         		// TA0CCR0
		P1OUT &= ~(CONTROL); // relais on bis
		if (i++ == 0) { // start reading
			onewire_reset(&ow);
			onewire_write_byte(&ow, 0xcc); // skip ROM command
			onewire_write_byte(&ow, 0x44); // convert T command
			onewire_line_high(&ow);
		}
		if (i >= 14) { // read values 16
			onewire_reset(&ow);
			onewire_write_byte(&ow, 0xcc); // skip ROM command
			onewire_write_byte(&ow, 0xbe); // read scratchpad command
			for (i = 0; i < 9; i++)
				scratchpad[i] = onewire_read_byte(&ow);
			temp = ((scratchpad[1] << 8) + scratchpad[0]) * 0.0625;
			// temp control
			if (temp < 32 & TA0CCR1 < 48000) { // increase pwm
			//TA0CCR1 = TA0CCR1 +1000;
			}
			if (temp > 32 & TA0CCR1 > 1000) { // decrease pwm
			//TA0CCR1 = TA0CCR1 -1000;
			}
			i = 0;
		}
		break;
	}
	case TA0IV_TACCR1: {		                        // TA0CCR1
		P1OUT |= CONTROL; // relais off
		break;
	}
	case TA0IV_TACCR2:                                 // TA0CCR2
	{
		break;
	}
	case 6:
		break;                          // TA0CCR3
	default:
		break;
	}
}
