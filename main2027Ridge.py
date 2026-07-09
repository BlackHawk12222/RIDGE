# ---------------------------------------------------------------------------- #
#                                                                              #
# 	Module:       main.py                                                      #
# 	Author:       Kyle V                                                       #
# 	Created:      7/7/2026, 4:52:46 PM                                         #
# 	Description:  V5 project                                                   #
#                                                                              #
# ---------------------------------------------------------------------------- #

# Library imports
from os import urandom

from vex import *

brain = Brain()

def autonomous():
    brain.screen.clear_screen()
    brain.screen.print("autonomous code")
    # place automonous code here

def user_control():
    brain.screen.clear_screen()
    brain.screen.print("driver control")
    # place driver control in this while loop
    while True:
        wait(20, MSEC)

# create competition instance
comp = Competition(user_control, autonomous)

# actions to do when the program starts
brain.screen.clear_screen()

# Robot configuration code
ColorSensor = Optical(Ports.PORT2)
DistanceSensor = Distance(Ports.PORT1)
Horizontal_rotation = Rotation(Ports.PORT4, False)
Vertical_Rotation = Rotation(Ports.PORT5, False)
Inertial_ = Inertial(Ports.PORT6)
Bar_Switcher = Motor(Ports.PORT3, GearSetting.RATIO_18_1, False)
controller_1 = Controller(PRIMARY)
left_motor_a = Motor(Ports.PORT19, GearSetting.RATIO_6_1, True)
left_motor_b = Motor(Ports.PORT20, GearSetting.RATIO_6_1, True)
left_drive_smart = MotorGroup(left_motor_a, left_motor_b)
right_motor_a = Motor(Ports.PORT17, GearSetting.RATIO_6_1, False)
right_motor_b = Motor(Ports.PORT18, GearSetting.RATIO_6_1, False)
right_drive_smart = MotorGroup(right_motor_a, right_motor_b)
drivetrain = DriveTrain(left_drive_smart, right_drive_smart, 319.19, 330.2, 254, MM, 0.75)
Horizontal_AID = Rotation(Ports.PORT7, False)


# wait for rotation sensor to fully initialize
wait(30, MSEC)


# Make random actually random
def initializeRandomSeed():
    wait(100, MSEC)
    random = brain.battery.voltage(MV) + brain.battery.current(CurrentUnits.AMP) * 100 + brain.timer.system_high_res()
    urandom.seed(int(random))
      
# Set random seed 
initializeRandomSeed()


# Color to String Helper
def convert_color_to_string(col):
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

def play_vexcode_sound(sound_name):
    # Helper to make playing sounds from the V5 in VEXcode easier and
    # keeps the code cleaner by making it clear what is happening.
    print("VEXPlaySound:" + sound_name)
    wait(5, MSEC)

# add a small delay to make sure we don't print in the middle of the REPL header
wait(200, MSEC)
# clear the console to make sure we don't have the REPL in the console
print("\033[2J")



# define variables used for controlling motors based on controller inputs
drivetrain_l_needs_to_be_stopped_controller_1 = False
drivetrain_r_needs_to_be_stopped_controller_1 = False

# define a task that will handle monitoring inputs from controller_1
def rc_auto_loop_function_controller_1():
    global drivetrain_l_needs_to_be_stopped_controller_1, drivetrain_r_needs_to_be_stopped_controller_1, remote_control_code_enabled
    # process the controller input every 20 milliseconds
    # update the motors based on the input values
    while True:
        if remote_control_code_enabled:
            
            # calculate the drivetrain motor velocities from the controller joystick axies
            # left = axis3 + axis1
            # right = axis3 - axis1
            drivetrain_left_side_speed = controller_1.axis3.position() + controller_1.axis1.position()
            drivetrain_right_side_speed = controller_1.axis3.position() - controller_1.axis1.position()
            
            # check if the value is inside of the deadband range
            if drivetrain_left_side_speed < 5 and drivetrain_left_side_speed > -5:
                # check if the left motor has already been stopped
                if drivetrain_l_needs_to_be_stopped_controller_1:
                    # stop the left drive motor
                    left_drive_smart.stop()
                    # tell the code that the left motor has been stopped
                    drivetrain_l_needs_to_be_stopped_controller_1 = False
            else:
                # reset the toggle so that the deadband code knows to stop the left motor next
                # time the input is in the deadband range
                drivetrain_l_needs_to_be_stopped_controller_1 = True
            # check if the value is inside of the deadband range
            if drivetrain_right_side_speed < 5 and drivetrain_right_side_speed > -5:
                # check if the right motor has already been stopped
                if drivetrain_r_needs_to_be_stopped_controller_1:
                    # stop the right drive motor
                    right_drive_smart.stop()
                    # tell the code that the right motor has been stopped
                    drivetrain_r_needs_to_be_stopped_controller_1 = False
            else:
                # reset the toggle so that the deadband code knows to stop the right motor next
                # time the input is in the deadband range
                drivetrain_r_needs_to_be_stopped_controller_1 = True
            
            # only tell the left drive motor to spin if the values are not in the deadband range
            if drivetrain_l_needs_to_be_stopped_controller_1:
                left_drive_smart.set_velocity(drivetrain_left_side_speed, PERCENT)
                left_drive_smart.spin(FORWARD)
            # only tell the right drive motor to spin if the values are not in the deadband range
            if drivetrain_r_needs_to_be_stopped_controller_1:
                right_drive_smart.set_velocity(drivetrain_right_side_speed, PERCENT)
                right_drive_smart.spin(FORWARD)
        # wait before repeating the process
        wait(20, MSEC)

# define variable for remote controller enable/disable
remote_control_code_enabled = True

rc_auto_loop_thread_controller_1 = Thread(rc_auto_loop_function_controller_1)

#endregion VEXcode Generated Robot Configuration

screen_precision = 0
console_precision = 0
controller_1_precision = 0
my_event = Event()
POINTS = ["0,0","0,20","20,20"]
Obstacle_Points = ["0,0"]
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
points_in_POINTS = 0
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
X_Rot_Accomodate = 0
p1_p2_diff__X__ = 0
X_assist_Current = 0
P1_P2_DIVISOR = 0
P2_X_scaled = 0

def scan_points():
    global points_in_POINTS, temp
    points_in_POINTS = 0
    temp = 1
    for repeat_count in range(int(len(POINTS))):
        if not POINTS[temp - 1] == 0:
            points_in_POINTS = points_in_POINTS + 1
        temp = temp + 1
        wait(5, MSEC)

def print_to_screen_commaSeparator(print_to_screen_text_separate_new_lines_with__22__22__text):
    global temp
    temp = 1
    for repeat_count2 in range(int(len(print_to_screen_text_separate_new_lines_with__22__22__text))):
        if (print_to_screen_text_separate_new_lines_with__22__22__text[temp - 1]) == ",":
            brain.screen.next_row()
        else:
            brain.screen.print(print_to_screen_text_separate_new_lines_with__22__22__text[temp - 1])
        temp = temp + 1
        wait(5, MSEC)

def Check_for_bar():
    check_dist()
    if Dist < 50:
        check_color_redundancy()
    spin_motor()

def check_dist():
    global Dist
    Color_Flips = 0
    Dist = DistanceSensor.object_distance(MM)

def check_color_redundancy():
    global Color_Flips
    Color_Flips = 1
    if ColorSensor.color() == Color.BLUE and MyColor == "blue" or ColorSensor.color() == Color.RED and MyColor == "red":
        Color_Flips = 0

def Do_APF():
    # APF - Artificial Polarity Force
    Find_Closest_Point()
    if Closest_Distance < Avoidance_Distance:
        Get_Direction_To_X_Y_x_y(Coords1, Coords2)
        if direction_to_turn == "right":
            drivetrain.set_turn_velocity((((Avoidance_Distance - Closest_Distance) / Avoidance_Distance) * 100), PERCENT)
        elif direction_to_turn == "left":
            drivetrain.set_turn_velocity((0 - ((Avoidance_Distance - Closest_Distance) / Avoidance_Distance) * 100), PERCENT)
        else:
            pass
        drivetrain.turn(RIGHT)

def spin_motor():
    if Color_Flips == 1:
        Bar_Switcher.spin_for(FORWARD, 180, DEGREES)
        wait(0.1, SECONDS)
        check_dist()
        if Dist < 50:
            check_color_redundancy()
        spin_motor()
    else:
        Bar_Switcher.stop()

def Find_Closest_Point():
    global Closest_ID, Closest_Distance, temp
    # Finds the closest point (obstacle) to the robot
    temp = 1
    Closest_ID = 1
    Closest_Distance = 999999999999
    for repeat_count3 in range(int(len(Obstacle_Points))):
        if Obstacle_Points[temp - 1] == 0:
            break
        Parse_A___closest_point()
        Parse_B___closest_point()
        Get_distance_to_x___y(Coords1, Coords2)
        if distance_to_point < Closest_Distance:
            Closest_Distance = distance_to_point
            Closest_ID = temp
        wait(5, MSEC)
    if Closest_Distance < Avoidance_Distance:
        temp = Closest_ID
        Parse_A___closest_point()
        Parse_B___closest_point()

def Parse_A___closest_point():
    global Temp2, i, Coords1
    # Parse the first half of the coordinates (X axis)
    Temp2 = ""
    i = 1
    for repeat_count4 in range(int(len(Obstacle_Points[temp - 1]))):
        if (Obstacle_Points[temp - 1][i - 1]) == ",":
            i = i + 1
            Coords1 = Temp2
            break
        else:
            Temp2 = str(Temp2) + str(Obstacle_Points[temp - 1][i - 1])
        i = i + 1
        wait(5, MSEC)

def Parse_B___closest_point():
    global Temp2, i, Coords2
    # Parse the Y axis from entry
    Temp2 = ""
    for repeat_count5 in range(int((len(Obstacle_Points[temp - 1])) - (i - 1))):
        Temp2 = str(Temp2) + str(Obstacle_Points[temp - 1][i - 1])
        i = i + 1
        wait(5, MSEC)
    Coords2 = Temp2

def ODOM_loop():
    global Heading, Vertical_current, Horizontal_current, X_assist_Current, Delta_forward, Delta_sideways, Last_forward, Last_sideways, Forward_movement, Sideways_movement, Change_x, Change_y, PosX, PosY
    # Find Heading & set variable to it
    Heading = Inertial_.heading(DEGREES)
    # Get current values for the odom pod
    Vertical_current = Vertical_Rotation.position(DEGREES)
    Horizontal_current = Horizontal_rotation.position(DEGREES)
    X_assist_Current = Horizontal_AID.position(DEGREES)
    # Accommodate for in - place rotation by multiplying delta heading by 2.12 (where 2.12*90=90 rot from horizontal wheel)
    offset_accommodation()
    # Get Delta values for the odom pod
    Delta_forward = Vertical_current - Last_forward
    Delta_sideways = (Horizontal_current - Last_sideways) - X_Rot_Accomodate
    # Set previous value
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
    PosY = PosY + Change_y

def offset_accommodation():
    global P1_P2_DIVISOR, p1_p2_diff__X__, P2_X_scaled, X_Rot_Accomodate
    P1_P2_DIVISOR = 2.5
    p1_p2_diff__X__ = X_assist_Current - Horizontal_current
    P2_X_scaled = p1_p2_diff__X__ / P1_P2_DIVISOR
    X_Rot_Accomodate = P2_X_scaled

def Get_heading_relative_to_robot(Get_heading_relative_to_robot__heading):
    global Relative_heading, corrected_heading, direction_to_turn
    Relative_heading = 360 - Inertial_.heading(DEGREES)
    corrected_heading = ((Relative_heading + Get_heading_relative_to_robot__heading) + 360) % 360
    if 270 > corrected_heading > 90:
        direction_to_turn = "none"
    elif math.fabs(corrected_heading) < 3:
        direction_to_turn = "right"
    elif corrected_heading < 180:
        direction_to_turn = "left"
    else:
        direction_to_turn = "right"

def Get_Direction_To_X_Y_x_y(Get_Direction_To_X_Y_x_y__x, Get_Direction_To_X_Y_x_y__y):
    global dx, dy, DX_Correction, Direction_To_Point
    dx = Get_Direction_To_X_Y_x_y__x - PosX
    dy = Get_Direction_To_X_Y_x_y__y - PosY
    if dx == 0:
        DX_Correction = 1
    else:
        DX_Correction = 0
    if dx < DX_Correction:
        Direction_To_Point = 180 + math.atan(dy / dx) / math.pi * 180
    else:
        Direction_To_Point = math.atan(dy / dx) / math.pi * 180
    Direction_To_Point = (Direction_To_Point + 360) % 360
    Get_heading_relative_to_robot(Direction_To_Point)

def Get_distance_to_x___y(Get_distance_to_x___y__x, Get_distance_to_x___y__y):
    global dx, dy, distance_to_point
    dx = Get_distance_to_x___y__x - PosX
    dy = Get_distance_to_x___y__y - PosY
    distance_to_point = math.sqrt(dx * dx + dy * dy)

def when_started1():
    global MyColor, drivetrain, Bar_Switcher, ColorSensor
    drivetrain.set_stopping(COAST)
    Bar_Switcher.set_velocity(100, PERCENT)
    MyColor = "red"
    ColorSensor.set_light(LedStateType.ON)
    ColorSensor.set_light_power(100, PERCENT)
    while True:
        Check_for_bar()
        wait(5, MSEC)

def when_started2():
    global SETUP_progression
    SETUP_progression = 0
    while not SETUP_progression > 0:
        brain.screen.clear_screen()
        brain.screen.set_cursor(1, 1)
        brain.screen.print("Setup: calibrating...")
        brain.screen.next_row()
        brain.screen.render()
        wait(5, MSEC)
    while not SETUP_progression > 1:
        brain.screen.clear_screen()
        brain.screen.set_cursor(1, 1)
        brain.screen.print("Setup: Scanning Autonomous POINTS...")
        brain.screen.next_row()
        brain.screen.render()
        wait(5, MSEC)
    while not SETUP_progression > 2:
        brain.screen.clear_screen()
        brain.screen.set_cursor(1, 1)
        brain.screen.print("Setup: Finishing...")
        brain.screen.next_row()
        brain.screen.render()
        wait(5, MSEC)

def when_started3():
    global SETUP_progression, Avoidance_Distance, Obstacle_Points, POINTS, PosX, PosY, Last_forward, Last_sideways
    my_event.broadcast()
    Inertial_.calibrate()
    while Inertial_.is_calibrating():
        sleep(50)
    SETUP_progression = SETUP_progression + 1
    # Add points to the list; leave all "empty" ones at zero
    Avoidance_Distance = 100
    scan_points()
    SETUP_progression = SETUP_progression + 1
    PosX = 0
    PosY = 0
    Last_forward = Vertical_Rotation.position(DEGREES)
    Last_sideways = Horizontal_rotation.position(DEGREES)
    SETUP_progression = SETUP_progression + 1
    while True:
        ODOM_loop()
        brain.screen.clear_screen()
        brain.screen.set_cursor(1, 1)
        brain.screen.print(str("horizontal current: ") + str(Horizontal_current))
        brain.screen.next_row()
        brain.screen.print(str("current vertical: ") + str(Vertical_current))
        brain.screen.next_row()
        brain.screen.print(str("X position: ") + str(PosX))
        brain.screen.next_row()
        brain.screen.print(str("Y position: ") + str(PosY))
        brain.screen.next_row()
        brain.screen.print(str("Heading: ") + str(Heading))
        brain.screen.next_row()
        brain.screen.print(str("Delta sideways") + str(Delta_forward))
        brain.screen.next_row()
        brain.screen.print(str("Delta vertical") + str(Delta_sideways))
        brain.screen.next_row()
        brain.screen.render()
        wait(0.05, SECONDS)
        wait(5, MSEC)

def my_event_callback_0():
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
        wait(5, MSEC)

# system event handlers
my_event(my_event_callback_0)
# add 15ms delay to make sure events are registered correctly.
wait(15, MSEC)

ws2 = Thread( when_started2 )
ws3 = Thread( when_started3 )
when_started1()