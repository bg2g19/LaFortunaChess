/* ruota.c
    Copyright Peter Dannegger (danni@specs.de)
    http://www.mikrocontroller.net/articles/Entprellung 
    http://www.mikrocontroller.net/articles/Drehgeber
    Slightly adapted by Klaus-Peter Zauner for FortunaOS, March 2015
*/

#include <avr/interrupt.h>

#include "ruota.h"
volatile uint8_t switch_state;   /* debounced and inverted key state:
                                 bit = 1: key pressed */
volatile uint8_t switch_press;   /* key press detect */
volatile uint8_t switch_rpt;     /* key long press and repeat */

void init_buttons() {

    DDRE &= ~_BV(SWC);   /* Central button */
	PORTE |= _BV(SWC);
	
	DDRC &= ~COMPASS_SWITCHES;  /* configure compass buttons for input */
	PORTC |= COMPASS_SWITCHES;  /* and turn on pull up resistors */

    EICRB |= _BV(ISC40) | _BV(ISC50) | _BV(ISC71);
}

void scan_switches() {
  static uint8_t ct0, ct1, rpt;
  uint8_t i;
 
  cli();
  /* 
     Overlay port E for central button of switch wheel and Port B
     for SD card detection switch:
  */ 
  i = switch_state ^ ~( (PINC|_BV(SWC)|_BV(OS_CD))	\
                   & (PINE|~_BV(SWC)) \
                   & (PINB|~_BV(OS_CD)));  /* switch has changed */
  ct0 = ~( ct0 & i );                      /* reset or count ct0 */
  ct1 = ct0 ^ (ct1 & i);                   /* reset or count ct1 */
  i &= ct0 & ct1;                          /* count until roll over ? */
  switch_state ^= i;                       /* then toggle debounced state */
  switch_press |= switch_state & i;        /* 0->1: key press detect */
 
  if( (switch_state & ALL_SWITCHES) == 0 )     /* check repeat function */
     rpt = REPEAT_START;                 /* start delay */
  if( --rpt == 0 ){
    rpt = REPEAT_NEXT;                   /* repeat delay */
    switch_rpt |= switch_state & ALL_SWITCHES;
  }
  sei();
}

uint8_t get_switch_press( uint8_t switch_mask ) {
  cli();                         /* read and clear atomic! */
  switch_mask &= switch_press;         /* read key(s) */
  switch_press ^= switch_mask;         /* clear key(s) */
  sei();
  return switch_mask;
}

uint8_t get_switch_rpt( uint8_t switch_mask ) {
  cli();                       /* read and clear atomic! */
  switch_mask &= switch_rpt;         /* read key(s) */
  switch_rpt ^= switch_mask;         /* clear key(s) */
  sei();
  return switch_mask;
}

uint8_t get_switch_state( uint8_t switch_mask ) {
	switch_mask &= switch_state;
	return switch_mask;
}

uint8_t get_switch_short( uint8_t switch_mask ) {
  cli();                                         
  return get_switch_press( ~switch_state & switch_mask );
}

uint8_t get_switch_long( uint8_t switch_mask ) {
  return get_switch_press( get_switch_rpt( switch_mask ));
}