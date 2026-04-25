// sensitivity.cpp
// ---------------------------------------------------------------
// Sensitivity analysis on individual physical parameters:
//   m1, m2, …   k1, k2, …   c1, c2, …
//
// For each scalar parameter p:
//
//   S_p(ω) = || H(ω; p + δ) – H(ω; p) ||_F  /  δ
//   mean_S_p = (1/N_ω) Σ_ω  S_p(ω)
//
// physical_params.txt format (written by the GUI):
//   Masses:
//   m1 = 2.5
//   m2 = 1.0
//
//   Springs:
//   k1 = 100   nodeA = 0   nodeB = -1
//   k2 = 200   nodeA = 0   nodeB = 1
//
//   Dampers:
//   c1 = 5     nodeA = 0   nodeB = -1
//
// nodeA / nodeB are DOF indices; -1 means wall/ground (fixed).
//
// Output:
//   sensitivity_output.csv      — ranked list, one row per parameter
//   sensitivity_vs_freq.csv     — S_p(ω) curve per parameter
// ---------------------------------------------------------------

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <complex>
#include <cmath>
#include <string>
#include <algorithm>
#include <iomanip>

using namespace std;

typedef complex<double> cd;
typedef vector<vector<cd>> CMatrix;
typedef vector<cd> CVector;

// ═══════════════════════════════════════════════════════════════════
// Data structures
// ═══════════════════════════════════════════════════════════════════

struct MassParam
{
    string name; // "m1", "m2", …
    int dof;     // 0-based DOF index (= insertion order)
    double value;
};

struct ConnectorParam
{
    string name; // "k1", "c2", …
    int nodeA;   // DOF index, -1 = wall/ground
    int nodeB;   // DOF index, -1 = wall/ground
    double value;
};

struct PhysicalParams
{
    int n = 0; // number of DOFs (= number of masses)
    vector<MassParam> masses;
    vector<ConnectorParam> springs;
    vector<ConnectorParam> dampers;
};

// ═══════════════════════════════════════════════════════════════════
// Parse physical_params.txt
// ═══════════════════════════════════════════════════════════════════

PhysicalParams readPhysicalParams(const string &filename)
{
    PhysicalParams p;
    ifstream file(filename);
    if (!file.is_open())
    {
        cout << "Cannot open " << filename << "\n";
        return p;
    }

    enum Section
    {
        NONE,
        MASSES,
        SPRINGS,
        DAMPERS
    } section = NONE;

    string line;
    while (getline(file, line))
    {
        // Strip trailing whitespace / CR
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();

        if (line.empty())
            continue;

        // Section headers
        if (line == "Masses:")
        {
            section = MASSES;
            continue;
        }
        if (line == "Springs:")
        {
            section = SPRINGS;
            continue;
        }
        if (line == "Dampers:")
        {
            section = DAMPERS;
            continue;
        }

        // Replace '=' so we can stream-parse cleanly
        for (char &c : line)
            if (c == '=')
                c = ' ';

        stringstream ss(line);

        if (section == MASSES)
        {
            // Format:  m1   2.5
            MassParam mp;
            if (!(ss >> mp.name >> mp.value))
                continue;
            mp.dof = (int)p.masses.size(); // DOF = insertion order
            p.masses.push_back(mp);
        }
        else if (section == SPRINGS || section == DAMPERS)
        {
            // Format:  k1   100   nodeA   0   nodeB   1
            ConnectorParam cp;
            string labelA, labelB;
            if (!(ss >> cp.name >> cp.value >> labelA >> cp.nodeA >> labelB >> cp.nodeB))
                continue;

            if (section == SPRINGS)
                p.springs.push_back(cp);
            else
                p.dampers.push_back(cp);
        }
    }

    p.n = (int)p.masses.size();
    return p;
}

// ═══════════════════════════════════════════════════════════════════
// Matrix assembly
// ═══════════════════════════════════════════════════════════════════

// Add contribution of one connector (value v) between nodeA and nodeB.
// nodeA or nodeB == -1 means that end is grounded (no DOF there).
void assembleConnector(vector<vector<double>> &mat, int n,
                       int nodeA, int nodeB, double v)
{
    if (nodeA >= 0 && nodeA < n)
        mat[nodeA][nodeA] += v;
    if (nodeB >= 0 && nodeB < n)
        mat[nodeB][nodeB] += v;
    if (nodeA >= 0 && nodeA < n && nodeB >= 0 && nodeB < n)
    {
        mat[nodeA][nodeB] -= v;
        mat[nodeB][nodeA] -= v;
    }
}

struct Matrices
{
    vector<vector<double>> M, K, C;
};

Matrices buildMatrices(const PhysicalParams &p)
{
    int n = p.n;
    Matrices sys;
    sys.M.assign(n, vector<double>(n, 0.0));
    sys.K.assign(n, vector<double>(n, 0.0));
    sys.C.assign(n, vector<double>(n, 0.0));

    for (auto &mp : p.masses)
        if (mp.dof >= 0 && mp.dof < n)
            sys.M[mp.dof][mp.dof] = mp.value;

    for (auto &sp : p.springs)
        assembleConnector(sys.K, n, sp.nodeA, sp.nodeB, sp.value);

    for (auto &dp : p.dampers)
        assembleConnector(sys.C, n, dp.nodeA, dp.nodeB, dp.value);

    return sys;
}

// ═══════════════════════════════════════════════════════════════════
// Natural frequencies
// ═══════════════════════════════════════════════════════════════════

vector<double> readFrequencies(const string &filename)
{
    vector<double> freq;
    ifstream file(filename);
    if (!file.is_open())
    {
        cout << "Cannot open " << filename << "\n";
        return freq;
    }
    string line;
    getline(file, line); // skip header
    while (getline(file, line))
    {
        stringstream ss(line);
        string mode;
        double val;
        getline(ss, mode, ',');
        if (ss >> val)
            freq.push_back(val);
    }
    return freq;
}

// ═══════════════════════════════════════════════════════════════════
// LU solve
// ═══════════════════════════════════════════════════════════════════

void LU_decompose(const CMatrix &A, CMatrix &L, CMatrix &U, int n)
{
    for (int i = 0; i < n; i++)
    {
        for (int k = i; k < n; k++)
        {
            cd sum = 0.0;
            for (int j = 0; j < i; j++)
                sum += L[i][j] * U[j][k];
            U[i][k] = A[i][k] - sum;
        }
        for (int k = i; k < n; k++)
        {
            if (i == k)
            {
                L[i][i] = 1.0;
                continue;
            }
            cd sum = 0.0;
            for (int j = 0; j < i; j++)
                sum += L[k][j] * U[j][i];
            L[k][i] = (A[k][i] - sum) / U[i][i];
        }
    }
}

CVector forward_sub(const CMatrix &L, const CVector &b, int n)
{
    CVector y(n);
    for (int i = 0; i < n; i++)
    {
        cd sum = 0.0;
        for (int j = 0; j < i; j++)
            sum += L[i][j] * y[j];
        y[i] = b[i] - sum;
    }
    return y;
}

CVector backward_sub(const CMatrix &U, const CVector &y, int n)
{
    CVector x(n);
    for (int i = n - 1; i >= 0; i--)
    {
        cd sum = 0.0;
        for (int j = i + 1; j < n; j++)
            sum += U[i][j] * x[j];
        x[i] = (y[i] - sum) / U[i][i];
    }
    return x;
}

CMatrix computeH(const vector<vector<double>> &M,
                 const vector<vector<double>> &K,
                 const vector<vector<double>> &C,
                 double omega, int n)
{
    // D = -ω²M + jωC + K
    CMatrix D(n, vector<cd>(n));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            D[i][j] = -omega * omega * M[i][j] + cd(0, omega) * C[i][j] + K[i][j];

    CMatrix L(n, vector<cd>(n, 0)), U(n, vector<cd>(n, 0));
    LU_decompose(D, L, U, n);

    CMatrix H(n, vector<cd>(n));
    for (int col = 0; col < n; col++)
    {
        CVector e(n, 0);
        e[col] = 1.0;
        CVector y = forward_sub(L, e, n);
        CVector x = backward_sub(U, y, n);
        for (int i = 0; i < n; i++)
            H[i][col] = x[i];
    }
    return H;
}

// ||(Hp – H0) / delta||_F
double sensitivityNorm(const CMatrix &H0, const CMatrix &Hp, double delta, int n)
{
    double norm2 = 0.0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
        {
            cd diff = (Hp[i][j] - H0[i][j]) / delta;
            norm2 += std::norm(diff);
        }
    return sqrt(norm2);
}

// ═══════════════════════════════════════════════════════════════════
// main
// ═══════════════════════════════════════════════════════════════════

int main()
{
    // ── 1. Read physical parameters + connectivity ───────────────────────
    PhysicalParams params = readPhysicalParams("physical_params.txt");
    if (params.n == 0)
    {
        cout << "No masses found in physical_params.txt\n";
        return 1;
    }

    int n = params.n;
    cout << "DOFs:    " << n << "\n";
    cout << "Masses:  " << params.masses.size() << "\n";
    cout << "Springs: " << params.springs.size() << "\n";
    cout << "Dampers: " << params.dampers.size() << "\n\n";

    // ── 2. Build baseline M, K, C ────────────────────────────────────────
    Matrices base = buildMatrices(params);

    // ── 3. Frequency range ───────────────────────────────────────────────
    vector<double> omega_n = readFrequencies("natural_frequencies.csv");
    if (omega_n.empty())
    {
        cout << "No frequencies found\n";
        return 1;
    }
    sort(omega_n.begin(), omega_n.end());

    double omega_min = omega_n.front() / 2.0;
    double omega_max = omega_n.back() * 2.0;
    int N_freq = 200;

    vector<double> omegas(N_freq);
    for (int k = 0; k < N_freq; k++)
        omegas[k] = omega_min + (omega_max - omega_min) * k / (N_freq - 1);

    // ── 4. Sensitivity sweep ─────────────────────────────────────────────
    const double DELTA_FRAC = 0.01; // 1% perturbation

    struct ParamResult
    {
        string name;
        double meanSens;
        vector<double> sensPerFreq;
    };
    vector<ParamResult> results;

    // Runs the frequency sweep comparing base vs perturbed matrices
    auto runParam = [&](const string &pname,
                        const vector<vector<double>> &M_p,
                        const vector<vector<double>> &K_p,
                        const vector<vector<double>> &C_p,
                        double delta) -> ParamResult
    {
        ParamResult res;
        res.name = pname;
        res.sensPerFreq.resize(N_freq);
        double sumSens = 0.0;

        for (int k = 0; k < N_freq; k++)
        {
            double w = omegas[k];
            CMatrix H0 = computeH(base.M, base.K, base.C, w, n);
            CMatrix Hp = computeH(M_p, K_p, C_p, w, n);
            double s = sensitivityNorm(H0, Hp, delta, n);
            res.sensPerFreq[k] = s;
            sumSens += s;
        }
        res.meanSens = sumSens / N_freq;
        return res;
    };

    // ── Masses: perturb M[dof][dof] only ────────────────────────────────
    for (auto &mp : params.masses)
    {
        double delta = DELTA_FRAC * (fabs(mp.value) > 1e-12 ? fabs(mp.value) : 1.0);
        auto M_p = base.M;
        M_p[mp.dof][mp.dof] += delta;
        results.push_back(runParam(mp.name, M_p, base.K, base.C, delta));
    }

    // ── Springs: perturb exactly the K entries this spring contributes ───
    for (auto &sp : params.springs)
    {
        double delta = DELTA_FRAC * (fabs(sp.value) > 1e-12 ? fabs(sp.value) : 1.0);
        auto K_p = base.K;
        assembleConnector(K_p, n, sp.nodeA, sp.nodeB, delta);
        results.push_back(runParam(sp.name, base.M, K_p, base.C, delta));
    }

    // ── Dampers: perturb exactly the C entries this damper contributes ───
    for (auto &dp : params.dampers)
    {
        double delta = DELTA_FRAC * (fabs(dp.value) > 1e-12 ? fabs(dp.value) : 1.0);
        auto C_p = base.C;
        assembleConnector(C_p, n, dp.nodeA, dp.nodeB, delta);
        results.push_back(runParam(dp.name, base.M, base.K, C_p, delta));
    }

    // ── 5. Sort descending ───────────────────────────────────────────────
    sort(results.begin(), results.end(),
         [](const ParamResult &a, const ParamResult &b)
         { return a.meanSens > b.meanSens; });

    // ── 6. sensitivity_output.csv ────────────────────────────────────────
    {
        ofstream fout("sensitivity_output.csv");
        if (!fout.is_open())
        {
            cout << "Cannot write sensitivity_output.csv\n";
            return 1;
        }
        fout << "Rank,Parameter,MeanSensitivity\n";
        for (int i = 0; i < (int)results.size(); i++)
            fout << (i + 1) << ","
                 << results[i].name << ","
                 << fixed << setprecision(6) << results[i].meanSens << "\n";
        fout.close();
        cout << "Saved sensitivity_output.csv\n";
    }

    // ── 7. sensitivity_vs_freq.csv ───────────────────────────────────────
    {
        ofstream fout("sensitivity_vs_freq.csv");
        if (!fout.is_open())
        {
            cout << "Cannot write sensitivity_vs_freq.csv\n";
            return 1;
        }
        fout << "omega";
        for (auto &r : results)
            fout << "," << r.name;
        fout << "\n";
        for (int k = 0; k < N_freq; k++)
        {
            fout << fixed << setprecision(6) << omegas[k];
            for (auto &r : results)
                fout << "," << r.sensPerFreq[k];
            fout << "\n";
        }
        fout.close();
        cout << "Saved sensitivity_vs_freq.csv\n";
    }

    // ── 8. Console summary ───────────────────────────────────────────────
    cout << "\n=== Sensitivity Ranking ===\n";
    cout << left << setw(6) << "Rank"
         << setw(14) << "Parameter"
         << "Mean ||S_p||_F\n";
    cout << string(40, '-') << "\n";
    for (int i = 0; i < (int)results.size(); i++)
        cout << left << setw(6) << (i + 1)
             << setw(14) << results[i].name
             << fixed << setprecision(6) << results[i].meanSens << "\n";

    return 0;
}