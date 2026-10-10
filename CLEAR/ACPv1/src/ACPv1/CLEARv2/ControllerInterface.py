from vex import *

brain = Brain()

ScreenBuffer = []
YPos = len(ScreenBuffer) - 1

def start(controller: Controller):
    global YPos
    OldYPos = 1

    controller.screen.clear_screen()
    controller.screen.set_cursor(1, 1)
    controller.screen.print("Starting. v1.0")
    wait(1, SECONDS)

    selection=0
    selected=0

    LeftTeamPoints=0
    RightTeamPoints=0
    OldLeftTeamPoints=0
    OldRightTeamPoints=0

    OldCurrent=0
    OldCapacity=0
    OldVoltage=0
    
    last_known_buffer = []

    while True:
        if controller.buttonA.pressing() and selection == 0:
            selection=1
        elif controller.buttonA.pressing() and selection==1:
            selection=2
        elif controller.buttonA.pressing() and selection==2:
            selection=0

        while controller.buttonA.pressing():
            wait(20, MSEC)

        # Keep only the last 30 items
        while len(ScreenBuffer) > 30:
            ScreenBuffer.pop(0)

        if selection == 0:
            if selected != 0:
                controller.screen.clear_screen()
                YPos=len(ScreenBuffer)-1
                OldYPos=-1
                selected=0
                wait(50, MSEC)

            if len(ScreenBuffer) > len(last_known_buffer):
                # Calculate the maximum scroll position to bring the newest item to the bottom row
                YPos = max(1, len(ScreenBuffer) - 2)

            # Button presses change the scrolling position
            if controller.buttonUp.pressing():
                YPos -= 1  # Scrolling up decreases the starting row index

                while controller.buttonUp.pressing():
                    wait(20, MSEC)

            if controller.buttonDown.pressing():
                YPos += 1  # Scrolling down increases the starting row index

                while controller.buttonUp.pressing():
                    wait(20, MSEC)
            

            # BOUNDARY CHECK: Prevent scrolling past the limits of your buffer
            # The maximum top row index possible is: (Total Items - 3 available screen rows)
            max_scroll = max(1, len(ScreenBuffer) - 2)
            if YPos < 1:
                YPos = 1
            elif YPos > max_scroll:
                YPos = max_scroll
                
            # Redraw the screen if the buffer changed or the user scrolled
            if ScreenBuffer != last_known_buffer or YPos != OldYPos:
                
                # Loop through the 3 available physical lines on the controller screen
                for i in range(3):
                    controller.screen.set_cursor(i + 1, 1)
                    
                    # Calculate exactly which item in the buffer belongs to this screen line
                    buffer_index = (YPos - 1) + i
                    
                    # Check if this calculated index actually exists in the buffer
                    if buffer_index < len(ScreenBuffer):
                        message = str(ScreenBuffer[buffer_index])
                        padded_message = message + " " * (20 - len(message)) # Pad to clear old text
                        controller.screen.print(padded_message)
                    else:
                        # Clear empty lines on the screen if the buffer has fewer than 3 items
                        controller.screen.print("                    ") 
                
                last_known_buffer = ScreenBuffer.copy()
                OldYPos = YPos
        elif selection == 1:
            if selected != 1:
                controller.screen.clear_screen()
                controller.screen.set_cursor(3, 1)
                controller.screen.print(RightTeamPoints)
                controller.screen.set_cursor(1, 1)
                controller.screen.print(LeftTeamPoints)
                selected= 1
                wait(50, MSEC)

            if controller.buttonR1.pressing():
                RightTeamPoints+=5

                while controller.buttonR1.pressing():
                    wait(20, MSEC)

            if controller.buttonR2.pressing():
                RightTeamPoints-=5

                while controller.buttonR2.pressing():
                    wait(20, MSEC)

            if controller.buttonL1.pressing():
                LeftTeamPoints+=5

                while controller.buttonL1.pressing():
                    wait(20, MSEC)

            if controller.buttonL2.pressing():
                LeftTeamPoints-=5

                while controller.buttonL2.pressing():
                    wait(20, MSEC)

            if RightTeamPoints != OldRightTeamPoints:
                controller.screen.set_cursor(3,1)
                controller.screen.print(str(RightTeamPoints) + "          ")
                OldRightTeamPoints = RightTeamPoints
                wait(50, MSEC)

            if LeftTeamPoints != OldLeftTeamPoints:
                controller.screen.set_cursor(1,1)
                controller.screen.print(str(LeftTeamPoints) + "          ")
                OldRightTeamPoints = LeftTeamPoints
        elif selection == 2:
            if selected != 2:
                controller.screen.clear_screen()
                OldVoltage=0
                OldCurrent=0
                OldCapacity=0
                selected=2
                wait(50, MSEC)

            Capacity=brain.battery.capacity()
            Current=brain.battery.current(CurrentUnits.AMP)
            Voltage=brain.battery.voltage(VOLT)

            if Capacity != OldCapacity:
                controller.screen.set_cursor(1, 1)
                controller.screen.print("%d%%"%(Capacity))
                OldCapacity=Capacity
                wait(50, MSEC)

            if Current != OldCurrent:
                controller.screen.set_cursor(2, 1)
                controller.screen.print("%02.1f/20A"%(Current))
                OldCurrent=Current
                wait(50, MSEC)

            if Voltage != OldVoltage:
                controller.screen.set_cursor(3, 1)
                controller.screen.print("%02.1fV"%(Voltage))
                OldVoltage=Voltage

        wait(50, MSEC) # Lower overall wait time for snappier button tracking

def add_to_buffer(data):
    global ScreenBuffer
    ScreenBuffer += [str(data) + "\n"]