// ============================================================
// 三自由度机械臂舵机调试工具
// 夹爪(D12) -> 中关节(D8) -> 根部(D3)
// 串口输入角度值，实时控制对应舵机
// ============================================================
#include <Servo.h>

Servo servoGripper;   // D12 夹爪
Servo servoMiddle;    // D8  中关节
Servo servoBase;      // D3  根部

const int PIN_GRIPPER = 12;
const int PIN_MIDDLE  = 8;
const int PIN_BASE    = 3;

// 各舵机当前角度
int angleGripper = 90;
int angleMiddle  = 90;
int angleBase    = 90;

// 当前选中的舵机: 0=夹爪, 1=中关节, 2=根部
int selectedServo = 0;

const char* servoNames[] = {"夹爪(D12)", "中关节(D8)", "根部(D3)"};

void setup() {
  Serial.begin(9600);

  servoGripper.attach(PIN_GRIPPER);
  servoMiddle.attach(PIN_MIDDLE);
  servoBase.attach(PIN_BASE);

  // 归中
  servoGripper.write(angleGripper);
  servoMiddle.write(angleMiddle);
  servoBase.write(angleBase);

  Serial.println(F("=== 机械臂舵机调试 ==="));
  Serial.println(F("指令:"));
  Serial.println(F("  g / m / b    — 选中 夹爪 / 中关节 / 根部"));
  Serial.println(F("  0~180        — 设置当前选中舵机角度"));
  Serial.println(F("  s            — 扫描当前选中舵机 0->180->0"));
  Serial.println(F("  p            — 打印当前所有角度"));
  Serial.println(F("  r            — 全部归中(90度)"));
  Serial.println(F("  所有舵机已归中到 90 度"));
  Serial.print(F("当前选中: ")); Serial.println(servoNames[selectedServo]);
}

void loop() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.length() == 0) return;

  // 选择舵机
  if (cmd == "g" || cmd == "G") {
    selectedServo = 0;
    Serial.print(F(">> 选中: ")); Serial.println(servoNames[0]);
    return;
  }
  if (cmd == "m" || cmd == "M") {
    selectedServo = 1;
    Serial.print(F(">> 选中: ")); Serial.println(servoNames[1]);
    return;
  }
  if (cmd == "b" || cmd == "B") {
    selectedServo = 2;
    Serial.print(F(">> 选中: ")); Serial.println(servoNames[2]);
    return;
  }

  // 扫描
  if (cmd == "s" || cmd == "S") {
    sweepSelected();
    return;
  }

  // 打印角度
  if (cmd == "p" || cmd == "P") {
    Serial.print(F("夹爪:"));   Serial.print(angleGripper);
    Serial.print(F("  中关节:")); Serial.print(angleMiddle);
    Serial.print(F("  根部:"));   Serial.println(angleBase);
    return;
  }

  // 全部归中
  if (cmd == "r" || cmd == "R") {
    angleGripper = 90; angleMiddle = 90; angleBase = 90;
    servoGripper.write(90); servoMiddle.write(90); servoBase.write(90);
    Serial.println(F("全部归中 90 度"));
    return;
  }

  // 数字 -> 设角度
  int angle = cmd.toInt();
  if (angle < 0 || angle > 180) {
    Serial.println(F("!! 角度范围 0~180"));
    return;
  }

  setSelectedAngle(angle);
}

void setSelectedAngle(int angle) {
  switch (selectedServo) {
    case 0:
      angleGripper = angle;
      servoGripper.write(angle);
      break;
    case 1:
      angleMiddle = angle;
      servoMiddle.write(angle);
      break;
    case 2:
      angleBase = angle;
      servoBase.write(angle);
      break;
  }
  Serial.print(servoNames[selectedServo]);
  Serial.print(F(" -> "));
  Serial.print(angle);
  Serial.println(F(" 度"));
}

void sweepSelected() {
  Serial.print(servoNames[selectedServo]);
  Serial.println(F(" 扫描中 0 -> 180 -> 0 ..."));

  // 0 -> 180
  for (int a = 0; a <= 180; a += 5) {
    setSelectedAngle(a);
    delay(80);
  }
  // 180 -> 0
  for (int a = 180; a >= 0; a -= 5) {
    setSelectedAngle(a);
    delay(80);
  }
  // 回到中间
  setSelectedAngle(90);
  Serial.println(F("扫描完成，已归中"));
}
