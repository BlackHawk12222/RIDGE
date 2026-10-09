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

import APF
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

#controllers
controller_1 = Controller(PRIMARY)

#motors, in order of port, lowest first
DistanceSensor = Distance(Ports.PORT1)#----------------------------ONE
ColorSensor = Optical(Ports.PORT2)#--------------------------------TWO
Bar_Switcher = Motor(Ports.PORT3, GearSetting.RATIO_18_1, False)#--THREE
Horizontal_rotation = Rotation(Ports.PORT4, False)#----------------FOUR
Vertical_Rotation = Rotation(Ports.PORT5, False)#------------------FIVE
Inertial_ = Inertial(Ports.PORT6)#---------------------------------SIX
Horizontal_AID = Rotation(Ports.PORT7, False)#---------------------SEVEN
right_motor_a = Motor(Ports.PORT17, GearSetting.RATIO_6_1, False)#-SEVENTEEN
right_motor_b = Motor(Ports.PORT18, GearSetting.RATIO_6_1, False)#-EIGHTEEN
left_motor_a = Motor(Ports.PORT19, GearSetting.RATIO_6_1, True)#---NINETEEN
left_motor_b = Motor(Ports.PORT20, GearSetting.RATIO_6_1, True)#---TWENTY   

#creating motor grouping
left_drive_smart = MotorGroup(left_motor_a, left_motor_b)
right_drive_smart = MotorGroup(right_motor_a, right_motor_b)
drivetrain = DriveTrain(left_drive_smart, right_drive_smart, 319.19, 330.2, 254, MM, 0.75)


# wait for rotation sensor to fully initialize
wait(30, MSEC)


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
CC_display = Event()
Obstacle_Points = ["0,600",0]
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
X_assist_Current = 0
xScale = 0
X_diff_scaled = 0



#BEGIN CODE HERE ###############################################################

#MISC CODE(custom printing, velocity setting etc.)

def printCS(text):
    #prints to the brain screen, separating new lines at commas
    global temp
    temp = 1
    for repeat_count2 in range(int(len(text))):
        if (text[temp - 1]) == ",":
            brain.screen.next_row()
        else:
            brain.screen.print(text[temp - 1])
        temp = temp + 1
        wait(5, MSEC)




def set_velocity(side, velocity):

    #set the drivetrain velocity for the individual sides of the drivetrain; useful for turning
    if side.lower() == "left":
        left_drive_smart.set_velocity(velocity, PERCENT)
    elif side.lower() == "right":
        right_drive_smart.set_velocity(velocity, PERCENT)
    else:
        left_drive_smart.set_velocity(velocity, PERCENT)
        right_drive_smart.set_velocity(velocity, PERCENT)



def AIM_drive(mm):
    #adding AIM (accurate inertial movement) code here
    pass



#BAR SWITCHING LOGIC SCRIPTS VVVVVVVVVVVVVVVVVVV


def Check_for_bar():
    #checks if an object (the control bar) is close enough. If it is, check to see if it's our own color, and activate the spin command
    check_dist()
    if Dist < 50:
        check_color_redundancy()
    spin_motor()



def check_dist():
    #used in Check_for_bar(): initializes Color_flips to 0(do not spin), then records the distance to the object.
    global Dist, Color_Flips
    Color_Flips = 0
    Dist = DistanceSensor.object_distance(MM)



def check_color_redundancy():
    #used in Check_for_bar(): sets Color_flips to 1(spin motor), then checks to see if the detected object is our color. If it is, set Color_flips to 0(do not spin).
    global Color_Flips
    Color_Flips = 1
    if ColorSensor.color() == Color.BLUE and MyColor == "blue" or ColorSensor.color() == Color.RED and MyColor == "red":
        Color_Flips = 0



def spin_motor():
    #spins Bar_Switcher if Color_fips is 1(spin motor). Continues to do so until distance is too far away(robot has moved away) or color is correct.
    while Color_Flips == 1:
        Bar_Switcher.spin_for(FORWARD, 180, DEGREES)
        wait(0.1, SECONDS)
        check_dist()
        if Dist < 50:
            check_color_redundancy()
    Bar_Switcher.stop()


# APF SCRIPTS VVVVVVVVVVVVVVVVVVVVVVVVVVVVV


def Do_APF():
    # APF - Artificial Polarity Force. Runs all associated APF scripts . Begins by finding the closest obstacle point, noted in OBSTACLE_POINTS[]
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
        else:
            pass



def Find_Closest_Point():
    global Closest_ID, Closest_Distance, temp
    # Finds the closest point (obstacle) to the robot
    
    temp = 1
    Closest_ID = 1
    Closest_Distance = 999999999999
    
    for i in range(int(len(Obstacle_Points))):
        
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



def get_relative_heading(get_relative_heading_heading):

    #corrects the input value to be relative to the robot's heading
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




def directionTo_xy(directionTo_xy__x, directionTo_xy__y):
    #get the distance to the x and y coordinated based on the robot's
    global dx, dy, Direction_To_Point

    #set dx and dy variables to use with aTan2()
    dx = directionTo_xy__x - PosX
    dy = directionTo_xy__y - PosY

    #use aTan2() to calculate the direction to the point
    Direction_To_Point = math.atan2(dy,dx) / math.pi * 180
    #clamp to 360 degrees, just to be safe
    Direction_To_Point = (Direction_To_Point + 360) % 360

    #send it over to get the heading in relation to the robot
    get_relative_heading(Direction_To_Point)




def distanceTo_xy(distanceTo_xy_x, distanceTo_xy_y):
    #uses the Pythagorean theorem to get the distance to a point
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


#ODOMETRY SCRIPTS VVVVVVVVVVVVVVVVVVVVVVVVVVV


def ODOM_loop():
    global Heading, Vertical_current, Horizontal_current, X_assist_Current, Delta_forward, Delta_sideways, Last_forward, Last_sideways, Forward_movement, Sideways_movement, Change_x, Change_y, PosX, PosY
    # Find Heading & set variable to it
    Heading = Inertial_.heading(DEGREES)
    
    # Get current values for the odom pod
    Vertical_current = Vertical_Rotation.position(DEGREES)
    Horizontal_current = Horizontal_rotation.position(DEGREES)
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



def offset_accommodation():
    #calculate the X_diff, which is subracted from the larger tracking wheel to find how much the robot has actually moved sideways; used to eliminate the X-change scripts picking up in-place rotation
    
    global xScale, X_diff, X_diff_scaled, X_ACAOM
    
    #initialize the xScale variable - calculate this here: https://scratch.mit.edu/projects/1375798690/editor/
    xScale = 1.22222
    
    #calculate the difference in the X tracking wheels. Horizontal_AID will be the "smaller" wheel(the one closer to the robot's center of rotation), and Horizontal_rotation the main wheel, farther away from the center of rotation.(I plan on adding the calibration program for this in the future. It's currently a blockly program right now.)
    X_diff = X_assist_Current - Horizontal_current
    
    #set the scaled difference, ready to be subtracted from the Horizontal_rotation sensor.
    X_diff_scaled = X_diff * xScale
    
    #give the X_diff_scaled a better name; X_ACOM is the final value used in the odometry.
    X_ACAOM = X_diff_scaled



#RUNNING SCRIPTS VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV



def color_switching():
    #initialize and run color sensing. Also set drive stopping and set Bar_switcher to 100% velocity
    global MyColor, drivetrain, Bar_Switcher, ColorSensor

    drivetrain.set_stopping(BRAKE)

    Bar_Switcher.set_velocity(100, PERCENT)

    MyColor = "red"

    ColorSensor.set_light(LedStateType.ON)
    ColorSensor.set_light_power(100, PERCENT)

    while True:
        Check_for_bar()
        wait(5, MSEC)




def vs_setup():
    #show setup progression on the brain screen
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




def prog_setup():
    #calibration, scanning, and setup code to run when the program starts
    global SETUP_progression, Avoidance_Distance, Obstacle_Points, PosX, PosY, Last_forward, Last_sideways
    
    #start CC(controller connectivity) display
    CC_display.broadcast()

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
    Last_forward = Vertical_Rotation.position(DEGREES)
    Last_sideways = Horizontal_rotation.position(DEGREES)
    
    SETUP_progression = SETUP_progression + 1
    
    #setup brain screen
    brain.screen.clear_screen()
    brain.screen.set_cursor(1, 1)
    brain.screen.print("Setup complete")
    
    #refer to bootAnim(); presents starting menu: side and color selection, followed by the actual animation
    bootAnim()

    #reset the fill color so as not to interfere with text drawing
    brain.screen.set_fill_color(Color.TRANSPARENT)
    
    #main loop; eventually motor and sensor stats will be here, as well as data bars
    while True:
        ODOM_loop()
        brain.screen.clear_screen()
        brain.screen.set_cursor(1, 1)
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
        brain.screen.render()
        wait(0.05, SECONDS)
        wait(5, MSEC)




def CC_display_callback():

    #connectivity code to show on controller throughout the program
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




def draw_button(draw_button_x, draw_button_y, draw_button_width, draw_button_height):
    # Draws a button on the brain screen and checks if it is pressed
    global button_pressed, Auto_Side, MyColor, v, sizeDrawRound, AnimProgress

    #draw the actual rectangle for the button
    brain.screen.draw_rectangle(draw_button_x, draw_button_y, draw_button_width, draw_button_height)
    
    #default button_pressed to 0(not pressed)
    button_pressed = 0

    #check to see if the button is pressed by checking if the brain screen is touched, and if so, if the X and Y positions fall into this button's area
    if brain.screen.pressing():
        if draw_button_x < brain.screen.x_position() < (draw_button_x + draw_button_width) and draw_button_y < brain.screen.y_position() < (draw_button_y + draw_button_height):
            button_pressed = 1




def bootAnim():
    #User interface and setup complete animation code
    global button_pressed, Auto_Side, MyColor, v, sizeDrawRound, AnimProgress, screen_precision, console_precision
    
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
    
    #clear the screen after side and color selection are compelte
    brain.screen.clear_screen()
    brain.screen.render()
    
    #initialize animation
    sizeDrawRound = 25
    AnimProgress = 0
    v = 0
    
    for i in range(45):

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
        wait(0.03, SECONDS)

    #show the finished animation for a second
    wait(1, SECONDS)

    #clear render the screen
    brain.screen.clear_screen()
    brain.screen.render()




# system event handlers
CC_display(CC_display_callback)


# add 15ms delay to make sure events are registered correctly.
wait(15, MSEC)


#PROGRAM RUN VVVVVVVVVVVVVVVVVVV



PROG = Thread( prog_setup )
VS = Thread( vs_setup )
color_switching()