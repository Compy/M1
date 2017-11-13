// Special version of this file for Xcode - RB

#ifndef _OSDDEFS_H_
#define _OSDDEFS_H_

/* platform specific stuff */


typedef signed   char      INT8;
typedef signed   short     INT16;
typedef signed   int       INT32;
typedef signed   long long INT64;
typedef unsigned char      UINT8;
typedef unsigned short     UINT16;
typedef unsigned int       UINT32;
typedef unsigned long long UINT64;

#define HAS_ADPCM		1
#define HAS_ADSP2100	1
#define HAS_ADSP2101	1
#define HAS_ADSP2104	1
#define HAS_ADSP2105	1
#define HAS_ADSP2115	1
#define HAS_AY8910		1
#define HAS_BSMT2000	1
#define HAS_C140		1
#define HAS_C352		1
#define HAS_CEM3394		1
#define HAS_CUSTOM		1
#define HAS_DAC			1
#define HAS_ES5505		1
#define HAS_ES5506		1
#define HAS_HC55516		1
#define HAS_HD63701		1
#define HAS_IREMGA20	1
#define HAS_K005289		1
#define HAS_K007232		1
#define HAS_K051649		1
#define HAS_K053260		1
#define HAS_K054539		1
#define HAS_M65C02		1
#define HAS_M6803		1
#define HAS_M6808		1
#define HAS_MPEG		1
#define HAS_MSM5205		1
#define HAS_MULTIPCM	1
#define HAS_N7751		1
#define HAS_NAMCO		1
#define HAS_OKIM6295	1
#define HAS_PIC16C55	1
#define HAS_PIC16C57	1
#define HAS_POKEY		1
#define HAS_QSOUND		1
#define HAS_RF5C68		1
#define HAS_SCSP		1
#define HAS_SEGAPCM		1
#define HAS_SN76496		1
#define HAS_TMS5220		1
#define HAS_UPD7759		1
#define HAS_VLM5030		1
#define HAS_Y8950		1
#define HAS_YM2151_ALT	1
#define HAS_YM2203		1
#define HAS_YM2413		1
#define HAS_YM2608		1
#define HAS_YM2610		1
#define HAS_YM2610B		1
#define HAS_YM2612		1
#define HAS_YM3438		1
#define HAS_YM3526		1
#define HAS_YM3812		1
#define HAS_YMF262		1
#define HAS_YMF271		1
#define HAS_YMF278B		1
#define HAS_YMZ280B		1
#define HAS_MSM5232		1

#define INLINE			static inline

#define M1				1

#define __MACOSX__		1

int change_to_app_directory(void);

#ifdef __BIG_ENDIAN__
	#define LSB_FIRST	0
#else
	#define LSB_FIRST	1
#endif

#endif // OSDDEFS_H