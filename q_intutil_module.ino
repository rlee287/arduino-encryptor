/* Blum Blum Shau Module
Origianlly from NIST statistical test suit
Generate random bits from a seed (s[]). Arraies p and q are large prime numbers that shoule be chance.

Math subroutines, part of NIST statistical test suit, for BBS calculations.
*/

#define  MAXPLEN   384
#define  FREE(A) if ( (A) ) { free((A)); (A) = NULL; }

//#ifndef  DIGIT
//typedef word DIGIT; /* 16-bit word */

#ifndef  DIGIT
typedef unsigned short DIGIT; /* 16-bit word */
#endif
#ifndef  USHORT
typedef unsigned short USHORT;
#endif
#ifndef ULONG
typedef unsigned long ULONG;
#endif

/*****************************************
** Mult - Multiply two integers          *
**                                       *
** A = B * C                             *
**                                       *
** Parameters:                           *
**                                       *
**  A      Address of the result         *
**  B      Address of the multiplier     *
**  C      Address of the multiplicand   *
**  LB      Length of B in bytes         *
**  LC      Length of C in bytes         *
**                                       *
**  NOTE:  A MUST be LB+LC in length     *
**                                       *
******************************************/
int Mult(byte *A, byte *B, int LB, byte *C, int LC)
{
	int   i, j, k;//, LA;
	DIGIT result;

	//LA = LB + LC;

	for ( i = LB - 1; i >= 0; i-- ) {
		result = 0;
		for ( j = LC - 1; j >= 0; j-- ) {
			k = i + j + 1;
			result = (word)A[k] + ((word)(B[i] * C[j])) + (result >> 8);
			A[k] = (byte)result;
		}
		A[--k] = (byte)(result >> 8);
	}

	return 0;
}

/*****************************************
** Square() - Square an integer          *
**                                       *
** A = B^2                               *
**                                       *
** Parameters:                           *
**                                       *
**  A      Address of the result         *
**  B      Address of the operand        *
**  L      Length of B in bytes          *
**                                       *
**  NOTE:  A MUST be 2*L in length       *
**                                       *
******************************************/
void Square(byte *A, byte *B, int L)
{
	Mult(A, B, L, B, L);
}

/*
   add()

   A = A + B

   LB must be <= LA

*/
byte add(byte *A, int LA, byte *B, int LB)
{
	int   i, indexA, indexB;
	DIGIT accum;

	indexA = LA - 1;  /* LSD of result */
	indexB = LB - 1;  /* LSD of B */

	accum = 0;
	for ( i = 0; i < LB; i++ ) {
		accum += A[indexA];
		accum += B[indexB--];
		A[indexA--] = (byte)(accum & 0xff);
		accum = accum >> 8;
	}

	if ( LA > LB )
		while ( accum  && (indexA >= 0) ) {
			accum += A[indexA];
			A[indexA--] = (byte)(accum & 0xff);
			accum = accum >> 8;
		}

	return (byte)accum;
}

/*****************************************
** greater - Test if x > y               *
**                                       *
** Returns TRUE (1) if x greater than y, *
** otherwise FALSE (0).                  *
**                                       *
** Parameters:                           *
**                                       *
**  x      Address of array x            *
**  y      Address of array y            *
**  l      Length both x and y in bytes  *
**                                       *
******************************************/
int greater(byte *x, byte *y, int l)
{
	int   i;

	for ( i = 0; i < l; i++ )
		if ( x[i] != y[i] )
			break;

	if ( i == l )
		return 0;

	if ( x[i] > y[i] )
		return 1;

	return 0;
}

/*****************************************
** less - Test if x < y                  *
**                                       *
** Returns TRUE (1) if x less than y,    *
** otherwise FALSE (0).                  *
**                                       *
** Parameters:                           *
**                                       *
**  x      Address of array x            *
**  y      Address of array y            *
**  l      Length both x and y in bytes  *
**                                       *
******************************************/
int less(byte *x, byte *y, int l)
{
	int   i;

	for ( i = 0; i < l; i++ )
		if ( x[i] != y[i] )
			break;

	if ( i == l ) {
		return 0;
	}

	if ( x[i] < y[i] ) {
		return 1;
	}

	return 0;
}

/*****************************************
** negate - Negate an integer            *
**                                       *
** A = -A                                *
**                                       *
**                                       *
** Parameters:                           *
**                                       *
**  A      Address of integer to negate  *
**  L      Length of A in bytes          *
**                                       *
******************************************/
int negate(byte *A, int L)
{
	int   i, tL;
	DIGIT accum;

	/* Take one's complement of A */
	for ( i = 0; i < L; i++ )
		A[i] = ~(A[i]);

	/* Add one to get two's complement of A */
	accum = 1;
	tL = L - 1;
	while ( accum && (tL >= 0) ) {
		accum += A[tL];
		A[tL--] = (byte)(accum & 0xff);
		accum = accum >> 8;
	}

	return accum;
}

/*****************************************
** sub - Subtract two integers           *
**                                       *
** A = A - B                             *
**                                       *
**                                       *
** Parameters:                           *
**                                       *
**  A      Address of subtrahend integer *
**  B      Address of subtractor integer *
**  L      Length of A and B in bytes    *
**                                       *
**  NOTE: In order to save RAM, B is     *
**        two's complemented twice,      *
**        rather than using a copy of B  *
**                                       *
******************************************/
void sub(byte *A, int LA, byte *B, int LB)
{
	byte  *tb;

	tb = (byte *)calloc(LA, 1);
	memcpy(tb, B, LB);
	negate(tb, LB);
	add(A, LA, tb, LA);

	FREE(tb);
}

/* DivMod:

     computes:
           quot = x / n
           rem = x % n
     returns:
           length of "quot"

    len of rem is lenx+1
*/
int DivMod(byte *x, int lenx, byte *n, int lenn, byte *quot, byte *rem)
{
	byte  *tx, *tn, *ttx, *ts, bmult[1];
	int   i, shift, lgth_x, lgth_n, t_len, lenq;
	DIGIT tMSn, mult;
	ULONG tMSx;
	int   underflow;

	tx = x;
	tn = n;

	/* point to the MSD of n  */
	for ( i = 0, lgth_n = lenn; i < lenn; i++, lgth_n-- ) {
		if ( *tn )
			break;
		tn++;
	}
	if ( !lgth_n )
		return 0;

	/* point to the MSD of x  */
	for ( i = 0, lgth_x = lenx; i < lenx; i++, lgth_x-- ) {
		if ( *tx )
			break;
		tx++;
	}
	if ( !lgth_x )
		return 0;

	if ( lgth_x < lgth_n )
		lenq = 1;
	else
		lenq = lgth_x - lgth_n + 1;
	memset(quot, 0x00, lenq);

	/* Loop while x > n,  WATCH OUT if lgth_x == lgth_n */
	while ( (lgth_x > lgth_n) || ((lgth_x == lgth_n) && !less(tx, tn, lgth_n)) ) {
		shift = 1;
		if ( lgth_n == 1 ) {
			if ( *tx < *tn ) {
				tMSx = (DIGIT) (((*tx) << 8) | *(tx + 1));
				tMSn = *tn;
				shift = 0;
			}
			else {
				tMSx = *tx;
				tMSn = *tn;
			}
		}
		else if ( lgth_n > 1 ) {
			tMSx = (DIGIT) (((*tx) << 8) | *(tx + 1));
			tMSn = (DIGIT) (((*tn) << 8) | *(tn + 1));
			if ( (tMSx < tMSn) || ((tMSx == tMSn) && less(tx, tn, lgth_n)) ) {
				tMSx = (tMSx << 8) | *(tx + 2);
				shift = 0;
			}
		}
		else {
			tMSx = (DIGIT) (((*tx) << 8) | *(tx + 1));
			tMSn = *tn;
			shift = 0;
		}

		mult = (DIGIT) (tMSx / tMSn);
		if ( mult > 0xff )
			mult = 0xff;
		bmult[0] = mult & 0xff;

		ts = rem;
		do {
			memset(ts, 0x00, lgth_x + 1);
			Mult(ts, tn, lgth_n, bmult, 1);

			underflow = 0;
			if ( shift ) {
				if ( ts[0] != 0 )
					underflow = 1;
				else {
					for ( i = 0; i < lgth_x; i++ )
						ts[i] = ts[i + 1];
					ts[lgth_x] = 0x00;
				}
			}
			if ( greater(ts, tx, lgth_x) || underflow ) {
				bmult[0]--;
				underflow = 1;
			}
			else
				underflow = 0;
		} while ( underflow );
		sub(tx, lgth_x, ts, lgth_x);
		if ( shift )
			quot[lenq - (lgth_x - lgth_n) - 1] = bmult[0];
		else
			quot[lenq - (lgth_x - lgth_n)] = bmult[0];

		ttx = tx;
		t_len = lgth_x;
		for ( i = 0, lgth_x = t_len; i < t_len; i++, lgth_x-- ) {
			if ( *ttx )
				break;
			ttx++;
		}
		tx = ttx;
	}
	memset(rem, 0x00, lenn);
	if ( lgth_x )
		memcpy(rem + lenn - lgth_x, tx, lgth_x);

	return lenq;
}


/*
   Mod - Computes an integer modulo another integer

   x = x (mod n)

*/
void Mod(byte *x, int lenx, byte *n, int lenn)
{
	byte  quot[MAXPLEN + 1], rem[2 * MAXPLEN + 1];

	memset(quot, 0x00, sizeof(quot));
	memset(rem, 0x00, sizeof(rem));
	if ( DivMod(x, lenx, n, lenn, quot, rem) ) {
		memset(x, 0x00, lenx);
		memcpy(x + lenx - lenn, rem, lenn);
	}
}

void ModSqr(byte *A, byte *B, int LB, byte *M, int LM)
{

	Square(A, B, LB);
	Mod(A, 2 * LB, M, LM);
}

void ahtopb (char *ascii_hex, byte *p_binary, int bin_len)
{
	byte    nibble;
	int     i;

	for ( i = 0; i < bin_len; i++ ) {
		nibble = ascii_hex[i * 2];
		if ( nibble > 'F' )
			nibble -= 0x20;
		if ( nibble > '9' )
			nibble -= 7;
		nibble -= '0';
		p_binary[i] = nibble << 4;

		nibble = ascii_hex[i * 2 + 1];
		if ( nibble > 'F' )
			nibble -= 0x20;
		if ( nibble > '9' )
			nibble -= 7;
		nibble -= '0';
		p_binary[i] += nibble;
	}
}


