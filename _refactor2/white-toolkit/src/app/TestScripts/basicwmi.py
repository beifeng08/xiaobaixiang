"""WMI sensor enumeration example.

Install:
    python.exe -m pip install pypiwin32
    python.exe -m pip install wmi

Usage:
    !! Make sure XiaobaiHardwareMonitor is running !!
    python basicwmi.py
"""

import wmi


hwmon = wmi.WMI(namespace=r"root\XiaobaiHardwareMonitor")
sensors = hwmon.Sensor(SensorType="Control")

for sensor in sensors:
    print(sensor)
