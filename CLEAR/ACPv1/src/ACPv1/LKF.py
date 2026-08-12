from MatrixMath import Matrix

class LinearKalmanFilter:
    def __init__(self, A: Matrix, B: Matrix, H: Matrix, Q: Matrix, R: Matrix, x0: Matrix, P0: Matrix):
        self.A = A  # State transition matrix
        self.B = B  # Control input matrix
        self.H = H  # Observation matrix
        self.Q = Q  # Process noise covariance
        self.R = R  # Measurement noise covariance
        self.x = x0  # Initial state estimate
        self.P = P0  # Initial estimate covariance

    def predict(self, u):
        # Predict the next state
        self.x = self.A * self.x + self.B * u
        # Predict the next estimate covariance
        self.P = self.A * self.P * self.A.transpose() + self.Q

    def update(self, z):
        # Compute the Kalman Gain
        S = self.H * self.P * self.H.transpose() + self.R
        K = self.P * self.H.transpose() * S.inverse()

        # Update the state estimate
        y = z - (self.H * self.x)  # Measurement residual
        self.x = self.x + K * y

        # Update the estimate covariance
        I = Matrix(rows=self.P.rows, cols=self.P.cols)
        for i in range(self.P.rows):
            I.data[i][i] = 1  # Identity matrix
        self.P = (I - K * self.H) * self.P