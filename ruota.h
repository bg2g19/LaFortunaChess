/* ruota.h */

#ifndef RUOTA_H
#define RUOTA_H

#include <stdint.h>

#define SWN     PC2
#define SWE     PC3
#define SWS     PC4
#define SWW     PC5
#define OS_CD   PB6
#define SWC     PE7

#define REPEAT_START    60      /* after 600ms */
#define REPEAT_NEXT     10      /* every 100ms */

#define COMPASS_SWITCHES (_BV(SWW)|_BV(SWS)|_BV(SWE)|_BV(SWN))
#define ALL_SWITCHES (_BV(SWC) | COMPASS_SWITCHES | _BV(OS_CD))

void init_buttons( void );
void scan_switches( void );
uint8_t get_switch_press( uint8_t switch_mask );
uint8_t get_switch_rpt( uint8_t switch_mask );
uint8_t get_switch_state( uint8_t switch_mask );
uint8_t get_switch_short( uint8_t switch_mask );
uint8_t get_switch_long( uint8_t switch_mask );

#endif