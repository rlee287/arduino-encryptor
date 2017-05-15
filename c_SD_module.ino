// set up variables using the SD utility library functions:
/*
  SD card read/write
This module is based on code example released by Igoe and Fitzgerald. 

 created   Nov 2010
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe

 This example code is in the public domain.

 */
 
// SD_setup init and check for SD cards procedure
boolean SD_setup (const int CS_pin) {

	//      1234567890123456
	MsgBox("Init'ing SD     ", 0, 50);
	SPI.begin();
	if (!SD.begin(CS_pin)) {
		MsgBox("SD init Failed!", 2, 50);
		SPI.end();
		digitalWrite (CS_pin, HIGH);
		return false;
	}
	else {
		MsgBox("Init Done.", 0, 50);
		SPI.end();
		digitalWrite (CS_pin, HIGH);
		return true;
	}
} //SD_setup end


//==========================================================================================
boolean SD_Getfile(const int CS_pin) {      // Select/get the file name to be processed
// CS_pin: input; pin number of the attach SD card reader

	File SD_root;
	boolean bFlag = false;

	SPI.begin();
	if (SD.begin(CS_pin)) {

		SD_root = SD.open("/");

		if (SD_root) {
			if (SD_selectfile (SD_root)) {
				OUTPUT_PRINTLN("select done! ");
				bFlag = true;
			}
			else {
				bFlag = false;
				OUTPUT_PRINTLN("select failed");
			}
		}
		else {
			bFlag = false;
			OUTPUT_PRINTLN("select failed");
		}

		SD_root.close();
		SPI.end();
		digitalWrite (CS_pin, HIGH);

	}
	else {
		OUTPUT_PRINTLN("Init Failed.    ");
		SPI.end();
		digitalWrite (CS_pin, HIGH);
		bFlag = false;
	}

	return bFlag;

} // SD_Readfile end


//==========================================================================================
boolean SD_Writefile(byte * bData, const int CS_pin, const unsigned int Write_size) {
// bData: Output; data pass into subrountine to be written to SD card.
// CS_pin: Input; Chip Select control pin number for SD card.
// Read_size: Input; number of bytes to be written.

	File SD_Wfile;

	SPI.begin();
	if (SD.begin(CS_pin)) {

		SD_Wfile = SD.open(OutputFile_Name, FILE_WRITE);

        if (SD_Wfile && SD_Wfile.seek(OutputFile_Pos)) {
			SD_Wfile.write(bData, Write_size);

			OutputFile_Pos += Write_size;
			SD_Wfile.close();                          // close the file:
		}
		else {
			//abcdefg1234567890123456
			OUTPUT_PRINTLN("error open file!");
		}

		SD_Wfile.close();
		SPI.end();
		digitalWrite (CS_pin, HIGH);
		return true;
	}
	else {
		OUTPUT_PRINTLN("Init Failed.    ");
		SPI.end();
		digitalWrite (CS_pin, HIGH);
		return false;
	}

} // SD_Writefile end


//==========================================================================================
//SD_Input_Data: list SD card file directory, allow user to choose file to process
boolean SD_selectfile(File dir) {

	char customKey;
	char filetype[5];

	while (true) {

		File entry =  dir.openNextFile();
		if (!entry) {
			entry.close();
			return false;
			break;
		}

		if (entry.isDirectory()) {
			SD_selectfile(entry);
		}
		else {
			lcd.clear();
			OUTPUT_PRINTLN(entry.name());
			lcd.setCursor(0, 1);
			//abcdefg1234567890123456
			lcd.print("*: Select File  ");
		} // if-else end

		customKey = 0;

		//Code for alternating display message.
		uint8_t change_counter = 0;

		while (!customKey) {
			unsigned long start = millis();
			if (change_counter == 1) {
				lcd.setCursor(0, 1);
				//abcdefg1234567890123456
				lcd.print(F("Other Key: Skip "));
				change_counter = 0;
			}
			else {
				lcd.setCursor(0, 1);
				//abcdefg1234567890123456
				lcd.print("*: Open File    ");
				change_counter = 1;
			}
			unsigned long elapsed = millis();
			do {
				customKey = customKeypad.getKey();
				if (customKey) {   
					break;
				}
				elapsed = millis();
			} while ((elapsed - start) < 1500);
		}

		if (customKey == '*') {   // select file to open

			InputFile_Name[0] = {'\0'};
			OutputFile_Name[0] = {'\0'};
			InputFile_Pos = 0;
			InputFile_Size = 0;

			strcpy(InputFile_Name, entry.name());
			for (uint8_t i = 0; i < sizeof(InputFile_Name); i++) {  // copy inputfilename into output file name
				if (InputFile_Name[i] != '.') {
					OutputFile_Name[i] = InputFile_Name[i];
				} // end if
				else {  // copy file type
					OutputFile_Name[i] = {'\0'};
					filetype [0] = {'.'};
					filetype [1] = InputFile_Name[i + 1];
					filetype [2] = InputFile_Name[i + 2];
					filetype [3] = InputFile_Name[i + 3];
					filetype [4] = {'\0'};
					break;
				} // end else
			} // end for

			if (strcmp (filetype, ".ENC") == 0) {
				strncat (OutputFile_Name, ".DEC", 4);
                SD.remove (OutputFile_Name);
			}
			else {
				strncat (OutputFile_Name, ".ENC", 4);
			} // end if-else

			InputFile_Size = entry.size();
			entry.close();
			break;

		} // end if customkey

	} // while (true) end

	return true;

} //SD_Input_Data (SD_selectfile) procedure end


//==========================================================================================
unsigned int SD_Readfile (byte * bData, const int CS_pin, const unsigned int Read_size) {
// bData: Output; data read from SD card to be pass back.
// CS_pin: Input; Chip Select control pin number for SD card.
// Read_size: Input; number of bytes to be read.

	File InputFile;
	unsigned int iLength;

	if (SD.begin(CS_pin)) {
		InputFile = SD.open(InputFile_Name, FILE_READ);

		if (InputFile && InputFile.seek(InputFile_Pos)) {
			iLength = InputFile.readBytes(bData, Read_size);        
            InputFile_Pos += Read_size;
           
			InputFile.close();
			SPI.end();
			digitalWrite (CS_pin, HIGH);
			return iLength;
		} // end if
		else {
			return 0;
		}

	}   // end if
	else {
		return 0;
	}

} // SD_Readfile procedure end



