from vex import *
import CLA, ASAP

def _none():
    pass

comp= Competition(_none, _none)
cla: Thread

def start(GearRatio, Wheelsize_MM, MotorMax_RPM, OdomWheelSize_MM, StickType="Tank", AtonFunc=_none) -> Competition:
    global comp, cla

    ObjList=dir()
    RightMotors: list[Motor]=[]
    LeftMotors: list[Motor]=[]

    for item in ObjList:
        try:
            item_type=str(type(eval(item)))
        except NameError:
            continue
        
        if item_type == "<class 'controller'>":
            controller=eval(item)
        elif item_type == "<class 'motor'>":
            if "Right" in item or "right" in item:
                RightMotors+=[eval(item)]
            elif "Left" in item or "left" in item:
                LeftMotors+=[eval(item)]
        elif item_type == "<class 'rotation'>" and ("Xodom" in item or "XOdom" in item or "xodom" in item):
            XOdom=eval(item)
    
    del ObjList

    def _driver():
        ASAP.Start(LeftMotors, RightMotors, GearRatio, Wheelsize_MM, MotorMax_RPM, controller, XOdom, OdomWheelSize_MM, StickType)

    comp=Competition(_driver, AtonFunc)

    cla=CLA.start()

    return comp     