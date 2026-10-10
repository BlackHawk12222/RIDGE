from vex import *

from ustruct import pack_into

brain = Brain()
timer=Timer()

class Encode:
    def __init__(self):
        self.axis2History=0
        self.axis3History=0
        self.degreesoffsetright=0
        self.degreesoffsetleft=0
        self.codeobjectoffset=0

    def rightmoveCLEAR(self, Motors: List[Motor], Velocity: int):
        for motor in Motors:
            motor.set_velocity(Velocity, PERCENT)
        
    def leftmoveCLEAR(self, Motors: List[Motor], Velocity: int):
        for motor in Motors:
            motor.set_velocity(Velocity, PERCENT)
    
    def rightDegreesCLEAR(self, Motors: List[Motor], degrees):
        for motor in Motors:
            motor.spin_for(FORWARD, degrees, DEGREES)

    def leftDegreesCELAR(self, Motors: List[Motor], degrees):
        for motor in Motors:
            motor.spin_for(FORWARD, degrees, DEGREES)

    def run(self, Name: str):
        exec(brain.sdcard.loadfile(Name + ".txt").decode("utf-8"))

    def encode(self, Name: str, RightMotorsName: List[str], LeftMotorsName: List[str], dotank=True):

        prelist:List[List[str]]=[]
        file=brain.sdcard.loadfile("RecordingData.csv").decode("utf-8").split("\n")
        for line in file:
            if line:
                prelist.append(line.split(','))

        codeobject=bytearray(50000)
        
        for i in range(len(prelist)):
            axis1=int(prelist[i][0].replace("A1", ""))
            axis2=int(prelist[i][1].replace("A2", ""))
            axis3=int(prelist[i][2].replace("A3", ""))
            axis4=int(prelist[i][3].replace("A4", ""))
            timestamp=int(prelist[i][4].replace("T", ""))
            rightposition=int(prelist[i][6].replace("RP", ""))
            leftposition=int(prelist[i][7].replace("LP", ""))
            pressedbuttons: List[str] = prelist[i][5].replace("BP", "").split(":")
            print(axis1, axis2, axis3, axis4, timestamp, pressedbuttons, rightposition, leftposition)

            if dotank:
                codestring=b", encode.rightmoveCLEAR(%s, %s), encode.leftmoveCLEAR(%s, %s)"%(RightMotorsName, axis2, LeftMotorsName, axis3)
            else:
                pass

            if axis2 > 0:
                axis2sign=True
            else:
                axis2sign=False
            
            if axis3 > 0:
                axis3sign=True
            else:
                axis3sign=False

            codestringdegreeright=b""
            codestringdegreeleft=b""
            
            if axis2sign != self.axis2History:
                self.axis2History=axis2sign
                degrees=rightposition-self.degreesoffsetright
                codestringdegreeright=b", rightDegreesCLEAR(%s, %s)"%(RightMotorsName, degrees)
                self.degreesoffsetright=rightposition

            if axis3sign != self.axis3History:
                self.axis3History=axis3sign
                degrees=leftposition-self.degreesoffsetleft
                codestringdegreeleft=b", leftDegreesCLEAR(%s, %s)"%(LeftMotorsName, degrees)
                self.degreesoffsetright=rightposition
            
            fullstring="%s%s%s"%(codestring.decode("utf-8"), codestringdegreeright.decode("utf-8"), codestringdegreeleft.decode("utf-8"))
            
            stringsize=len(fullstring)
            pack_into("=%ds"%(stringsize), codeobject, self.codeobjectoffset, fullstring)

            self.codeobjectoffset+=stringsize 

        brain.sdcard.savefile(Name + ".txt", codeobject[0:self.codeobjectoffset])


class Recording:
    """
    Main class for recording.
    """

    def __init__(self):
        self.record=False

    def start(self, controller:Controller, Right: Motor, Left: Motor):
        brain.sdcard.savefile("RecordingData.csv")
        self.record=True
        self._record_loop(controller, Right, Left)

    def stop(self, NameOfFile: str, RightMotorsName: List[str], LeftMotorsName: List[str]):
        self.record=False
        encode.encode(NameOfFile, RightMotorsName, LeftMotorsName)

    def _record_loop(self, controller: Controller, Right: Motor, Left: Motor):
        while True:
            
            if not self.record:
                break
            
            self.axis1=controller.axis1.position()
            self.axis2=controller.axis2.position()
            self.axis3=controller.axis3.position()
            self.axis4=controller.axis4.position()
            self.time_stamp=timer.time()
            self.buttonspressed=bytearray()

            if controller.buttonA.pressing():
                self.buttonspressed.extend(b"A: ")

            if controller.buttonB.pressing():
                self.buttonspressed.extend(b"B: ")

            if controller.buttonX.pressing():
                self.buttonspressed.extend(b"X: ")

            if controller.buttonY.pressing():
                self.buttonspressed.extend(b"Y: ")

            if controller.buttonUp.pressing():
                self.buttonspressed.extend(b"Up: ")

            if controller.buttonDown.pressing():
                self.buttonspressed.extend(b"Down: ")

            if controller.buttonLeft.pressing():
                self.buttonspressed.extend(b"Left: ")

            if controller.buttonRight.pressing():
                self.buttonspressed.extend(b"Right: ")

            if controller.buttonR1.pressing():
                self.buttonspressed.extend(b"R1: ")

            if controller.buttonR2.pressing():
                self.buttonspressed.extend(b"R2: ")

            if controller.buttonL1.pressing():
                self.buttonspressed.extend(b"L1: ")

            if controller.buttonL2.pressing():
                self.buttonspressed.extend(b"L2: ")

            brain.sdcard.appendfile("RecordingData.csv", bytearray(b"%d A1, %d A2, %d A3, %d A4, %d T, %s BP, %d RP, %d LP \n"%(self.axis1, self.axis2 , self.axis3, self.axis4, self.time_stamp , self.buttonspressed.decode("utf-8"), Right.position(DEGREES), Left.position(DEGREES))))

            wait(20, MSEC)

recording=Recording()
encode=Encode()

def archive_recording(self, recordingname: str) -> None:
    """
    Archives recording file. 
    Enter full name of file.

    Args:
    recordingname= String
    """

    print("Archiving...")
    filename=str(recordingname).replace(".txt", "_archived.txt")
    brain.sdcard.savefile(filename)
    with open(recordingname, 'rb') as recording:
        chunk_buffer=bytearray(10240)
        buffer=bytearray()
        while True:
            chunk=recording.readinto(chunk_buffer)
            if not chunk:
                break
            
            list=chunk_buffer.split(b"\n")
            for line in list:
                prelist=line.split(b' ')
                buffer.extend(b"%b %b %b %b \n" %(prelist[2], prelist[9], prelist[10], prelist[11]))
            brain.sdcard.appendfile(filename, buffer)

    brain.sdcard.savefile(recordingname)

def recall_recording(self, name: str) -> None:
    """
    Restores recording file to an uncompressed state. 
    Enter full name of the archived file.

    Args:
    name=String, full name of file
    """

    recording=brain.sdcard.loadfile(name).decode("utf-8").split('\n')
    filename=name.replace("_archived.txt", ".txt")
    brain.sdcard.savefile(filename)
    for item in recording:
        prelist=item.split(' ')
        if "Moved" in item:
            brain.sdcard.appendfile(filename, bytearray("[',', '0', %s ':Controller', 'DATA:', 'Axis', 'Changed.', 'Axis:', '', %s %s 'Moved', '0', 'Degrees', ''] \n"%(prelist[0], prelist[1], prelist[2]), "utf-8"))
        elif "Pressed" in item or "Released" in item:
            brain.sdcard.appendfile(filename, bytearray("[',', '0', %s ':Controller', 'DATA:', 'Button', 'Changed.', 'Button:', '', %s %s %s ''] \n"%(prelist[0], prelist[1], prelist[2], prelist[3]), "utf-8"))