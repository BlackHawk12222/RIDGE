from vex import *

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
    def __init__(self, PD_controller: PD):
        self.Name = PD_controller.Name + "_AutoTune"
        self.tuning = False  
        self.PD_controller = PD_controller
        self.Kp = PD_controller.Kp
        self.Kd = PD_controller.Kd

    def start_tuning(self):
        self.tuning = True
        while self.tuning:
            pass

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