import math

class NLADRC():
    def __init__(self, Kp, KpScale, b0, Omega_o, alpha=0.5, delta=0.1, MaxAccel=5.0, dt=0.02):
        self.Kp = Kp
        self.KpScale = KpScale
        self.b0 = b0
        self.Omega_o = Omega_o
        self.alpha = alpha     
        self.delta = delta     
        self.dt = dt
        self.MaxAccel = MaxAccel

        self.SmoothedTarget = 0.0

        self.beta1 = 2 * self.Omega_o
        self.beta2 = self.Omega_o ** 2

        self.z1 = 0.0  # Estimated Measurement
        self.z2 = 0.0  # Estimated Disturbance
        self.Past_U = 0.0

    def _fal(self, error):
        """ Classic Han's FAL function to avoid chattering near zero error """
        abs_err = abs(error)
        if abs_err > self.delta:
            return math.copysign(abs_err ** self.alpha, error)
        else:
            return error / (self.delta ** (1 - self.alpha))

    def compute(self, Target, Measurement, dt=0.02, MaxOutput=12, MinOutput=-12):
        self.dt = dt
        
        # Target smoother compute.
        TargetError = Target - self.SmoothedTarget
        if TargetError >= self.MaxAccel:
            self.SmoothedTarget += self.MaxAccel
        elif TargetError <= -self.MaxAccel:
            self.SmoothedTarget -= self.MaxAccel
        else:
            self.SmoothedTarget = Target

        # Extended State Observer (ESO) compute.
        EsoError = Measurement - self.z1
        FalEsoError = self._fal(EsoError)
        
        # Euler integration for Extended State Observer
        self.z1 += (self.z2 + self.beta1 * FalEsoError + self.b0 * self.Past_U) * self.dt
        self.z2 += (self.beta2 * FalEsoError) * self.dt

        ControlError = self.SmoothedTarget - self.z1

        dynamic_Kp = self.Kp / (1 + abs(ControlError) * self.KpScale) 
        
        proportional = dynamic_Kp * ControlError
        
        # Disturbance Rejection Control Law
        if self.b0 != 0:
            u = (proportional - self.z2) / self.b0
        else:
            u = 0.0

        # Anti-windup clamp
        UClamped = max(min(u, MaxOutput), MinOutput)
        self.Past_U = UClamped

        return UClamped

    def reset(self, Measurement):
        self.z1 = Measurement
        self.z2 = 0.0
        self.Past_U = 0.0
        self.SmoothedTarget = Measurement
