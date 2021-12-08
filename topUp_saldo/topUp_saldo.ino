#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         0           // Configurable, see typical pin layout above
#define SS_PIN          5          // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

MFRC522::MIFARE_Key key;

bool notif = true;
bool isiSaldo = false;
String input;
long saldo;
int digit;

long OLDsaldo;
int OLDdigit;

void setup() {
    Serial.begin(115200); // Initialize serial communications with the PC
    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card

    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    Serial.println("Silahkan Cek atau Tambah Saldo anda");

    Serial.println();

    Serial.println("Saldo akan di simpan pada RFID Card pada sector #2 blocks #10");
}

void loop() {
  if (Serial.available()){
    isiSaldo = true;
    input = "";
    while(Serial.available()>0){
      input += char(Serial.read());
    }
    //Serial.println(input);
    saldo = input.toInt();
    if (saldo > 255){
      saldo = 0;
      Serial.println("Saldo tidak boleh lebih dari 255");
    }
    if (saldo < 0){
      saldo = 0;
      Serial.println("Saldo tidak boleh kurang dari 0");
    }
    digit = saldo;
    saldo *= 1000;
    Serial.print("saldo yang di input : ");
    Serial.println(saldo);
    Serial.println("Silahkan tap kartu untuk menambah saldo kartu");
  }
  if (notif){
    notif = false;
    Serial.println();
    Serial.println("________________________________________________________________________________");
    Serial.println("Silahkan input jumlah saldo dan tap kartu");
    Serial.println("Contoh input 5, maka saldo akan menjadi Rp5000, saldo maksimal Rp255.000");
    Serial.println();
    Serial.println("Silahkan Tap untuk cek saldo Anda");
  }
  if ( ! mfrc522.PICC_IsNewCardPresent()){
      return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()){
      return;
  }
  
  Serial.println();
  Serial.print("Card UID:");
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  
  Serial.println();
  Serial.print("Tipe Kartu : ");
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  // Cek kesesuaian kartu
  if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
      Serial.println(F("Kode ini hanya dapat digunakan pada MIFARE Classic cards 1KB - 13.56MHz."));
      notif = true;
      delay(2000);
      resetReader();
      return;
  }

  // that is: sector #2, covering block #10 up to and including block #11
  byte sector         = 2;
  byte blockAddr      = 10;
  
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);

  // Show the whole sector as it currently is
  //Serial.println("Current data in sector:");
  mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();

  if (isiSaldo){
    // Baca Saldo yang ada dari RFID Card
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Gagal Baca Kartu RFID");
        //Serial.println(mfrc522.GetStatusCodeName(status));
        resetReader();
        return;
    }
    OLDdigit = buffer[0];
    OLDsaldo = OLDdigit;
    OLDsaldo *= 1000;
    Serial.print("Saldo Kartu Sebelumnya : ");
    Serial.println(OLDsaldo);
    Serial.println();
  
    // Tambah saldo dan Write Saldo pada RFID Card
    saldo += OLDsaldo;
    digit += OLDdigit;
    
    if (digit > 255){
      saldo = 0;
      digit = 0;
      Serial.println("Saldo sebelum di tambah saldo baru melebihi 255 ribu");
      Serial.println("GAGAL tambah saldo");
      resetReader();
      return;
    }
    
    byte dataBlock[]    = {
        //0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15
        digit, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("GAGAL Write Saldo pada Kartu RFID");
        //Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println();
  
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Gagal Baca Kartu RFID");
        //Serial.println(mfrc522.GetStatusCodeName(status));
    }

    Serial.println();
  
    Serial.println("Menambahkan Saldo...");
    if (buffer[0] == dataBlock[0]){
      //Serial.print("data digit ke 0 : ");
      //Serial.println(buffer[0]);
      Serial.print("Saldo anda sekarang : ");
      Serial.println(saldo);
      Serial.println("_________ Berhasil isi saldo pada kartu ___________");
    }else{
      Serial.println("------------ GAGAL ISI SALDO --------------");
    }
  }else{
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Gagal Baca Kartu RFID");
        //Serial.println(mfrc522.GetStatusCodeName(status));
        saldo = 0;
        digit = 0;
        resetReader();
        return;
    }

    Serial.println();
  
    Serial.println("Cek Saldo Kartu");
    //Serial.print("data digit ke 0 : ");
    //Serial.println(buffer[0]);
    saldo = buffer[0];
    saldo *= 1000;
    Serial.print("Saldo anda sekarang : ");
    Serial.println(saldo);
  }

  saldo = 0;
  digit = 0;

  Serial.println();

  // Dump the sector data
  //Serial.println("Current data in sector:");
  //mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();

  resetReader();
}

void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

void resetReader(){
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();

  notif = true;
  isiSaldo = false;
}
