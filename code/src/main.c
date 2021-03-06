#include "lib.h"

double clearcache [30000000];
#define NUM_EVENTS 2

void clearCache (void)
{
    for(int i = 0; i < 30000000; ++i)
        clearcache[i] = i;
}

void compute (float* x, float* y, float* r)
{
    (*r) += (*x) * (*y);
}

void multiplication_ijk(float** a, float** b, float** c, int size)
{
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++)
            for (int k = 0; k < size; k++)
                compute(&a[i][k], &b[k][j], &c[i][j]);
}

void multiplication_ijk_transpose(float** a, float** b, float** c, int size)
{
    transpose_matrix(b, size);
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++)
            for (int k = 0; k < size; k++)
                compute(&a[i][k], &b[j][k], &c[i][j]);
}

void multiplication_ikj(float** a, float** b, float** c, int size)
{
    for (int i = 0; i < size; i++)
        for (int k = 0; k < size; k++)
            for (int j = 0; j < size; j++)
                compute(&a[i][k], &b[k][j], &c[i][j]);
}

void multiplication_jki(float** a, float** b, float** c, int size)
{
    for (int j = 0; j < size; j++)
        for (int k = 0; k < size; k++)
            for (int i = 0; i < size; i++)
                compute(&a[i][k], &b[k][j], &c[i][j]);
}

void multiplication_jki_transpose(float** a, float** b, float** c, int size)
{
    transpose_matrix(a, size);
    transpose_matrix(b, size);

    for (int j = 0; j < size; ++j)
        for (int k = 0; k < size; ++k)
            for (int i = 0; i < size; ++i)
                compute(&a[k][i], &b[j][k], &c[j][i]);

    transpose_matrix(c, size);
}

void multiplication_block(float **a, float **b, float **c, int size) {
    int block_size = 16;
    float tmp;
    for (int jj = 0; jj < size; jj += block_size)
    {
        for (int kk = 0; kk < size; kk += block_size)
        {
            for (int i = 0; i < size; i++)
            {
                for (int j = jj; j < ((jj + block_size) > size ? size : (jj + block_size)); j++)
                {
                    tmp = 0.0f;
                    for (int k = kk; k < ((kk + block_size) > size ? size : (kk + block_size)); k++)
                    {
                        tmp += a[i][k] * b[k][j];
                    }
                    c[i][j] += tmp;
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    //srand(time(0));

    init_app(argc);
    int size = atoi(argv[1]);
    int option = atoi(argv[2]);

    double start, end;
    // int Events[NUM_EVENTS] = {PAPI_L2_DCR, PAPI_LD_INS};
    // int Events[NUM_EVENTS] = {PAPI_L3_DCR, PAPI_L2_DCR};
    // int Events[NUM_EVENTS] = {PAPI_L3_TCM, PAPI_L2_TCM};
    // int Events[NUM_EVENTS] = {PAPI_L3_TCM, PAPI_TOT_INS};
    // int Events[NUM_EVENTS] = {PAPI_L3_TCM, PAPI_L3_TCA};
    int Events[NUM_EVENTS] = {PAPI_L3_TCM, PAPI_FP_INS};
    int EventSet = PAPI_NULL;
    long long papi[NUM_EVENTS];
    int retval = 0;
    papi[0]=0; papi[1]=0; //papi[2]=0;

    float** a = init_matrix(size);
    float** b = init_matrix(size);
    float** c1 = init_matrix(size);
    //float** c2 = init_matrix(size);

    fill_matrix(a, size, 9);
    fill_matrix(b, size, 1);

    retval = PAPI_library_init(PAPI_VER_CURRENT);

    if (retval != PAPI_VER_CURRENT && retval > 0)
        exit(1);

    if((retval = PAPI_create_eventset(&EventSet)) != PAPI_OK)
        exit(1);

    if((retval = PAPI_add_events(EventSet, Events, NUM_EVENTS)) != PAPI_OK)
        exit(1);

    if((retval = PAPI_start(EventSet)) != PAPI_OK)
        exit(1);

    clearCache();

    start = omp_get_wtime();
    switch(option){
    case 1:
        multiplication_ijk(a, b, c1, size);
        break;
    case 2:
        multiplication_ikj(a, b, c1, size);
        break;
    case 3:
        multiplication_jki(a, b, c1, size);
        break;
    case 4:
        multiplication_ijk_transpose(a, b, c1, size);
        break;
    case 5:
        multiplication_jki_transpose(a, b, c1, size);
        break;
    case 6:
        multiplication_block(a, b, c1, size);
        break;
    default:
        printf("Option invalid.\n");
        break;
    }
    end = omp_get_wtime();

    if((retval = PAPI_stop(EventSet,papi)) != PAPI_OK)
        exit(1);

    double timeMs = (end-start)*1000;

    FILE* fp = fopen("resultados.csv", "a");
    fprintf(fp, "%d\n%lf\n%lld, %lld\n", option, timeMs, papi[0], papi[1]);
    //fprintf(fp, "%d\n", timeMs);
    fclose(fp);

    free_matrices(a, b, c1, size);

    return 0;
}
