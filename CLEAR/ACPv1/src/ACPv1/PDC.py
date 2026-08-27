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
        self.filename="PDCconfig%s.txt"%(self.Name)
        if brain.sdcard.exists(self.filename):
            print("Loading config for %s"%(self.Name))
            Configfile = brain.sdcard.loadfile(self.filename)

            if Configfile is not None:
                ConfigData = Configfile.decode("utf-8").split("\n")
            else:
                return

            ConfigDataList:list[str]=[]
            for line in ConfigData:
                ConfigDataList.append(line)

            for i in range(len(ConfigDataList)):
                if self.Name in ConfigDataList[i]:
                    print(ConfigDataList[i+1].split(":")[1].strip())
                    self.kp=float(ConfigDataList[i+1].split(":")[1].strip())
                    self.kd=float(ConfigDataList[i+2].split(":")[1].strip())
                    self.Zeta=float(ConfigDataList[i+3].split(":")[1].strip())
                    self.Omega=float(ConfigDataList[i+4].split(":")[1].strip())
                    Thetalist=list(ConfigDataList[i+5].strip(":")[1])
                    data=[]
                    for numberpair in Thetalist:
                        data.append(list(numberpair))

                    self.Theta=Matrix(data)
                    break
        else:
            print("No config found for %s, creating new config"%(self.Name))
            print(brain.sdcard.savefile(self.filename, bytearray(b"%s: \n KP: %1.5f \n KD: %1.5f \n Zeta: %1.5f \n Omega: %1.5f \n Theta: %s"%(self.Name, self.Kp, self.Kd, self.Zeta, self.Omega, str(self.Theta)))))
    
    def update_gains(self, new_Kp, new_Kd):
        if self.tuning:
            self.Kp = new_Kp
            self.Kd = new_Kd

            # Update the PD controller's gains
            self.PD_controller.Kp = new_Kp
            self.PD_controller.Kd = new_Kd

    def start_tuning(self,y, u, a1=0, a0=0, B0=0, B1=0):

        self.tuning = True
        LastWrite=0
        self.RLS_filter = RLS(self.Name + "_RLS", 0.98, Matrix([[1000, 0, 0, 0], [0, 1000, 0, 0], [0, 0, 1000, 0], [0, 0, 0, 1000]]), Matrix([[0], [0], [0], [0]]))
        print("Starting AutoTune for %s" % self.Name)
        while self.tuning:
            StartTime=timer.time()

            start1=timer.time()
            self.Theta = self.RLS_filter.update(y, u)
            end1=timer.time()

            a0=self.Theta.data[0][0]
            a1=self.Theta.data[1][0]
            B0=self.Theta.data[2][0]
            B1=self.Theta.data[3][0]

            desired_s1_coff = 2 * self.Zeta * self.Omega
            desired_s0_coff = self.Omega ** 2

            current_s1_coff = desired_s1_coff + a1
            current_s0_coff = desired_s0_coff + a0

            if B0 != 0:
                self.Kp = max(min(current_s1_coff / B0, 0.0), 1.0)
            if B1 != 0:
                self.Kd = max(min(current_s0_coff / B1, 0.0), 0.5)

            self.update_gains(self.Kp, self.Kd)

            start= timer.time()
            if timer.time()-LastWrite >= 3000:
                with open(self.filename, "wb") as file:
                    file.write(b"%s: \n KP: %1.5f \n KD: %1.5f \n Zeta: %1.5f \n Omega: %1.5f \n Theta: %s"%(self.Name, self.Kp, self.Kd, self.Zeta, self.Omega, str(self.Theta)))
                    LastWrite=timer.time()
            end= timer.time()

            print("Total time: %d, writeing: %d, updateing RLS: %d"%(timer.time()-StartTime, start-end, start1-end1))

            wait(20 - (timer.time()-StartTime), MSEC)

    def stop_tuning(self):
        self.tuning = False
