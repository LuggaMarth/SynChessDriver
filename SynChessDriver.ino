/*
* ---------------------------------------
* Wiring of the Scanner Bus to SCArduino
* ---------------------------------------
* pcf -> 5V and GND
* rc522 -> 3,3V and GND
* RST -> Pin 9
* MOSI -> Pin 11
* MISO -> Pin 12
* SCK -> Pin 13
* 
* X: 4325
*
*/

#include "Arduino.h"
#include <SPI.h>
#include <MFRC522.h>

// NFC reader stuff
#define BLOCK 4

// chess figure defines
#define WHITE_BAUER_1 '1'
#define WHITE_BAUER_2 '2'
#define WHITE_BAUER_3 '3'
#define WHITE_BAUER_4 '4'
#define WHITE_BAUER_5 '5'
#define WHITE_BAUER_6 '6'
#define WHITE_BAUER_7 '7'
#define WHITE_BAUER_8 '8'
#define WHITE_TURM_L '9'
#define WHITE_TURM_R 'A'
#define WHITE_SPRINGER_L 'B'
#define WHITE_SPRINGER_R 'C'
#define WHITE_LAEUFER_L 'D'
#define WHITE_LAEUFER_R 'E'
#define WHITE_KOENIGIN 'F'
#define WHITE_KOENIG 'G'

#define BLACK_BAUER_1 'H'
#define BLACK_BAUER_2 'I'
#define BLACK_BAUER_3 'J'
#define BLACK_BAUER_4 'K'
#define BLACK_BAUER_5 'L'
#define BLACK_BAUER_6 'M'
#define BLACK_BAUER_7 'N'
#define BLACK_BAUER_8 'O'
#define BLACK_TURM_L 'P'
#define BLACK_TURM_R 'Q'
#define BLACK_SPRINGER_L 'R'
#define BLACK_SPRINGER_R 'S'
#define BLACK_LAEUFER_L 'T'
#define BLACK_LAEUFER_R 'U'
#define BLACK_KOENIGIN 'V'
#define BLACK_KOENIG 'W'

#define EMPTY_FIELD '0'

#define DRIVER_CMD_READ_CENTER 'B'
#define DRIVER_CMD_READ_OUT_WHITE 'W'
#define DRIVER_CMD_READ_OUT_BLACK 'O'
#define DRIVER_CMD_READ 'R'
#define DRIVER_CMD_AVAILABLE 'A'
#define DRIVER_CMD_STP 'S'
#define DRIVER_CMD_HOME 'H'
#define DRIVER_STATUS_OK 'X'
#define DRIVER_CMD_MAGNET 'M'

// ****** Pin mappings ******
#define M1_DIR 2
#define M1_STP 3
#define M2_DIR 4
#define M2_STP 5

#define LS_HOME_X 6
#define LS_HOME_Y1 7
#define LS_HOME_Y2 8

#define CHIP_SELECT 10
#define RST_BUS 9

#define PIN_MAGNET 22

// ****** Motor Settings ******
#define DEFAULT_STP_TIME 100
#define MICROSTEPS 8

MFRC522 chessReader(RST_BUS, CHIP_SELECT);
bool doneCommandLoop = false;

/**
 * mapHexToColorValue(): Takes two color inputs and outputs the corresponding value to the color hex.
 * @param whiteValue White value to be outputted
 * @param blackValue Black value to be outputted
 * @param colorHex HEX - Color Code
 * @return Corresponding value
 */
char mapHexToColorValue(char whiteValue, char blackValue, byte colorHex) {
    if(colorHex == 0xA0) {
        return whiteValue;
    } else if(colorHex == 0xB0) {
        return blackValue;
    } else {
        return EMPTY_FIELD;
    }
}

/**
 * readRFID(): Reads the given RFID - Sensor and returns the converted contents
 * from block 4 
 * @param mfrc522 Sensor instance
 * @return char code
 */
char readRFID() {
    // Look for a card
    if (chessReader.PICC_IsNewCardPresent()) {
        // Authentificate for block with key A (default key)
        MFRC522::MIFARE_Key key;
        for (byte i = 0; i < 6; i++) {
            key.keyByte[i] = 0xFF;
        }

        // buffer to read the data
        byte buffer[18];
        byte size = sizeof(buffer);

        // Authenticate the block 4 (if not ok, return)
        MFRC522::StatusCode status = chessReader.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, BLOCK, &key, &(chessReader.uid));
        if (status != MFRC522::STATUS_OK) {
            return EMPTY_FIELD;
        }
        // Read data from block 4
        status = chessReader.MIFARE_Read(BLOCK, buffer, &size);
        if (status != MFRC522::STATUS_OK) {
            return EMPTY_FIELD;
        }

        // Stop encryption on the PICC
        chessReader.PICC_HaltA();
        chessReader.PCD_StopCrypto1();

        // return
        return convertByteInChess(buffer);
    } else {
        return EMPTY_FIELD;
    }
}

/**
 * convertByteInChess(): Converts the given 4 Bytes to the equal chess figure
 * @param buffer Bytecode
 * @return char code from converted figure
 */
char convertByteInChess(byte buffer[]) {
    // checksum?
    if(buffer[3] != 0xEF) {
        return EMPTY_FIELD;
    }

    // bauern
    if(buffer[1] == 0x01) {
        switch(buffer[2]) {
            case 0x01:
                return mapHexToColorValue(WHITE_BAUER_1, BLACK_BAUER_1, buffer[0]);
                break;
            case 0x02:
                return mapHexToColorValue(WHITE_BAUER_2, BLACK_BAUER_2, buffer[0]);
                break;
            case 0x03:
                return mapHexToColorValue(WHITE_BAUER_3, BLACK_BAUER_3, buffer[0]);
                break;
            case 0x04:
                return mapHexToColorValue(WHITE_BAUER_4, BLACK_BAUER_4, buffer[0]);
                break;
            case 0x05:
                return mapHexToColorValue(WHITE_BAUER_5, BLACK_BAUER_5, buffer[0]);
                break;
            case 0x06:
                return mapHexToColorValue(WHITE_BAUER_6, BLACK_BAUER_6, buffer[0]);
                break;
            case 0x07:
                return mapHexToColorValue(WHITE_BAUER_7, BLACK_BAUER_7, buffer[0]);
                break;
            case 0x08:
                return mapHexToColorValue(WHITE_BAUER_8, BLACK_BAUER_8, buffer[0]);
                break;
        }
    }

    // turm
    if(buffer[1] == 0x02) {
        if(buffer[2] == 0x01) return mapHexToColorValue(WHITE_TURM_L, BLACK_TURM_L, buffer[0]);
        else return mapHexToColorValue(WHITE_TURM_R, BLACK_TURM_R, buffer[0]);;
    }

    // springer
    if(buffer[1] == 0x03) {
        if(buffer[2] == 0x01) return mapHexToColorValue(WHITE_SPRINGER_L, BLACK_SPRINGER_L, buffer[0]);
        else return mapHexToColorValue(WHITE_SPRINGER_R, BLACK_SPRINGER_R, buffer[0]);
    }

    // laufer
    if(buffer[1] == 0x04) {
        if(buffer[2] == 0x01) return mapHexToColorValue(WHITE_LAEUFER_L, BLACK_LAEUFER_L, buffer[0]);
        else return mapHexToColorValue(WHITE_LAEUFER_R, BLACK_LAEUFER_R, buffer[0]);
    }

    // queen
    if(buffer[1] == 0x05) return mapHexToColorValue(WHITE_KOENIGIN, BLACK_KOENIGIN, buffer[0]);

    // king
    if(buffer[1] == 0x06) return mapHexToColorValue(WHITE_KOENIG, BLACK_KOENIG, buffer[0]);
}

/**
 * stepCommand(): Executes the given step command
 * @param command Command to execute
*/ 
void stepCommand(String command) {
  char direction = command.charAt(0);
  int steps = atoi(command.substring(1, command.length()).c_str());

  // direction switch
  switch (direction) {
    case 'L':
      digitalWrite(M1_DIR, LOW);
      digitalWrite(M2_DIR, LOW);
      break;
    case 'R':
      digitalWrite(M1_DIR, HIGH);
      digitalWrite(M2_DIR, HIGH);
      break;
    case 'U':
      digitalWrite(M1_DIR, LOW);
      digitalWrite(M2_DIR, HIGH);
      break;
    case 'D':
      digitalWrite(M1_DIR, HIGH);
      digitalWrite(M2_DIR, LOW);
      break;
  }

  int share = steps/9;
  int perStep = 400/share;
  int curr = DEFAULT_STP_TIME;

  // step normal until the last nineth
  for(int i = 0; i < steps-share; i++) {
    for(int j = 0; j < MICROSTEPS; j++) {
      oneStep(DEFAULT_STP_TIME);
    }
  }
  
  // deccelerate
  for(int i = steps - share; i < steps; i++) {
    curr += perStep;

    for(int j = 0; j < MICROSTEPS; j++) {
      oneStep(curr);
    }
  }

  digitalWrite(M1_DIR, LOW);
  digitalWrite(M1_STP, LOW);
  digitalWrite(M2_DIR, LOW);
  digitalWrite(M2_STP, LOW);
}

/**
 * oneStep(): Sends one Pulse to the STP - Pin with the on-time of
 * <speed>
 * @param speed time of the rectangular signal
*/
void oneStep(int speed) {
  digitalWrite(M1_STP, HIGH);
  digitalWrite(M2_STP, HIGH);
  delayMicroseconds(speed);
  digitalWrite(M1_STP, LOW);
  digitalWrite(M2_STP, LOW);
  delayMicroseconds(speed);
}

/**
 * home(): Moves the Core - XY to (0 0)
*/
void home() {
  // move x
  digitalWrite(M1_DIR, LOW);
  digitalWrite(M2_DIR, LOW);

  while(digitalRead(LS_HOME_X) != HIGH) {
    oneStep(DEFAULT_STP_TIME);
  }

  // move y
  digitalWrite(M1_DIR, LOW);
  digitalWrite(M2_DIR, HIGH);

  while(digitalRead(LS_HOME_Y1) != HIGH && digitalRead(LS_HOME_Y2) != HIGH) {
    oneStep(DEFAULT_STP_TIME);
  }
}

/**
 * switchMagnet(): Switches the magnet on and off
*/
void switchMagnet(int state) {
  if(state == 1) {
    digitalWrite(PIN_MAGNET, HIGH);
  } else if(state == 0){
    digitalWrite(PIN_MAGNET, LOW);
  }
}

/**
 * setup(): Setup runs once to set up the components
 */
void setup() {
    // open serial and wait til opened
    Serial.begin(9600);
    while(!Serial) {  }

    // init spi
    SPI.begin();

    // initialize scanner
    chessReader.PCD_Init();

    // // setup motor pins
    pinMode(M1_DIR, OUTPUT);
    pinMode(M2_DIR, OUTPUT);
    pinMode(M1_STP, OUTPUT);
    pinMode(M2_STP, OUTPUT);

    pinMode(LS_HOME_X, INPUT);
    pinMode(LS_HOME_Y1, INPUT);
    pinMode(LS_HOME_Y2, INPUT);

    pinMode(PIN_MAGNET, OUTPUT);
    digitalWrite(PIN_MAGNET, LOW);

    digitalWrite(M1_DIR, LOW);
    digitalWrite(M1_STP, LOW);
    digitalWrite(M2_DIR, LOW);
    digitalWrite(M2_STP, LOW);
}

/**
 * loop(): Runs every 10ms to control the program flow.
 */
void loop() {
    String readVal;

    if(Serial.available() > 0) {
      // format is command;command;
      readVal = Serial.readStringUntil(';');

      // if command is read all
      if(readVal.charAt(0) == DRIVER_CMD_READ_CENTER) {
        char scan[128];

        Serial.println(scan);
      }

      // if command is step
      else if(readVal.charAt(0) == DRIVER_CMD_STP) {
        stepCommand(readVal.substring(1,readVal.length()));
        doneCommandLoop = true;
      }

      // if command is home
      else if(readVal.charAt(0) == DRIVER_CMD_HOME) {
        home();
        doneCommandLoop = true;
      }

      // if command is are you still there
      else if(readVal.charAt(0) == 'A') {
        doneCommandLoop = true;
      }

      else if(readVal.charAt(0) == DRIVER_CMD_MAGNET) {
        switchMagnet(atoi(readVal.substring(1,readVal.length()).c_str()));
      }
    } 

    else if(doneCommandLoop) {
      doneCommandLoop = false;
      Serial.println(DRIVER_STATUS_OK);
    }

    // sleep 10ms
    delay(10);
}
