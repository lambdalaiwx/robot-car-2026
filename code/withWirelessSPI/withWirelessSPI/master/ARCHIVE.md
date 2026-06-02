# master/ — 已归档

此目录中的 `master.ino` 是原无线串口遥控端代码，使用 Arduino UNO + 遥控手柄（双摇杆+按键）。

## 归档原因

遥控方式已升级为**手机蓝牙控制**，不再需要专用的 Arduino 遥控端。

- 手机蓝牙 APP（Bluetooth Serial Controller）替代摇杆和按键
- HC-05/HC-06 蓝牙模块替代原无线串口模块
- 主车端 `slave/in.ino` 无需修改

详见 [docs/bluetooth-setup.md](../docs/bluetooth-setup.md)。

## 此文件保留原因

- 保留原始摇杆控制逻辑作为参考
- 如需恢复物理手柄遥控，可重新使用
