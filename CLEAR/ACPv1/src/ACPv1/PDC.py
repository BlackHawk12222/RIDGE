from vex import *

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
    def __init__(self, PD_controller: PD, InitalZeta, InitalOmega):
        self.Name = PD_controller.Name + "_AutoTune"
        self.tuning = False  
        self.PD_controller = PD_controller
        self.Kp = PD_controller.Kp
        self.Kd = PD_controller.Kd
        self.Zeta= InitalZeta
        self.Omega = InitalOmega
        if brain.sdcard.exists("PDC_config.txt"):
            ConfigData = brain.sdcard.loadfile("PDC_config.txt").decode("utf-8")
            if self.Name not in ConfigData:
                brain.sdcard.appendfile("PDC_config.txt", bytearray(b"%s: \n KP: %1.5f \n KD: %1.5f \n Zeta: %1.5f \n Omega: %1.5f"%(self.Name, self.Kd, self.Kp, self.Zeta, self.Omega)))
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
                        break
        else:
            brain.sdcard.appendfile("PDC_config.txt", bytearray(b"%s: \n KP: %1.5f \n KD: %1.5f \n Zeta: %1.5f \n Omega: %1.5f"%(self.Name, self.Kd, self.Kp, self.Zeta, self.Omega)))

    def start_tuning(self, a1, a0, B):
        self.tuning = True
        while self.tuning:
            desired_s1_coff = 2 * self.Zeta * self.Omega
            desired_s0_coff = self.Omega ** 2
    
            self.Kp = (desired_s1_coff + a1) / B
            self.Kd = (desired_s0_coff + a0) / B

    def stop_tuning(self):
        self.tuning = False
        # Additional logic for stopping the tuning process can be added here

    def update_gains(self, new_Kp, new_Kd):
        if self.tuning:
            self.Kp = new_Kp
            self.Kd = new_Kd

            # Update the PD controller's gains
            self.PD_controller.Kp = new_Kp
            self.PD_controller.Kd = new_Kd