import numpy as np
import requests
from params import URL

_blank = None

def record(on):
    try:
        response = requests.get(
            URL + "/control?var=record&val={}".format(1 if on else 0),
            timeout=2
        )
        response.raise_for_status()
        print("SD recording start command sent" if on else "SD recording stop command sent")
    except requests.RequestException as e:
        print("Record command failed:", e)

def init(res=(320, 240), fps=30, threading=True):
    global _blank
    w, h = res
    _blank = np.zeros((h, w, 3), dtype=np.uint8)
    print("SD recording ready")

def read_frame():
    return _blank

def stop():
    record(False)
