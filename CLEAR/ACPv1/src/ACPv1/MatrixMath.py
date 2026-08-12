class Matrix():
    def __init__(self, data=None, rows=0, cols=0):
        self.rows = rows
        self.cols = cols
        if data is not None:
            self.data: list[list[float]] = data
        else:
            self.data: list[list[float]] = [[0 for _ in range(cols)] for _ in range(rows)]

    def __getitem__(self, row: int, col: int):
        return self.data[row][col]

    def __setitem__(self, row, col, value):
        self.data[row][col] = value

    def __add__(self, other: Matrix):
        if isinstance(other, Matrix):

            if self.rows != other.rows or self.cols != other.cols:
                raise ValueError("Matrices must have the same dimensions for addition.")
            
            result = Matrix(rows=self.rows, cols=self.cols)

            for i in range(self.rows):

                for j in range(self.cols):
                    result.data[i][j] = self.data[i][j] + other.data[i][j]

            return result
        else:
            raise ValueError("The other operand must be a Matrix.")

    def __sub__(self, other):
        if isinstance(other, Matrix):

            if self.rows != other.rows or self.cols != other.cols:
                raise ValueError("Matrices must have the same dimensions for subtraction.")
            
            result = Matrix(rows=self.rows, cols=self.cols)

            for i in range(self.rows):

                for j in range(self.cols):
                    result.data[i][j] = self.data[i][j] - other.data[i][j]

            return result
        else:
            raise ValueError("The other operand must be a Matrix.")
        
    def __mul__(self, other):
        if isinstance(other, Matrix):

            if self.cols != other.rows:
                raise ValueError("Matrices must have compatible dimensions for multiplication.")
            
            result = Matrix(rows=self.rows, cols=other.cols)

            for i in range(self.rows):

                for j in range(other.cols):
                    result.data[i][j] = sum(self.data[i][k] * other.data[k][j] for k in range(self.cols))

            return result
        else:
            raise ValueError("The other operand must be a Matrix.")

    def matrix_multiply(self, other):
        if not isinstance(other, Matrix):

            if self.cols != other.rows:
                raise ValueError("Matrices must have compatible dimensions for multiplication.")
            
            result = Matrix(rows=self.rows, cols=other.cols)

            for i in range(self.rows):

                for j in range(other.cols):
                    result.data[i][j] = sum(self.data[i][k] * other.data[k][j] for k in range(self.cols))

            return result
        else:
            raise ValueError("The other operand must be a Matrix.")

    def transpose(self):
        result = Matrix(rows=self.cols, cols=self.rows)

        for i in range(self.rows):

            for j in range(self.cols):
                result.data[j][i] = self.data[i][j]

        return result

    def inverse(self):
        if self.rows != self.cols:
            raise ValueError("Only square matrices can be inverted.")
        
        n = self.rows
        A = Matrix([row[:] for row in self.data])  # Create a copy of the matrix
        I = Matrix([[1 if i == j else 0 for j in range(n)] for i in range(n)])  # Identity matrix

        for i in range(n):
            # Find the pivot
            pivot = A.data[i][i]
            if pivot == 0:
                raise ValueError("Matrix is singular and cannot be inverted.")
            
            # Normalize the pivot row
            for j in range(n):
                A.data[i][j] /= pivot
                I.data[i][j] /= pivot
            
            # Eliminate other rows
            for k in range(n):
                if k != i:
                    factor = A.data[k][i]
                    for j in range(n):
                        A.data[k][j] -= factor * A.data[i][j]
                        I.data[k][j] -= factor * I.data[i][j]

        return Matrix(data=I, rows=n, cols=n)