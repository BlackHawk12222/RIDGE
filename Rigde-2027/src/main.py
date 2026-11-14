# ---------------------------------------------------------------------------- #
#                                                                              #
# 	Module:       main.py                                                      #
# 	Author:       Kyle V                                                       #
# 	Created:      7/7/2026, 4:52:46 PM                                         #
# 	Description:  V5 project                                                   #
#                                                                              #
# ---------------------------------------------------------------------------- #

# Library imports
from vex import *

brain = Brain()

if brain.sdcard.is_inserted() and brain.sdcard.exists("ACPv1"):
    import ACPv1

# Actions to do when the program starts.
brain.screen.clear_screen()

# Robot configuration code.

#controllers
controller_1 = Controller(PRIMARY)
Interface =  Controller(PARTNER)

#3-wire ports
intakeLift = DigitalOut(brain.three_wire_port.a)
grabber = DigitalOut(brain.three_wire_port.b)

#other objects, in order of port, lowest first
Yodom = Rotation(Ports.PORT1, False)#------------------------------ONE
Horzontal_rotation = Rotation(Ports.PORT2, False)#-----------------TWO
#------------------------------------------------horizontal_AID on FOURTEEN
ColorSensor = Optical(Ports.PORT4)#-------------------------------FOUR
Bar_Switcher = Motor(Ports.PORT5, GearSetting.RATIO_18_1, False)#--FIVE

DistanceSensor = Distance(Ports.PORT6)#----------------------------SIX

Inertial_ = Inertial(Ports.PORT7)#---------------------------------SEVEN

Intake = Motor(Ports.PORT8)#---------------------------------------EIGHT
#---------------------------------------------------------radio on NINE
Claw = Motor(Ports.PORT10)#----------------------------------------TEN

Horizontal_AID = Rotation(Ports.PORT14, False)#--------------------FOURTEEN

liftOne = Motor(Ports.PORT15, GearSetting.RATIO_6_1, False)#-------FIFTEEN
liftTwo = Motor(Ports.PORT16,GearSetting.RATIO_6_1,True)#---------SIXTEEN
right_motor_a = Motor(Ports.PORT17, GearSetting.RATIO_6_1, False)#-SEVENTEEN
right_motor_b = Motor(Ports.PORT18, GearSetting.RATIO_6_1, False)#-EIGHTEEN
left_motor_a = Motor(Ports.PORT19, GearSetting.RATIO_6_1, True)#---NINETEEN
left_motor_b = Motor(Ports.PORT20, GearSetting.RATIO_6_1, True)#---TWENTY

# Creating motor grouping.
liftGroup = MotorGroup(liftOne,liftTwo)
left_drive_smart = MotorGroup(left_motor_a, left_motor_b)
right_drive_smart = MotorGroup(right_motor_a, right_motor_b)
drivetrain = DriveTrain(left_drive_smart, right_drive_smart, 219.44, 317.5, 254, MM, 0.75)

# Config for Motors
Intake.set_velocity(100, PERCENT)
Claw.set_stopping(HOLD)

# Wait for rotation sensor to fully initialize.
wait(30, MSEC)

# Color to String Helper.
def convert_color_to_string(col: Color) -> str:
    if col == Color.RED:
        return "red"
    if col == Color.GREEN:
        return "green"
    if col == Color.BLUE:
        return "blue"
    if col == Color.WHITE:
        return "white"
    if col == Color.YELLOW:
        return "yellow"
    if col == Color.ORANGE:
        return "orange"
    if col == Color.PURPLE:
        return "purple"
    if col == Color.CYAN:
        return "cyan"
    if col == Color.BLACK:
        return "black"
    if col == Color.TRANSPARENT:
        return "transparent"
    return ""

# add a small delay to make sure we don't print in the middle of the REPL header
wait(200, MSEC)
# clear the console to make sure we don't have the REPL in the console
print("\033[2J")

# define variables used for controlling motors based on controller inputs
drivetrain_l_needs_to_be_stopped_controller_1 = False
drivetrain_r_needs_to_be_stopped_controller_1 = False

#endregion VEXcode Generated Robot Configuration

screen_precision: int = 0
"""Int build into VexCode Python for precision on brain.screen properties."""

console_precision: int = 0
"""Int build into VexCode Python for precision on print()."""

controller_1_precision: int = 0
"""Int build into VexCode Python for precision on controller_1.screen properties."""

CC_display = Event()
"""Event for the Controller response animation."""

lift_run = Event()
"""Event for the Robot Lift's and Claw's Ticks."""

Obstacle_Points = ["0,600",0]
"""Unknown."""

motors: list[Motor]=[]
"""List of detected Motor classes from MC.MotorConfig()."""

motorNames: list[str]=[]
"""List of detected Motor classes names as strings from MC.MotorConfig()."""

motorPorts: list[int]=[]
"""List of detected Motor classes ports as int from MC.MotorConfig()."""

Dist = 0
MyColor = 0
Color_Flips = 0
PosX = 0
PosY = 0
Heading = 0
Last_forward = 0
Last_sideways = 0
Delta_forward = 0
Delta_sideways = 0
Vertical_current = 0
Horizontal_current = 0
Forward_movement = 0
Sideways_movement = 0
Change_x = 0
Change_y = 0
temp = 0
distance_to_point = 0
dx = 0
dy = 0
SETUP_progression = 0
last_heading = 0
delta_heading = 0
Closest_ID = 0
Closest_Distance = 0
Temp2 = 0
i = 0
Coords1 = 0
Coords2 = 0
Avoidance_Distance = 0
Direction_To_Point = 0
Relative_heading = 0
corrected_heading = 0
direction_to_turn = 0
DX_Correction = 0
X_ACAOM = 0
X_diff = 0
oldX_diff = 0
X_assist_Current = 0
xScale = 0
X_diff_scaled = 0
LiftVelocity = 0

setUp = False
"""Bool to check if robot setup is done."""

UseACP = False
"""Bool to check if ACP should be used."""

Loading = False
"""Bool to check if Loading pin and cup into Claw."""

liftUp = False
"""Bool to check if Claw if in Up/Open position."""

RunAuto = False
"""Bool for runnig auto in driver control."""

# BEGIN CODE HERE

class robot():
    """MISC CODE(custom printing, velocity setting etc.)"""

    def __init__(self) -> None:
        self.liftRaised = False
        """Bool For check if Claw is out and Lift if up"""

    def proportional(self, val: float , pwr: int = 2) -> float:
        """
        Returns ((val**pwr)/10**pwr) and returns same sign as val.
        
        ### Args:

        val = float;
        pwr = int : 2;
        
        ### Returns
        float;
        """

        if val < 0:
            return(-1*(val**pwr)/10**pwr)
        
        else:
            return((val**pwr)/10**pwr)
    
    def TankDrive(self, buffer: int = 5) -> None:
        """
        Tank drive code for when ACP is not being used.
        
        ### Args:
        buffer = int : 5;

        ### Returns
        None;
        """

        while True:

            if comp.is_driver_control() and not RunAuto:

                if not self.liftRaised:

                    left_drive_smart.set_velocity(self.proportional(controller_1.axis3.position()), PERCENT)
                    right_drive_smart.set_velocity(self.proportional(controller_1.axis2.position()), PERCENT)

                else:

                    left_drive_smart.set_velocity(controller_1.axis3.position()/2, PERCENT)
                    right_drive_smart.set_velocity(controller_1.axis2.position()/2, PERCENT)

                if math.fabs(controller_1.axis3.position()) < buffer:
                    left_drive_smart.set_velocity(0,PERCENT)

                if math.fabs(controller_1.axis2.position()) < buffer:
                    right_drive_smart.set_velocity(0,PERCENT)

                left_drive_smart.spin(FORWARD)
                right_drive_smart.spin(FORWARD)

            wait(10, MSEC)

    def autoload(self) -> None:
        """
        Loads and lifts a cup and pin stack in the intake.

        ### Args:
        None;

        ### Returns
        None;
        """

        global Loading, liftUp

        while True:
            if controller_1.buttonA.pressing():

                Loading = True
                liftUp = True

                liftGroup.set_velocity(100, PERCENT)
                Claw.set_velocity(100, PERCENT)

                if self.liftRaised:

                    self.liftRaised = False

                    grabber.set(False)
                    liftGroup.spin_to_position(0, DEGREES, 100, PERCENT, False)
                    Claw.spin_to_position(-40, DEGREES)  # Set to -40 to take out slack in Claw.

                    while not liftGroup.position() < 10:
                        wait(5, MSEC)

                else:
                    self.liftRaised = True
                    liftUp = False

                    controller_1.screen.print("running")

                    grabber.set(False)
                    liftGroup.spin_to_position(0, DEGREES)
                    Claw.spin_to_position(-40, DEGREES)  # Set to -40 to take out slack in Claw.

                    while math.fabs(liftGroup.position(DEGREES)) > 10 and math.fabs(Claw.position(DEGREES)) > 10:
                        wait(5, MSEC)

                    Intake.spin(FORWARD)
                    wait(0.5, SECONDS)

                    grabber.set(True)
                    wait(100, MSEC)

                    liftGroup.spin_to_position(700, DEGREES, 100, PERCENT, False)
                    Claw.spin_to_position(205, DEGREES)
                    wait(500, MSEC)

                    liftGroup.stop()
                    Intake.stop()

                Loading = False

            else:
                wait(50, MSEC)

    def clawUp(self) -> None:
        """
        Brings Claw up to score.
        
        ### Args:
        None;

        ### Returns:
        None;
        """

        liftGroup.set_velocity(100, PERCENT)
        Claw.set_velocity(100, PERCENT)
        grabber.set(True)
        wait(100, MSEC)

        liftGroup.spin_to_position(700, DEGREES, 100, PERCENT, False)
        Claw.spin_to_position(190, DEGREES)

        while not liftGroup.is_done():
            wait(10, MSEC)

    def clawDown(self) -> None:
        """
        Returns Claw to start to pickup pin and cup.
        
        ### Args:
        None;

        ### Returns:
        None;
        """

        global liftUp

        self.liftRaised = True
        liftUp = False

        controller_1.screen.print("running")
        grabber.set(False)
        liftGroup.spin_to_position(0, DEGREES)
        Claw.spin_to_position(-40, DEGREES)  # Set to -40 to take out slack in Claw.

        while math.fabs(liftGroup.position(DEGREES)) > 10 and math.fabs(Claw.position(DEGREES)) > 10:
            wait(10, MSEC)

    def clawLoad(self) -> None:
        """
        Brings pin and cup into the claw.
        
        ### Args:
        None;

        ### Returns:
        None;
        """

        Intake.spin(FORWARD)
        wait(0.5, SECONDS)

        grabber.set(True)
        wait(100, MSEC)

        liftGroup.spin_to_position(700, DEGREES, 100, PERCENT, False)
        Claw.spin_to_position(205, DEGREES)
        wait(500, MSEC)

        liftGroup.stop()
        Intake.stop()

    def clawScore(self) -> None:
        """
        Released Claw grabber and move Claw out of the way.

        ### Args:
        None;

        ### Returns:
        None;
        """

        Claw.set_velocity(100,PERCENT)
        grabber.set(False)
        wait(0.1,SECONDS)

        Claw.spin_to_position(300,DEGREES)

class display():
    """Brain.screen Atrabutes and common code functions."""

    def __init__(self, object: Brain) -> None:
        self.BO: Brain = object
        """Brain Object to get Brain Properties"""

    def drawImage(self, name: str, x: int, y: int) -> None:
        """
        Same as brain.screen.draw_image_from_file().
        
        ### Args:
        name = str;
        x = int;
        y = int;

        ### Returns:
        None;
        """

        self.BO.screen.draw_image_from_file(name,x,y)

    def reset(self, render: bool = False) -> None:
        """
        Clears brain screen and resets the cursor.
        
        ### Args:
        render = bool : False;

        ### Returns:
        None;
        """
        
        self.BO.screen.clear_screen()
        self.BO.screen.set_cursor(1,1)
        if render == True:
            self.BO.screen.render()

    def printCS(self, text: str) -> None:
        """
        Takes full string and splits , into new lines.
        
        ### Args:
        text = str;

        ### Returns:
        None
        """

        split = text.split(",")

        for entry in split:
            self.BO.screen.print(entry)
            self.BO.screen.next_row()

    def drawPB(self, x: int, y: int, width: int, height: int, percent: int, border: int, barColor: Color, horizontalBar: bool = False) -> None:
        """
        Draws progress bar for brain screen.
        
        ### Args:
        x = int;
        y = int;
        width = int;
        hight = int;
        percent = int;  # Amount of bar filled in %.
        border = int;
        barColor = Color;
        horizontalBar = bool : False;

        ### Returns:
        None
        """

        if horizontalBar:
            self.BO.screen.set_pen_width(0)
            self.BO.screen.set_fill_color(Color.WHITE)
            self.BO.screen.draw_rectangle(x, y, width + border, height + border)
            self.BO.screen.set_fill_color(barColor)
            self.BO.screen.draw_rectangle(x + border / 2, y + border / 2, width * (percent / 100), height)
        else:
            self.BO.screen.set_pen_width(0)
            self.BO.screen.set_fill_color(Color.WHITE)
            self.BO.screen.draw_rectangle(x, y, width + border, height + border)
            self.BO.screen.set_fill_color(barColor)
            self.BO.screen.draw_rectangle(x + border / 2, y + border / 2 + (height - height * (percent / 100)), width, height * (percent / 100))
        self.BO.screen.set_fill_color(Color.TRANSPARENT)

    def printMotors(self) -> None:
        """
        Prints all motors in motors list.

        ### Args:
        None;

        ### Returns:
        None;
        """

        for i in range(len(motorNames)):
            self.BO.screen.print("%d %s %s %d" %(motorPorts[i], motorNames[i], motors[i].temperature(PERCENT)))
            self.BO.screen.next_row()
            
class MotorTicks:
    """Ticks for Misc. funtions with Motors in loops."""

    def __init__(self):
        self.intakeOn = False
        self.clawOpen = True
        self.Xrelease = True
        self.Uprelease = True
        pass

    def lift_tick(self) -> None:
        """
        Tick in loop for Driver controler for lifting and droping the lift.

        ### Args:
        None;

        ### Returns:
        None;
        """

        if comp.is_driver_control():
            if not Loading:
                if controller_1.buttonL1.pressing():
                    liftGroup.spin(FORWARD)
                elif controller_1.buttonL2.pressing():
                    liftGroup.spin(REVERSE)
                else:
                    liftGroup.stop()

    def intake_tick(self):
        """
        Tick in loop for Driver controler for spinning the intake motor.

        ### Args:
        None;

        ### Returns:
        None;
        """
        
        if not Loading:
            if controller_1.buttonR1.pressing():
                Intake.spin(FORWARD)
            elif controller_1.buttonR2.pressing():
                Intake.spin(REVERSE)
            else:
                Intake.stop()

    def liftTick(self):
        """
        Tick in loop for Driver controler for toggle for the intake hight.

        ### Args:
        None;

        ### Returns:
        None;
        """
        
        if controller_1.buttonUp.pressing():
            if self.Uprelease:
                self.Uprelease = False
                if not self.intakeOn:
                    intakeLift.set(True)
                    self.intakeOn = True
                else:
                    intakeLift.set(False)
                    self.intakeOn = False
        else:
            self.Uprelease = True

    def grabberTick(self):
        """
        Tick in loop for Driver controler for grabber toggle on the Claw.

        ### Args:
        None;

        ### Returns:
        None;
        """

        if controller_1.buttonX.pressing():
            if self.Xrelease:
                self.Xrelease = False
                Claw.set_velocity(100,PERCENT)
                grabber.set(False)
                wait(0.1,SECONDS)
                Claw.spin_to_position(300,DEGREES)
                self.clawOpen = False
        else:
            self.Xrelease = True

class MC:
    """Motor config to find all motors and get there port numbers."""
    
    def MotorConfig(self) -> None:
        """
        Finds and stores all motors configured.
        
        ### Args:
        None;

        ### Returns:
        None;
        """

        global motors, motorNames
        data = dir()

        for item in data:
            try:
                itemType = str(type(eval(item)))
            except NameError:
                continue
            if itemType == "<class 'motor'>":
                motorNames.append(item)
                motors.append(eval(item))
                motorPorts.append(self.getMotorPort(eval(item)))

    def getMotorPort(self, motor: Motor) -> int:
        """
        Finds and stores all motors configured.
        
        ### Args:
        motor = Motor;

        ### Returns:
        int;
        """
        
        for port in range(Ports.PORT1, Ports.PORT20):
            m=Motor(port)
            if m.installed:
                if m.power(PowerUnits.WATT) == motor.power(PowerUnits.WATT):
                    return(port)
                
        return -1

class AIMDrive():
    """Accurate Inertial Movement"""
    def set_velocity(self, side: str, velocity: float) -> None:
        """
        Set the drivetrain velocity for the individual sides of the drivetrain; useful for turning.

        ### Args:
        side = str;
        velocity = float;

        ### Returns:
        None;
        """

        if side.lower() == "left":
            left_drive_smart.set_velocity(velocity, PERCENT)
        elif side.lower() == "right":
            right_drive_smart.set_velocity(velocity, PERCENT)
        else:
            left_drive_smart.set_velocity(velocity, PERCENT)
            right_drive_smart.set_velocity(velocity, PERCENT)

    def resetDrivetrain(self):
        """
        Finds and stores all motors configured.
        
        ### Args:
        None;

        ### Returns:
        None;
        """

        right_drive_smart.set_velocity(100, PERCENT)
        left_drive_smart.set_velocity(100, PERCENT)
        left_drive_smart.stop()
        right_drive_smart.stop()

    def AIM_drive(self, mm: int, driveVelocity: int, err: float = 0.0) -> None:
        """
        Accurate Inertial Movement main function to drive a distance.

        ### Args:
        mm = int;
        driveVelocity = int;
        err = float : 0.0;

        ### Returns:
        None;
        """

        mmToDeg = (mm / ((69.85 * 3.14159) / 360)) / 0.75
        err = 0

        left_drive_smart.set_position(0, DEGREES)
        right_drive_smart.set_position(0, DEGREES)
        Inertial_.set_rotation(0, DEGREES)
        left_drive_smart.spin_for(mmToDeg, DEGREES)
        right_drive_smart.spin_for(mmToDeg, DEGREES)

        while True:
            err = Inertial_.rotation() * 0.5
            left_drive_smart.set_velocity(driveVelocity - err, PERCENT)
            right_drive_smart.set_velocity(driveVelocity + err, PERCENT)

            if math.fabs(mm-right_drive_smart.position(DEGREES)) < 5:
                self.resetDrivetrain()
                break

    def AIM_turn(self, d: float, v: int = 100, p:int = 3) -> None:
        """
        Accurate Inertial Movement main function to turn to a heading.

        ### Args:
        d = float;
        v = int : 100;
        p = int : 3;

        ### Returns:
        None;
        """
        deg = d
        vel = v
        pro = p
        err = 0
        Inertial_.set_rotation(0)
        rot = Inertial_.rotation()
        err = deg - rot
        while not math.fabs(err) < 5:
            rot = Inertial_.rotation()
            err = deg - rot

            drivetrain.set_turn_velocity(err/pro)

            if err/pro < 5:
                drivetrain.set_turn_velocity(5)

            elif err/pro > vel:
                drivetrain.set_turn_velocity(vel)

            
            if deg < 0:
                drivetrain.turn(LEFT)
            else:
                drivetrain.turn(RIGHT)

class BarColor():
    def Check_for_bar(self) -> None:
        """
        Checks if an object (the control bar) is close enough. If it is, check to see if it's our own color, and activate the spin command.
        
        ### Args:
        None;

        ### Returns:
        None;
        """

        self.check_dist()

        if Dist < 240:
            self.check_color_redundancy()
        
        self.spin_motor()

    def check_dist(self) -> None:
        """
        Used in Check_for_bar(): initializes Color_flips to 0(do not spin), then records the distance to the object.
        
        ### Args:
        None;

        ### Returns:
        None;
        """

        global Dist, Color_Flips

        Color_Flips = 0

        Dist = DistanceSensor.object_distance(MM)

    def check_color_redundancy(self) -> None:
        """
        Used in Check_for_bar(): sets Color_flips to 1(spin motor), then checks to see if the detected object is our color. If it is, set Color_flips to 0(do not spin).
        
        ### Args:
        None;

        ### Returns:
        None;
        """
        
        global Color_Flips
        Color_Flips = 1

        if ColorSensor.color() == Color.BLUE and MyColor == "blue" or ColorSensor.color() == Color.RED and MyColor == "red":
            Color_Flips = 0

    def spin_motor(self) -> None:
        """
        Spins Bar_Switcher if Color_fips is 1(spin motor). Continues to do so until distance is too far away(robot has moved away) or color is correct.
        
        ### Args:
        None;

        ### Returns:
        None;
        """

        while Color_Flips == 1:

            Bar_Switcher.spin_for(FORWARD, 100, DEGREES)
            wait(0.15, SECONDS)

            self.check_dist()

            if Dist < 240:
                self.check_color_redundancy()

        Bar_Switcher.stop()

colorBar = BarColor()

# APF SCRIPTS

def Find_Closest_Point() -> None:
    """
    Finds the closest point (obstacle) to the robot.

    ### Args:
    None;

    ### Returns:
    None;
    """

    global Closest_ID, Closest_Distance, temp
    
    temp = 1
    Closest_ID = 1
    Closest_Distance = 999999999999
    
    for _ in range(int(len(Obstacle_Points))):
        
        if Obstacle_Points[temp - 1] == 0:
            #if the object is "0," consider it empty and break
            break
        
        #parse current point
        Parse_all()

        #get the distace to the point
        distanceTo_xy(Coords1, Coords2)

        #if this is the closest point, update the closest point ID and distance variables to match
        if distance_to_point < Closest_Distance:
            Closest_Distance = distance_to_point
            Closest_ID = temp

    #if the point is within the aviodance distance(it's too close), set var temp(tracks which coordinate to parse) to its ID and parse it        
    if Closest_Distance < Avoidance_Distance:
        temp = Closest_ID
        Parse_all()

def Do_APF() -> None:
    """
    APF - Artificial Polarity Force. Runs all associated APF scripts . Begins by finding the closest obstacle point, noted in OBSTACLE_POINTS[].

    ### Args:
    None;

    ### Returns:
    None;
    """

    Find_Closest_Point()

    #after finding the closest point, check to see if it's too close
    if Closest_Distance < Avoidance_Distance:

        #if it is, get the direction (relative to the robot) to the obstacle
        directionTo_xy(Coords1, Coords2)

        if direction_to_turn == "right":

            #turn right, scaled to object distance
            drivetrain.set_turn_velocity((((Avoidance_Distance - Closest_Distance) / Avoidance_Distance) * 100), PERCENT)
            drivetrain.turn(RIGHT)
        elif direction_to_turn == "left":

            #turn left, scaled to object distance
            drivetrain.set_turn_velocity((((Avoidance_Distance - Closest_Distance) / Avoidance_Distance) * 100), PERCENT)
            drivetrain.turn(LEFT)

def get_relative_heading(get_relative_heading_heading: float) -> None:
    """
    Corrects the input value to be relative to the robot's heading.

    ### Args:
    get_relative_heading_heading = float;

    ### Returns:
    None;
    """

    global Relative_heading, corrected_heading, direction_to_turn

    #kind of cheezing the system here(not really, but that sounds funny. So.); I figured out that by subtracting the robot's heading from 360, then adding it to the target heading, you get the fixed heading, which is a lot easier than how I used to do it.
    Relative_heading = 360 - Inertial_.heading(DEGREES)
    
    #clamp the corrected heading to 360 degrees
    corrected_heading = ((Relative_heading + get_relative_heading_heading) + 360) % 360
    
    #sets the direction_to_turn based on the corrected heading
    if 270 > corrected_heading > 90:

        #if the corrected heading falls into a deadzone behind the robot, ignore polarity and allow the robot to continue (this would come into play while scoring, when the robot backs up to a goal)
        direction_to_turn = "none"

    elif math.fabs(corrected_heading) < 3:
        #straight-on scenario; turn right if we're going straight at it
        direction_to_turn = "right"

    elif corrected_heading < 180:
        direction_to_turn = "left"

    else:
        direction_to_turn = "right"

def directionTo_xy(directionTo_xy__x: float, directionTo_xy__y: float) -> None:
    """
    Get the distance to the x and y coordinated based on the robot's.

    ### Args:
    directionTo_xy__x = float;
    directionTo_xy__y = float;

    ### Returns:
    None;
    """

    global dx, dy, Direction_To_Point

    #set dx and dy variables to use with aTan2()
    dx = directionTo_xy__x - PosX
    dy = directionTo_xy__y - PosY

    #use aTan2() to calculate the direction to the point
    Direction_To_Point = math.atan2(dy, dx) / math.pi * 180
    #clamp to 360 degrees, just to be safe
    Direction_To_Point = (Direction_To_Point + 360) % 360

    #send it over to get the heading in relation to the robot
    get_relative_heading(Direction_To_Point)

def distanceTo_xy(distanceTo_xy_x, distanceTo_xy_y) -> None:
    """
    Uses the Pythagorean theorem to get the distance to a point.

    ### Args:
    directionTo_xy__x = float;
    directionTo_xy__y = float;

    ### Returns:
    None;
    """

    global dx, dy, distance_to_point

    #calculate dx and dy, to be used in the equation
    dx = distanceTo_xy_x - PosX
    dy = distanceTo_xy_y - PosY

    #set distance_to_point to the output of the calculation
    distance_to_point = math.sqrt(dx * dx + dy * dy)

def Parse_all():
    global Temp2, i, Coords1, Coords2
    # Parse the both halves of the coordinates (X axis and Y axis) and set Coords1 and Coords2
    Coords1, Coords2 = Obstacle_Points[temp - 1].split(",")
    Coords1 = int(Coords1)
    Coords2 = int(Coords2)

#ODOMETRY SCRIPTS

def ODOM_loop() -> None:
    """
    Main loop For ODOM.

    ### Args:
    None;

    ### Returns:
    None;
    """

    global Heading, Vertical_current, Horizontal_current, X_assist_Current, Delta_forward, Delta_sideways, Last_forward, Last_sideways, Forward_movement, Sideways_movement, Change_x, Change_y, PosX, PosY

    # Find Heading & set variable to it
    Heading = Inertial_.heading(DEGREES)
    
    # Get current values for the odom pod
    Vertical_current = Yodom.position(DEGREES)
    Horizontal_current = Horzontal_rotation.position(DEGREES)
    X_assist_Current = Horizontal_AID.position(DEGREES)
    
    #run offset accomodation, which calculates X_ACOM, which is subtracted from the Horizontal_rotation to cancel out in-place rotation
    offset_accommodation()

    # Get Delta values for the odom pod
    Delta_forward = Vertical_current - Last_forward
    Delta_sideways = (Horizontal_current - Last_sideways) - X_ACAOM
    
    # Set previous values for calculating delta values next iteration
    Last_forward = Vertical_current
    Last_sideways = Horizontal_current
    
    # Find out how far we've traveled in each direction
    Forward_movement = Delta_forward * 0.66463333333
    Sideways_movement = Delta_sideways * 0.66463333333
    
    # Set real X and Y changes
    Change_x = Forward_movement * math.sin(Heading / 180.0 * math.pi) + Sideways_movement * math.cos(Heading / 180.0 * math.pi)
    Change_y = Forward_movement * math.cos(Heading / 180.0 * math.pi) - Sideways_movement * math.sin(Heading / 180.0 * math.pi)
    
    # Update field position
    PosX = PosX + Change_x
    PosY = PosY + -1 *Change_y
    wait(5, MSEC)

def offset_accommodation() -> None:
    """
    Calculate the X_diff, which is subracted from the larger tracking wheel to find how much the robot has actually moved sideways; used to eliminate the X-change scripts picking up in-place rotation.

    ### Args:
    None;

    ### Returns:
    None;
    """

    X_ACOM = 0
    
    global xScale, X_diff, X_diff_scaled, X_ACAOM,oldX_diff
    
    #initialize the xScale variable - calculate this here: https://scratch.mit.edu/projects/1375798690/
    xScale = 0.36
    
    #calculate the difference in the X tracking wheels. Horizontal_AID will be the "smaller" wheel(the one closer to the robot's center of rotation), and Horizontal_rotation the main wheel, farther away from the center of rotation.(I plan on adding the calibration program for this in the future. It's currently a blockly program right now.)
    X_diff = (Horizontal_current - X_assist_Current) - oldX_diff 
    
    #set the scaled difference, ready to be subtracted from the Horizontal_rotation sensor.
    X_diff_scaled = X_diff * xScale
    
    #give the X_diff_scaled a better name; X_ACOM is the final value used in the odometry.
    X_ACAOM = X_diff_scaled
    oldX_diff += X_diff

#RUNNING SCRIPTS

def color_switching() -> None:
    """
    Initialize and run color sensing. Also set drive stopping and set Bar_switcher to 100% velocity.

    ### Args:
    None;

    ### Returns:
    None;
    """

    global MyColor, drivetrain, Bar_Switcher, ColorSensor

    Bar_Switcher.set_velocity(100, PERCENT)

    #MyColor = "red"

    ColorSensor.set_light(LedStateType.ON)
    ColorSensor.set_light_power(100, PERCENT)

    while True:
        colorBar.Check_for_bar()
        wait(5, MSEC)

def vs_setup() -> None:
    """
    Show setup progression on the brain screen.

    ### Args:
    None;

    ### Returns:
    None;
    """

    global SETUP_progression

    while not SETUP_progression > 0:
        brain.screen.clear_screen()
        brain.screen.set_cursor(1, 1)
        brain.screen.print("Setup: calibrating...",SETUP_progression)
        brain.screen.next_row()
        brain.screen.render()
        wait(5, MSEC)

    while not SETUP_progression > 1:
        brain.screen.clear_screen()
        brain.screen.set_cursor(1, 1)
        brain.screen.print("Setup: Scanning Autonomous POINTS...",SETUP_progression)
        brain.screen.next_row()
        brain.screen.render()
        wait(5, MSEC)

    while not SETUP_progression > 2:
        brain.screen.clear_screen()
        brain.screen.set_cursor(1, 1)
        brain.screen.print("Setup: Finishing...",SETUP_progression)
        brain.screen.next_row()
        brain.screen.render()
        wait(5, MSEC)

def prog_setup() -> None:
    """
    Calibration, scanning, and setup code to run when the program starts.

    ### Args:
    None;

    ### Args:
    None;
    """

    global SETUP_progression, Avoidance_Distance, Obstacle_Points, PosX, PosY, Last_forward, Last_sideways, setUp

    setUp = True
    
    #start CC(controller connectivity) display
    CC_display.broadcast()
    lift_run.broadcast()

    #initialize setup progression variable
    SETUP_progression = 1

    #first step of progression: initialize the inertial sensor
    Inertial_.calibrate()
    while Inertial_.is_calibrating():
        sleep(50)
    #next step, shown by SETUP_progression
    SETUP_progression = SETUP_progression + 1

    Avoidance_Distance = 100
    
    SETUP_progression = SETUP_progression + 1
    
    #initalize variables
    PosX = 0
    PosY = 0
    Last_forward = Yodom.position(DEGREES)
    Last_sideways = Horzontal_rotation.position(DEGREES)
    
    SETUP_progression = SETUP_progression + 1
    
    #setup brain screen
    brain.screen.clear_screen()
    brain.screen.set_cursor(1, 1)
    brain.screen.print("Setup complete")
    
    #refer to bootAnim(); presents starting menu: side and color selection, followed by the actual animation
    bootAnim()
    setUp = False

    #reset the fill color so as not to interfere with text drawing
    brain.screen.set_fill_color(Color.TRANSPARENT)

    #create the display object for the brain
    screen = display(brain)
    mc=MC()
    #main loop; eventually motor and sensor stats will be here, as well as data bars
    Horizontal_AID.set_position(0,DEGREES)
    Horzontal_rotation.set_position(0,DEGREES)
    Yodom.set_position(0,DEGREES)
    while True:
        ODOM_loop()
        screen.reset()
        screen.drawImage("images/RidgeIcon.png",100,0)
        brain.screen.print(str("horizontal current: ") + str(Horizontal_current))
        brain.screen.next_row()
        brain.screen.print(str("current vertical: ") + str(Vertical_current))
        brain.screen.next_row()
        brain.screen.next_row()
        brain.screen.print(str("Y position: ") + str(PosY))
        brain.screen.next_row()
        brain.screen.print(str("X position: ") + str(PosX))
        brain.screen.next_row()
        brain.screen.print(str("Heading: ") + str(Heading))
        brain.screen.next_row()
        brain.screen.print(str("Delta sideways") + str(Delta_forward))
        brain.screen.next_row()
        brain.screen.print(str("Delta vertical") + str(Delta_sideways))
        brain.screen.next_row()
        brain.screen.print(str("XACOM") + str(X_ACAOM))
        brain.screen.next_row()
        brain.screen.print(str("XDIFF ") + str(X_diff))
        brain.screen.render()
        wait(5, MSEC)

def CC_display_callback() -> None:
    """
    Connectivity code to show on controller throughout the program.

    ### Args:
    None;

    ### Returns:
    None;
    """

    while True:
        controller_1.screen.clear_row(3)
        controller_1.screen.set_cursor(controller_1.screen.row(), 1)
        controller_1.screen.set_cursor(3, 1)
        controller_1.screen.print("^")
        wait(0.5, SECONDS)
        
        controller_1.screen.clear_row(3)
        controller_1.screen.set_cursor(controller_1.screen.row(), 1)
        controller_1.screen.set_cursor(3, 1)
        controller_1.screen.print(">")
        wait(0.5, SECONDS)

        controller_1.screen.clear_row(3)
        controller_1.screen.set_cursor(controller_1.screen.row(), 1)
        controller_1.screen.set_cursor(3, 1)
        controller_1.screen.print("v")
        wait(0.5, SECONDS)

        controller_1.screen.clear_row(3)
        controller_1.screen.set_cursor(controller_1.screen.row(), 1)
        controller_1.screen.set_cursor(3, 1)
        controller_1.screen.print("<")
        wait(0.5, SECONDS)

def draw_button(draw_button_x: int, draw_button_y: int, draw_button_width: int, draw_button_height: int) -> None:
    """
    Draws a button on the brain screen and checks if it is pressed.

    ### Args:
    draw_button_x = int; draw_button_y = int; draw_button_width = int; draw_button_height = int;

    ### Returns:
    None;
    """

    global button_pressed, Auto_Side, MyColor, v, sizeDrawRound, AnimProgress

    #draw the actual rectangle for the button
    brain.screen.draw_rectangle(draw_button_x, draw_button_y, draw_button_width, draw_button_height)
    
    #default button_pressed to 0(not pressed)
    button_pressed = 0

    #check to see if the button is pressed by checking if the brain screen is touched, and if so, if the X and Y positions fall into this button's area
    if brain.screen.pressing():
        if draw_button_x < brain.screen.x_position() < (draw_button_x + draw_button_width) and draw_button_y < brain.screen.y_position() < (draw_button_y + draw_button_height):
            button_pressed = 1

def bootAnim() -> None:
    """
    User interface and setup complete animation code
    
    ### Args:
    None;

    ### Returns:
    None;
    """

    global button_pressed, Auto_Side, MyColor, v, sizeDrawRound, AnimProgress, screen_precision, console_precision, UseACP, RunAuto
    
    #print out side selection menu
    while True:
        
        #initialize brain screen each loop and print text to it
        brain.screen.clear_screen()
        brain.screen.set_cursor(1, 1)
        brain.screen.print("Pick left/right")

        #Set pen and fill colors for this button
        brain.screen.set_pen_color(Color.GREEN)
        brain.screen.set_fill_color(Color.TRANSPARENT)
        
        #use the draw_button() function to put a button on the screen and check if it's being pressed
        draw_button(100, 100, 100, 100)

        #if the button is being pressed(in this case, the left side button), set Auto_Side to "l" and stop this loop, moving on to color selection
        if button_pressed == 1:
            Auto_Side = "l"
            break
        
        #draw the right side button; refer to left side button for how this works
        brain.screen.set_pen_color(Color.PURPLE)
        brain.screen.set_fill_color(Color.TRANSPARENT)

        draw_button(210, 100, 100, 100)
        
        if button_pressed == 1:
            Auto_Side = "r"
            break
        
        #separate from the button drawing, render the screen, showing everything that's been drawn
        brain.screen.render()
        wait(5, MSEC)
    
    #clear the brain screen and wait until it's not being pressed before moving on to color selection. This avoids having problems with a single touch registering both side and color
    brain.screen.clear_screen()
    while brain.screen.pressing():
        wait(5, MSEC)

    while True:
        #draw color selection buttons. Refer to side selection button drawing for how this works.
        brain.screen.clear_screen()
        brain.screen.set_cursor(1, 1)

        brain.screen.print("Pick Red/Blue")
        
        brain.screen.set_pen_color(Color.RED)
        brain.screen.set_fill_color(Color.TRANSPARENT)
        
        draw_button(100, 100, 100, 100)
        
        if button_pressed == 1:
            MyColor = "red"
            break
        
        brain.screen.set_pen_color(Color.BLUE)
        brain.screen.set_fill_color(Color.TRANSPARENT)
        
        draw_button(210, 100, 100, 100)
        
        if button_pressed == 1:
            MyColor = "blue"
            break
        
        brain.screen.render()
        wait(5, MSEC)
        #clear the brain screen and wait until it's not being pressed before moving on to color selection. This avoids having problems with a single touch registering both side and color
    
    brain.screen.clear_screen()
    while brain.screen.pressing():
        wait(5, MSEC)
    if brain.sdcard.is_inserted():
        while True:
            #draw color selection buttons. Refer to side selection button drawing for how this works.
            brain.screen.clear_screen()
            brain.screen.set_cursor(1, 1)

            brain.screen.print("use ACP?")
            
            brain.screen.set_pen_color(Color.CYAN)
            brain.screen.set_fill_color(Color.TRANSPARENT)
            
            draw_button(100, 100, 100, 100)
            
            if button_pressed == 1:
                UseACP = True
                break
            
            brain.screen.set_pen_color(Color.RED)
            brain.screen.set_fill_color(Color.TRANSPARENT)
            
            draw_button(210, 100, 100, 100)
            
            if button_pressed == 1:
                UseACP = False
                break
            
            brain.screen.render()
            wait(5, MSEC)
    if not comp.is_field_control():
        while brain.screen.pressing():
            wait(5, MSEC)
        while True:
            #draw color selection buttons. Refer to side selection button drawing for how this works.
            brain.screen.clear_screen()
            brain.screen.set_cursor(1, 1)

            brain.screen.print("Run auto?")
            
            brain.screen.set_pen_color(Color.GREEN)
            brain.screen.set_fill_color(Color.TRANSPARENT)
            
            draw_button(100, 100, 100, 100)
            
            if button_pressed == 1:
                RunAuto = True
                break
            
            brain.screen.set_pen_color(Color.RED)
            brain.screen.set_fill_color(Color.TRANSPARENT)
            
            draw_button(210, 100, 100, 100)
            
            if button_pressed == 1:
                RunAuto = False
                break
            
            brain.screen.render()
            wait(5, MSEC)
    #clear the screen after side and color selection are compelte
    brain.screen.clear_screen()
    brain.screen.render()
    
    #initialize animation
    sizeDrawRound = 25
    AnimProgress = 0
    v = 0
    
    for _ in range(45):

        #clear the screen
        brain.screen.clear_screen()
        
        #use spring physics to create a nice animation for a green circle bouncing to size
        v += (100 - sizeDrawRound) * 0.5
        v = v * 0.7
        
        #change the size of said circle be the velocity
        sizeDrawRound = sizeDrawRound + v
        
        #draw the actual circle
        brain.screen.set_pen_color(Color.GREEN)
        brain.screen.set_fill_color(Color.GREEN)
        
        brain.screen.draw_circle(225, 125, sizeDrawRound)
        
        #initialize to draw the white checkmark
        brain.screen.set_pen_width(26)
        brain.screen.set_pen_color(Color.WHITE)
        brain.screen.set_fill_color(Color.WHITE)
        
        #this part figures out how far the animation is in progress, and draws white lines of the correct size and at the right time based on that
        if AnimProgress < 6:
            #first, left-hand side line
            brain.screen.draw_line(150, 100, 150 + 75 * (AnimProgress / 5), 100 + 75 * (AnimProgress / 5))

        elif AnimProgress < 11:
            #a small circle to make the otherwise messed up bottom of the check mark look okay, plus earlier line as new one in progress
            brain.screen.draw_line(150, 100, 225, 175)
            brain.screen.draw_circle(225, 175, 1)
            brain.screen.draw_line(225, 175, 225 + 100 * (AnimProgress / 10), 175 - 100 * (AnimProgress / 10))

        else:
            #all lines and the circle, waiting for the animation to end
            brain.screen.draw_line(150, 100, 225, 175)
            brain.screen.draw_circle(225, 175, 1)
            brain.screen.draw_line(225, 175, 325, 75)

        #reset pen width
        brain.screen.set_pen_width(0)
        
        #render the screen so everything shows up
        brain.screen.render()

        #change the AnimProgress by 1 to signify the next frame
        AnimProgress = AnimProgress + 1

        #lock the animation to 30 fps
        wait(0.03333, SECONDS)

    #show the finished animation for a second
    wait(1, SECONDS)

    #clear render the screen
    brain.screen.clear_screen()
    brain.screen.render()


def liftTick_callback() -> None:
    """
    Main loop for all ticks for Driver Control.

    ### Args:
    None;

    ### Returns:
    None;
    """

    MT = MotorTicks()
    liftGroup.set_velocity(100,PERCENT)
    liftGroup.set_stopping(HOLD)
    Claw.spin_to_position(-20,DEGREES)
    while True:
        MT.lift_tick()
        MT.intake_tick()
        MT.liftTick()
        MT.grabberTick()

# Event activation
CC_display(CC_display_callback)

lift_run(liftTick_callback)

# add 15ms delay to make sure events are registered correctly.
wait(15, MSEC)

#Start APF
brain.screen.next_row()
brain.screen.print("debug started. If nothing appears after this, APF has encountered an error.")
brain.screen.next_row()

if brain.sdcard.is_inserted():
    #apfCode.init(Inertial_, Vertical_Rotation, Horizontal_rotation, Horizontal_AID, PosX, PosY, Avoidance_Distance, drivetrain, brain, 7764)
    #apf = Thread(apfCode.apfTracking())
    pass

brain.screen.next_row()
brain.screen.print("Debug for APF finished.")
brain.screen.next_row()

#Activate auto program; currently, this actually runs in ACPv1
def auto() -> None:
    """
    Aton script used in the Auto period.
    
    ### Args:
    None;

    ### Returns:
    None;
    """

    color = Thread( color_switching )
    AIM = AIMDrive()

    global MyColor

    Inertial_.reset_heading()

    # Temperary Auto setup to make Aton
    MyColor = "red"

    # Inital Right turn for color bar.
    right_drive_smart.spin(FORWARD, 10, PERCENT)
    wait(0.5,SECONDS)
    drivetrain.stop()
    wait(1,SECONDS)

    # Return to center before moving
    CurrentHeading = Inertial_.heading()
    if CurrentHeading > 180:  # make sure we dont turn more than 180.
        drivetrain.turn_for(RIGHT, 360 - CurrentHeading, DEGREES, 100, PERCENT, True)
    else:
        drivetrain.turn_for(LEFT, CurrentHeading, DEGREES, 100, PERCENT, True)

    # Drive Forward to setup for turn to right pin slot.
    drivetrain.set_drive_velocity(80, PERCENT)
    drivetrain.set_turn_velocity(80, PERCENT)
    drivetrain.drive_for(REVERSE, 260, MM, 80, PERCENT)

    wait(50, MSEC)  # Added wait to stableize robot before turn

    # Turn to Right pin slot.
    CurrentHeading = Inertial_.heading()
    if CurrentHeading > 180:  # make sure we dont turn more than 180.
        drivetrain.turn_for(RIGHT, 80 - (360 - CurrentHeading), DEGREES, 50, PERCENT, True)
    else:
        drivetrain.turn_for(RIGHT, 80 - CurrentHeading, DEGREES, 50, PERCENT, True)

    # Bring Claw up to get ready to drop pre load pin
    K56721.clawUp()

    # Waits for drive train to stop before drop.
    while not drivetrain.is_done():
        wait(5,MSEC)

    # Drive forward to right pin slot.
    drivetrain.drive_for(REVERSE, 300, MM)
    wait(1,SECONDS)
    drivetrain.drive_for(REVERSE, 100, MM, 20, PERCENT, False)
    wait(0.4, SECONDS)
    # Score preload pin
    liftGroup.spin_to_position(-500, DEGREES, 100, PERCENT)
    K56721.clawScore()

    # return lift to up position
    wait(100, MSEC)
    drivetrain.drive_for(FORWARD, 200, MM)
    liftGroup.spin_to_position(0, DEGREES, 100, PERCENT)
    K56721.clawDown()

    # Stops Color turn Thread.
    color.stop()

def driver():
    """
    Driver Control init.

    ### Args:
    None;

    ### Returns:
    None;
    """

    claw  = Thread(K56721.autoload)
    drive = Thread(lambda: K56721.TankDrive(2))
    color_switching()

#PROGRAM RUN

K56721 = robot()
grabber.set(True)
PROG = Thread( prog_setup )
VS = Thread( vs_setup )

comp = Competition(driver,auto)

if comp.is_driver_control():
    if brain.sdcard.is_inserted() and UseACP:
        comp=ACPv1.start(GearRatio = 0.75, Wheelsize_MM = 69.85, MotorMax_RPM = 600, OdomWheelSize_MM = 50.8, StickType = "Tank", AtonFunc = auto)
    else:
        wait(1, SECONDS)
        while not setUp:
            wait(5, MSEC)
            controller_1.screen.print(".")
        if RunAuto:
            controller_1.screen.print("auto")
            auto()
            RunAut0 = False
        claw  = Thread(K56721.autoload)
        drive = Thread(lambda: K56721.TankDrive(2))
        color_switching()