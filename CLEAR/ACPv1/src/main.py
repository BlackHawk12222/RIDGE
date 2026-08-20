#region VEXcode Generated Robot Configuration
from vex import *
import ACPv1

# Brain should be defined by default
brain=Brain()

# Robot configuration code
controller_1 = Controller(PRIMARY)
Right1 = Motor(Ports.PORT13, GearSetting.RATIO_6_1, True)
Right2 = Motor(Ports.PORT14, GearSetting.RATIO_6_1, False)
left1 = Motor(Ports.PORT11, GearSetting.RATIO_6_1, False)
left2 = Motor(Ports.PORT12, GearSetting.RATIO_6_1, True)
OtherMotor=Motor(Ports.PORT2)
inertial = Inertial(Ports.PORT4)
Xodom = Rotation(Ports.PORT1)

# add a small delay to make sure we don't print in the middle of the REPL header
wait(200, MSEC)
# clear the console to make sure we don't have the REPL in the console
print("\033[2J")

inertial.calibrate()

#Future aton test
def aton():
    pass

# Aton Functions
comp=ACPv1.start(GearRatio=0.75, Wheelsize_MM=69.85, MotorMax_RPM=600, OdomWheelSize_MM=50.8, StickType="Arcade", AtonFunc=aton)