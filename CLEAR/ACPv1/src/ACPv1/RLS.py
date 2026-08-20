from .MatrixMath import Matrix

class RLS:
    def __init__(self, Name, lambda_, delta: Matrix, Theta0: Matrix = Matrix([[0], [0], [0], [0]])):
        self.Name = Name
        self.lambda_ = lambda_  # Forgetting factor
        self.delta_matrix = delta  # Initial covariance matrix
        self.Theta = Theta0  # Initial parameter vector
        self.yhistory = [0.0, 0.0]  # History of output measurements
        self.uhistory = [0.0, 0.0]  # History of input measurements
        self.y=0.0
        self.u=0.0

    def update(self, y: float, u: float):
        self.yhistory.append(y)
        self.uhistory.append(u)

        # Keep only the last two measurements
        if len(self.yhistory) > 2:
            self.yhistory.pop(0)
            self.uhistory.pop(0)

        # Create the regression vector phi
        phi = Matrix([[self.uhistory[-1]], [self.uhistory[-2]], [self.yhistory[-1]], [self.yhistory[-2]]])

        # Compute the Kalman gain
        P_phi = self.delta_matrix * phi
        gain_denominator = (self.lambda_ + (phi.transpose() * P_phi).data[0][0])
        K = P_phi * (1 / gain_denominator)

        # Update the parameter estimates
        prediction_error = y - (phi.transpose() * self.Theta).data[0][0]
        self.Theta = self.Theta + K * prediction_error

        # Update the covariance matrix
        self.delta_matrix = (self.delta_matrix - (K * phi.transpose() * self.delta_matrix)) / self.lambda_

        return self.Theta.data
        