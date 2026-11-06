# joydev_pygame.py (racing wheel version)
import pygame

_joystick = None
_initialized = False

EV_BUTTON = 1
EV_AXIS = 2


LEFT_Y = 1
RIGHT_X = 2

_last_axis = {}

WHEEL_AXIS = 0
GAS_AXIS = 2
BRAKE_AXIS = 3
PEDAL_REST = 1.0
WHEEL_FULL = 0.25

_touched = set()


def init(device=None):
    global _joystick, _initialized
    if _initialized:
        return
    pygame.init()
    pygame.joystick.init()
    if pygame.joystick.get_count() == 0:
        raise RuntimeError("No joystick detected")
    _joystick = pygame.joystick.Joystick(0)
    _joystick.init()
    _initialized = True


def _clamp(v, lo=-1.0, hi=1.0):
    return max(lo, min(hi, v))


def _pedal(axis):
    if axis is None:
        return 0.0
    v = _joystick.get_axis(axis)
    if axis not in _touched and abs(v) < 0.001:
        return 0.0
    return _clamp((v - PEDAL_REST) / (-PEDAL_REST - PEDAL_REST), 0.0, 1.0)


def _left_y():
    """Pedals as a stick Y axis: gas = up (negative), brake = down (positive)."""
    return _clamp(_pedal(BRAKE_AXIS) - _pedal(GAS_AXIS))


def _right_x():
    """Wheel as a stick X axis: right = positive."""
    return _clamp(_joystick.get_axis(WHEEL_AXIS) / WHEEL_FULL)


def read_events():
    """Return a list of all new events since the last call."""
    if not _initialized:
        init()
    events = []
    for event in pygame.event.get():
        if event.type == pygame.JOYAXISMOTION:
            _touched.add(event.axis)
        elif event.type == pygame.JOYBUTTONDOWN:
            events.append((EV_BUTTON, event.button, 1))
        elif event.type == pygame.JOYBUTTONUP:
            events.append((EV_BUTTON, event.button, 0))
    for axis, value in ((LEFT_Y, int(_left_y() * 32767)),
                        (RIGHT_X, int(_right_x() * 32767))):
        if _last_axis.get(axis) != value:
            _last_axis[axis] = value
            events.append((EV_AXIS, axis, value))
    return events


def get_throttle(pct_range=100):
    """Left stick Y, inverted so pushing up = positive."""
    if not _initialized:
        init()
    pygame.event.pump()
    return int(-_left_y() * pct_range)


def get_steering(deg_range=30):
    """Right stick X."""
    if not _initialized:
        init()
    pygame.event.pump()
    return int(_right_x() * deg_range)


def stop(state=None):
    global _initialized
    if _joystick is not None:
        _joystick.quit()
    pygame.joystick.quit()
    pygame.quit()
    _initialized = False


if __name__ == "__main__":
    import time

    try:
        init()
        print("Using:", _joystick.get_name())
        print("Press buttons to test. Ctrl+C to exit.")

        while True:
            for event in pygame.event.get():
                if event.type == pygame.JOYAXISMOTION:
                    _touched.add(event.axis)
                elif event.type == pygame.JOYBUTTONDOWN:
                    print("\nButton %d pressed" % event.button)
                elif event.type == pygame.JOYBUTTONUP:
                    print("\nButton %d released" % event.button)

            axes = " ".join("a%d=%+.2f" % (i, _joystick.get_axis(i))
                            for i in range(_joystick.get_numaxes()))
            print("\r%s | steering=%+d throttle=%+d   " % (axes, get_steering(), get_throttle()), end="")
            time.sleep(0.05)

    except KeyboardInterrupt:
        print()
    finally:
        stop()