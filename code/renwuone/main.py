import sensor, image, time
from pyb import UART

# 1. 初始化摄像头 (直接使用 QVGA 灰度图，帧率更高，处理速度极快)
sensor.reset()
sensor.set_pixformat(sensor.GRAYSCALE)
sensor.set_framesize(sensor.QVGA)      # 320x240 直接出图，减少处理延迟
sensor.skip_frames(time = 2000)
sensor.set_auto_gain(False)
sensor.set_auto_whitebal(False)

# 2. 定义二维码主体的灰度阈值
# 二维码整体偏黑，在灰度模式下，黑位一般在 0 ~ 80 之间 (根据赛场光线微调)
# 只要是这个区间内的暗色区域，都会被视作色块
QR_BLACK_THRESHOLD = (0, 80)

uart = UART(3, 9600)
print("OpenMV 黑色色块预警系统已就绪...")

# 状态锁：True 寻找黑色轮廓阴影（高速状态）；False 精准扫码（减速状态）
is_searching_blob = True

while(True):
    img = sensor.snapshot()

    if is_searching_blob:
        # --- 阶段 1：利用黑色色块进行高速预警 (无视运动模糊) ---
        # pixels_threshold 和 area_threshold 设为 500，过滤掉远处微小的杂质噪点
        blobs = img.find_blobs([QR_BLACK_THRESHOLD], pixels_threshold=500, area_threshold=500, merge=True)

        for b in blobs:
            # 验证长宽比，二维码是正方形，比例应该接近 1.0 (容错范围 0.7 ~ 1.4)
            aspect_ratio = b.w() / b.h()
            if 0.7 < aspect_ratio < 1.4 and b.w() > 40:

                # 框出这个黑色阴影区域
                img.draw_rectangle(b.rect(), color = 255)
                img.draw_cross(b.cx(), b.cy(), color = 255)

                print("⚠️ [预警] 捕获到前方黑色目标块，通知小车减速！")
                uart.write("SLOW\n")

                # 立刻关掉色块搜索，进入阶段 2 准备扫码，防止重复发送 SLOW 导致串口拥堵
                is_searching_blob = False
                break

    else:
        # --- 阶段 2：小车减速后，画面恢复清晰，开始精准扫码 ---
        qrcodes = img.find_qrcodes()

        if qrcodes:
            for code in qrcodes:
                img.draw_rectangle(code.rect(), color = 255)
                message = code.payload().strip()

                print("✅ [成功] 成功在低速下解析二维码:", message)
                # 将最终数据发回 Arduino (带有互换方向后的直行与刹车逻辑)
                uart.write(message + '\n')

                # 扫码彻底完成后进入死循环休眠，等待重新通电或复位
                print(">>> 任务结束，视觉中枢进入休眠 <<<")
                while(True):
                    time.sleep_ms(1000)

    time.sleep_ms(5) # 极短延迟，保证系统最高循环帧率
