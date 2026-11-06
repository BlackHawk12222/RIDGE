#region VEXcode Generated Robot Configuration
from vex import *
import ACPv1

# Brain should be defined by default
brain=Brain()

# Robot configuration code
controller_1 = Controller(PRIMARY)
controller_interface = Controller(PARTNER)
Right1 = Motor(Ports.PORT17, GearSetting.RATIO_6_1, False)
Right2 = Motor(Ports.PORT18, GearSetting.RATIO_6_1, False)
left1 = Motor(Ports.PORT19, GearSetting.RATIO_6_1, True)
left2 = Motor(Ports.PORT20, GearSetting.RATIO_6_1, True)
inertial = Inertial(Ports.PORT7)
Xodom = Rotation(Ports.PORT5)

# add a small delay to make sure we don't print in the middle of the REPL header
wait(200, MSEC)
# clear the console to make sure we don't have the REPL in the console
print("\033[2J")

#Future aton test
def aton():
    pass

# Aton Functions
comp=ACPv1.start(GearRatio=0.75, Wheelsize_MM=69.85, MotorMax_RPM=600, OdomWheelSize_MM=50.8, StickType="Tank", AtonFunc=aton)

while True:
    wait(20, MSEC)