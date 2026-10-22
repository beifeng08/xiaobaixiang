"""Load-based fan control example.

Install:
    python -m pip install colorama
    python -m pip install requests

Usage:
    MAKE SURE YOU READ THE WARNING
    python LiquidCool.py

Warning:
    This script is written as an example and may or may not kill your PC!
    Use at your own risk!
"""

import colorama
import json
import math
import time

import requests


URL = "http://127.0.0.1:8085"

pos = lambda x, y: "\x1b[%d;%dH" % (y, x)
clear = lambda: "\033[2J\033[1;1f"


def _check_result(result):
    """Raise if the server returned an error."""
    if result["result"] != "ok":
        message = result["message"].replace("\\n", "\n").replace("\\r", "")
        raise Exception("Server returned error:\n " + message)


def get_value(sensor_id):
    params = dict(id=sensor_id, action="Get")
    resp = requests.post(url=URL + "/Sensor", params=params, timeout=10)
    result = json.loads(resp.text)
    _check_result(result)
    if result["value"] is None:
        return None
    return float(result["value"])


def set_value(sensor_id, sensor_value):
    if sensor_value is None:
        sensor_value = "null"
    params = dict(id=sensor_id, action="Set", value=sensor_value)
    resp = requests.post(url=URL + "/Sensor", params=params, timeout=10)
    result = json.loads(resp.text)
    _check_result(result)


def draw_progress_bar(x, y, description, progress):
    p = "#" * math.floor(progress / 5)
    p1 = "-" * math.ceil((100 - progress) / 5)
    print(
        "{0}{1}[{2}{3}] {4}%   ".format(
            pos(x, y), description, p, p1, math.floor(progress * 100) / 100
        ),
        end="",
    )


def integrate(old_val, new_val):
    return float(old_val) * 0.8 + float(new_val) * 0.2


def main():
    colorama.init()
    print(clear())

    try:
        integrated_cpu = get_value("/intelcpu/0/load/0")
        integrated_gpu = get_value("/nvidiagpu/0/load/0")
        org_fan1 = get_value("/lpc/it8620e/control/0")
        org_fan2 = get_value("/lpc/it8620e/control/1")

        while True:
            cpu_load = get_value("/intelcpu/0/load/0")
            gpu_load = get_value("/nvidiagpu/0/load/0")

            integrated_cpu = integrate(integrated_cpu, cpu_load)
            integrated_gpu = integrate(integrated_gpu, gpu_load)
            fan_val = max(integrated_cpu, integrated_gpu)
            fan_val = math.floor(fan_val * 0.75)

            draw_progress_bar(1, 1, "CPU:  ", cpu_load)
            draw_progress_bar(1, 2, "GPU:  ", gpu_load)
            draw_progress_bar(1, 3, "IGPU: ", integrated_cpu)
            draw_progress_bar(1, 4, "IGPU: ", integrated_gpu)
            draw_progress_bar(1, 5, "Fans: ", fan_val)

            set_value("/lpc/it8620e/control/0", fan_val)
            set_value("/lpc/it8620e/control/1", fan_val)

            print(pos(1, 1))
            time.sleep(1)
    except Exception:
        print(clear())
        set_value("/lpc/it8620e/control/0", org_fan1)
        set_value("/lpc/it8620e/control/1", org_fan2)
        raise


if __name__ == "__main__":
    main()
