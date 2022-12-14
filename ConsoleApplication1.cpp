#include <iostream>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <omp.h>

using namespace std;

class Timer {
    typedef std::chrono::high_resolution_clock clock_;
    typedef std::chrono::duration<double, std::ratio<1, 1000000>> second_;
    std::chrono::time_point<clock_> beg_;
    const char* header;
public:
    Timer(const char* header = "") : beg_(clock_::now()), header(header) {}
    ~Timer() {
        double e = elapsed();
        cout << header << ": " << e / 1000000 << " seconds" << endl;
    }
    void reset() {
        beg_ = clock_::now();
    }
    double elapsed() const {
        return std::chrono::duration_cast<second_>(clock_::now() - beg_).count();
    }
};

void crout_0(double** A, double** L, double** U, int n) {
    int i, j, k;
    double sum = 0;
    for (i = 0; i < n; i++) {
        U[i][i] = 1;
    }
    for (j = 0; j < n; j++) {
        for (i = j; i < n; i++) {
            sum = 0;
            for (k = 0; k < j; k++) {
                sum = sum + L[i][k] * U[k][j];
            }
            L[i][j] = A[i][j] - sum;
        }
        for (i = j; i < n; i++) {
            sum = 0;
            for (k = 0; k < j; k++) {
                sum = sum + L[j][k] * U[k][i];
            }
            if (L[j][j] == 0) {
                exit(0);
            }
            U[j][i] = (A[j][i] - sum) / L[j][j];
        }
    }
}

void crout_1(double** A, double** L, double** U, int n) {
    int i, j, k;
    double sum = 0;
    for (i = 0; i < n; i++) {
        U[i][i] = 1;
    }
    for (j = 0; j < n; j++) {
#pragma omp parallel for private(i,k,sum)
        for (i = j; i < n; i++) {
            sum = 0;
            for (k = 0; k < j; k++) {
                sum = sum + L[i][k] * U[k][j];
            }
            L[i][j] = A[i][j] - sum;
        }
#pragma omp parallel for private(i,k,sum)
        for (i = j; i < n; i++) {
            sum = 0;
            for (k = 0; k < j; k++) {
                sum = sum + L[j][k] * U[k][i];
            }
            if (L[j][j] == 0) {
                exit(0);
            }
            U[j][i] = (A[j][i] - sum) / L[j][j];
        }
    }
}

void crout_2(double** A, double** L, double** U, int n) {
    int i, j, k;
    double sum = 0;
    for (i = 0; i < n; i++) {
        U[i][i] = 1;
    }
    for (j = 0; j < n; j++) {
        //i=j case
        sum = 0;
        for (k = 0; k < j; k++) {
            sum = sum + L[j][k] * U[k][j];
        }
        L[j][j] = A[j][j] - sum;
        int x = j + (n - j) / 2;
#pragma omp parallel sections
        {
#pragma omp section
            {
                for (int i = j + 1; i < x; i++) {
                    double sum = 0;
                    for (int k = 0; k < j; k++) {
                        sum = sum + L[i][k] * U[k][j];
                    }
                    L[i][j] = A[i][j] - sum;
                }
            }
#pragma omp section
            {
                for (int i = x; i < n; i++) {
                    double sum = 0;
                    for (int k = 0; k < j; k++) {
                        sum = sum + L[i][k] * U[k][j];
                    }
                    L[i][j] = A[i][j] - sum;
                }

            }
#pragma omp section
            {
                for (int i = j; i < x; i++) {
                    double sum = 0;
                    for (int k = 0; k < j; k++) {
                        sum = sum + L[j][k] * U[k][i];
                    }
                    if (L[j][j] == 0) {
                        exit(0);
                    }
                    U[j][i] = (A[j][i] - sum) / L[j][j];
                }
            }
#pragma omp section
            {
                for (int i = x; i < n; i++) {
                    double sum = 0;
                    for (int k = 0; k < j; k++) {
                        sum = sum + L[j][k] * U[k][i];
                    }
                    if (L[j][j] == 0) {
                        exit(0);
                    }
                    U[j][i] = (A[j][i] - sum) / L[j][j];
                }
            }
        }
    }

}

void testCroutSequencial(double** A, size_t n) {
    double** L = (double**)malloc(sizeof(double*) * n);
    for (size_t i = 0; i < n; i++) {
        L[i] = (double*)malloc(sizeof(double) * n);
    }

    double** U = (double**)malloc(sizeof(double*) * n);
    for (size_t i = 0; i < n; i++) {
        U[i] = (double*)malloc(sizeof(double) * n);
    }

    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            L[i][j] = 0;
            U[i][j] = 0;
        }
    }
    
    {
        Timer t("SEQUENTIAL: ");
        crout_0(A, L, U, n);
    }

    for (size_t i = 0; i < n; i++) {
        free(L[i]);
        free(U[i]);
    }

    free(L);
    free(U);
}

void testCroutParallel1(double** A, size_t n) {
    double** L = (double**)malloc(sizeof(double*) * n);
    for (size_t i = 0; i < n; i++) {
        L[i] = (double*)malloc(sizeof(double) * n);
    }

    double** U = (double**)malloc(sizeof(double*) * n);
    for (size_t i = 0; i < n; i++) {
        U[i] = (double*)malloc(sizeof(double) * n);
    }

    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            L[i][j] = 0;
            U[i][j] = 0;
        }
    }

    {
        Timer t("PARALLEL 1: ");
        crout_1(A, L, U, n);
    }

    for (size_t i = 0; i < n; i++) {
        free(L[i]);
        free(U[i]);
    }

    free(L);
    free(U);
}

void testCroutParallel2(double** A, size_t n) {
    double** L = (double**)malloc(sizeof(double*) * n);
    for (size_t i = 0; i < n; i++) {
        L[i] = (double*)malloc(sizeof(double) * n);
    }

    double** U = (double**)malloc(sizeof(double*) * n);
    for (size_t i = 0; i < n; i++) {
        U[i] = (double*)malloc(sizeof(double) * n);
    }

    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            L[i][j] = 0;
            U[i][j] = 0;
        }
    }

    {
        Timer t("PARALLEL 2: ");
        crout_2(A, L, U, n);
    }

    for (size_t i = 0; i < n; i++) {
        free(L[i]);
        free(U[i]);
    }

    free(L);
    free(U);
}

int main() {
    cout << "Unesite velicinu niza: ";
    int n;
    cin >> n;
    cout << endl;
    double** A = (double**)malloc(sizeof(double*) * n);
    for (size_t i = 0; i < n; i++) {
        A[i] = (double*)malloc(sizeof(double) * n);
    }

    srand(time(0));

    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            A[i][j] = rand() % 100;
        }
    }

    //testCroutSequencial(A,n);

    /*cout << endl << "Unesite broj threadova: ";
    int thr;
    cin >> thr;*/

    omp_lock_t lock;

    omp_init_lock(&lock);

    omp_set_dynamic(0);
    omp_set_nested(1);
    omp_set_num_threads(4);
   
    testCroutParallel1(A, n);
    testCroutParallel2(A, n);

    for (size_t i = 0; i < n; i++) {
        free(A[i]);
    }
    free(A);

    return 0;
}