#include <iostream>
#include <vector>
#include <complex>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

using namespace std;

typedef complex<double> cd;
typedef vector<vector<cd>> Matrix;
typedef vector<cd> Vector;

// ---------------- READ MATRIX ----------------
vector<vector<double>> readMatrix(ifstream &file, int n)
{
    vector<vector<double>> mat(n, vector<double>(n));
    string line;

    for (int i = 0; i < n; i++)
    {
        getline(file, line);
        replace(line.begin(), line.end(), ',', ' ');
        stringstream ss(line);
        for (int j = 0; j < n; j++)
            ss >> mat[i][j];
    }
    return mat;
}

// ---------------- READ NATURAL FREQUENCIES ----------------
vector<double> readFrequencies(string filename)
{
    ifstream file(filename);
    vector<double> freq;
    string line;

    if (!file.is_open())
    {
        cout << "Error opening " << filename << "\n";
        return freq;
    }

    // Skip header
    getline(file, line);

    // Read data
    while (getline(file, line))
    {
        stringstream ss(line);
        string mode;
        double val;

        getline(ss, mode, ','); // Mode0
        ss >> val;              // Frequency

        freq.push_back(val);
    }

    return freq;
}

// ---------------- LU DECOMPOSITION ----------------
void LU_decompose(const Matrix &A, Matrix &L, Matrix &U, int n)
{
    for (int i = 0; i < n; i++)
    {

        // Upper
        for (int k = i; k < n; k++)
        {
            cd sum = 0.0;
            for (int j = 0; j < i; j++)
                sum += L[i][j] * U[j][k];

            U[i][k] = A[i][k] - sum;
        }

        // Lower
        for (int k = i; k < n; k++)
        {
            if (i == k)
                L[i][i] = 1.0;
            else
            {
                cd sum = 0.0;
                for (int j = 0; j < i; j++)
                    sum += L[k][j] * U[j][i];

                L[k][i] = (A[k][i] - sum) / U[i][i];
            }
        }
    }
}

// ---------------- FORWARD SUB ----------------
Vector forward_sub(const Matrix &L, const Vector &b, int n)
{
    Vector y(n);

    for (int i = 0; i < n; i++)
    {
        cd sum = 0.0;
        for (int j = 0; j < i; j++)
            sum += L[i][j] * y[j];

        y[i] = b[i] - sum;
    }
    return y;
}

// ---------------- BACKWARD SUB ----------------
Vector backward_sub(const Matrix &U, const Vector &y, int n)
{
    Vector x(n);

    for (int i = n - 1; i >= 0; i--)
    {
        cd sum = 0.0;
        for (int j = i + 1; j < n; j++)
            sum += U[i][j] * x[j];

        x[i] = (y[i] - sum) / U[i][i];
    }
    return x;
}

// ---------------- MAIN ----------------
int main()
{

    // -------- READ MATRIX FILE --------
    ifstream file("matrix.txt");
    if (!file.is_open())
    {
        cout << "Error opening matrix.txt\n";
        return -1;
    }

    string line;
    int n = 0;

    while (getline(file, line))
    {
        if (line.find("Degrees of Freedom") != string::npos)
        {
            sscanf(line.c_str(), "Degrees of Freedom (n): %d", &n);
            break;
        }
    }

    while (getline(file, line))
        if (line.find("Mass") != string::npos)
            break;
    auto M_real = readMatrix(file, n);

    while (getline(file, line))
        if (line.find("Stiffness") != string::npos)
            break;
    auto K_real = readMatrix(file, n);

    while (getline(file, line))
        if (line.find("Damping") != string::npos)
            break;
    auto C_real = readMatrix(file, n);

    // Convert to complex
    Matrix M(n, vector<cd>(n)), K(n, vector<cd>(n)), C(n, vector<cd>(n));

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
        {
            M[i][j] = M_real[i][j];
            K[i][j] = K_real[i][j];
            C[i][j] = C_real[i][j];
        }

    // -------- READ NATURAL FREQUENCIES --------
    vector<double> omega_n = readFrequencies("natural_frequencies.csv");

    if (omega_n.empty())
    {
        cout << "Error: nat_freq.txt is empty\n";
        return -1;
    }

    sort(omega_n.begin(), omega_n.end());

    double omega_min = omega_n.front() / 2.0;
    double omega_max = omega_n.back() * 2.0;

    cout << "omega_min = " << omega_min << endl;
    cout << "omega_max = " << omega_max << endl;

    // -------- OPEN CSV FILE --------
    ofstream fout("H_output.csv");
    if (!fout.is_open())
    {
        cout << "Error opening H_output.csv\n";
        return -1;
    }

    // -------- WRITE HEADER --------
    fout << "omega";
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            fout << ",H" << i << j;
    fout << "\n";

    // -------- FREQUENCY SWEEP --------
    int N_freq = 500;

    for (int k = 0; k < N_freq; k++)
    {

        double omega = omega_min +
                       (omega_max - omega_min) * k / (N_freq - 1);

        // Build D
        Matrix D(n, vector<cd>(n));

        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                D[i][j] = -omega * omega * M[i][j] + cd(0, omega) * C[i][j] + K[i][j];

        // LU
        Matrix L(n, vector<cd>(n, 0)), U(n, vector<cd>(n, 0));
        LU_decompose(D, L, U, n);

        // Solve H
        Matrix H(n, vector<cd>(n));

        for (int col = 0; col < n; col++)
        {

            Vector e(n, 0);
            e[col] = 1.0;

            Vector y = forward_sub(L, e, n);
            Vector x = backward_sub(U, y, n);

            for (int i = 0; i < n; i++)
                H[i][col] = x[i];
        }

        // -------- WRITE ROW --------
        fout << omega;

        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                fout << "," << abs(H[i][j]); // magnitude

        fout << "\n";
    }

    fout.close();

    cout << "Saved H(omega) to H_output.csv\n";

    return 0;
}