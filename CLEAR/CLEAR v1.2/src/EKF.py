class Martix:
    def __init__(self, data):
        self.data = data
        self.rows = len(data)
        self.cols = len(data[0]) if self.rows > 0 else 0

    def __getitem__(self, idx):
        return self.data[idx]

    def __setitem__(self, idx, value):
        self.data[idx] = value

    def __str__(self):
        return '\n'.join(['\t'.join(map(str, row)) for row in self.data])

    def __add__(self, other):
        if isinstance(other, Martix) and self.rows == other.rows and self.cols == other.cols:
            return Martix([[self.data[i][j] + other.data[i][j] for j in range(self.cols)] for i in range(self.rows)])
        else:
            raise ValueError("Matrices must have the same dimensions for addition.")

    def __sub__(self, other):
        if isinstance(other, Martix) and self.rows == other.rows and self.cols == other.cols:
            return Martix([[self.data[i][j] - other.data[i][j] for j in range(self.cols)] for i in range(self.rows)])
        else:
            raise ValueError("Matrices must have the same dimensions for subtraction.")

    def __mul__(self, other):
        if isinstance(other, (int, float)):
            return Martix([[self.data[i][j] * other for j in range(self.cols)] for i in range(self.rows)])
        elif isinstance(other, Martix) and self.cols == other.rows:
            result = [[sum(self.data[i][k] * other.data[k][j] for k in range(self.cols)) for j in range(other.cols)] for i in range(self.rows)]
            return Martix(result)
        else:
            raise ValueError("Invalid multiplication operation.")

    def transpose(self):
        return Martix([[self.data[j][i] for j in range(self.rows)] for i in range(self.cols)])

    def determinant(self):
        if self.rows != self.cols:
            raise ValueError("Determinant is only defined for square matrices.")
        if self.rows == 1:
            return self.data[0][0]
        elif self.rows == 2:
            return self.data[0][0] * self.data[1][1] - self.data[0][1] * self.data[1][0]
        else:
            det = 0
            for c in range(self.cols):
                minor = Martix([[self.data[i][j] for j in range(self.cols) if j != c] for i in range(1, self.rows)])
                det += ((-1) ** c) * self.data[0][c] * minor.determinant()
            return det
        
class ExstendedKalmanFilter:
    pass