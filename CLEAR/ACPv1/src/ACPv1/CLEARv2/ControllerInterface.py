from vex import *

ScreenBuffer = []
YPos = 1

def start(controller: Controller):
    controller.screen.clear_screen()
    controller.screen.set_cursor(1, 1)
    controller.screen.print("Starting... CLEARv2 Controler Interface v0.1")

    while True:
        if len(ScreenBuffer) >= 5:
            for _ in range(len(ScreenBuffer) - 6):
                ScreenBuffer.pop(0)
        controller.screen.set_cursor(1, 1)
        controller.screen.print(ScreenBuffer)
        wait(200, MSEC)

def add_to_buffer(data):
    global ScreenBuffer
    ScreenBuffer += [str(data) + "\n"]