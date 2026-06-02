import time
from pyb import UART

# 1. 初始化串口 (UART 3: P4->TX 接 Nano A0, P5->RX 接 Nano A1)
uart = UART(3, 9600)

print("OpenMV 通信测试模块已启动。等待大鱼/Nano板呼叫...")

input_buffer = ""

while(True):
    # 2. 检查串口是否有数据
    if uart.any():
        # 核心修正：彻底删掉 decode 里面不支持的 errors='ignore' 参数
        # 在 MicroPython 中，直接调用 .decode() 默认就是 utf-8 解码
        char = uart.read(1).decode()

        if char != '\n':
            input_buffer += char
        else:
            # 遇到换行符，说明收到完整指令
            input_buffer = input_buffer.strip()

            if input_buffer == "PING":
                print("收到来自 Nano 板的 PING! 立刻回复 PONG...")
                uart.write("PONG\n") # 发送回复

            input_buffer = "" # 清空缓存，准备下一次接收

    time.sleep_ms(10)
