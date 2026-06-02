import sensor, image, time
from pyb import UART

# 1. 初始化摄像头 (必须改为 RGB 彩色模式)
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)  # 使用 QVGA (320x240)，兼顾扫码和色块帧率
sensor.skip_frames(time = 2000)
sensor.set_auto_gain(False)        # 颜色识别必须关闭自动增益
sensor.set_auto_whitebal(False)    # 颜色识别必须关闭白平衡

uart = UART(3, 9600)

# 2. 颜色 LAB 阈值 (请根据赛场光线微调)
red_threshold   = (30, 100, 15, 127, 15, 127)   # 1: 红色
yellow_threshold= (30, 100, -20, 20, 30, 127)   # 2: 黄色
blue_threshold  = (0, 50, -128, 30, -128, -20)  # 3: 蓝色

current_mode = "IDLE"  # 当前任务模式: IDLE, SCAN, FIND_1, FIND_2, FIND_3
input_buffer = ""

print("OpenMV 视觉中枢启动，等待指令...")

while(True):
    # --- 阶段 A：接收 Arduino 指令 ---
    if uart.any():
        char = uart.read(1).decode()
        if char != '\n':
            input_buffer += char
        else:
            input_buffer = input_buffer.strip()
            print("收到指令:", input_buffer)
            if input_buffer == "SCAN":
                current_mode = "SCAN"
            elif input_buffer in ["FIND_1", "FIND_2", "FIND_3"]:
                current_mode = input_buffer
            input_buffer = ""

    img = sensor.snapshot()

    # --- 阶段 B：执行二维码扫描 ---
    if current_mode == "SCAN":
        for code in img.find_qrcodes():
            msg = code.payload().strip()
            # 假设扫到 "123"
            print("✅ 扫码成功:", msg)
            uart.write(msg + '\n')
            current_mode = "IDLE" # 扫完进入静默，等下一步指示
            break

    # --- 阶段 C：执行颜色识别与对齐 ---
    elif current_mode.startswith("FIND_"):
        target_threshold = red_threshold
        if current_mode == "FIND_2": target_threshold = yellow_threshold
        if current_mode == "FIND_3": target_threshold = blue_threshold

        blobs = img.find_blobs([target_threshold], pixels_threshold=200, area_threshold=200, merge=True)

        if blobs:
            # 找到最大的色块
            largest_blob = max(blobs, key=lambda b: b.pixels())
            img.draw_rectangle(largest_blob.rect(), color=(0,255,0))
            img.draw_cross(largest_blob.cx(), largest_blob.cy(), color=(0,255,0))

            # 判断逻辑：如果色块中心在画面中间区域，且面积足够大（说明车开到了物料面前）
            # QVGA 宽度是 320。中间大概是 120 ~ 200
            if (120 < largest_blob.cx() < 200) and (largest_blob.area() > 3000):
                print("🎯 目标已锁定，通知 Arduino 抓取！")
                uart.write("OK\n")
                current_mode = "IDLE" # 通知完一次就休息，等 Arduino 抓完发下一个指令

    time.sleep_ms(10)
