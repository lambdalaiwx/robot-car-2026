#include <Servo.h>

Servo testServo;

// --- 测试引脚设定 ---
const int TEST_PIN = 12; 

// 初始安全角度 (夹爪一般90度处于半开状态，比较安全)
int currentAngle = 90; 

void setup() {
  Serial.begin(9600);
  
  // 安全上电逻辑：先给指令，再连接引脚
  testServo.write(currentAngle);
  testServo.attach(TEST_PIN);
  
  // 打印操作指引
  Serial.println("========================================");
  Serial.println("单轴舵机测试程序已启动！");
  Serial.println("当前测试端口: D12");
  Serial.println("状态: 其他舵机已彻底屏蔽，保持未通电状态。");
  Serial.println("----------------------------------------");
  Serial.println("操作说明：");
  Serial.println("请在上方输入框直接输入角度数字 (0-180) 并按回车。");
  Serial.println("建议每次步进 5~10 度，观察机械结构极限。");
  Serial.println("========================================");
}

void loop() {
  // 检查串口是否有数据输入
  if (Serial.available() > 0) {
    // 稍微延时，等待整个数字字符串进入缓冲区
    delay(10); 
    
    // 直接读取输入的整数
    int targetAngle = Serial.parseInt();
    
    // 防呆设计：忽略无效输入或回车换行造成的 0 (如果你真的想测 0 度，直接输入 0 也会被捕捉)
    // 限制范围在 0 到 180 之间
    if (targetAngle >= 0 && targetAngle <= 180) {
      currentAngle = targetAngle;
      
      // 写入新角度
      testServo.write(currentAngle);
      
      // 串口反馈实时数据，方便你做记录
      Serial.print("=> 执行指令... D12 当前角度: ");
      Serial.println(currentAngle);
    }
    
    // 清空缓冲区中残留的结束符（如 \n 或 \r）
    while(Serial.available() > 0) {
      Serial.read();
    }
  }
}