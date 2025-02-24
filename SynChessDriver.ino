/**
 * SynChessDriver is the programm to control the electrical chess board
 * built for the diploma thesis SynChess - Synchronized Chess. It acts as
 * a serial-connection slave, which accepts commands on the serial line and
 * controls the stepper motors and the sensors.
 *
 * @author Luca Marth (luca.marth@outlook.com)
 */
#include <MFRC522.h>
#include <SPI.h>

// ********** PINs **********
#define M1_DIR 8
#define M1_STP 9
#define M2_DIR 10
#define M2_STP 11

#define LS_HOME_X 23
#define LS_HOME_Y1 24
#define LS_HOME_Y2 25

#define CHIP_SELECT 53
#define RST_BUS 5

#define PIN_MAGNET 23
// **************************


// ********** Motor Settings **********
#define DEFAULT_STP_TIME 100
#define MICROSTEPS 8

#define LEFT 0
#define RIGHT 1
#define UP 2
#define DOWN 3

#define FULL_FIELD_STEP 300
// ************************************


// ********** Sensor Settings **********
#define BLOCK 4
// *************************************

// ********** Commands **********
#define DRIVER_CMD_READ 'R'

#define READ_ZONE_OUT_WHITE '1'
#define READ_ZONE_CENTER '2'
#define READ_ZONE_OUT_BLACK '3'

#define DRIVER_CMD_AVAILABLE 'A'
#define DRIVER_CMD_STP 'S'
#define DRIVER_CMD_HOME 'H'
#define DRIVER_CMD_MAGNET 'M'

#define DRIVER_STATUS_OK 'X'
// ******************************


// ********** Fields **********
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
// ****************************


// ********** Variables **********
String readVal;
bool doneCommandLoop = false;

MFRC522 chessReader(CHIP_SELECT, RST_BUS);
// *******************************


// ********** Method headers **********
char mapHexToColorValue(char, char, byte);
char convertByteInChess(byte*);
String reverse(String);

void step(int, int);
void setDirection(int);
void oneStep(int);
void home();

void setInitialReaderHeadPositionCenter();
void setInitialReaderHeadPositionOutWhite();
void setInitialReaderHeadPositionOutBlack();
void startReadingProcess(String, int, int);
char readRFID();

void switchMagnet(int);
// ************************************


// ********** Arduino Methods **********
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

    // setup motor pins
    pinMode(M1_DIR, OUTPUT);
    pinMode(M2_DIR, OUTPUT);
    pinMode(M1_STP, OUTPUT);
    pinMode(M2_STP, OUTPUT);

    digitalWrite(M1_DIR, LOW);
    digitalWrite(M1_STP, LOW);
    digitalWrite(M2_DIR, LOW);
    digitalWrite(M2_STP, LOW);

    // setup homing pins
    pinMode(LS_HOME_X, INPUT);
    pinMode(LS_HOME_Y1, INPUT);
    pinMode(LS_HOME_Y2, INPUT);

    // setup magnet pins
    pinMode(PIN_MAGNET, OUTPUT);
    digitalWrite(PIN_MAGNET, LOW);
}

/**
 * loop(): Runs every 10ms to control the program flow.
 */
void loop() {
  if(Serial.available() > 0) {
    // format is command;command;
    readVal = Serial.readStringUntil(';');

    // if command is read
    if(readVal.charAt(0) == DRIVER_CMD_READ) {
      char zone = readVal.charAt(1);
      int width = 0;

      // set initial reading positions
      if(zone == READ_ZONE_OUT_WHITE) {
        width = 2;
        setInitialReaderHeadPositionOutWhite();
      } 
      else if(zone == READ_ZONE_CENTER) {
        width = 7;
        setInitialReaderHeadPositionCenter();
      }
      else if(zone == READ_ZONE_OUT_BLACK) {
        width = 2;
        setInitialReaderHeadPositionOutBlack();
      }
      
      // read and write
      Serial.println(startReadingProcess(width, 2));
    }

    // if command is step
    else if(readVal.charAt(0) == DRIVER_CMD_STP) {
      int steps = atoi(readVal.substring(2, readVal.length()).c_str());
      int direction;

      switch (readVal.charAt(1)) {
        case 'L':
        case 'l':
          direction = 0;
          break;
        case 'R': // r
        case 'r':
          direction = 1;
          break;
        case 'U': // u
        case 'u':
          direction = 2;
          break;
        case 'D': // d
        case 'd':
          direction = 3;
          break;
      }

      step(direction, steps);
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
// *************************************


// ********** Utility Methods **********
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
 * reverse(): Reverses a given string
 * @param x string to be reversed
 * @return reversed string
 */
String reverse(String x) {
  String reversed = "";

  for (int i = x.length() - 1; i >= 0; i--) {
    reversed += x[i];
  }

  return reversed;
}
// *************************************


// ********** Stepper Commands **********
/**
 * step(): Moves the Core-XY mechanism by the given steps in the given direction.
 * @param direction in which direction should the head move
 * @param steps how many steps the head should move
 */
void step(int direction, int steps) {
  // set the direction
  setDirection(direction);

  // step without accelaration
  for(int i = 0; i < steps; i++) {
    oneStep(DEFAULT_STP_TIME);
  }

  // reset pins
  digitalWrite(M1_DIR, LOW);
  digitalWrite(M1_STP, LOW);
  digitalWrite(M2_DIR, LOW);
  digitalWrite(M2_STP, LOW);
}

/**
 * setDirection(): Alternates the direction pins based on parameter.
 * @param direction which direction should be activated
 */
void setDirection(int direction) {
  switch (direction) {
    case LEFT:
      digitalWrite(M1_DIR, LOW);
      digitalWrite(M2_DIR, LOW);
      break;
    case RIGHT:
      digitalWrite(M1_DIR, HIGH);
      digitalWrite(M2_DIR, HIGH);
      break;
    case UP:
      digitalWrite(M1_DIR, LOW);
      digitalWrite(M2_DIR, HIGH);
      break;
    case DOWN: 
      digitalWrite(M1_DIR, HIGH);
      digitalWrite(M2_DIR, LOW);
      break;
  }
}

/**
 * oneStep(): Sends one Pulse to the STP - Pin with the on-time of
 * <speed>
 * @param speed time of the rectangular signal
*/
void oneStep(int speed) {
  for(int i = 0; i < MICROSTEPS; i++) {
    digitalWrite(M1_STP, HIGH);
    digitalWrite(M2_STP, HIGH);
    delayMicroseconds(speed); // Delay based on current speed
    digitalWrite(M1_STP, LOW);
    digitalWrite(M2_STP, LOW);
    delayMicroseconds(speed); // Delay based on current speed
  }
}

/**
 * home(): Moves the head to the position (0 0)
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
// **************************************


// ********** Sensor Commands **********
/**
 * setInitialReaderHeadPositionCenter(): Homes the head and moves it to the first field.
*/
void setInitialReaderHeadPositionCenter() {
  //home();
  // step down 250
  step(DOWN, 250);

  // step right 1345
  step(RIGHT, 1345);

  delay(100);
}

/**
 * setInitialReaderHeadPositionOutWhite(): Homes the head and moves it to the first field.
*/
void setInitialReaderHeadPositionOutWhite() {
  //home();
}

/**
 * setInitialReaderHeadPositionOutBlack(): Homes the head and moves it to the first field.
*/
void setInitialReaderHeadPositionOutBlack() {
  //home();
}

/**
 * startReadingProcess(): Alternates the direction pins based on parameter.
 * @param width width of the field to be scanned
 * @param height height of the field to be scanned
 * @return scanned board
 */
String startReadingProcess(int width, int height) {
  String out = "";
  String currentReadRow;

  int direction = RIGHT;

  for(int i = 0; i < height; i++) {
    currentReadRow = "";

    for(int j = 0; j < width; j++) {
      currentReadRow += readRFID();
      step(direction, FULL_FIELD_STEP);
    }

    // read the last one
    currentReadRow += readRFID();

    // step down
    step(3, FULL_FIELD_STEP);
    
    // switch direction
    if(direction == RIGHT) {
      direction = LEFT;
    } else {
      currentReadRow = reverse(currentReadRow);
      direction = RIGHT;
    }

    // append string
    out += currentReadRow;
  }

  
  return out;
}

/**
 * readRFID(): Reads the given RFID - Sensor and returns the converted contents
 * from block 4 
 * @param mfrc522 Sensor instance
 * @return char code
 */
char readRFID() {
    // Look for a card
    if (chessReader.PICC_IsNewCardPresent() && chessReader.PICC_ReadCardSerial()) {

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
// *************************************


// ********** Magnet Commands **********
/**
 * switchMagnet(): Switches the magnet on or off based on the parameter
 * @param state which state should the magnet be put in
 */
void switchMagnet(int state) {
  if(state == 1) {
    digitalWrite(PIN_MAGNET, HIGH);
  } else if(state == 0){
    digitalWrite(PIN_MAGNET, LOW);
  }
}
// *************************************