// Crypto module for encrypting and decrypting files

void Encryption (byte Plaintext[], byte sp_key[], const unsigned int Array_size) {
// Plaintext: input/output; info pass into sub and output as ciphertext
// sp_key: input/output; sponge key for current iteration and generate next iteration
// Array_size: input; Plaintext array size

	byte hash_1 [SHA256_BLOCK_SIZE];

	for (unsigned int i = 0; i < Block_size; i++) {		// 1st 16 byte of plain text XOR with 1st 16 byte of sponge key
		Plaintext[i] ^= sp_key[i];
	}

	memcpy (sp_key, Plaintext, Block_size);
	SHA256_hash (sp_key, hash_1, Array_size);
	memcpy (sp_key, hash_1, SHA256_BLOCK_SIZE);
	SHA256_hash (hash_1, hash_1, SHA256_BLOCK_SIZE);				// 2nd 32 bytes of sponge key
	memcpy (sp_key + SHA256_BLOCK_SIZE, hash_1, SHA256_BLOCK_SIZE);

}   // end Encryption subrountine


//=========================================================================
void Decryption (byte CipherText[], byte sp_key[], const unsigned int Array_size) {
// Plaintext: input/output; ciphertext pass into sub and output as plaintext
// sp_key: input/output; sponge key for current iteration and generate next iteration
// Array_size: input; Plaintext array size

	byte hash_1 [SHA256_BLOCK_SIZE];
	byte cTemp [Block_size];

	for (unsigned int i = 0; i < Block_size; i++) {
		cTemp[i] = CipherText[i];
		CipherText[i] ^= sp_key[i];
	}

	memcpy (sp_key, cTemp, Block_size);
	SHA256_hash (sp_key, hash_1, Array_size);
	memcpy (sp_key, hash_1, SHA256_BLOCK_SIZE);
	SHA256_hash (hash_1, hash_1, SHA256_BLOCK_SIZE);               // 2nd 32 bytes of sponge key
	memcpy (sp_key + SHA256_BLOCK_SIZE, hash_1, SHA256_BLOCK_SIZE);

}   // end Decryption subrountine


//=========================================================================
// Generate the random seed from world with a analoy joystick input.
void Entropy_j (byte BBS_seed[]) {   
// BBS_seed: output; pass back, filled from analog entropy from joystick

	int bit_1 = 0;
	int bit_0 = 0;
    int sensorValue_1;     // variable to store the value from the sensor
	int sensorValue_2;     // variable to store the value from the sensor
	int ave_valueX = 0;
	int ave_valueY = 0;
	unsigned int icount = 0;
	uint8_t bytearray[Seed_bit_size];
	int index = 0;
	uint8_t j = 0;
	byte byte_BBS = 0;
	char bit_grph[lcd_cols + 1] = {'\0'};

	bit_grph[0] = '+';
	bit_grph[lcd_cols] = {'\0'};

	for (icount = 0; icount <100; icount += 1) {
		sensorValue_1 = analogRead(X_pin);
		sensorValue_2 = analogRead(Y_pin);
		ave_valueX += sensorValue_1;
		ave_valueY += sensorValue_2;
	}
	ave_valueX = ave_valueX / icount;
	ave_valueY = ave_valueY / icount;
//           123456789012345678
	MsgBox ("Move joystick to", 0, 0);
	MsgBox ("generate entropy",2,0);

	Serial.print ("ave_value ");
	Serial.print (ave_valueX);
	Serial.print ("   ");
	Serial.println (ave_valueY);

	do {
		sensorValue_1 = analogRead(X_pin);
		sensorValue_2 = analogRead(Y_pin);

		if ((abs(sensorValue_1 - ave_valueX) > Entropy_rng) &&
		    (abs(sensorValue_2 - ave_valueY) > Entropy_rng)) {

//           Serial.print (sensorValue_1 - ave_valueX);
//           Serial.print ("  ");
//			Serial.print (abs_val(sensorValue_1 - ave_valueX));
//			Serial.print ("  ");
//			Serial.println (abs_val(sensorValue_1 - ave_valueX)>Entropy_rng);

			sensorValue_1 &= 0x01;
			sensorValue_2 &= 0x01;
			sensorValue_1 ^= sensorValue_2;

			j = index % 8;
			if (sensorValue_1 == 1) {
				bit_1++;
				bytearray[index] = 1;
				byte_BBS += (1 << (7 - j));
			}
			else {
				bit_0++;
				bytearray[index] = 0;
				byte_BBS += (0 << (7 - j));
			}

			if ((index + 1) % 8 == 0) {
				BBS_seed[(int)(index / 8)] = byte_BBS;
				byte_BBS = 0;
			}
			index++;
			bit_grph[(int)(index * (lcd_cols - 1) / Seed_bit_size)] = '+';
			MsgBox (bit_grph,2,0);
//			}

		}   // end if

	} while (index < Seed_bit_size);    // end while

	Serial.print("number of 0: ");
	Serial.print(bit_0, DEC);
	Serial.print("    number of 1: ");
	Serial.println(bit_1, DEC);

	for (index =0; index < Seed_bit_size; index++) {
		Serial.print(bytearray[index], DEC);
	}   // end for
	Serial.println();

}   // end Entropy_j subrountine


//=========================================================================
/* Check this! */
void Zero_out (void * cTemp, const int cTemp_size) {      // Erase array content
// cTemp: input/output; input array to be erased with zeros when returned
// cTemp_size: input; cTemp array size

	memset(cTemp, 0x00, cTemp_size);
}


//=========================================================================
// setup post-padding
void Post_padding (byte bpadding[], const uint8_t iremainder) {
// bpadding: output; return calcualted bpaddiang
// iremainder: input; size of bpadding in bytes

	byte bTemp[Block_size] = {0x80, 0x01,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
	memcpy (bpadding, bTemp, Block_size);

	if (iremainder == 1) {
		bpadding [0] = 0x81;
	}
	else {
		for (uint8_t ict = 1; ict < iremainder - 1; ict++) {
			bpadding [ict] = {0};
		}   // end for loop
	}   // end else if (iremainder)
}
