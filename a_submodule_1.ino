#pragma GCC optimize ("-O2")
#pragma GCC optimize ("-finline-functions")

#define OUTPUT_PRINTLN(s)                                         \
	Serial.println(s);                                              \
	lcd.print(s)

#define OUTPUT_PRINT(s)                                           \
	Serial.print(s);                                                \
	lcd.print(s)



// //Print to both Serial port and LCD, println
// inline void OUTPUT_PRINTLN(const char * sText) {
// 	Serial.println(sText);
// 	lcd.print(sText);
// } // OUTPUT_PRINTLN end
//
// //Print to both Serial port and LCD, print
// inline void OUTPUT_PRINT(const char * sText) {
// 	Serial.print(sText);
// 	lcd.print(sText);
// } // OUTPUT_PRINT end


//==============================================================================
// Smarter version of delay that does not consume all CPU
void delay_time (uint32_t wait) {
//	wait: input; dealay time in milliseconds

	unsigned long lngbegin = millis();
	unsigned long now;
	
	do {
		now = millis();
	} while ((now - lngbegin) < wait);
	
}	// end delay_time sub


//==============================================================================
//Output message to LCD
void MsgBox (char sText[], const uint8_t Pos, const uint16_t iDelay) {
//Pos options: 0-clear home, 1-line 1, 2-line2, 3-home
//sText[]: input; text to be displayed
//Pos: input; where to display on the LCD
//iDelay: input; how long the display should pause in millisecond

	uint8_t tPos = Pos;
	unsigned int leng = strlen(sText);
	char writeText[lcd_cols + 1] = {'\0'};
	uint8_t ptr = 0;

	if (Pos > lcd_rows + 1) {
		tPos = lcd_rows;
	}
	
	switch (Pos) {

	case 0:
		lcd.clear();
		break;

	case 1:
	case 2:
		lcd.setCursor (0, tPos - 1);
		break;

	default:
		lcd.home();
		break;
	}

	if (leng < lcd_cols) {

		while (ptr < leng) {
			writeText[ptr] = sText[ptr];
			ptr++;
		}
		while (ptr < lcd_cols) {
			writeText[ptr]=0x20; // space
			ptr++;
		}
		lcd.print(writeText);

	} else {	// leng > lcd_cols
		lcd.print(sText);
	}

	delay_time(iDelay);

}   // MsgBox end


//==============================================================================
//Function input box to LCD

char InputBox (const char sText[], const uint8_t Pos) {
//Pos options: 0-clear home, 1-line 1, 2-line2

	uint8_t tPos = Pos;

	if (Pos > lcd_rows) {
		tPos = lcd_rows;
	}
	
	switch (Pos) {

	case 0:
		lcd.clear();
		break;

	case 1:
	case 2:
		lcd.setCursor (0, tPos - 1);
		break;

	default:
		lcd.home();
		break;
	}
	lcd.print(sText);
	char customKey = customKeypad.getKey();
	return customKey;
}                        // InputBox end


//==============================================================================
void print_hash(unsigned char hash[]) {		//helper function

	unsigned int ptr;
	for (ptr = 0; ptr < 32; ptr++)
		Serial.print(hash[ptr],HEX);
	Serial.println("");
}


//==============================================================================
// Helper routine to dump a byte array as hex values to Serial.
void dump_byte_array(byte *Buf_temp, int bufferSize) {

	for (int i = 0; i < bufferSize; i++) {
		Serial.print(Buf_temp[i] < 0x10 ? " 0" : " ");
		Serial.print(Buf_temp[i], HEX);
	} 		// for loop end
	Serial.println("");
} 		// dump_byte_array procedure end


//==============================================================================
uint8_t Get_password (byte password[]) {   // get password. return password length

	uint8_t pwcnt = 0;
	char passkey = '\0';
	char hidekey[16] = {'\0'};

	lcd.clear();
	do {
		while (!passkey) {
			passkey = InputBox ("Input password: ", 1);
		}
		password[pwcnt] = passkey;
		passkey = {'\0'};
		hidekey[pwcnt] = '*';
		MsgBox (hidekey,2,0);
		pwcnt++;
	} while (pwcnt < pwd_size);
	password[pwcnt] = {'\0'};

	return pwcnt;
}


//==============================================================================
// map using hashed password from Buf_RF into encryption_key
int Map_key (byte encryption_key[], byte hash[], int key_b) {
// key_b: starting position on the RFID card memory location
// encryption_key: populated key for encryption and decryption
// hash: byte array which bits content to indicate with RFID memory location to use

	int key_sign = -1;
	int ict = 3;
	int kct = 1;

	if (hash[1] & 0x01) {           // determine direction of array read
		key_sign = 1;
	}
	encryption_key[0] = Buf_RF[key_b];

	do {    						// maps from hash bits onto Buf_RF to form encryption key
		for (uint8_t bitct = 7; bitct > 0; bitct--) {
			key_b = (key_b + key_sign) % (RF_buf_size-1);

			if ((hash[ict] & (1<<bitct)) && (kct < Encrypt_Size)) {
				encryption_key[kct] = Buf_RF[key_b];
				kct++;
				if (key_b <= 0) {
					key_b = RF_buf_size - 1;
				}
			}   // end if

		}   // end for loop

		if (ict <= SHA256_BLOCK_SIZE) {
			ict ++;
		}   // end if
		else {
			OUTPUT_PRINTLN ("Key gen. failed ");
			break;
		}    // end else if

	} while (kct < Encrypt_Size);
	return key_b + 1;
}   // end Map_key function

unsigned int abs_val (const int x) {
	if (x < 0) {
		return - x;
	}
	else {
		return x;
	}
}

