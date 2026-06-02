#include <Servo.h>

Servo myBase;
const int PIN_BASE = 3; // D3 底座关节
int currentAngle = 120; // 初始安全角度

void setup() {
  Serial.begin(9600);
  
  // 安全上电复位
  myBase.write(currentAngle);
  myBase.attach(PIN_BASE);
  
  Serial.println("D3 单独调试接收端已就绪...");
}

void loop() {
  if (Serial.available() > 0) {
    // 稍微延时，等待数字完全进入缓冲区
    delay(10); 
    
    // 直接读取遥控器发来的唯一一个数字
    int targetAngle = Serial.parseInt();
    
    // 严格的安全卡控：只有在 75-180 之间的数据才允许执行！
    // 这一步彻底防止了因为串口乱码读出 '0' 导致的死锁烧毁
    if (targetAngle >= 75 && targetAngle <= 180) {
      currentAngle = targetAngle;
      myBase.write(currentAngle);
    }
    
    // 清空缓冲区里的换行符等杂质
    while(Serial.available() > 0) {
      Serial.read();
    }
  }
}