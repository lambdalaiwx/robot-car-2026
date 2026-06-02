import sensor, image, time
from pyb import UART

# 1. 初始化串口3，波特率 115200
# 硬件对应引脚: P4 (TX), P5 (RX)
uart = UART(3, 115200)

# 2. 初始化摄像头
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time = 2000)

clock = time.clock()
last_send_time = 0

print("OpenMV UART Ready!")

while(True):
    clock.tick()
    img = sensor.snapshot()

    # 假设这里是你的视觉处理逻辑...
    # 我们模拟生成一对目标坐标 x=150, y=120
    target_x = 150
    target_y = 120

    # 3. 使用时间戳控制发送频率 (非阻塞延时，防止画面卡死)
    current_time = time.ticks_ms()
    if (current_time - last_send_time) > 1000: # 每 1000 毫秒发送一次

        # 将数据格式化为字符串，并在末尾加上 '\n' 换行符
        data_str = "X:{},Y:{}\n".format(target_x, target_y)

        # 发送给 Arduino
        uart.write(data_str)
        print("发送数据:", data_str.strip()) # 电脑端监视器打印

        last_send_time = current_time
