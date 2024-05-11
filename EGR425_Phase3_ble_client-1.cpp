// Some useful resources on BLE and ESP32:
//      https://github.com/nkolban/ESP32_BLE_Arduino/blob/master/examples/BLE_notify/BLE_notify.ino
//      https://microcontrollerslab.com/esp32-bluetooth-low-energy-ble-using-arduino-ide/
//      https://randomnerdtutorials.com/esp32-bluetooth-low-energy-ble-arduino-ide/
//      https://www.electronicshub.org/esp32-ble-tutorial/
#include <BLEDevice.h>
#include <BLE2902.h>
#include <M5Core2.h>

///////////////////////////////////////////////////////////////
// Variables
///////////////////////////////////////////////////////////////
static BLERemoteCharacteristic *bleRemoteCharacteristic;
static BLEAdvertisedDevice *bleRemoteServer;
static boolean doConnect = false;
static boolean doScan = false;
bool deviceConnected = false;
Point matrix[6][4];
int win[6][4];

// See the following for generating UUIDs: https://www.uuidgenerator.net/
static BLEUUID SERVICE_UUID("a45752ae-c36e-11ed-afa1-0242ac120002");        // Dr. Dan's Service
static BLEUUID CHARACTERISTIC_UUID("c6f0593a-b7b8-11ed-afa1-0242ac120002"); // Dr. Dan's Characteristic

///////////////////////////////////////////////////////////////
// Forward Declarations
///////////////////////////////////////////////////////////////
void drawScreenTextWithBackground(String text, int backgroundColor);
void drawGameBoard();
bool checkwin();

///////////////////////////////////////////////////////////////
// BLE Client Callback Methods
// This method is called when the server that this client is
// connected to NOTIFIES this client (or any client listening)
// that it has changed the remote characteristic
///////////////////////////////////////////////////////////////
static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    Serial.printf("Notify callback for characteristic %s of data length %d\n", pBLERemoteCharacteristic->getUUID().toString().c_str(), length);
    Serial.printf("\tData: %s", (char *)pData);
    std::string value = pBLERemoteCharacteristic->readValue();
    drawScreenTextWithBackground("Initial characteristic value read from server:\n\n" + String(value.c_str()), TFT_GREEN);
    Serial.printf("\tValue was: %s", value.c_str());
}

///////////////////////////////////////////////////////////////
// BLE Server Callback Method
// These methods are called upon connection and disconnection
// to BLE service.
///////////////////////////////////////////////////////////////
class MyClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
        deviceConnected = true;
        Serial.println("Device connected...");
        // drawGameBoard();
    }

    void onDisconnect(BLEClient *pclient)
    {
        deviceConnected = false;
        Serial.println("Device disconnected...");
        // drawScreenTextWithBackground("LOST connection to device.\n\nAttempting re-connection...", TFT_RED);
    }
};

///////////////////////////////////////////////////////////////
// Method is called to connect to server
///////////////////////////////////////////////////////////////
bool connectToServer()
{
    // Create the client
    Serial.printf("Forming a connection to %s\n", bleRemoteServer->getName().c_str());
    BLEClient *bleClient = BLEDevice::createClient();
    bleClient->setClientCallbacks(new MyClientCallback());
    Serial.println("\tClient connected");

    // Connect to the remote BLE Server.
    if (!bleClient->connect(bleRemoteServer))
        Serial.printf("FAILED to connect to server (%s)\n", bleRemoteServer->getName().c_str());
    Serial.printf("\tConnected to server (%s)\n", bleRemoteServer->getName().c_str());

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService *bleRemoteService = bleClient->getService(SERVICE_UUID);
    if (bleRemoteService == nullptr)
    {
        Serial.printf("Failed to find our service UUID: %s\n", SERVICE_UUID.toString().c_str());
        bleClient->disconnect();
        return false;
    }
    Serial.printf("\tFound our service UUID: %s\n", SERVICE_UUID.toString().c_str());

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    bleRemoteCharacteristic = bleRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
    if (bleRemoteCharacteristic == nullptr)
    {
        Serial.printf("Failed to find our characteristic UUID: %s\n", CHARACTERISTIC_UUID.toString().c_str());
        bleClient->disconnect();
        return false;
    }
    Serial.printf("\tFound our characteristic UUID: %s\n", CHARACTERISTIC_UUID.toString().c_str());

    // Read the value of the characteristic
    // if (bleRemoteCharacteristic->canRead())
    // {
    //     std::string value = bleRemoteCharacteristic->readValue();
    //     Serial.printf("The characteristic value was: %s", value.c_str());
    //     drawScreenTextWithBackground("Initial characteristic value read from server:\n\n" + String(value.c_str()), TFT_GREEN);
    //     delay(3000);
    // }

    // Check if server's characteristic can notify client of changes and register to listen if so
    if (bleRemoteCharacteristic->canNotify())
        bleRemoteCharacteristic->registerForNotify(notifyCallback);

    // deviceConnected = true;
    return true;
}

///////////////////////////////////////////////////////////////
// Scan for BLE servers and find the first one that advertises
// the service we are looking for.
///////////////////////////////////////////////////////////////
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    /**
     * Called for each advertising BLE server.
     */
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        // Print device found
        Serial.print("BLE Advertised Device found:");
        Serial.printf("\tName: %s\n", advertisedDevice.getName().c_str());

        // More debugging print
        // Serial.printf("\tAddress: %s\n", advertisedDevice.getAddress().toString().c_str());
        // Serial.printf("\tHas a ServiceUUID: %s\n", advertisedDevice.haveServiceUUID() ? "True" : "False");
        // for (int i = 0; i < advertisedDevice.getServiceUUIDCount(); i++) {
        //    Serial.printf("\t\t%s\n", advertisedDevice.getServiceUUID(i).toString().c_str());
        // }
        // Serial.printf("\tHas our service: %s\n\n", advertisedDevice.isAdvertisingService(SERVICE_UUID) ? "True" : "False");

        // We have found a device, let us now see if it contains the service we are looking for.
        if (advertisedDevice.haveServiceUUID() &&
            advertisedDevice.isAdvertisingService(SERVICE_UUID) &&
            advertisedDevice.getName() == "Sams M5Core2023")
        {
            BLEDevice::getScan()->stop();
            bleRemoteServer = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = true;
        }
    }
};

///////////////////////////////////////////////////////////////
// Put your setup code here, to run once
///////////////////////////////////////////////////////////////
void setup()
{
    // Init device
    M5.begin();
    M5.Lcd.setTextSize(3);

    BLEDevice::init("");

    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 5 seconds.
    BLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);
    drawScreenTextWithBackground("Scanning for BLE server...", TFT_BLUE);
}

///////////////////////////////////////////////////////////////
// Put your main code here, to run repeatedly
///////////////////////////////////////////////////////////////

 String lastx = "Initial";
 String lasty = "Initial";
 bool isPlaying = true;
 int whoWon = 0;
void loop()
{
    // If the flag "doConnect" is true then we have scanned for and found the desired
    // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
    // connected we set the connected flag to be false.
    if (doConnect == true)
    {
        if (connectToServer())
        {
            Serial.println("We are now connected to the BLE Server.");
            drawScreenTextWithBackground("Connected to BLE server: " + String(bleRemoteServer->getName().c_str()), TFT_GREEN);
            doConnect = false;
            delay(3000);
            drawGameBoard();
        }
        else
        {
            Serial.println("We have failed to connect to the server; there is nothin more we will do.");
            drawScreenTextWithBackground("FAILED to connect to BLE server: " + String(bleRemoteServer->getName().c_str()), TFT_GREEN);
            delay(3000);
        }
    }

    // If we are connected to a peer BLE Server, update the characteristic each time we are reached
    // with the current time since boot.
   
    if (deviceConnected && isPlaying)
    {
        // Format string to send to server
        // String newValue = "Time since boot: " + String(millis() / 1000);
        // Serial.println("Setting new characteristic value to \"" + newValue + "\"");

        // Set the characteristic's value to be the array of bytes that is actually a string.
        // bleRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());

// prints their piece 

        std::string readValue = bleRemoteCharacteristic->readValue();
        Serial.printf(readValue.c_str());
        String newString = (String)readValue.c_str();
        int split = readValue.find(" ");
        int lastIndex = newString.length();
        String xCoord = newString.substring(0, split);
        String yCoord = newString.substring(split + 1, lastIndex);

        if (xCoord != lastx && yCoord != lasty)
        {
            
            for (int i = 0; i < 6; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    int b = atoi(xCoord.c_str());
                    int c = atoi(yCoord.c_str());
                    if ((b >= matrix[i][j].x - 17 && b <= matrix[i][j].x + 17) && (c >= matrix[i][j].y - 17 && c <= matrix[i][j].y + 17))
                    {
                         Serial.printf("The new characteristic value as a STRING is: %s, %s\n", xCoord, yCoord);
                        M5.Lcd.fillCircle(matrix[i][j].x, matrix[i][j].y, 17, TFT_RED);
                        lastx = xCoord;
                        lasty = yCoord;
                        win[i][j] = 2;
                        Serial.println(checkwin());
                    }
                }
            }
        }
// print my piece 
        TouchPoint_t coordinate = M5.Touch.getPressPoint();
        Serial.printf("x:%d, y:%d \r\n", coordinate.x, coordinate.y);
        if (coordinate.x > -1 && coordinate.y > -1)
        {
            for (int i = 0; i < 6; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    int b = coordinate.x;
                    int c = coordinate.y;
                    if ((b >= matrix[i][j].x - 17 && b <= matrix[i][j].x + 17) && (c >= matrix[i][j].y - 17 && c <= matrix[i][j].y + 17))
                    {
                        M5.Lcd.fillCircle(matrix[i][j].x, matrix[i][j].y, 17, TFT_YELLOW);
                        lastx = (String)b;
                        lasty = (String)c;
                        String newValue = lastx + " " + lasty;
                        bleRemoteCharacteristic->writeValue(newValue.c_str());
                        win[i][j] = 1;
                        Serial.println(checkwin());
                    }
                }
            }
        
        }
        
    }
    else if(deviceConnected && isPlaying == false) {
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
            isPlaying == true;
        }
    }
    else if (doScan)
    {
        drawScreenTextWithBackground("Disconnected....re-scanning for BLE server...", TFT_ORANGE);
        BLEDevice::getScan()->start(0); // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
    }
    delay(100); // Delay a second between loops.
}

///////////////////////////////////////////////////////////////
// Colors the background and then writes the text on top
///////////////////////////////////////////////////////////////
void drawScreenTextWithBackground(String text, int backgroundColor)
{
    M5.Lcd.fillScreen(backgroundColor);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println(text);
}

void drawGameBoard()
{
    M5.Lcd.fillScreen(TFT_BLUE);
    M5.Lcd.fillRect(20, 20, 280, 200, TFT_BLACK);
    M5.Lcd.fillRect(260, 220, 40, 20, TFT_BLACK);
    M5.Lcd.fillRect(20, 220, 40, 20, TFT_BLACK);
    int width = 47;
    int height = 47;
    Point p;

    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            p = Point(width, height);
            matrix[i][j] = p;
            win[i][j] = 0;
            M5.Lcd.fillCircle(width, height, 17, TFT_BLUE);
            height += 45;
        }
        height = 47;
        width += 45;
    }
}bool checkwin() {
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