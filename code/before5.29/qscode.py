import sensor, image, time, gc
from pyb import UART

# 1. 初始化串口通信
uart = UART(3, 115200)

# 2. 初始化摄像头传感器
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time = 2000)

print("摄像头准备就绪，请出示二维码...")

# 新增：记录上一次发送数据的时间戳
last_send_time = 0

# 3. 主循环
while(True):
    # 拍一张照片
    img = sensor.snapshot()

    # 在照片里寻找所有的二维码
    for code in img.find_qrcodes():

        # 画红框，方便对准
        img.draw_rectangle(code.rect(), color = (255, 0, 0))
        qr_text = code.payload()

        # 【核心优化】：获取当前运行的毫秒数
        current_time = time.ticks_ms()

        # 判断：如果现在距离上一次发送的时间超过了 1000 毫秒 (1秒)
        if (current_time - last_send_time) > 1000:
            print("扫码成功，内容是:", qr_text)
            uart.write(qr_text + '\n')

            # 更新上一次发送的时间戳
            last_send_time = current_time

    # 【保命符】：强制进行内存垃圾回收，防止内存撑爆卡死
    gc.collect()
