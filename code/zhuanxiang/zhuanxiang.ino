// ============================================================
// 底盘转向独立测试/校准代码 (适配车身 180 度互换)
// ============================================================

// 物理连线：原左电机 (现在在当前车身的右侧！)
const int MOTOR_LEFT_F = 9;     
const int MOTOR_LEFT_B = 10;    
// 物理连线：原右电机 (现在在当前车身的左侧！)
const int MOTOR_RIGHT_F = 5;    
const int MOTOR_RIGHT_B = 6;    

// 填入你之前直行测试出的校准系数
float left_ratio = 0.95;  
float right_ratio = 1.0;  

void setup() {
  Serial.begin(9600);
  
  pinMode(MOTOR_LEFT_F, OUTPUT);
  pinMode(MOTOR_LEFT_B, OUTPUT);
  pinMode(MOTOR_RIGHT_F, OUTPUT);
  pinMode(MOTOR_RIGHT_B, OUTPUT);
  
  stopCar();
  Serial.println("=================================");
  Serial.println(">>> 转向校准程序启动，3秒后开始动作 <<<");
  Serial.println("=================================");
  delay(3000);
}

void loop() {
  // --- 动作 1：直行 ---
  Serial.println("⬆️ 测试：直行 1 秒");
  moveForward(150); 
  delay(600);
  stopCar();
  delay(1000);
  
  // --- 动作 3：左转 ---
  Serial.println("➡️ 测试：原地右转 1 秒");
  turnRight(150); 
  delay(600);
  stopCar();
  delay(1000);
  
  // --- 动作 1：直行 ---
  Serial.println("⬆️ 测试：直行 1 秒");
  moveForward(150); 
  delay(500);
  stopCar();
  delay(1000);  

  // --- 动作 2：右转 ---
  Serial.println("⬅️ 测试：原地左转 1 秒");
  turnLeft(150); 
  delay(550);
  stopCar();
  delay(2000); // 休息 2 秒后循环
}

// ================= 重新梳理的核心控制函数 =================

void moveForward(int speed) {
  speed = constrain(speed, 0, 255); 
  // 直行逻辑没问题：原左电机（现右）向原车尾转，原右电机（现左）向原车尾转
  analogWrite(MOTOR_LEFT_F, 0);
  analogWrite(MOTOR_LEFT_B, speed * left_ratio);
  
  analogWrite(MOTOR_RIGHT_F, 0);
  analogWrite(MOTOR_RIGHT_B, speed * right_ratio);
}

void turnLeft(int speed) {
  speed = constrain(speed, 0, 255);
  // 💡 真实左转逻辑：当前车身左轮向后，当前车身右轮向前
  
  // 1. 驱动当前车身左轮 (即原 RIGHT 引脚) 向车后转 
  // 原车头方向(F) = 现车尾方向
  analogWrite(MOTOR_RIGHT_F, speed);
  analogWrite(MOTOR_RIGHT_B, 0);
  
  // 2. 驱动当前车身右轮 (即原 LEFT 引脚) 向车前转 
  // 原车尾方向(B) = 现车头方向
  analogWrite(MOTOR_LEFT_F, 0);
  analogWrite(MOTOR_LEFT_B, speed);
}

void turnRight(int speed) {
  speed = constrain(speed, 0, 255);
  // 💡 真实右转逻辑：当前车身左轮向前，当前车身右轮向后
  
  // 1. 驱动当前车身左轮 (即原 RIGHT 引脚) 向车前转
  analogWrite(MOTOR_RIGHT_F, 0);
  analogWrite(MOTOR_RIGHT_B, speed);
  
  // 2. 驱动当前车身右轮 (即原 LEFT 引脚) 向车后转
  analogWrite(MOTOR_LEFT_F, speed);
  analogWrite(MOTOR_LEFT_B, 0);
}

void stopCar() {
  analogWrite(MOTOR_LEFT_F, 0);
  analogWrite(MOTOR_LEFT_B, 0);
  analogWrite(MOTOR_RIGHT_F, 0);
  analogWrite(MOTOR_RIGHT_B, 0);
}