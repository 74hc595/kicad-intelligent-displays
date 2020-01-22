#pragma once

#include <avr/io.h>

/* those golden oldies */
#define XPASTE_(x,y)    x##y
#define PASTE_(x,y)     XPASTE_(x,y)
#define XPASTE3_(x,y,z) x##y##z
#define PASTE3_(x,y,z)  XPASTE3_(x,y,z)
#define STR_(x)         #x
#define XSTR_(x)        STR_(x)

#define VPORT(p)        PASTE_(VPORT, p)
#define PORT(p)         PASTE_(PORT, p)

#define PORT_OUTPUTS_(p)      VPORT(p).DIR = 0xFF
#define PORT_INPUTS_(p)       VPORT(p).DIR = 0x00
#define PORT_OUT_(p,d)        VPORT(p).OUT = (d)
#define PORT_VALUE_(p)        VPORT(p).IN
#define PIN_INPUT_(p,n)       VPORT(p).DIR &= ~_BV(n)
#define PIN_OUTPUT_(p,n)      VPORT(p).DIR |= _BV(n)
#define PIN_LOW_(p,n)         VPORT(p).OUT &= ~_BV(n)
#define PIN_HIGH_(p,n)        VPORT(p).OUT |= _BV(n)
#define PIN_TOGGLE_(p,n)      VPORT(p).IN |= _BV(n)
#define PIN_IS_HIGH_(p,n)     (VPORT(p).IN & _BV(n))
#define PIN_IS_LOW_(p,n)      (!PIN_IS_HIGH_(p,n))
#define PIN_VALUE_(p,n)       (!PIN_IS_LOW_(p,n))
#define PIN_INTFLAG_(p,n)     ((VPORT(p).INTFLAGS & _BV(n))!=0)
#define PIN_INTCLEAR_(p,n)    VPORT(p).INTFLAGS |= _BV(n)
#define PIN_CTRL_(p,n)        PORT(p).PASTE3_(PIN,n,CTRL)

#define port_outputs(pn)      PORT_OUTPUTS_(pn##_PORT)
#define port_inputs(pn)       PORT_INPUTS_(pn##_PORT)
#define port_out(pn,d)        PORT_OUT_(pn##_PORT, d)
#define port_value(pn)        PORT_VALUE_(pn##_PORT)
#define pin_input(pn)         PIN_INPUT_(pn##_PORT, pn##_PIN)
#define pin_output(pn)        PIN_OUTPUT_(pn##_PORT, pn##_PIN)
#define pin_low(pn)           PIN_LOW_(pn##_PORT, pn##_PIN)
#define pin_high(pn)          PIN_HIGH_(pn##_PORT, pn##_PIN)
#define pin_toggle(pn)        PIN_TOGGLE_(pn##_PORT, pn##_PIN)
#define pin_is_high(pn)       PIN_IS_HIGH_(pn##_PORT, pn##_PIN)
#define pin_is_low(pn)        PIN_IS_LOW_(pn##_PORT, pn##_PIN)
#define pin_value(pn)         PIN_VALUE_(pn##_PORT, pn##_PIN)
#define pin_intflag(pn)       PIN_INTFLAG_(pn##_PORT, pn##_PIN)
#define pin_intclear(pn)      PIN_INTCLEAR_(pn##_PORT, pn##_PIN)
#define pin_ctrl(pn)          PIN_CTRL_(pn##_PORT, pn##_PIN)
#define pin_pullup(pn)        pin_ctrl(pn) |= PORT_PULLUPEN_bm
#define pin_output_low(pn)    do { pin_output(pn); pin_low(pn);    } while (0)
#define pin_output_high(pn)   do { pin_output(pn); pin_high(pn);   } while (0)
#define pin_input_pullup(pn)  do { pin_input(pn);  pin_pullup(pn); } while (0)
