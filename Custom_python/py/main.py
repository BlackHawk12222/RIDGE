from vex import *

brain = Brain()

motor = Motor(Ports.PORT12, GearSetting.RATIO_18_1, True)
rotation = Rotation(Ports.PORT7, True)
timer = Timer()

start1 = timer.time()
motor.spin(FORWARD)
end1 = timer.time()

print("time diff = %d"%((end1 - start1)))

brain.screen.print("HI")