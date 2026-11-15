class ScalarKalmanFilter:
    def __init__(self, A: float, B: float, H: float, Q: float, R: float, x0: float, P0: float):
        self.A = A  # State transition matrix
        self.B = B  # Control input matrix
        self.H = H  # Observation matrix
        self.Q = Q  # Process noise covariance
        self.R = R  # Measurement noise covariance
        self.x = x0  # Initial state estimate
        self.P = P0  # Initial estimate covariance

    def __call__(self) -> float:
        return self.x

    # --- Allow float(kf), print(kf), and f-strings ---
    def __float__(self) -> float:
        return float(self.x)

    def __repr__(self) -> str:
        return repr(self.x)

    # --- Comparisons (<, <=, ==, !=, >, >=) ---
    def __lt__(self, other): return self.x < float(other)
    def __le__(self, other): return self.x <= float(other)
    def __eq__(self, other): return self.x == float(other)
    def __ne__(self, other): return self.x != float(other)
    def __gt__(self, other): return self.x > float(other)
    def __ge__(self, other): return self.x >= float(other)

    # --- Math operations (+, -, *, /, etc.) ---
    def __add__(self, other): return self.x + float(other)
    def __radd__(self, other): return float(other) + self.x
    def __sub__(self, other): return self.x - float(other)
    def __rsub__(self, other): return float(other) - self.x
    def __mul__(self, other): return self.x * float(other)
    def __rmul__(self, other): return float(other) * self.x
    def __truediv__(self, other): return self.x / float(other)
    def __rtruediv__(self, other): return float(other) / self.x
    def __neg__(self): return -self.x
    def __abs__(self): return abs(self.x)

    def predict(self, u):
        # Predict the next state
        self.x = self.A * self.x + self.B * u
        # Predict the next estimate covariance
        self.P = self.A * self.P * self.A + self.Q

    def update(self, z):
        # Compute the Kalman Gain
        S = self.H * self.P * self.H + self.R
        K = self.P * self.H * S

        # Update the state estimate
        y = z - (self.H * self.x)  # Measurement residual
        self.x = self.x + K * y

        # Update the estimate covariance
        I = 1  # Identity

        self.P = (I - K * self.H) * self.P