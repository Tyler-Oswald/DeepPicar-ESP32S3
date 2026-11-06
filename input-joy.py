# joydev_pygame.py
import pygame

_joystick = None
_initialized = False

EV_BUTTON = 1
EV_AXIS = 2


LEFT_Y = 1
RIGHT_X = 2

_last_axis = {}


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


def read_events():
    """Return a list of all new events since the last call."""
    if not _initialized:
        init()
    events = []
    for event in pygame.event.get():
        if event.type == pygame.JOYAXISMOTION:
            value = int(event.value * 32767)
            if _last_axis.get(event.axis) != value:
                _last_axis[event.axis] = value
                events.append((EV_AXIS, event.axis, value))
        elif event.type == pygame.JOYBUTTONDOWN:
            events.append((EV_BUTTON, event.button, 1))
        elif event.type == pygame.JOYBUTTONUP:
            events.append((EV_BUTTON, event.button, 0))
    return events


def get_throttle(pct_range=100):
    """Get y value from left joystick"""
    if not _initialized:
        init()
    pygame.event.pump()
    return int(-_joystick.get_axis(LEFT_Y) * pct_range)


def get_steering(deg_range=30):
    """Get the X value from the Right stick."""
    if not _initialized:
        init()
    pygame.event.pump()
    return int(_joystick.get_axis(RIGHT_X) * deg_range)


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
                if event.type == pygame.JOYBUTTONDOWN:
                    print("Button %d pressed" % event.button)
                elif event.type == pygame.JOYBUTTONUP:
                    print("Button %d released" % event.button)

            time.sleep(0.01)

    except KeyboardInterrupt:
        pass
    finally:
        stop()