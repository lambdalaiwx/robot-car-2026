import sensor, image, time
from pyb import UART

# 1. 初始化摄像头
sensor.reset()
sensor.set_pixformat(sensor.GRAYSCALE) # 灰度图扫码更快
sensor.set_framesize(sensor.VGA)
sensor.set_windowing((320, 240))       # 截取中心，提升帧率
sensor.skip_frames(time = 2000)
sensor.set_auto_gain(False)
sensor.set_auto_whitebal(False)

# 2. 初始化串口 (UART 3: P4->TX 接 Nano A0, P5->RX 接 Nano A1)
# ⚠️ 这里是经过验证的安全配置，无 timeout 参数
uart = UART(3, 9600)

print("OpenMV 视觉模块已就绪。等待 Nano 发送 SCAN 指令...")

scan_flag = False
input_buffer = ""

while(True):
    # 3. 检查 Nano 板有没有发来命令
    if uart.any():
        # ⚠️ 这里是经过验证的安全解码，去掉了错误的 errors='ignore'
        char = uart.read(1).decode()

        if char != '\n':
            input_buffer += char
        else:
            input_buffer = input_buffer.strip()
            # 收到 Nano 板发来的扫码触发指令
            if input_buffer == "SCAN":
                print("收到 'SCAN' 指令! 立刻启动扫描...")
                scan_flag = True
            input_buffer = ""

    # 4. 执行扫码逻辑
    if scan_flag:
        img = sensor.snapshot()
        qrcodes = img.find_qrcodes()

        if qrcodes:
            for code in qrcodes:
                img.draw_rectangle(code.rect(), color = 127)
                message = code.payload().strip()

                print("✅ 扫码成功! 识别内容:", message)

                # 将扫码内容发回 Nano 板，末尾加 \n
                uart.write(message + '\n')

                # 扫码成功后，关闭扫码锁，重新进入静默等待状态
                scan_flag = False
                break

    time.sleep_ms(10)
