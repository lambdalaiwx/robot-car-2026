#include <Servo.h>

Servo testServo;

// --- 测试引脚设定 ---
const int TEST_PIN = 4; // 当前测试：D4 (从根部往上数第二个关节)

// 初始安全角度 (大臂通常 90 度是垂直或中间斜向上的安全位置)
int currentAngle = 90; 

void setup() {
  Serial.begin(9600);
  
  // 安全上电逻辑：先给指令，再连接引脚
  testServo.write(currentAngle);
  testServo.attach(TEST_PIN);
  
  // 打印操作指引
  Serial.println("========================================");
  Serial.println("单轴舵机测试程序已启动！");
  Serial.println("当前测试端口: D4 (大臂/肩部关节)");
  Serial.println("状态: 其他舵机已彻底屏蔽，保持未通电状态。");
  Serial.println("----------------------------------------");
  Serial.println("操作说明：");
  Serial.println("请在上方输入框直接输入角度数字 (0-180) 并按回车。");
  Serial.println("【安全提示】: 该关节负载较大，请以 5 度为单位小幅调整！");
  Serial.println("========================================");
}

void loop() {
  // 检查串口是否有数据输入
  if (Serial.available() > 0) {
    // 稍微延时，等待整个数字进入缓冲区
    delay(10); 
    
    // 直接读取输入的整数
    int targetAngle = Serial.parseInt();
    
    // 限制范围在 0 到 180 之间
    if (targetAngle >= 0 && targetAngle <= 180) {
      currentAngle = targetAngle;
      
      // 写入新角度
      testServo.write(currentAngle);
      
      // 串口反馈实时数据
      Serial.print("=> 执行指令... D4 当前角度: ");
      Serial.println(currentAngle);
    }
    
    // 清空缓冲区中残留的结束符
    while(Serial.available() > 0) {
      Serial.read();
    }
  }
}