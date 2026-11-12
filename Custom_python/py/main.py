from vex import *

import time, math, cmath, sys, gc, random

brain = Brain()

motor = Motor(Ports.PORT12, GearSetting.RATIO_18_1, True)
rotation = Rotation(Ports.PORT7, True)
timer = Timer()

def say_hello():
    print("hello")

event = Event(say_hello)

start1 = time.tick_us()  # type: ignore
motor.spin(FORWARD)
end1 = time.tick_us()  # type: ignore

start2 = time.tick_us()  # type: ignore
motor.spin_for(FORWARD, 600, DEGREES)
end2 = time.tick_us()  # type: ignore

start3 = time.tick_us()  # type: ignore
rotation.position()
end3 = time.tick_us()  # type: ignore

start4 = time.tick_us()  # type: ignore
print("This is a test for speed")
end4 = time.tick_us()  # type: ignore

print("time diff motor.spin = %d"%((end1 - start1)))
print("time diff motor.spin_for = %d"%((end2 - start2)))
print("time diff rotation.position = %d"%((end3 - start3)))
print("time diff print = %d"%((end4 - start4)))

brain.screen.print("HI")
event.broadcast()