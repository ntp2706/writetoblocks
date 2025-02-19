#include <SPI.h>
#include <MFRC522.h>

#define SSID "NTP"
#define PASSWORD "qwert123"

#define SS_PIN 15 // D8
#define RST_PIN 2 // D4
#define BUZZER 5 // D1

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

bool addMode = true;

unsigned long buzzerStartTime = 0;
unsigned long buzzerTime = 0;
bool buzzerActive = false;

int blockNum; 
byte blockData[16];
byte bufferLen = 18;
byte readBlockData[18];

String numberReceive;
String ownerReceive;
String timestampReceive;
String expirationDateReceive;

void writeDataToBlock(int blockNum, byte blockData[]);
void writeStringToBlocks(int startBlock, String data);
void readDataFromBlock(int blockNum, byte readBlockData[]);
String readStringFromBlocks(int startBlock, int blockCount);

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);

  SPI.begin();            
  mfrc522.PCD_Init();     
  delay(4);               
  mfrc522.PCD_DumpVersionToSerial();  

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println("Nhập dữ liệu lần lượt:");
  Serial.println("1. Chủ sở hữu (ownerReceive): ");
  Serial.println("2. Biển số xe (numberReceive): ");
  Serial.println("3. Ngày hết hạn (expirationDateReceive): ");
  Serial.println("4. Tên ảnh/timestamp (timestampReceive): ");
}

void loop() {
  static int inputStep = 0;

  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim(); // Loại bỏ ký tự xuống dòng hoặc khoảng trắng dư thừa.

    switch (inputStep) {
      case 0:
        ownerReceive = input;
        Serial.println("Chủ sở hữu đã nhập: " + ownerReceive);
        Serial.println("Nhập biển số xe (numberReceive): ");
        inputStep++;
        break;

      case 1:
        numberReceive = input;
        Serial.println("Biển số xe đã nhập: " + numberReceive);
        Serial.println("Nhập ngày hết hạn (expirationDateReceive): ");
        inputStep++;
        break;

      case 2:
        expirationDateReceive = input;
        Serial.println("Ngày hết hạn đã nhập: " + expirationDateReceive);
        Serial.println("Nhập tên ảnh/timestamp (timestampReceive): ");
        inputStep++;
        break;

      case 3:
        timestampReceive = input;
        Serial.println("Tên ảnh đã nhập: " + timestampReceive);
        Serial.println("Dữ liệu đã sẵn sàng, quẹt thẻ để ghi.");
        inputStep = 0; // Reset để nhận lại từ đầu nếu cần.
        break;

      default:
        inputStep = 0;
        break;
    }
  }

  // Phần kiểm tra thẻ RFID và ghi dữ liệu vẫn giữ nguyên.
  if (buzzerActive && (millis() - buzzerStartTime >= buzzerTime)) {
    digitalWrite(BUZZER, LOW);
    buzzerActive = false;
  }

  if ((!mfrc522.PICC_IsNewCardPresent()) || (!mfrc522.PICC_ReadCardSerial())) {
    return;
  }

  if (addMode) {
    buzzerStartTime = millis();
    buzzerTime = 200;
    buzzerActive = true;
    digitalWrite(BUZZER, HIGH);

    writeStringToBlocks(4, ownerReceive);
    writeStringToBlocks(8, numberReceive);
    writeStringToBlocks(12, expirationDateReceive);
    writeStringToBlocks(16, timestampReceive);

    Serial.println("Đã ghi dữ liệu vào thẻ: ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();
    Serial.println("Chủ sở hữu: " + ownerReceive);
    Serial.println("Biển số xe: " + numberReceive);
    Serial.println("Ngày hết hạn: " + expirationDateReceive);
    Serial.println("Tên ảnh: " + timestampReceive + ".jpg");
    Serial.println();
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}


void writeDataToBlock(int blockNum, byte blockData[]) {
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    return;
  }
  status = mfrc522.MIFARE_Write(blockNum, blockData, 16);
  if (status != MFRC522::STATUS_OK) {
    return;
  }
}

void writeStringToBlocks(int startBlock, String data) {
  int len = data.length();
  byte buffer[16];
  for (int i = 0; i < 3; i++) {
    int offset = i * 16;
    for (int j = 0; j < 16; j++) {
      if (offset + j < len) {
        buffer[j] = data[offset + j];
      } else {
          buffer[j] = 0;
        }
    }
    writeDataToBlock(startBlock + i, buffer);
  }
}

void readDataFromBlock(int blockNum, byte readBlockData[]) {
  byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    return;
  }
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    return;
  }
}

String readStringFromBlocks(int startBlock, int blockCount) {
  String result = "";
  byte buffer[18];
  for (int i = 0; i < blockCount; i++) {
    readDataFromBlock(startBlock + i, buffer);
    for (int j = 0; j < 16; j++) {
      if (buffer[j] != 0) {
        result += (char)buffer[j];
      }
    }
  }
  return result;
}
