from vex import *

brain = Brain()

def Touching_button(XCorner: int, YCorner: int, Width: int, Height: int) -> bool:
    if (brain.screen.x_position() > XCorner) and (brain.screen.x_position() < XCorner + Width) and (brain.screen.y_position() > YCorner) and (brain.screen.y_position() < YCorner + Height):
        return True
    else:
        return False
    
def Button_loop( XCorner: int, YCorner: int, Width: int, Height: int, FunctionPressed: Callable, FunctionReleased: Callable, Colorpressed: Color, Colorreleased: Color, ColorText: Color,  Text: str,) -> None:
    pressing=False
    
    while True:
        if Touching_button(XCorner, YCorner, Width, Height):

            if not pressing:

                brain.screen.set_fill_color(Colorpressed)
                brain.screen.draw_rectangle(XCorner, YCorner, Width, Height)
                brain.screen.set_pen_color(ColorText)
                brain.screen.print_at(Text, XCorner + (Width / 2), YCorner + (Height / 2), True)

            FunctionPressed()
            pressing=True
        elif pressing:

            brain.screen.set_fill_color(Colorreleased)
            brain.screen.draw_rectangle(XCorner, YCorner, Width, Height)
            brain.screen.set_pen_color(ColorText)
            brain.screen.print_at(Text, XCorner + (Width / 2), YCorner + (Height / 2), True)

            FunctionReleased()
            pressing=False
        
        wait(20, MSEC)

def Make_button(Name: str, XCorner: int, YCorner: int, Width: int, Height: int, Text: str, FunctionPressed: Callable, FunctionReleased: Callable, Colorpressed: Color=Color(0, 255, 0), Colorreleased: Color=Color(0, 0, 255), ColorText: Color=Color(255, 255, 255)) -> None:
    brain.screen.set_fill_color(Colorpressed)
    brain.screen.draw_rectangle(XCorner, YCorner, Width, Height)
    brain.screen.set_pen_color(ColorText)
    brain.screen.print_at(Text, XCorner + (Width / 2), YCorner + (Height / 2), True)
    exec("%s=Thread(Button_loop, (XCorner, YCorner, Width, Height, FunctionPressed, FunctionReleased, Colorpressed, Colorreleased, ColorText, Text))"%(Name))