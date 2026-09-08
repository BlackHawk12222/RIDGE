
class LADRC():
    def __init__(self, Kp, b0, Omega_o, dt=0.02):
        self.Kp = Kp
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
        error = Measurement - self.z1
        self.z1 += (self.z2 + self.beta1 * error + self.b0 * self.Past_U) * self.dt
        self.z2 += (self.beta2 * error) * self.dt
        protional = self.Kp * Target - self.z1
        
        if protional != 0 and self.b0 != 0:
            self.u = (protional - self.z2) / self.b0

        self.Past_U = self.u

        UClamped = max(min(self.u, MaxOutput), MinOutput)

        return UClamped

    def reset(self, Measurement):
        self.z1 = Measurement
        self.z2 = 0.0
        self.Past_U = 0.0