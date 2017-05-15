/* Blum Blum Shau Module
Origianlly from NIST statistical test suit
Generate random bits from a seed (s[]). Arraies p and q are large prime numbers that shoule be chance.
*/

void PRNG_BBS(const unsigned int tp_n, byte s[]) {
// tp_n: input; number of random bits to be generated
// s[]: intput; byte arrary containing PRNG's seed

	unsigned long lngtime;
	unsigned int   i;
	byte  p[Seed_byte_size], q[Seed_byte_size], n[128], x[256];
	byte  byte_BBS = 0;
	uint8_t j = 0;
	char bit_grph[lcd_cols + 1] = {'\0'};

	bit_grph[0] = '*';
	bit_grph[lcd_cols] = {'\0'};
	MsgBox ("Running PRNG...", 0, 0);
	lngtime = millis();

	ahtopb("E65097BAEC92E70478CAF4ED0ED94E1C94B154466BFB9EC9BE37B2B0FF8526C222B76E0E915017535AE8B9207250257D0A0C87C0DACEF78E17D1EF9DC44FD91F", p, 64);
	ahtopb("E029AEFCF8EA2C29D99CB53DD5FA9BC1D0176F5DF8D9110FD16EE21F32E37BA86FF42F00531AD5B8A43073182CC2E15F5C86E8DA059E346777C9A985F7D8A867", q, 64);
	memset(n, 0x00, 128);
	Mult(n, p, 64, q, 64);
	memset(s, 0x00, 64);
	memset(x, 0x00, 256);
	ModSqr(x, s, 64, n, 128);

	Serial.print ("begin BBS ");

	for ( i = 0; i < tp_n; i++ ) {
		ModSqr(x, x, 128, n, 128);
		Serial.print (i, DEC);
		Serial.print (", ");


		memcpy(x, x + 128, 128);
		j = i % 8;
		if ( (x[127] & 0x01) == 0 ) {
			byte_BBS += (0 << (7 - j));
		}
		else {
			byte_BBS += (1 << (7 - j));
		}

		if ((i + 1) % 8 == 0) {
			Buf_RF[(int)(i / 8)] = byte_BBS;
			byte_BBS = 0;
		}
		
		bit_grph[(int)(i * (lcd_cols - 1) / tp_n)] = '*';
		MsgBox (bit_grph,2,0);
	}   // end for "i"

	Serial.print ("BBS done: ");
	Serial.println (millis() - lngtime);
}

