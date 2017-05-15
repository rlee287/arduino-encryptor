/*
  Listfiles
This module is based on code example released by Igoe and Fitzgerald. 

 created   Nov 2010
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe
 modified 2 Feb 2014
 by Scott Fitzgerald

 This example code is in the public domain.

 */
boolean listfiles(uint8_t chipSelect) {
	File root;

	Serial.print("Initializing SD card...");

	if (!SD.begin(chipSelect)) {
		Serial.println("initialization failed!");
		return false;
	}
	Serial.println("initialization done.");

	root = SD.open("/");
    List_dir(root,0);
    root.close();

    root = SD.open("/");
    Serial.println();
	printDirectory(root, 0);
    root.close();
	Serial.println("done!");
    return true;
}

void List_dir(File dir, int numTabs) {
    while (true) {

        File entry =  dir.openNextFile();
        if (! entry) {
            // no more files
            break;
        }
        for (uint8_t i = 0; i < numTabs; i++) {
            Serial.print('\t');
        }
        Serial.print(entry.name());

        if (entry.isDirectory()) {
            Serial.println("/");
            List_dir(entry, numTabs + 1);
        } else {
            // files have sizes, directories do not
            Serial.print("\t\t");
            Serial.println(entry.size(), DEC);
            lcd.println(entry.name());
        }
        entry.close();
    }
}

void printDirectory(File dir, int numTabs) {
	while (true) {

		File entry =  dir.openNextFile();
		if (! entry) {
			// no more files
			break;
		}
		for (uint8_t i = 0; i < numTabs; i++) {
			Serial.print('\t');
		}
		Serial.println(entry.name());
     dumpfile (entry.name());
		if (entry.isDirectory()) {
			Serial.println("/");
			printDirectory(entry, numTabs + 1);
		} else {

			lcd.print(entry.name());
		}
		entry.close();
	}
}

void dumpfile (const String filename) {
  File myFile;

  // re-open the file for reading:
  myFile = SD.open(filename);
  if (myFile) {
    Serial.println(filename);

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close();     // close the file:
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }

}
