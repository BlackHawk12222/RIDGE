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
        if brain.sdcard.exists("PDCconfig.txt"):
            print("Loading config for %s"%(self.Name))
            Configfile = brain.sdcard.loadfile("PDCconfig.txt")

            if Configfile is not None:
                ConfigData = Configfile.decode("utf-8").split("\n")
            else:
                return

            if self.Name not in ConfigData:
                self.configdata="%s: \n KP: %1.5f \n KD: %1.5f \n Zeta: %1.5f \n Omega: %1.5f, Theta: %s"%(self.Name, self.Kd, self.Kp, self.Zeta, self.Omega, str(self.Theta))
                brain.sdcard.appendfile("PDCconfig.txt", bytearray(self.configdata, "utf-8"))
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
                        self.configdata="%s: \n KP: %1.5f \n KD: %1.5f \n Zeta: %1.5f \n Omega: %1.5f, Theta: %s"%(self.Name, self.Kd, self.Kp, self.Zeta, self.Omega, str(self.Theta))
                        break
        else:
            print("No config found for %s, creating new config"%(self.Name))
            print(brain.sdcard.savefile("PDCconfig.txt", bytearray(b"%s: \n KP: %1.5f \n KD: %1.5f \n Zeta: %1.5f \n Omega: %1.5f, Theta: %s"%(self.Name, self.Kd, self.Kp, self.Zeta, self.Omega, str(self.Theta)))))
            self.configdata="%s: \n KP: %1.5f \n KD: %1.5f \n Zeta: %1.5f \n Omega: %1.5f, Theta: %s"%(self.Name, self.Kd, self.Kp, self.Zeta, self.Omega, str(self.Theta))
    
    def update_gains(self, new_Kp, new_Kd):
        if self.tuning:
            self.Kp = new_Kp
            self.Kd = new_Kd

            # Update the PD controller's gains
            self.PD_controller.Kp = new_Kp
            self.PD_controller.Kd = new_Kd

    def start_tuning(self,y, u, a1=0, a0=0, B0=0, B1=0):

        self.tuning = True
        self.RLS_filter = RLS(self.Name + "_RLS", 0.98, Matrix([[1000, 0, 0, 0], [0, 1000, 0, 0], [0, 0, 1000, 0], [0, 0, 0, 1000]]), Matrix([[0], [0], [0], [0]]))
        print("Starting AutoTune for %s" % self.Name)
        while self.tuning:
            StartTime=timer.time()

            self.Theta = self.RLS_filter.update(y, u)

            a0=self.Theta[0][0]
            a1=self.Theta[1][0]
            B0=self.Theta[2][0]
            B1=self.Theta[3][0]

            desired_s1_coff = 2 * self.Zeta * self.Omega
            desired_s0_coff = self.Omega ** 2

            current_s1_coff = desired_s1_coff + a1
            current_s0_coff = desired_s0_coff + a0

            if B0 != 0:
                self.Kp = max(min(current_s1_coff / B0, 0.0), 1.0)
            if B1 != 0:
                self.Kd = max(min(current_s0_coff / B1, 0.0), 0.5)

            self.update_gains(self.Kp, self.Kd)

            with open("PDCconfig.txt", "r") as file:
                data= file.read()

                data.replace(self.configdata, "%s: \n KP: %1.5f \n KD: %1.5f \n Zeta: %1.5f \n Omega: %1.5f, Theta: %s"%(self.Name, self.Kd, self.Kp, self.Zeta, self.Omega, str(self.Theta)))
                self.configdata="%s: \n KP: %1.5f \n KD: %1.5f \n Zeta: %1.5f \n Omega: %1.5f, Theta: %s"%(self.Name, self.Kd, self.Kp, self.Zeta, self.Omega, str(self.Theta))

            with open("PDCconfig.txt", "w") as file:
                file.write(data)

            print(timer.time()-StartTime)

            wait(20 - (timer.time()-StartTime), MSEC)

    def stop_tuning(self):
        self.tuning = False
