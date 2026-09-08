
class LADRC():
    def __init__(self, Kp, KpScale, b0, Omega_o, dt=0.02):
        self.Kp = Kp
        self.KpScale = KpScale
        self.b0 = b0
        self.Omega_o = Omega_o
        self.dt = dt

        self.beta1= 2 * self.Omega_o
        self.beta2= self.Omega_o ** 2

        self.z1 = 0.0
        self.z2 = 0.0
        self.u = 0.0
        self.Past_U = 0.0

    def compute(self, Target, Measurement, dt=0.02, MaxOutput=12, MinOutput=-12):
        self.dt = dt
        Error = Measurement - self.z1
        TrackingError = Target - self.z1
        FalError = Error / (1 + abs(Error) ** self.KpScale)  # Nonlinear error function for better disturbance rejection
        self.z1 += (self.z2 + self.beta1 * FalError + self.b0 * self.Past_U) * self.dt
        self.z2 += (self.beta2 * FalError) * self.dt
        dynamic_Kp = self.Kp * (1 + abs(TrackingError) * self.KpScale)  # Dynamic proportional gain based on error magnitude
        protional = dynamic_Kp * (Target - self.z1)
        
        if protional != 0 and self.b0 != 0:
            self.u = (protional - self.z2) / self.b0

        UClamped = max(min(self.u, MaxOutput), MinOutput)

        self.Past_U = UClamped

        return UClamped

    def reset(self, Measurement):
        self.z1 = Measurement
        self.z2 = 0.0
        self.Past_U = 0.0