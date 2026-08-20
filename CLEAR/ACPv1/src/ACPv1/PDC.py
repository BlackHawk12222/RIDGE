from vex import *
from .RLS import RLS, Matrix

brain = Brain()

timer = Timer()

class PD:
    def __init__(self, Name, Kp, Kd):
        self.Name = Name
        self.Kp = Kp  # Proportional gain
        self.Kd = Kd  # Derivative gain
        self.prev_error = 0.0  # Previous error for derivative calculation

    def compute(self, setpoint, measurement, dt, MaxOutput, MinOutput):
        error = setpoint - measurement

        proprtional = self.Kp * error

        derivative = ((error - self.prev_error) / dt) * self.Kd if dt > 0 else 0.0

        output = proprtional + derivative

        # Update previous error
        self.prev_error = error


        OutputClamped = max(min(output, MaxOutput), MinOutput)

        return OutputClamped

class AutoTune:
    def __init__(self, PD_controller: PD, InitalZeta, InitalOmega, InitalTheta: Matrix):
        self.Name = PD_controller.Name + "_AutoTune"
        self.tuning = False  
        self.PD_controller = PD_controller
        self.Kp = PD_controller.Kp
        self.Kd = PD_controller.Kd
        self.Zeta= InitalZeta
        self.Omega = InitalOmega
        self.Theta = InitalTheta
        if brain.sdcard.exists("PDCconfig%s.txt"%(self.Name)):
            ConfigData = brain.sdcard.loadfile("PDCconfig%s.txt"%(self.Name)).decode("utf-8")
            if self.Name not in ConfigData:
                brain.sdcard.appendfile("PDCconfig%s.txt"%(self.Name), bytearray(b"%s: \n KP: %1.5f \n KD: %1.5f \n Zeta: %1.5f \n Omega: %1.5f, Theta: %s"%(self.Name, self.Kd, self.Kp, self.Zeta, self.Omega, str(self.Theta))))
            else:
                ConfigDataList=[] 
                for line in ConfigData:
                    ConfigDataList.append(line)

                for i in range(len(ConfigDataList)):
                    if self.Name in ConfigDataList[i]:
                        self.kp=ConfigDataList[i+1]
                        self.kd=ConfigDataList[i+2]
                        self.Zeta=ConfigDataList[i+3]
                        self.Omega=ConfigDataList[i+4]
                        self.Theta=Matrix([[float(x) for x in ConfigDataList[i+5].split(": ")[1].split(", ")]])
                        break
        else:
            print(brain.sdcard.savefile("PDCconfig%s.txt"%(self.Name), bytearray(b"%s: \n KP: %1.5f \n KD: %1.5f \n Zeta: %1.5f \n Omega: %1.5f, Theta: %s"%(self.Name, self.Kd, self.Kp, self.Zeta, self.Omega, str(self.Theta)))))

    def start_tuning(self,y, u, a1=0, a0=0, B0=0, B1=0):
        self.tuning = True
        self.RLS_filter = RLS(self.Name + "_RLS", 0.98, Matrix([[1000, 0, 0, 0], [0, 1000, 0, 0], [0, 0, 1000, 0], [0, 0, 0, 1000]]), Matrix([[0], [0], [0], [0]]))
        print("Starting AutoTune for %s" % self.Name)
        while self.tuning:
            self.Theta = self.RLS_filter.update(y, u)

            a0=self.Theta[0][0]
            a1=self.Theta[1][0]
            B0=self.Theta[2][0]
            B1=self.Theta[3][0]

            desired_s1_coff = 2 * self.Zeta * self.Omega
            desired_s0_coff = self.Omega ** 2
    
            self.Kp = max(min((desired_s1_coff + a1) / B0, 0.0), 1.0)
            self.Kd = max(min((desired_s0_coff + a0) / B1, 0.0), 0.5)

            brain.sdcard.savefile("PDCconfig%s.txt"%(self.Name), bytearray(b"%s: \n KP: %1.5f \n KD: %1.5f \n Zeta: %1.5f \n Omega: %1.5f, Theta: %s"%(self.Name, self.Kd, self.Kp, self.Zeta, self.Omega, str(self.Theta))))

            self.update_gains(self.Kp, self.Kd)

            wait(10, MSEC)


    def stop_tuning(self):
        self.tuning = False

    def update_gains(self, new_Kp, new_Kd):
        if self.tuning:
            self.Kp = new_Kp
            self.Kd = new_Kd

            # Update the PD controller's gains
            self.PD_controller.Kp = new_Kp
            self.PD_controller.Kd = new_Kd