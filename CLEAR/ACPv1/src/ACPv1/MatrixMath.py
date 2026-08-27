class Matrix():
    def __init__(self, data=None, rows=0, cols=0):
        if data is not None:
            self.data: list[list[float]] = data
            self.rows = rows or len(data)
            self.cols = cols or (len(data[0]) if data else 0)
        else:
            self.rows = rows
            self.cols = cols
            self.data: list[list[float]] = [[0.0 for _ in range(cols)] for _ in range(rows)]

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
        elif isinstance(other, (int, float)):
            result = Matrix(rows=self.rows, cols=self.cols)

            for i in range(self.rows):

                for j in range(self.cols):
                    result.data[i][j] = self.data[i][j] + other

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
        elif isinstance(other, (int, float)):
            result = Matrix(rows=self.rows, cols=self.cols)

            for i in range(self.rows):

                for j in range(self.cols):
                    result.data[i][j] = self.data[i][j] - other

            return result
        else:
            raise ValueError("The other operand must be a Matrix.")
        
    def __mul__(self, other):
        if isinstance(other, Matrix):

            if self.cols != other.rows:
                raise ValueError("Matrices must have compatible dimensions for multiplication.")

            data=[[0.0 for _ in range(other.cols)] for _ in range(self.rows)]

            for i in range(self.rows):

                for j in range(other.cols):
                    data[i][j] = sum(self.data[i][k] * other.data[k][j] for k in range(self.cols))

            result = Matrix(data=data, rows=self.rows, cols=other.cols)
        elif isinstance(other, (int, float)):
            result = Matrix(rows=self.rows, cols=self.cols)

            for i in range(self.rows):

                for j in range(self.cols):
                    result.data[i][j] = self.data[i][j] * other
        else:
            raise ValueError("The other operand must be a Matrix.")
        
        return result

    def __truediv__(self, other):
        if isinstance(other, (int, float)):
            result = Matrix(rows=self.rows, cols=self.cols)

            for i in range(self.rows):

                for j in range(self.cols):
                    result.data[i][j] = self.data[i][j] / other

            return result
        else:
            raise ValueError("The other operand must be a scalar (int or float).")

    def __str__(self):
        string = ""

        for row in self.data:
            string += "[%s] \n"%(str(row))

        return string

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

        size = self.rows
        augmented = [self.data[row][:] + [1.0 if row == col else 0.0 for col in range(size)] for row in range(size)]

        for column in range(size):
            pivot_row = max(range(column, size),key=lambda row: abs(augmented[row][column]))
            pivot = augmented[pivot_row][column]

            if abs(pivot) <= 1e-12:
                raise ValueError("Matrix is singular and cannot be inverted.")

            if pivot_row != column:
                augmented[column], augmented[pivot_row] = (augmented[pivot_row], augmented[column])

            pivot = augmented[column][column]
            augmented[column] = [value / pivot for value in augmented[column]]

            for row in range(size):
                if row == column:
                    continue

                factor = augmented[row][column]
                augmented[row] = [current - factor * pivot_value for current, pivot_value in zip(augmented[row], augmented[column])]

        return Matrix(data=[row[size:] for row in augmented], rows=size, cols=size)