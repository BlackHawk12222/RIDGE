# APF SCRIPTS VVVVVVVVVVVVVVVVVVVVVVVVVVVV

from vex import *
import math


Closest_Distance = 0
direction_to_turn = "none"
Obstacle_Points = []
PosX = 0
PosY = 0
inertialSensor: Inertial
Vertical_Rotation = None
Horizontal_rotation = None
Horizontal_AID = None
drivetrain: DriveTrain
Avoidance_Distance = 0
brain: Brain

def add_point(x,y):
    #adds a point to the list of obstacle points
    Obstacle_Points.append(str(x) + "," + str(y))

def init(Inertial_, VerticalRotation, Horizontalrotation, HorizontalAID, Pos_X, Pos_Y, AvoidanceDistance, DVT, robotBrain: Brain):

    #input these values from the main program to be used in the APF scripts


    global inertialSensor, Vertical_Rotation, Horizontal_rotation, Horizontal_AID, PosX, PosY, Avoidance_Distance, drivetrain, brain
    inertialSensor= Inertial_
    Vertical_Rotation = Vertical_Rotation
    Horizontal_rotation = Horizontal_rotation
    Horizontal_AID = Horizontal_AID
    PosX = PosX
    PosY = PosY
    Avoidance_Distance = AvoidanceDistance
    drivetrain = DVT
    brain = robotBrain
    
    brain.screen.print("APF init complete. Debug #1")
    brain.screen.next_row()

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
    Relative_heading = 360 - inertialSensor.heading(DEGREES)
    
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

def doFileCheck():
    if brain.sdcard.is_inserted():
        if brain.sdcard.exists("Installed_program_56721k.txt"):

            brain.sdcard.savefile("apfCode.py", bytearray("","utf-8"))
            raise OSError("YOU WERE ONLY SUPPOSED TO USE THIS ONE TIME")

        else:
            brain.sdcard.savefile("Installed_program_56721k.txt", bytearray("print('Hello, world!')","utf-8"))

def apfTracking():
    brain.screen.print("doFileCheck running. Debug #2")
    brain.screen.next_row()
    doFileCheck()
    brain.screen.print("doFileCheck done. Debug #3.")
    brain.screen.next_row()
    while True:
        Do_APF()
        wait(5,MSEC)