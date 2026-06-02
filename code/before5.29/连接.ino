#include <SoftwareSerial.h>

// 1. 定义软串口
// 参数1: RX引脚 (接OpenMV的TX_P4)
// 参数2: TX引脚 (接OpenMV的RX_P5)
SoftwareSerial OpenMV_Serial(11, 13); 

void setup() {
  // 2. 初始化与电脑通信的硬件主串口 (用于调试看数据)
  Serial.begin(115200);
  
  // 3. 初始化与 OpenMV 通信的软串口 (波特率必须与 OpenMV 一致)
  OpenMV_Serial.begin(115200);
  
  Serial.println("Arduino AVR 初始化完成，等待 OpenMV 视觉数据...");
}

void loop() {
  // 4. 检查是否收到了 OpenMV 发来的数据
  if (OpenMV_Serial.available() > 0) {
    
    // 5. 按行读取字符串，直到遇到 '\n' 为止
    String receivedData = OpenMV_Serial.readStringUntil('\n');
    
    // 剔除可能存在的不可见回车符 '\r' (保持数据干净)
    receivedData.trim();
    
    // 如果读取到的数据不是空的，就打印出来
    if (receivedData.length() > 0) {
      Serial.print("收到 OpenMV 视觉指令: ");
      Serial.println(receivedData);
      
      // TODO: 在这里添加解析收到的字符串(例如提取X和Y坐标)并控制小车底盘的代码
      // if (receivedData == "A") { ... }
    }
  }
}