#define PIN_STICK_Y      A3
#define STICK_DEADZONE   80
#define STICK_STEP       2
#define STICK_STEP_INTERVAL_MS 15

#define JOINT1_HOME  120
#define JOINT1_MIN   75
#define JOINT1_MAX   180

int currentAngle = JOINT1_HOME;
unsigned long lastStepMs = 0;
unsigned long lastSendMs = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  unsigned long now = millis();

  // 1. 读取摇杆并计算角度
  if (now - lastStepMs >= STICK_STEP_INTERVAL_MS) {
    lastStepMs = now;
    int raw = analogRead(PIN_STICK_Y);
    int centered = raw - 512; // 将 0~1023 转换为 -512~511

    // 超过死区才动作
    if (abs(centered) >= STICK_DEADZONE) {
      // 摇杆往上推 raw 值变小/变大取决于你的硬件接线，这里保持你原代码的逻辑
      int delta = (centered > 0) ? -STICK_STEP : STICK_STEP; 
      currentAngle = constrain(currentAngle + delta, JOINT1_MIN, JOINT1_MAX);
    }
  }

  // 2. 每 50ms 发送一次数据 (不再发逗号和一长串，只发这一个角度)
  if (now - lastSendMs >= 50) {
    lastSendMs = now;
    Serial.println(currentAngle); 
  }
}