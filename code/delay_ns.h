/**
 * Simple sub-microsecond-precision delays using strings of NOPs.
 * Usable resolution is limited by clock speed. e.g. at 20MHz, delays must be
 * a multiple of 50 nanoseconds. If the delay value is not an even multiple of
 * (1/F_CPU) nanoseconds, the delay duration will be rounded down to the nearest
 * cycle.
 */
#pragma once

#define NS_PER_CYCLE      (1000000000/(F_CPU))
#define NS_TO_CYCLES(ns)  ((ns)/NS_PER_CYCLE)
#define delay_ns_max(ns)  asm volatile(".rept %0\nnop\n.endr" : : "i" (NS_TO_CYCLES(ns)))
