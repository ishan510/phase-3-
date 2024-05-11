// Some useful resources on BLE and ESP32:
//      https://github.com/nkolban/ESP32_BLE_Arduino/blob/master/examples/BLE_notify/BLE_notify.ino
//      https://microcontrollerslab.com/esp32-bluetooth-low-energy-ble-using-arduino-ide/
//      https://randomnerdtutorials.com/esp32-bluetooth-low-energy-ble-arduino-ide/
//      https://www.electronicshub.org/esp32-ble-tutorial/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <M5Core2.h>
#include <string>


///////////////////////////////////////////////////////////////
// Variables
///////////////////////////////////////////////////////////////
BLEServer *bleServer;
BLEService *bleService;
BLECharacteristic *bleCharacteristic;
static BLERemoteCharacteristic *bleRemoteCharacteristic;
bool deviceConnected = false;
bool previouslyConnected = false;
Point matrix[6][4];
int win[6][4];
int timer = 0;
void drawGameBoard();

// See the following for generating UUIDs: https://www.uuidgenerator.net/
#define SERVICE_UUID "a45752ae-c36e-11ed-afa1-0242ac120002"
#define CHARACTERISTIC_UUID "c6f0593a-b7b8-11ed-afa1-0242ac120002"

///////////////////////////////////////////////////////////////
// BLE Server Callback Methods
///////////////////////////////////////////////////////////////
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) {
        deviceConnected = true;
        previouslyConnected = true;
        Serial.println("Device connected...");
        drawGameBoard(); //only draws the game board once when the devices connect
    }
    void onDisconnect(BLEServer *pServer) {
        deviceConnected = false;
        Serial.println("Device disconnected...");
    }
};

///////////////////////////////////////////////////////////////
// Forward Declarations
///////////////////////////////////////////////////////////////
void broadcastBleServer();
bool checkwin();
void drawScreenTextWithBackground(String text, int backgroundColor);

///////////////////////////////////////////////////////////////
// Put your setup code here, to run once
///////////////////////////////////////////////////////////////
void setup()
{
    // Init device
    M5.begin();
    M5.Lcd.setTextSize(3);

    // Initialize M5Core2 as a BLE server
    Serial.print("Starting BLE...");
    String bleDeviceName = "Sams M5Core2023";
    BLEDevice::init(bleDeviceName.c_str());

    // Broadcast the BLE server
    drawScreenTextWithBackground("Initializing BLE...", TFT_CYAN);
    broadcastBleServer();
    drawScreenTextWithBackground("Broadcasting as BLE server named:\n\n" + bleDeviceName, TFT_BLUE);

}

///////////////////////////////////////////////////////////////
// Put your main code here, to run repeatedly
///////////////////////////////////////////////////////////////
String lastx = "-1";
String lasty = "-1";
String oppx = "-1";
String oppy = "-1";
bool isPlaying = true;
int whoWon = 0;
void loop()
{
    if (deviceConnected && isPlaying == true) {
        std::string readValue = bleCharacteristic->getValue();
        String newString = (String)readValue.c_str();
        int split = readValue.find(" ");
        int lastIndex = newString.length();
        String xCoord = newString.substring(0, split);
        String yCoord = newString.substring(split+1, lastIndex);

        //opposing player's piece
        if(xCoord != lastx && yCoord != lasty){
            for(int i=0;i<6;i++){
                for(int j=0;j<4;j++){
                    int b = atoi(xCoord.c_str());
                    int c = atoi(yCoord.c_str());
                    if((b >= matrix[i][j].x - 17 && b <= matrix[i][j].x +17) && (c >= matrix[i][j].y - 17 && c <= matrix[i][j].y +17) ){
                        M5.Lcd.fillCircle(matrix[i][j].x, matrix[i][j].y, 17, TFT_YELLOW);
                        oppx = xCoord;
                        oppy = yCoord;
                        win[i][j] = 2;
                        Serial.println(checkwin());
                    }
                }
            }
        }
       
        //your players piece
        TouchPoint_t coordinate = M5.Touch.getPressPoint();
        Serial.printf("x:%d, y:%d \r\n", coordinate.x, coordinate.y);
        if(coordinate.x > -1 && coordinate.y > -1 && (String)coordinate.x != oppx && (String)coordinate.y != oppy){
            for(int i=0;i<6;i++){
                for(int j=0;j<4;j++){
                    if((coordinate.x >= matrix[i][j].x - 17 && coordinate.x <= matrix[i][j].x +17) && (coordinate.y >= matrix[i][j].y - 17 && coordinate.y <= matrix[i][j].y +17) ){
                        M5.Lcd.fillCircle(matrix[i][j].x, matrix[i][j].y, 17, TFT_RED);
                        lastx = (String)coordinate.x;
                        lasty = (String)coordinate.y;    
                        String newValue = lastx + " " + lasty;
                        bleCharacteristic-> setValue(newValue.c_str());
                        win[i][j] = 1;
                        Serial.println(checkwin());
                    }
                }
            }
        }
    }else if(deviceConnected && isPlaying == false){
        M5.update();
        if(whoWon == 1){
            drawScreenTextWithBackground("YOU WIN", TFT_GREEN);
        }else if(whoWon == 2){
            drawScreenTextWithBackground("YOU LOSE", TFT_RED);
        }
        if(M5.BtnA.isPressed()){
            drawGameBoard();
            whoWon = 0;
            lastx = "-1";
            lasty = "-1";
            oppx = "-1";
            oppy = "-1";
            isPlaying == true;
        }
        
    } else if (previouslyConnected) {
        drawScreenTextWithBackground("Disconnected. Reset M5 device to reinitialize BLE.", TFT_RED); // Give feedback on screen
        timer = 0;
    }
    
    // Only update the timer (if connected) every 1 second
    delay(100);
}

///////////////////////////////////////////////////////////////
// Colors the background and then writes the text on top
///////////////////////////////////////////////////////////////
void drawScreenTextWithBackground(String text, int backgroundColor) {
    M5.Lcd.fillScreen(backgroundColor);
    M5.Lcd.setCursor(0,0);
    M5.Lcd.println(text);
}

void drawGameBoard(){

    M5.Lcd.fillScreen(TFT_BLUE);
    M5.Lcd.fillRect(20,20,280,200,TFT_BLACK);
    M5.Lcd.fillRect(260,220,40,20,TFT_BLACK);
    M5.Lcd.fillRect(20,220,40,20,TFT_BLACK);
    int width = 47;
    int height = 47;
    Point p;

    for(int i=0; i < 6; i++){
        for(int j = 0; j< 4; j++){
            p = Point(width, height);
            matrix[i][j] = p;
            win[i][j]=0;
            M5.Lcd.fillCircle(width, height, 17, TFT_BLUE);
            height += 45;
        }
        
        height = 47;
        width += 45;
    }

}

bool checkwin() {
  // Check rows
  for (int i = 0; i < 6; i++) {
    if (win[i][0]!=0 && win[i][1]!=0 && win[i][2]!=0 && win[i][0] == win[i][1] && win[i][1] == win[i][2]) {
        if(win[i][0] == 1){
            Serial.println("PLAYER 1 WINS");
            whoWon = 1;
        }else if(win[i][0] == 2){
            Serial.println("PLAYER 2 WINS");
            whoWon = 2;
        }
      isPlaying = false;
      return true;
    }
  }

  // Check columns
  for (int j = 0; j < 4; j++) {
    if (win[0][j]!=0 && win[1][j]!=0 && win[2][j]!=0 && win[0][j] == win[1][j] && win[1][j] == win[2][j]) {
        if(win[0][j] == 1){
            Serial.println("PLAYER 1 WINS");
            whoWon = 1;
        }else if(win[0][j] == 2){
            Serial.println("PLAYER 2 WINS");
            whoWon = 2;
        }
      isPlaying = false;
      return true;
    }
  }

  // Check diagonals
  if (win[0][0]!=0 && win[1][1]!=0 && win[2][2]!=0 && win[0][0] == win[1][1] && win[1][1] == win[2][2] || win[1][1]!=0 && win[2][2]!=0 && win[3][3]!=0 && win[1][1] == win[2][2] && win[2][2] == win[3][3]) {
    if(win[1][1] == 1){
        Serial.println("PLAYER 1 WINS");
        whoWon = 1;
    }else if(win[1][1] == 2){
        Serial.println("PLAYER 2 WINS");
        whoWon = 2;
    }
    isPlaying = false;
    return true;
  }

  if (win[0][2]!=0 && win[1][1]!=0 && win[2][0]!=0 && win[0][2] == win[1][1] && win[1][1] == win[2][0]) {
    if(win[0][2] == 1){
        Serial.println("PLAYER 1 WINS");
        whoWon = 1;
    }else if(win[0][2] == 2){
        Serial.println("PLAYER 2 WINS");
        whoWon = 2;
    }
    isPlaying = false;
    return true;
  }

  // No win condition found
  return false;
}

///////////////////////////////////////////////////////////////
// This code creates the BLE server and broadcasts it
///////////////////////////////////////////////////////////////
void broadcastBleServer() {    
    // Initializing the server, a service and a characteristic 
    bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(new MyServerCallbacks());
    bleService = bleServer->createService(SERVICE_UUID);
    bleCharacteristic = bleService->createCharacteristic(CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );
    bleCharacteristic->setValue("Hello BLE World from Dr. Dan!");
    bleService->start();

    // Start broadcasting (advertising) BLE service
    BLEAdvertising *bleAdvertising = BLEDevice::getAdvertising();
    bleAdvertising->addServiceUUID(SERVICE_UUID);
    bleAdvertising->setScanResponse(true);
    bleAdvertising->setMinPreferred(0x12); // Use this value most of the time 
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined...you can connect with your phone!"); 

}