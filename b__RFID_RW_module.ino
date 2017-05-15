//  RFID IO module


boolean RFID_setup(const uint8_t cFlag_RW) {            // check if RF card is present

	SPI.begin();                          // Init SPI bus
	MFRC522 RF_record(SS_PIN, RST_PIN);   // Create MFRC522 instance.
	MFRC522::MIFARE_Key key;
	RF_record.PCD_Init();                 // Init MFRC522 card

	// using FFFFFFFFFFFFh which is the default at chip delivery from the factory
	memset(key.keyByte, 0xFF, 6);

	// Look for new cards, and select one if present
	if ( ! RF_record.PICC_IsNewCardPresent() || ! RF_record.PICC_ReadCardSerial() ) {
		RF_record.PICC_HaltA();         // Halt PICC
		RF_record.PCD_StopCrypto1();    // Stop encryption on PCD
		SPI.end();
		digitalWrite (SS_PIN, HIGH);
		return false;
	} // if end
	else {
		switch (cFlag_RW) {

		case 1:     // Read RFID card content
			RFID_Read (RF_record, key);
			break;

		case 2:     // Wrtie to RFID card
			RFID_Write (RF_record, key);
			break;

		default:
			MsgBox("RFID init done. ", 0, 50);
			break;
		}

		SPI.end();
		digitalWrite (SS_PIN, HIGH);
		return true;
	}  // else end

} // RFID_setup function end


//==============================================================================
// RFID Read procedure. Read from card all blocks one at a time.
void RFID_Read( MFRC522 mfrc522, MFRC522::MIFARE_Key key) {

	byte blockAddr      = 0;
	byte trailerBlock   = 0;

	MFRC522::StatusCode b_status;
	byte Buf_temp[18];                  // buffer needs to be 18 elements
	byte b_size = sizeof(Buf_temp);
	unsigned int ict  = 0;

	for (uint8_t isector = 0; isector <= 15; isector++) {

		blockAddr = isector * 4;
		trailerBlock = blockAddr + 3;

		// Authenticate using key A
		b_status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));

		if (b_status != MFRC522::STATUS_OK) {
			OUTPUT_PRINT ("Auth'cte fail: ");
			Serial.println (mfrc522.GetStatusCodeName(b_status));
//			return false;
		}
		else {
			// Read data from the block
			if (blockAddr == 0) {
				blockAddr++;
			}
			for (uint8_t iblock = blockAddr; iblock < trailerBlock; iblock++) {

				b_status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(iblock, Buf_temp, &b_size);
				if (b_status != MFRC522::STATUS_OK) {
					Serial.print(F("MIFARE_Read() failed: "));
					Serial.println(mfrc522.GetStatusCodeName(b_status));
				}
				else {
					memcpy (Buf_RF + ict, Buf_temp, 16);
					ict += 16;
				}
			} //end for block
		} //	end else if
	} // end for sector

	mfrc522.PICC_HaltA();         // Halt PICC
	mfrc522.PCD_StopCrypto1();    // Stop encryption on PCD
} // end RFID_Read procedure


//==============================================================================
// RFID Write procedure. Write to card all blocks one at a time.
void RFID_Write( MFRC522 mfrc522, MFRC522::MIFARE_Key key ) {

	byte blockAddr      = 0;
	byte trailerBlock   = 0;

	MFRC522::StatusCode b_status;
	byte Buf_temp[18];                  // buffer needs to be 18 elements
//	byte b_size = sizeof(Buf_temp);
	unsigned int ict  = 0;

	for (uint8_t isector = 0; isector <= 15; isector += 1) {

		blockAddr = isector * 4;
		trailerBlock = blockAddr + 3;

		// Authenticate using key A
		b_status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));

		if (b_status != MFRC522::STATUS_OK) {
			OUTPUT_PRINT ("Auth'cte fail: ");
			Serial.println (mfrc522.GetStatusCodeName(b_status));
//			return false;
		}
		else {
			// Write data from the block
			if (blockAddr == 0) {
				blockAddr++;
			}
			for (uint8_t iblock = blockAddr; iblock < trailerBlock; iblock++) {
				memcpy (Buf_temp, Buf_RF + ict, 16);
				b_status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(iblock, Buf_temp, 16);
				ict += 16;

				if (b_status != MFRC522::STATUS_OK) {
					Serial.print(F("MIFARE_Read() failed: "));
					Serial.println(mfrc522.GetStatusCodeName(b_status));
				}
			} //end for block
		}
	} // end for sector

	mfrc522.PICC_HaltA();         // Halt PICC
	mfrc522.PCD_StopCrypto1();    // Stop encryption on PCD
} // end RFID_Write procedure

/*
  // Check that data in block is what we have written by counting the number of bytes that are equal
  Serial.println(F("Checking result..."));
  byte count = 0;
  for (byte i = 0; i < 16; i++) {
    // Compare Buf_temp (= what we've read) with dataBlock (= what we've written)
    if (Buf_temp[i] == dataBlock[i])
      count++;
  }
  Serial.print(F("Number of bytes that match = ")); Serial.println(count);
  if (count == 16) {
    Serial.println(F("Success :-)"));
  } else {
    Serial.println(F("Failure, no match :-("));
    //    Serial.println(F("  perhaps the write didn't work properly..."));
  }
  Serial.println();
*/
