#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <algorithm>

using namespace std;

typedef vector<vector<double>> Matrix;

Matrix identity(int n)
{
    Matrix I(n, vector<double>(n, 0.0));
    for (int i = 0; i < n; ++i)
        I[i][i] = 1.0;
    return I;
}

Matrix multiply(const Matrix &A, const Matrix &B)
{
    int n = A.size();
    Matrix C(n, vector<double>(n, 0.0));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            for (int k = 0; k < n; k++)
                C[i][j] += A[i][k] * B[k][j];
    return C;
}

void qr_decomposition(const Matrix &A, Matrix &Q, Matrix &R)
{
    int n = A.size();
    Q = Matrix(n, vector<double>(n, 0.0));
    R = Matrix(n, vector<double>(n, 0.0));
    for (int j = 0; j < n; j++)
    {
        vector<double> v(n);
        for (int i = 0; i < n; i++)
            v[i] = A[i][j];
        for (int i = 0; i < j; i++)
        {
            R[i][j] = 0;
            for (int k = 0; k < n; k++)
                R[i][j] += Q[k][i] * A[k][j];
            for (int k = 0; k < n; k++)
                v[k] -= R[i][j] * Q[k][i];
        }
        double norm = 0;
        for (double val : v)
            norm += val * val;
        norm = sqrt(norm);
        R[j][j] = norm;
        for (int i = 0; i < n; i++)
            Q[i][j] = v[i] / (norm + 1e-15);
    }
}

Matrix parseMatrix(ifstream &file, int n)
{
    Matrix mat(n, vector<double>(n));
    string line;
    for (int i = 0; i < n; ++i)
    {
        if (!getline(file, line))
            break;
        replace(line.begin(), line.end(), ',', ' ');
        stringstream ss(line);
        for (int j = 0; j < n; ++j)
            ss >> mat[i][j];
    }
    return mat;
}

int main(int argc, char *argv[])
{
    // Use the filename passed as an argument from the Qt GUI
    string filename = (argc > 1) ? argv[1] : "matrix.txt";
    ifstream file(filename);
    if (!file.is_open())
        return 1;

    string line;
    int n = 0;
    while (getline(file, line))
    {
        if (line.find("Degrees of Freedom (n):") != string::npos)
        {
            n = stoi(line.substr(line.find(":") + 1));
        }
        else if (line.find("Mass (M) Matrix") != string::npos)
        {
            Matrix M = parseMatrix(file, n);
            while (getline(file, line) && line.find("Stiffness (K) Matrix") == string::npos)
                ;
            Matrix K = parseMatrix(file, n);

            // Build A = M^-1 * K
            // NOTE: This assumes M is diagonal. For a full M matrix,
            // you would need a proper linear solve (e.g. LU decomposition).
            Matrix A(n, vector<double>(n));

            // Build A = M^{-1/2} * K * M^{-1/2}
            for (int i = 0; i < n; i++)
            {
                for (int j = 0; j < n; j++)
                {
                    double mi = sqrt(M[i][i] + 1e-15);
                    double mj = sqrt(M[j][j] + 1e-15);
                    A[i][j] = K[i][j] / (mi * mj);
                }
            }

            Matrix Ak = A;
            Matrix Eigenvectors = identity(n);
            for (int iter = 0; iter < 1000; iter++) // increased for better convergence
            {
                Matrix Q, R;
                qr_decomposition(Ak, Q, R);
                Ak = multiply(R, Q);
                Eigenvectors = multiply(Eigenvectors, Q);

                double off = 0.0;
                for (int i = 0; i < n; i++)
                    for (int j = 0; j < n; j++)
                        if (i != j)
                            off += abs(Ak[i][j]);

                if (off < 1e-6)
                    break;
            }

            // Pair each eigenvalue with its original column index so mode
            // shapes stay matched after sorting by ascending frequency.
            vector<pair<double, int>> omega_indexed(n);
            for (int i = 0; i < n; i++)
                omega_indexed[i] = {sqrt(abs(Ak[i][i])), i};
            sort(omega_indexed.begin(), omega_indexed.end());

            cout << fixed << setprecision(4);

            // -------- SAVE NATURAL FREQUENCIES --------
            ofstream fout_freq("natural_frequencies.csv");
            if (!fout_freq.is_open())
            {
                cout << "Error opening natural_frequencies.csv\n";
                return 1;
            }
            fout_freq << "Mode,Frequency\n";
            for (int i = 0; i < n; i++)
                fout_freq << "Mode" << (i + 1) << "," << omega_indexed[i].first << "\n";
            fout_freq.close();

            // -------- SAVE MODAL SHAPES --------
            // Reorder eigenvector columns using the same permutation as frequencies.
            ofstream fout_modes("mode_shapes.csv");
            if (!fout_modes.is_open())
            {
                cout << "Error opening mode_shapes.csv\n";
                return 1;
            }
            fout_modes << "Mode";
            for (int i = 0; i < n; i++)
                fout_modes << ",DOF" << (i + 1);
            fout_modes << "\n";

            for (int i = 0; i < n; i++)
            {
                int col = omega_indexed[i].second; // original eigenvector column for this mode
                fout_modes << "Mode" << (i + 1);

                // Normalize each mode shape by its largest absolute component
                double max_val = 0.0;
                for (int j = 0; j < n; j++)
                    max_val = max(max_val, abs(Eigenvectors[j][col]));

                for (int j = 0; j < n; j++)
                {
                    double mj = sqrt(M[j][j] + 1e-15);
                    double val = Eigenvectors[j][col] / mj;
                    fout_modes << "," << val / (max_val + 1e-15);
                }
                fout_modes << "\n";
            }
            fout_modes.close();
            break;
        }
    }
    return 0;
}