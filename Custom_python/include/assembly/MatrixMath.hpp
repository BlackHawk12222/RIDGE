#ifndef MATRIXMATH_H
#define MATRIXMATH_H

extern "C" {
    // We pass raw pointers to float arrays, along with the dimensions (rows, cols)
    void asm_matrix_add(float* out, const float* a, const float* b, int rows, int cols);
    void asm_matrix_sub(float* out, const float* a, const float* b, int rows, int cols);
    void asm_matrix_mul(float* out, const float* a, const float* b, int r1, int c1, int c2);
    void asm_matrix_div(float* out, const float* a, const float* b, int rows, int cols); // element-wise
}

#include <vector>

class Matrix{
    private:
        int rows;
        int cols;
        std::vector<std::vector<float>> value;

        // Helper functions to flatten and unflatten vector data for assembly consumption
        std::vector<float> flatten() const;
        void unflatten(const std::vector<float>& flat_data);

    public:
        Matrix(std::vector<std::vector<float>> val);

        Matrix operator+(const Matrix& other) const;

        Matrix operator-(const Matrix& other) const;

        Matrix operator*(const Matrix& other) const;

        Matrix operator/(const Matrix& other) const;


};
#endif // MATRIXMATH_H