#include <Servo.h>

Servo testServo;

// --- 测试引脚设定 ---
const int TEST_PIN = 3; // 当前测试：D3 (底座/Base)

// 初始安全角度 (通常 120 度是正视前方的中心位置)
int currentAngle = 120; 

void setup() {
  Serial.begin(9600);
  
  // 安全上电逻辑：先给指令，再连接引脚
  testServo.write(currentAngle);
  testServo.attach(TEST_PIN);
  
  // 打印操作指引
  Serial.println("========================================");
  Serial.println("单轴舵机测试程序已启动！");
  Serial.println("当前测试端口: D3 (底座/回转关节)");
  Serial.println("状态: 其他舵机已彻底屏蔽，保持未通电状态。");
  Serial.println("----------------------------------------");
  Serial.println("操作说明：");
  Serial.println("请在上方输入框直接输入角度数字 (0-180) 并按回车。");
  Serial.println("【注意】: 确保回转半径内无障碍物！");
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
    if (targetAngle >= 0 && targetAngle <= 220) {
      currentAngle = targetAngle;
      
      // 写入新角度
      testServo.write(currentAngle);
      
      // 串口反馈实时数据
      Serial.print("=> 执行指令... D3 当前角度: ");
      Serial.println(currentAngle);
    }
    
    // 清空缓冲区中残留的结束符
    while(Serial.available() > 0) {
      Serial.read();
    }
  }
}