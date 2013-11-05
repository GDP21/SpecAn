/*
** FILE NAME:   $RCSfile: maths_funcs.h,v $
**
** TITLE:       32 bit saturating add
**
** PROJECT:		UCCP Common
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Arithmetric support functions.
**
**				Copyright (C) 2009, Imagination Technologies Ltd.
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

/*! \file maths_funcs.h
*******************************************************************************
	\brief	Collection of fixed point arithmetic functions
*******************************************************************************/
/*! \mainpage Maths Functions
*******************************************************************************
 \section intro Introduction

 This set of linked HTML pages provides the public interface to a set of
 fixed point arithmetic functions.
 These function do not perform thorough bounds checking, so need to be used
 with care.

 <b>Copyright (C) 2009, Imagination Technologies Ltd.</b>

*******************************************************************************/

/*! Number of fractional bits in Q8 number format */
#define MATHS_SHIFT_Q8                                (8)
/*! Number of fractional bits in Q15 number format */
#define MATHS_SHIFT_Q15                               (15)
/*! Number of fractional bits in Q16 number format */
#define MATHS_SHIFT_Q16                               (16)
/*! Number of fractional bits in Q24 number format */
#define MATHS_SHIFT_Q24                               (24)

/*! Constant for unity in Q8 number format */
#define MATHS_ONE_Q8                                  (1<<MATHS_SHIFT_Q8)
/*! Constant for unity in Q15 number format */
#define MATHS_ONE_Q15                                 (1<<MATHS_SHIFT_Q15)
/*! Constant for unity in Q16 number format */
#define MATHS_ONE_Q16                                 (1<<MATHS_SHIFT_Q16)
/*! Constant for unity in Q24 number format */
#define MATHS_ONE_Q24                                 (1<<MATHS_SHIFT_Q24)


/*! Functional macro for rounding from Q8 number format to Q0 */
#define MATHS_ROUNDQ8toQ0(a)                          (((a) + (1<<(MATHS_SHIFT_Q8-1)))>>MATHS_SHIFT_Q8)

/*! Q.8 x Q.8 multiplication giving Q.8 result */
#define MATHS_MUL_Q8(a,b)                             (MATHS_roundAndShift((a)*(b), MATHS_SHIFT_Q8))
/*! Q.16 x Q.8 multiplication giving Q.16 result (less likely to saturate than using the Q.8xQ.8) */
#define MATHS_MUL_Q16xQ8(a,b)                         ((MATHS_roundAndShift((a), MATHS_SHIFT_Q8))*(b))

/*! log2 approximation with rounding, input and output values in non-fractional format */
#define MATHS_log2Rnd(a)	(MATHS_roundAndShift(MATHS_log2(a),MATHS_SHIFT_Q16) + MATHS_SHIFT_Q16)


/*!
******************************************************************************

 @Function @MATHS_divQ16

 <b>Description:</b>\n

 This function divides two Q.16 numbers giving a Q.16 result.

 \param[in] numerator	Numerator of division in numerator format
 \param[in] denominator	Denominator of division in numerator format

 \return 				Result of division in Q.16 format

******************************************************************************/
long MATHS_divQ16(long numerator, long denominator);

/*!
******************************************************************************

 @Function @MATHS_log10

 <b>Description:</b>\n

 This function is a log10 approximation with Q.16 format input and Q.16 format
 output.

 \param[in] linear	Linear input value in Q.16 format

 \return 			Result of log10 of input value, in Q.16 format

******************************************************************************/
long MATHS_log10(long linear);

/*!
******************************************************************************

 @Function @MATHS_log2

 <b>Description:</b>\n

 This function is a log2 approximation with Q.16 format input and Q.16 format
 output.

 \param[in] linear	Linear input value in Q.16 format

 \return 			Result of log2 of input value, in Q.16 format

******************************************************************************/
long MATHS_log2(long linear);

/*!
******************************************************************************

 @Function @MATHS_saturatingAdd32

 <b>Description:</b>\n

 This function performs a saturating add of the two signed 32-bit input values.

 \param[in] a,b		Input values

 \return 			Sum of input values saturated to the limits of \e long.

******************************************************************************/
long MATHS_saturatingAdd32(long a, long b);

/*!
******************************************************************************

 @Function @MATHS_restrictRange

 <b>Description:</b>\n

 This function restricts the range of a value by saturating to the \e max and
 \e min limits given.

 \param[in] value	Value to be restricted
 \param[in] min		Minimum allowable value
 \param[in] max		Maximum allowable value

 \return 			The input value restricted to the given limits

******************************************************************************/
long MATHS_restrictRange(long value, long min, long max);

/*!
******************************************************************************

 @Function @MATHS_core

 <b>Description:</b>\n

 This function cores a value. That is zeroing any input value with a magnitude
 smaller than a given threshold.

 \param[in] value	Value to be cored
 \param[in] coreVal	The absolute coring threshold

 \return 			Result of coring

******************************************************************************/
long MATHS_core(long value, long coreVal);

/*!
******************************************************************************

 @Function @MATHS_roundAndScale

 <b>Description:</b>\n

 This function scales down a number applying rounding in the process.

 \param[in] value		The value to be scaled down
 \param[in] denominator	The denominator

 \return 				round(value/denominator)

******************************************************************************/
long MATHS_roundAndScale(long value, long denominator);

/*!
******************************************************************************

 @Function @MATHS_roundAndShift

 <b>Description:</b>\n

 This function scales down a number using a right shift applying rounding in
 the process.

 \param[in] value		The value to be scaled down
 \param[in] shift		The denominator as a right shift

 \return 				round(value * 2^-shift)

******************************************************************************/
long MATHS_roundAndShift(long value, long shift);

/*!
******************************************************************************

 @Function @MATHS_highPrecisionDiv64

 <b>Description:</b>\n

 This function preforms a high precision division of two signed 64-bit numbers
 implemented by normalisation of the numerator to maintain optimum precision.

 \param[in] num		The numerator
 \param[in] den		The denominator

 \return 			Result of division

******************************************************************************/
long long MATHS_highPrecisionDiv64(long long num, long long den);

