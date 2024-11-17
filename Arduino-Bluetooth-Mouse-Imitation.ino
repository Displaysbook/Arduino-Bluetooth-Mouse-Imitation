#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEHIDDevice.h>
#include <HIDTypes.h>
#include <HIDKeyboardTypes.h>
#include <sdkconfig.h>

BLEHIDDevice* hidDevice;
BLECharacteristic* inputMouse;

bool deviceConnected = false;
bool prevConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("Device Connected");
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("Device Disconnected");
  }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init("ESP32_BLE_Mouse");

  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  hidDevice = new BLEHIDDevice(pServer);
  inputMouse = hidDevice->inputReport(1); // Mouse input report

  // 配置HID描述符
  hidDevice->manufacturer()->setValue("ESP32 Mouse");
  hidDevice->pnp(0x02, 0xe502, 0xa111, 0x0210);
  hidDevice->hidInfo(0x00, 0x01);

  // HID报告描述符
  const uint8_t REPORT_MAP[] = {
      0x05, 0x01,                    // Usage Page (Generic Desktop)
      0x09, 0x02,                    // Usage (Mouse)
      0xA1, 0x01,                    // Collection (Application)
      0x85, 0x01,                    // Report ID (1)
      0x09, 0x01,                    // Usage (Pointer)
      0xA1, 0x00,                    // Collection (Physical)
      0x05, 0x09,                    // Usage Page (Buttons)
      0x19, 0x01,                    // Usage Minimum (1)
      0x29, 0x03,                    // Usage Maximum (3)
      0x15, 0x00,                    // Logical Minimum (0)
      0x25, 0x01,                    // Logical Maximum (1)
      0x95, 0x03,                    // Report Count (3)
      0x75, 0x01,                    // Report Size (1)
      0x81, 0x02,                    // Input (Data, Variable, Absolute)
      0x95, 0x01,                    // Report Count (1)
      0x75, 0x05,                    // Report Size (5)
      0x81, 0x01,                    // Input (Constant) for padding
      0x05, 0x01,                    // Usage Page (Generic Desktop)
      0x09, 0x30,                    // Usage (X)
      0x09, 0x31,                    // Usage (Y)
      0x15, 0x81,                    // Logical Minimum (-127)
      0x25, 0x7F,                    // Logical Maximum (127)
      0x75, 0x08,                    // Report Size (8)
      0x95, 0x02,                    // Report Count (2)
      0x81, 0x06,                    // Input (Data, Variable, Relative)
      0xC0,                          // End Collection (Physical)
      0xC0                           // End Collection (Application)
  };

  hidDevice->reportMap((uint8_t*)REPORT_MAP, sizeof(REPORT_MAP));
  hidDevice->startServices();

  BLEAdvertising* pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_MOUSE);
  pAdvertising->addServiceUUID(hidDevice->hidService()->getUUID());
  pAdvertising->start();
  
  hidDevice->setBatteryLevel(100);
  Serial.println("BLE Mouse is ready to pair");
}

void loop() {
  // 检查连接状态
  if (deviceConnected) {
    // 模拟鼠标移动
    uint8_t m[3];
    m[0] = 0;      // 按键状态，0表示无按键
    m[1] = 10;     // X轴移动
    m[2] = 0;      // Y轴移动
    inputMouse->setValue(m, sizeof(m));
    inputMouse->notify();
    delay(100);    // 控制鼠标移动的速度

    // 改变方向
    m[1] = -m[1];
    inputMouse->setValue(m, sizeof(m));
    inputMouse->notify();
    delay(100);
  }

  // 检查是否重新连接
  if (!deviceConnected && prevConnected) {
    delay(500);                   // 等待一些时间以避免快速循环
    BLEDevice::startAdvertising(); // 重新开启广告
    Serial.println("Start advertising");
    prevConnected = deviceConnected;
  }

  // 检测新的连接状态
  if (deviceConnected && !prevConnected) {
    prevConnected = deviceConnected;
  }
}
