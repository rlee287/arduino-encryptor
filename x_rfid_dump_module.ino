// This module is for debugging only.

//#pragma GCC optimize ("-O2")
//#pragma GCC optimize ("-finline-functions")

// Read RFID card infomation
boolean RFID_dump(void) {

	// initialize RFID reader
	MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
	MFRC522::MIFARE_Key key;

	SPI.begin();         // Init SPI bus
	mfrc522.PCD_Init();  // Init MFRC522 card
	// Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
	for (byte i = 0; i < 6; i++) {
		key.keyByte[i] = 0xFF;
	}

	// Look for new cards, and select one if present
	if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial() ) {
		//   delay_time(50);
		return false;
	}

	// Now a card is selected. The UID and SAK is in mfrc522.uid.

	// Dump UID
	Serial.print(F("Card UID:"));
	for (byte i = 0; i < mfrc522.uid.size; i++) {
		Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
		Serial.print(mfrc522.uid.uidByte[i], HEX);
	}
	Serial.println();


	// Set new UID
	byte newUid[] = NEW_UID;
	if ( mfrc522.MIFARE_SetUid(newUid, (byte)4, true) ) {
		Serial.println(F("Wrote new UID to card."));
	}


	// Halt PICC and re-select it so DumpToSerial doesn't get confused
	mfrc522.PICC_HaltA();
	if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial() ) {
		return false;
	}

	// Dump the new memory contents
	Serial.println(F("New UID and contents:"));
	mfrc522.PICC_DumpToSerial(&(mfrc522.uid));

	// delay_time(2000);
	SPI.end();
	digitalWrite (SS_PIN, HIGH);
	return true;
}
