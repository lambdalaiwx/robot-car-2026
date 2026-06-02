import sensor, image, time
from pyb import UART

# 1. 初始化摄像头
sensor.reset()
sensor.set_pixformat(sensor.GRAYSCALE)
sensor.set_framesize(sensor.VGA)
sensor.set_windowing((320, 240))
sensor.skip_frames(time = 2000)
sensor.set_auto_gain(False)
sensor.set_auto_whitebal(False)

# 2. 初始化串口 (UART 3: P4->TX, P5->RX)
# 安全波特率设为 9600
uart = UART(3, 9600, timeout=10)

print("OpenMV Initialization Complete. Waiting for Bigfish command...")

scan_flag = False
input_buffer = ""

while(True):
    # 3. 检查大鱼板有没有发来命令
    if uart.any():
        char = uart.read(1).decode('utf-8', errors='ignore')
        if char != '\n':
            input_buffer += char
        else:
            input_buffer = input_buffer.strip()
            if input_buffer == "SCAN":
                print("Received 'SCAN' command! Start scanning...")
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

                print("Scan Success! Content:", message)

                # 将扫码内容发回大鱼板，末尾加 \n
                uart.write(message + '\n')
                scan_flag = False
                break

    time.sleep_ms(10)
