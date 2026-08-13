from MatrixMath import Matrix

class RLS:
    def __init__(self, Name, lambda_, delta):
        self.Name = Name
        self.lambda_ = lambda_  # Forgetting factor
        self.delta = delta  # Initial covariance
        self.delta_matrix = Matrix([[delta]])  # Initial covariance matrix
        