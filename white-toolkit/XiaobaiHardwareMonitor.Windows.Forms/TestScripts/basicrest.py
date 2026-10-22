"""A simple test of the REST Like API.

Install:
    python -m pip install requests

Usage:
    python basicrest.py
"""

import json
import time

import requests


URL = "http://127.0.0.1:8085"


def _check_result(result):
    """Raise if the server returned an error."""
    if result["result"] != "ok":
        message = result["message"].replace("\\n", "\n").replace("\\r", "")
        raise Exception("Server returned error:\n " + message)


def find_sensors(node):
    sensors = {}
    if node["Children"]:
        for child in node["Children"]:
            sensors.update(find_sensors(child))
    elif "Type" in node:
        sensors[node["SensorId"]] = node
    return sensors


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


def main():
    print("Fetching all sensor ids:")
    resp = requests.get(url=URL + "/data.json", timeout=10)
    data = json.loads(resp.text)
    sensors = find_sensors(data)

    for key, value in sensors.items():
        v = get_value(key)
        print(key, ":", v)

    # Change the id to one of yours
    print("Setting GPU Fan to full speed!")
    set_value("/nvidiagpu/0/control/0", "100.0")
    time.sleep(10)
    print("Returning GPU Fan speed to default")
    set_value("/nvidiagpu/0/control/0", None)


if __name__ == "__main__":
    main()
