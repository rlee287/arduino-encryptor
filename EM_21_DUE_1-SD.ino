/*
Arduino program Encryptor, optimized for DUE

This program is created as part of a Synopsys Science Fair project entry for the year 2017.

By Ryan Lee, March 14, 2017

This program is in the public domain.
	
*/

#pragma GCC optimize ("-O2")
#pragma GCC optimize ("-finline-functions")

// include the library code:
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <SD.h>
#include <SPI.h>
#include <MFRC522.h>

// Initialization or variables
// Hardware related
#define SD_Encry_CS 49          // Plain text input SD 1 reader pin select
//#define SD_Decry_CS 51        // Plain text input SD 2 reader pin select
#define SD_Decry_CS 49        	// Plain text input SD 2 reader pin select
#define sensorPin A0            // select the analog input  pin for  the sound sensor
#define X_pin A1                // analog pin connected to X output
#define Y_pin A2                // analog pin connected to Y output
#define ROWS 4                  // keypad row size, 4 rows
#define COLS 4                  // keypad colums size, 4 colums
#define RST_PIN 48            	// RFID pin
#define SS_PIN 53           	// RFID pin
#define NEW_UID {0xDE, 0xAD, 0xBE, 0xEF}    // RFID card ID, non-critical
#define lcd_rows 2				// LCD display number of rows
#define lcd_cols 16				// LCD display number of columns

// Variables, array sizes
#define BA_size 256             // number of bytes per read/encrypt/write
#define Block_size 16           // Block size in byte for each en/decryption call
#define RF_buf_size 752         // Total RFID card memory in bytes = ((16 sector X 3 blocks) - 1 block) X 16 bytes
#define SHA256_BLOCK_SIZE 32        // SHA256 outputs a 32 byte digest
#define Encrypt_Size 64             // Pre-padding block size in bytes
#define Seed_byte_size 64           // BBS PRNG seed size in bytes
#define Seed_bit_size 512           // BBS PRNG seed size in bits = Seed_byte_size * 8
#define Entropy_rng 50           	// Threshold value to minimize analog input noise
#define pwd_size 16                 // number of password's characters 

#define OUTPUT_PRINTLN(s)                                         \
	Serial.println(s);                                              \
	lcd.print(s)

#define OUTPUT_PRINT(s)                                           \
	Serial.print(s);                                                \
	lcd.print(s)

//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {22, 23, 24, 25};  //connect to the row pinouts of the keypad
byte colPins[COLS] = {26, 27, 28, 29};  //connect to the column pinouts of the keypad

//================================================================================
//Globe variable for file IO and RFID IO
byte Buf_in[BA_size + Block_size];     		// SD file input buffer
byte Buf_out[BA_size + Block_size];         // SD file output buffer
byte Buf_RF[RF_buf_size];                 	// RFID card buffer

/* 8.3 file convention */
char InputFile_Name[14];                  	// Global File parameters
char OutputFile_Name[14];
unsigned long InputFile_Pos = 0;
unsigned long OutputFile_Pos = 0;
unsigned long InputFile_Size = 0;

//================================================================================
//Setup and init for SHA256 hash function
typedef unsigned long  WORD;             	// 32-bit word, change to "long" for 16-bit machines
typedef struct {
	byte data[64];
	WORD datalen;
	unsigned long long bitlen;
	WORD state[8];
} SHA256_CTX;

//================================================================================
//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(30, 31, 32, 33, 34, 35);


//==================================================================================
void setup() {                          // Setup variables

	Serial.begin(38400);                // Open serial communications
	boolean bSD1_status = true;         // Status flag
	boolean bSD2_status = true;         // Status flag
	boolean bRF_status = true;          // Status flag

	memset (Buf_RF, 0x00, RF_buf_size);
	customKeypad.setHoldTime(100);          // set up keypad delay
	customKeypad.setDebounceTime(100);      // Default is 1000mS
	lcd.begin(lcd_cols, lcd_rows);                       // set up the LCD's number of columns and rows:
	lcd.clear();

	pinMode (SD_Encry_CS, OUTPUT);          // Setup SD and RFID card select pins states
	digitalWrite (SD_Encry_CS, HIGH);
	pinMode (SD_Decry_CS, OUTPUT);
	digitalWrite (SD_Decry_CS, HIGH);
	pinMode (SS_PIN, OUTPUT);
	digitalWrite (SS_PIN, HIGH);
	pinMode(sensorPin,INPUT);
	pinMode(X_pin,INPUT);
	pinMode(Y_pin,INPUT);
	delay_time (1);

	//abcdefg1234567890123456
	MsgBox ("Welcome to the  ", 0, 0);
	MsgBox ("Encryptor!      ", 2, 1500);

	if (!SD_setup(SD_Encry_CS)) {           // Check if Input SD1 card present
		MsgBox ("SD1 card missing", 0, 1500);
		bSD1_status = false;
	}
	if (!SD_setup(SD_Decry_CS)) {           // Check if output SD2 card present
		MsgBox ("SD2 card missing", 0, 1500);
		bSD2_status = false;
	}
	if (!RFID_setup(0)) {                   // Check if output SD card present
		MsgBox ("RF card Missing", 0, 1500);
		bRF_status = false;
	}

	if (bSD1_status && bSD2_status && bRF_status) {
		OUTPUT_PRINTLN("Init Done!     ");
		lcd.clear();
	}
	else {
		MsgBox ("Init failed", 0, 500);
		//                 1234567890123456
		while (!InputBox ("Any key to cont.", 2)) {}
	}

} // setup end


//=================================================================================
// Read file from SD cards. Input SD and output SD pin select number
boolean Select_Input(const uint8_t Input_PS, const uint8_t Output_PS,
                     const boolean bEncrypt_flag, byte encryption_key[],
                     byte BBS_seed[]) {

	byte cTemp[Block_size];
	unsigned int iLength;					//number of bytes read into Buf_in
	unsigned int Read_size = 0;
	unsigned int Write_size = 0;
	uint8_t iremainder = 0;
	byte bpadding[Block_size];
	byte bpadding_post[Block_size];
	boolean b1st_cycle_flg = true;
	boolean bDecrypt_ok = false;
	unsigned long OutputFile_Size = 0;

	InputFile_Pos = 0;
	OutputFile_Pos = 0;

	//       1234567890123456
	MsgBox ("Select File     ", 0, 0);

	if (SD_Getfile(Input_PS)) {

		if (InputFile_Size < BA_size) {
			Read_size = InputFile_Size;
		}
		else {
			Read_size = BA_size;
		}

		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// add remove padding // adding padding to beggining and end of file.

		if (bEncrypt_flag) {             // Encryption: add pre-padding and calculate post-padding

			iremainder = Block_size - (InputFile_Size % Block_size);
			BBS_seed[0] = byte(iremainder);
			Post_padding (bpadding, iremainder);

			for (unsigned int i = 0; i < Encrypt_Size; i+= Block_size) {    // Calcualte & add pre-padding
				memcpy (cTemp, BBS_seed + i, Block_size);
				Encryption (cTemp, encryption_key, Encrypt_Size);
				memcpy (Buf_out + i, cTemp, Block_size);
			}   // end for loop

			OutputFile_Size = InputFile_Size + Encrypt_Size + iremainder;
			SD_Writefile (Buf_out, Output_PS, Encrypt_Size);                // Write pre-padding to SD card
			bDecrypt_ok = true;

		}   // end if (bEncryption_flag)
		else {                  // Decryption: remove pre-padding

			iLength = SD_Readfile(Buf_in, Input_PS, Encrypt_Size);          // input data from SD card file
			if (iLength == Encrypt_Size) {

				for (unsigned int i = 0; i < Encrypt_Size; i+= Block_size) {// decrypt pre-padding
					memcpy (cTemp, Buf_in + i, Block_size);
					Decryption (cTemp, encryption_key, Encrypt_Size);

					if (b1st_cycle_flg) {                                   // execute only at 1st time, get post-padding size						
						iremainder = int(cTemp[0]);
						if ((iremainder > Block_size) || (iremainder < 0)) {
							iremainder = Block_size;
						}
						Post_padding (bpadding, iremainder);
						b1st_cycle_flg = false;
					}   // end if (b1st_cycle_flg)

				}   // end for loop
				Read_size -= Encrypt_Size;
				OutputFile_Size = InputFile_Size - Encrypt_Size - iremainder;
				bDecrypt_ok = true;

			}   // end if (iLength == Encrypt_Size)
			else {
				bDecrypt_ok = false;
			}
		}   // end else if (bEncrypt_flag)

		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		while (InputFile_Pos < InputFile_Size) {

			iLength = SD_Readfile(Buf_in, Input_PS, Read_size);				// input data from SD card file

			if ((iLength < BA_size) && (bEncrypt_flag)) {                   // add post-padding to buffer for encryption
				memcpy (Buf_in + iLength, bpadding ,iremainder);
				iLength += iremainder;
			}   // end if

			for (unsigned int i = 0; i < iLength; i += Block_size) {
				memcpy (cTemp, Buf_in + i, Block_size);						// cTemp contains original text

				if (bEncrypt_flag) {                                        // call Encryption subroutine;
					Encryption (cTemp, encryption_key, Encrypt_Size);
					memcpy (Buf_out + i, cTemp, Block_size);                // after encryption, cTemp contains ciphered text
				}
				else {                                                      // call Decryption subrountine;
					Decryption (cTemp, encryption_key, Encrypt_Size);

					if (OutputFile_Pos + i == OutputFile_Size) {         	// remove post-padding
						iLength -= iremainder;
						memcpy (Buf_out + i, cTemp, Block_size - iremainder);
						memcpy (bpadding_post, cTemp  + (Block_size - iremainder), iremainder);
					}   // end if
					else {                                                   // skipped writting to SD if it is post-padding
						memcpy (Buf_out + i, cTemp, Block_size);            // after decryption, cTemp contains plaintext
					}

				}   // end else if

			}       // for loop end (bEncrypt_flag)

			Write_size = iLength;
			SD_Writefile (Buf_out, Output_PS, Write_size);                    // output (write) data to 2nd SD card

		} // while loop end

Serial.println("en/decrypt done!");

        if (!bEncrypt_flag && bDecrypt_ok) {               // check if decrypted post-padding is correct
            for (uint8_t i = 0; i < iremainder; i++) {
                if (bpadding[i] != bpadding_post[i]) {
                    bDecrypt_ok = false;
                    return bDecrypt_ok;
                }   // end if (bpadding)
            }   // end for loop
        }   // end if (!bEncrypt_flag)

//		if (!bEncrypt_flag && bDecrypt_ok) {               // check if decrypted post-padding is correct
//			uint16_t check_if_ok = 0;
//			for (uint8_t i = 0; i < iremainder; i++) {
//				/* Deliberately tap into uninitialized stuff here
//				 * The unpredictable value returned by malloc works to advantage here
//				 * This prevents the compiler from performing constant propagation
//				 * and possibly optimizing out this part completely
//				 */
//				uint8_t * addval_ptr = (uint8_t*)malloc(sizeof(uint8_t));
//				if (*addval_ptr == 0) {
//					*addval_ptr += 0x12;
//				}
//				if (bpadding[i] != bpadding_post[i]) {
//					check_if_ok += (*addval_ptr);
//				}   // end if (bpadding)
//				free(addval_ptr);
//			}   // end for loop
//			if (check_if_ok != 0) {
//				bDecrypt_ok = false;
//				return bDecrypt_ok;
//			}
//		}   // end if (!bEncrypt_flag)
		return bDecrypt_ok;

	}   //	end if: SD_Getfile(Input_PS)

	else {
		OUTPUT_PRINTLN ("SD card error");
		return false;
	}   //    end else if: SD_Getfile(Input_PS)

}   // function Select_Input end


//================================================================================
boolean decrypt(void) { // RF read

	lcd.clear();
	//              1234567890123456
//	OUTPUT_PRINTLN("Decrypting...   ");

	boolean bTemp = RFID_setup(1);
	if (!bTemp) {
		OUTPUT_PRINTLN("RF not working");
	}
	return bTemp;
}   // function decrypt end


//================================================================================
boolean encrypt(void) { // RF write

	lcd.clear();
	//              1234567890123456
//	OUTPUT_PRINTLN("Encrypting...   ");

	boolean bTemp = RFID_setup(2);
	if (!bTemp) {
		OUTPUT_PRINTLN("RF not working");
	}
	return bTemp;
}   // function decrypt end


//================================================================================
void loop() {

	char cOption;
	int key_b = 0;
	byte password [pwd_size + 1] = {'\x00'};
	unsigned int pass_leng;
	byte encryption_key [Encrypt_Size];
	byte hash[SHA256_BLOCK_SIZE];
	byte hash_2[SHA256_BLOCK_SIZE];
	byte BBS_seed[Seed_byte_size];
	byte seed_2 [Seed_byte_size];

	Zero_out (Buf_RF, RF_buf_size);

	//       1234567890123456
	MsgBox ("1:Encrypt File  ", 1, 0);
	cOption = InputBox("2:Decrypt File  ", 2);

	if (cOption) {
		switch (cOption) {

		case '1': {      // Encryption. generate entropy, feed into BBS then create one time pad

//		             1234567890123456
			MsgBox ("Encryption      ", 0, 0);
			MsgBox ("Selected        ", 2, 1000);
           
			Entropy_j (BBS_seed);                           //Read analog input bits to generate PRNG's seed
			memcpy (seed_2, BBS_seed, Seed_byte_size);
//			PRNG_BBS (RF_buf_size*8, Seed_2);				//in actual program (PRNG_BBS(RF_buf_size*8, Seed_2);)
			PRNG_BBS (512, seed_2);                         

//			if (encrypt()) { // temperory disable for demo/debugging
			if (decrypt()) {

//       		         1234567890123456
				MsgBox ("Encryption key", 0, 0);
				MsgBox ("written to RFID", 0, 1000);
				Serial.println ("Encryption key written to RFID");

				pass_leng = Get_password (password);
				SHA256_hash (password, hash, pass_leng);                    // pass password to Hash function;
				key_b = int((hash[0] << 1) | (hash[1] & 0x80)) + hash[2];   // begining byte of the encrytion key
				key_b = Map_key(encryption_key, hash, key_b);

				Zero_out (password, pwd_size);
				Zero_out (hash, SHA256_BLOCK_SIZE);
				Zero_out (Buf_RF, RF_buf_size);

				SHA256_hash (encryption_key, hash, Encrypt_Size);		// 1st 32 bytes of sponge key
				memcpy (encryption_key, hash, SHA256_BLOCK_SIZE);
				SHA256_hash (hash, hash_2, SHA256_BLOCK_SIZE);			// 2nd 32 bytes of sponge key
				memcpy (encryption_key + SHA256_BLOCK_SIZE, hash_2, SHA256_BLOCK_SIZE);
				Zero_out (hash, SHA256_BLOCK_SIZE);
				Zero_out (hash_2, SHA256_BLOCK_SIZE);

				// read input file to be encrypted
				if (Select_Input(SD_Encry_CS, SD_Decry_CS, true, encryption_key, BBS_seed)) {
					Zero_out (Buf_in, BA_size);
					Zero_out (Buf_out, BA_size);
					Zero_out (encryption_key, Encrypt_Size);
					Zero_out (BBS_seed, Seed_byte_size);
					//              1234567890123456
					OUTPUT_PRINTLN("Encrypt succcess");
					MsgBox("Encrypt succcess",0, 1000);
				} // end if
				else {
					OUTPUT_PRINTLN("SD Read failed  ");
					MsgBox("Encrypt Failed",0, 3000);
					break;
				} // end else if

			}	// end if (encrypt)

			else {
				/* Randomness is NOT used as a one time pad!
				 *  It is a key or a pad!
				 */
				Serial.println ("Key Generation failed");
				MsgBox ("Key Generation  ", 0, 0);
				MsgBox ("failed          ", 2, 2000);
			}		// end else if (encrypt)

			break;
		}		// end switch case '1'


		case '2': {		// Decryption routine.

			//		 1234567890123456
			MsgBox ("Decryption      ", 0, 0);
			MsgBox ("Selected        ", 2, 1000);

			if (decrypt()) {	                                            // selection file for decryption
//                       1234567890123456
				MsgBox ("RFID card read", 0, 0);
				MsgBox ("successful.  ", 2, 1000);

				pass_leng = Get_password (password);
				SHA256_hash (password, hash, pass_leng);                    // pass password to Hash function;
				key_b = int((hash[0] << 1) | (hash[1] & 0x80)) + hash[2];   // begining byte of the encrytion key
				key_b = Map_key(encryption_key, hash, key_b);

				Zero_out (password, pwd_size);
				Zero_out (hash,SHA256_BLOCK_SIZE);
				Zero_out (Buf_RF, RF_buf_size);

				SHA256_hash (encryption_key, hash, Encrypt_Size);           // 1st 32 bytes of sponge key
				memcpy (encryption_key, hash, SHA256_BLOCK_SIZE);
				SHA256_hash (hash, hash_2, SHA256_BLOCK_SIZE);              // 2nd 32 bytes of sponge key
				memcpy (encryption_key + SHA256_BLOCK_SIZE, hash_2, SHA256_BLOCK_SIZE);
				Zero_out (hash, SHA256_BLOCK_SIZE);
				Zero_out (hash_2, SHA256_BLOCK_SIZE);

				// read input file to be Decrypted
				if (Select_Input(SD_Decry_CS, SD_Encry_CS, false, encryption_key, BBS_seed)) {
					Zero_out (Buf_in, BA_size);
					Zero_out (Buf_out, BA_size);
					Zero_out (encryption_key, Encrypt_Size);
					//              1234567890123456
					OUTPUT_PRINTLN("Decrypt succcess");
					MsgBox("Decrypt succcess",0, 1000);
				} // end if
				else {
					OUTPUT_PRINTLN("Decrypt failed  ");
					MsgBox("Decrypt failed",0, 3000);
					break;
				} // end else if

			}	// end if (decrypt)
			else {
				Serial.println ("Read RD card for decryption key failed");
				MsgBox ("RFID card read", 0, 0);
				MsgBox ("Failed.  ", 2, 2000);

			}   // end else if (encrypt)
			break;
		}		// end switch case '2''


		case '3':     // RFID data dump
			lcd.clear();
			OUTPUT_PRINTLN("RFID dump        ");
			RFID_dump();
			break;


		case '4': // BBS bits
			lcd.clear();
			OUTPUT_PRINTLN("BBS        ");
			PRNG_BBS(80, BBS_seed);   // number of random bits generated
			break;


		case '5':       //list dir and file on 1st SD card
			if (listfiles(SD_Encry_CS)) {
				OUTPUT_PRINTLN("1st SD Scuss");
			}
			else {
				OUTPUT_PRINTLN("1st SD failed");
			}
			break;


		case '6':       //list dir and file on 2nd SD card
			if (listfiles(SD_Decry_CS)) {
				OUTPUT_PRINTLN("2nd SD Scuss");
			}
			else {
				OUTPUT_PRINTLN("2nd SD failed");
			}
			break;

		default:
			MsgBox(":Invalid option", 2, 2000);
			lcd.clear();
			break;
		} // end switch case

	} // Input loop end-if

} // loop procedure end
