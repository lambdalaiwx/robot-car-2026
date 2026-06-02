#include <Arduino.h>
#include <U8x8lib.h>
#include <Wire.h>

// --- 屏幕驱动声明 ---
// 默认尝试 SH1106 芯片驱动 (如果你的屏幕是 1.3寸，或者是较新的 0.96寸，通常是这个)
U8X8_SH1106_128X64_NONAME_HW_I2C display(/* reset=*/ U8X8_PIN_NONE);

// 💡 备用驱动：如果你用上面的代码上传后屏幕亮了但是【画面错位/花屏】，
// 请把上面那行前面加 // 注释掉，并把下面这行前面的 // 删掉，换成 SSD1306 驱动再传一次：
// U8X8_SSD1306_128X64_NONAME_HW_I2C display(/* reset=*/ U8X8_PIN_NONE);

void setup(void) {
  Serial.begin(9600);
  Serial.println("===========================");
  Serial.println("  U8g2 屏幕终极点亮测试");
  Serial.println("===========================");
  
  // 初始化屏幕
  display.begin();
  display.setPowerSave(0); // 唤醒屏幕
  
  // 设置字体并显示文字
  display.setFont(u8x8_font_chroma48medium8_r); 
  
  display.drawString(0, 2, "Hello World!");
  display.drawString(0, 4, "Screen is OK!");
  
  Serial.println("指令已发送，请看屏幕有没有出现 Hello World！");
}

void loop(void) {
  delay(1000);
}