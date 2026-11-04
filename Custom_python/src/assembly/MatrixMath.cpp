#include "assembly/MatrixMath.hpp"
#include <stdexcept>

Matrix::Matrix(std::vector<std::vector<float>> val) : value(val) {
    rows = val.size();
    cols = rows > 0 ? val[0].size() : 0;
}

// Convert 2D vector to 1D flat vector array
std::vector<float> Matrix::flatten() const {
    std::vector<float> flat;
    flat.reserve(rows * cols);
    for (const auto& row : value) {
        flat.insert(flat.end(), row.begin(), row.end());
    }
    return flat;
}

// Convert 1D flat vector back to 2D vector layout
void Matrix::unflatten(const std::vector<float>& flat_data) {
    int index = 0;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            value[r][c] = flat_data[index++];
        }
    }
}

// Addition implementation passing data down to assembly
Matrix Matrix::operator+(const Matrix& other) const {
    if (this->rows != other.rows || this->cols != other.cols) {
        throw std::invalid_argument("Matrix dimensions must match for addition!");
    }

    Matrix result(this->value); // Clone matrix shell layout
    std::vector<float> flat_A = this->flatten();
    std::vector<float> flat_B = other.flatten();
    std::vector<float> flat_Out(rows * cols);

    // CALL THE ARM ASSEMBLY CORE:
    // .data() yields a raw float* pointer that assembly understands natively!
    asm_matrix_add(flat_Out.data(), flat_A.data(), flat_B.data(), rows, cols);

    result.unflatten(flat_Out);
    return result;
}

// Standard Matrix Multiplication (Dot Product of Row x Col)
Matrix Matrix::operator*(const Matrix& other) const {
    if (this->cols != other.rows) {
        throw std::invalid_argument("Matrix inner dimensions must match for multiplication!");
    }

    // Output dimension configuration
    std::vector<std::vector<float>> empty_shell(this->rows, std::vector<float>(other.cols, 0.0f));
    Matrix result(empty_shell);

    std::vector<float> flat_A = this->flatten();
    std::vector<float> flat_B = other.flatten();
    std::vector<float> flat_Out(this->rows * other.cols);

    // CALL THE ARM ASSEMBLY CORE:
    asm_matrix_mul(flat_Out.data(), flat_A.data(), flat_B.data(), this->rows, this->cols, other.cols);

    result.unflatten(flat_Out);
    return result;
}