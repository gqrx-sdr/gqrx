/*
 *  ss_stdint.h
 *  suitsat_os
 *
 *  Created by N9WXU on 3/16/08.
 *  Copyright 2008 AMSAT. All rights reserved.
 *
 */

#ifndef _SS_STDINT_H
#define _SS_STDINT_H

/**********************************************************/
/* Basic Types - Modify for each compiler/architecture    */
/**********************************************************/

#ifdef HI_TECH_C
#if defined(_PIC14) || defined(_PIC14E)
/*
 * Type definitions for PIC16 Architecture,
 * Using the Hitech PICC compiler
 */
typedef unsigned char  bool;
typedef unsigned char  u8;
typedef unsigned int   u16;
typedef unsigned long  u32;

typedef struct {			// 64 Bit value is not supported natively, so we'll make it a structure
    u32 low;
    u32 high;
} u64;


typedef signed char	s8;
typedef signed int	s16;
typedef signed long	s32;
typedef struct {			// 64 Bit value is not supported natively, so we'll make it a structure
    u32 low;
    s32 high;
} s64;

typedef s8		int8;
typedef s16		int16;
typedef s32		int32;
typedef s64     int64; 		// Reuse signed structure we created
//
// PACKED doesn't mean anything to an 8 bit machine
#define PACKED

#endif // _PIC14
#endif // HI_TECH_C


#ifdef __PIC24__
/*
 * Type definitions for PIC24 Architecture,
 * Using the Microchip C30 compiler
 */

typedef unsigned char  bool;
typedef unsigned char  u8;
typedef unsigned int   u16;
typedef unsigned long  u32;
typedef unsigned long long u64;

typedef char	int8;
typedef int		int16;
typedef long	int32;
typedef long long int64;

typedef char	s8;
typedef short	s16;
typedef int		s32;
typedef long long s64;

#define PACKED   __attribute__ ((packed))

#endif

#ifdef __PIC32MX__
/*
 * Type definitions for PIC32 Architecture,
 * Using the Microchip C32 compiler
 */
#define PACKED   __attribute__ ((packed))
typedef unsigned char  		bool;
typedef unsigned char  		u8;
typedef unsigned short 		u16;
typedef unsigned int   		u32;
typedef unsigned long long  u64;

typedef signed char			s8;
typedef signed short		s16;
typedef signed int			s32;
typedef signed long long    s64;
#endif

#define _LIN

#if defined(_LIN) || defined (_WIN32)
/*
 * Type definitions for IA32 Architecture,
 * Using the Microsoft C compiler
 */

#ifndef PACKED
/* GCC already defines packed on this architecture, so skip it if it's already there */
#define PACKED   __attribute__ ((packed))
#endif

//typedef unsigned char  bool;
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long long  u64;

typedef char	s8;
typedef short	s16;
typedef int		s32;
typedef signed long long    s64;

#endif 

/*
 * Types defined for all architectures!
 */

// 48 Bit type is needed due to limited memory on the PSU.
typedef struct {			// 48 Bit value is not supported natively, so we'll make it a structure
    u32 low;
    u16 high;
} PACKED u48;
typedef struct {			// 48 Bit value is not supported natively, so we'll make it a structure
    u32 low;
    s16 high;
} PACKED s48;

// To make our life easy converting U48 to U64 on architectures that support it, here's some macros
#define U48TOU64(v48)		(	 ( ((u64) v48.high) << 32 ) + ( (v48).low ) )
#define S48TOS64(v48)		( (	((s64) v48.high) << 32 ) + ( (v48).low ) )

#endif
