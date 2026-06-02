// ============================================================
// 底盘电机 PWM 调速专项测试代码
// ============================================================

// --- 底盘电机引脚定义 (全部支持 PWM 调速) ---
const int MOTOR_LEFT_F = 9;     // 左前
const int MOTOR_LEFT_B = 10;    // 左后
const int MOTOR_RIGHT_F = 5;    // 右前
const int MOTOR_RIGHT_B = 6;    // 右后

void setup() {
  Serial.begin(9600);
  
  // 设置电机引脚为输出模式
  pinMode(MOTOR_LEFT_F, OUTPUT);
  pinMode(MOTOR_LEFT_B, OUTPUT);
  pinMode(MOTOR_RIGHT_F, OUTPUT);
  pinMode(MOTOR_RIGHT_B, OUTPUT);
  
  // 确保开机是停止状态
  stopCar();
  
  Serial.println("===============================");
  Serial.println("🏎️ 电机 PWM 调速测试程序已启动！");
  Serial.println("请打开电池总开关！");
  Serial.println("3秒后开始演示...");
  Serial.println("===============================");
  delay(3000); 
}

void loop() {
  // --- 阶段 1：平滑加速 (0 升到 255) ---
  Serial.println("\n>>> 阶段 1：正在平滑加速...");
  for(int speed = 0; speed <= 255; speed += 5) {
    moveForward(speed);
    Serial.print("当前发送 PWM 速度: ");
    Serial.println(speed);
    delay(100); // 每升一点速度停顿0.1秒，形成平滑过渡
  }

  // --- 阶段 2：全速狂奔 ---
  Serial.println("\n>>> 阶段 2：到达最高速，保持 2 秒！");
  delay(2000);

  // --- 阶段 3：平滑减速 (255 降到 0) ---
  Serial.println("\n>>> 阶段 3：正在平滑减速...");
  for(int speed = 255; speed >= 0; speed -= 5) {
    moveForward(speed);
    Serial.print("当前发送 PWM 速度: ");
    Serial.println(speed);
    delay(100);
  }

  // --- 阶段 4：停车休息 ---
  Serial.println("\n>>> 阶段 4：刹车停止，休息 3 秒...");
  stopCar();
  delay(3000);
}

// ================= 核心控制函数 =================

// 调速前进函数
void moveForward(int speed) {
  // 安全限制：强制把数字框在 0~255 之间
  speed = constrain(speed, 0, 255); 
  
  // 左轮控制 (前转，后停)
  analogWrite(MOTOR_LEFT_F, speed);
  analogWrite(MOTOR_LEFT_B, 0);
  
  // 右轮控制 (前转，后停)
  analogWrite(MOTOR_RIGHT_F, speed);
  analogWrite(MOTOR_RIGHT_B, 0);
}

// 停止函数
void stopCar() {
  analogWrite(MOTOR_LEFT_F, 0);
  analogWrite(MOTOR_LEFT_B, 0);
  analogWrite(MOTOR_RIGHT_F, 0);
  analogWrite(MOTOR_RIGHT_B, 0);
}