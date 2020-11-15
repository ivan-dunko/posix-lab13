#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>

#define ERROR_CODE -1
#define SUCCESS_CODE 0
#define PRINT_CNT 10
#define THREAD_MSG "routine\n"
#define MAIN_MSG "main\n"

typedef struct Context{
    sem_t *sem_main, *sem_rtn;
    int return_code;
} Context;

void exitWithFailure(const char *msg, int err){
    errno = err;
    fprintf(stderr, "%.256s : %.256s", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

void assertSuccess(const char *msg, int errcode){
    if (errcode != SUCCESS_CODE)
        exitWithFailure(msg, errcode);
}

void assertInThreadSuccess(int errcode, Context *cntx){
    if (errcode != SUCCESS_CODE){
        cntx->return_code = errcode;
        pthread_exit((void*)ERROR_CODE);
    }
}

void initSemSuccessAssertion(
    sem_t *sem,
    int pshared,
    unsigned int value,
    const char *err_msg){
    
    if (sem == NULL)
        return;

    int err = sem_init(sem, pshared, value);
    assertSuccess(err_msg, err);
}

void destroySemSuccessAssertion(sem_t *sem, const char *err_msg){
    if (sem == NULL)
        return;

    int err = sem_destroy(sem);
    assertSuccess(err_msg, err);
}

int initContext(
    Context *cntx,
    sem_t *sem_main,
    sem_t *sem_rtn){
    
    if (cntx == NULL || sem_main == NULL
        || sem_rtn == NULL)
        return EINVAL;

    cntx->sem_main = sem_main;
    cntx->sem_rtn = sem_rtn;
    cntx->return_code = SUCCESS_CODE;

    return SUCCESS_CODE;
}

void releaseResources(Context *cntx){
    if (cntx == NULL || cntx->sem_main == NULL || cntx->sem_rtn == NULL)
        return;
    
    destroySemSuccessAssertion(cntx->sem_rtn, "releaseResources");
    destroySemSuccessAssertion(cntx->sem_main, "releaseResources");
}

void semWaitSuccessAssertion(sem_t *sem, const char *err_msg){
    if (sem == NULL)
        return;

    int err = sem_wait(sem);
    assertSuccess(err_msg, err);
}

void semPostSuccessAssertion(sem_t *sem, const char *err_msg){
    if (sem == NULL)
        return;

    int err = sem_post(sem);
    assertSuccess(err_msg, err);
}

void iteration(
    sem_t *sem_to_post,
    sem_t *sem_to_wait,
    const char *msg,
    const char *err_msg){

    semWaitSuccessAssertion(sem_to_wait, err_msg);
    printf(msg);
    semPostSuccessAssertion(sem_to_post, err_msg);
}

void *routine(void *data){
    if (data == NULL)
        exitWithFailure("routine", EINVAL);

    Context *cntx = (Context*)data;
    for (int i = 0; i < PRINT_CNT; ++i)
        iteration(cntx->sem_main, cntx->sem_rtn, THREAD_MSG, "routine");

    pthread_exit((void*)SUCCESS_CODE);
}

int main(int argc, char **argv){
    pthread_t pid;
    sem_t sem_main, sem_rtn;

    initSemSuccessAssertion(&sem_main, 0, 1, "initSem");
    initSemSuccessAssertion(&sem_rtn, 0, 0, "initSem");

    Context cntx;
    int err = initContext(&cntx, &sem_main, &sem_rtn);
    assertSuccess("main", err);

    err = pthread_create(&pid, NULL, routine, (void*)(&cntx));
    if (err != SUCCESS_CODE)
        exitWithFailure("main", err);

    for (int i = 0; i < PRINT_CNT; ++i)
        iteration(cntx.sem_rtn, cntx.sem_main, MAIN_MSG, "main");

    err = pthread_join(pid, NULL);
    assertSuccess("main", err);

    releaseResources(&cntx);

    pthread_exit((void*)SUCCESS_CODE);
}
