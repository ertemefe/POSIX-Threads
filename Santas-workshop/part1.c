#include "queue.c"
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#define TRUE 1
#define FALSE 0
#define ARRAY_SIZE 256
#define ARRAY_SIZE_2 3000

int simulationTime = 120;    // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 30; // frequency of emergency gift requests from New Zealand
int n = 2;

void printQueues(int currTime);
void appendLogFile(Task *task);
void initializeLogFile();
void getSecond();

void *ElfA(void *arg); // the one that can paint
void *ElfB(void *arg); // the one that can assemble
void *Santa(void *arg);
void *ControlThread(void *arg); // handles printing and queues (up to you)

Queue *PaintingQue;
Queue *AssemblyQue;
Queue *QAQue;
Queue *PackagingQue;
Queue *DeliveryQue;

pthread_t elf1_Thread;
pthread_t elf2_Thread;
pthread_t santa_Thread;
pthread_t controller_thread;

int taskID;
int giftID;

FILE *logFile;

int *packagingCanBeDone;

// time Stuct Library
struct timeval tv;
struct timeval start_tv;
struct timezone tz;
struct tm *start_time;
struct tm *lg_time;
int time2;

// more locks probably needed
pthread_mutex_t lockQue, lockID;
pthread_mutex_t lockGift4, lockGift5;
pthread_mutex_t lockFile;

// pthread sleeper function
int pthread_sleep(int seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if (pthread_mutex_init(&mutex, NULL))
    {
        return -1;
    }
    if (pthread_cond_init(&conditionvar, NULL))
    {
        return -1;
    }
    struct timeval tp;
    // When to expire is an absolute time, so get the current time and add it to our delay time
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds;
    timetoexpire.tv_nsec = tp.tv_usec * 1000;

    pthread_mutex_lock(&mutex);
    int res = pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);

    // Upon successful completion, a value of zero shall be returned
    return res;
}

int main(int argc, char **argv)
{
    // -t (int) => simulation time in seconds
    // -s (int) => change the random seed
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-t"))
        {
            simulationTime = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-s"))
        {
            seed = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-n"))
        {
            n = atoi(argv[++i]);
        }
    }

    srand(seed); // feed the seed

    /* Queue usage example
        Queue *myQ = ConstructQueue(1000);
        Task t;
        t.ID = myID;
        t.type = 2;
        Enqueue(myQ, t);
        Task ret = Dequeue(myQ);
        DestructQueue(myQ);
    */

    // your code goes here
    giftID = 1;
    taskID = 1;

    gettimeofday(&start_tv, &tz);
    start_time = localtime(&start_tv.tv_sec);


    // Initialized the queues
    PaintingQue = ConstructQueue(1000);
    AssemblyQue = ConstructQueue(1000);
    QAQue = ConstructQueue(1000);
    PackagingQue = ConstructQueue(1000);
    DeliveryQue = ConstructQueue(1000);

    pthread_mutex_init(&lockQue, NULL);
    pthread_mutex_init(&lockID, NULL);
    pthread_mutex_init(&lockGift4, NULL);
    pthread_mutex_init(&lockGift5, NULL);
    pthread_mutex_init(&lockFile, NULL);

    packagingCanBeDone = malloc(sizeof(int) * (simulationTime + 10));
    memset(packagingCanBeDone, 0, sizeof(int) * (simulationTime + 10));

    initializeLogFile();

    // Initialize (Launch) the threads
    pthread_create(&elf1_Thread, NULL, ElfA, NULL);
    pthread_create(&elf2_Thread, NULL, ElfB, NULL);
    pthread_create(&santa_Thread, NULL, Santa, NULL);
    pthread_create(&controller_thread, NULL, ControlThread, NULL);

    pthread_sleep(simulationTime);

    pthread_cancel(elf1_Thread);
    pthread_cancel(elf2_Thread);
    pthread_cancel(santa_Thread);
    pthread_cancel(controller_thread);

    pthread_mutex_destroy(&lockQue);
    pthread_mutex_destroy(&lockID);
    pthread_mutex_destroy(&lockGift4);
    pthread_mutex_destroy(&lockGift5);
    pthread_mutex_destroy(&lockFile);

    printf("Bitti \n");

    return 0;
}

void *ElfA(void *arg)
{

    while (TRUE)
    {
        Task *nextTask;
        Task *newTask = malloc(sizeof(Task));

        int timeFinished;
        int timeArrival;

        pthread_mutex_lock(&lockQue);
        if (!isEmpty(PackagingQue))
        {
            *nextTask = Dequeue(PackagingQue);
            pthread_mutex_unlock(&lockQue);
            nextTask->Responsible = 'A';

            pthread_sleep(1);

            getSecond();
            timeFinished = time2;

            // Old Task
            nextTask->TT = timeFinished - nextTask->TaskArrival;

            appendLogFile(nextTask);

            // Lock taskID before updating it
            pthread_mutex_lock(&lockID);
            int tempTaskID = taskID;
            taskID++;
            pthread_mutex_unlock(&lockID);

            getSecond();
            timeArrival = time2;

            // Create new Task
            newTask->ID = tempTaskID;
            newTask->GiftID = nextTask->GiftID;
            newTask->GiftType = nextTask->GiftType;
            newTask->TaskType = 'D';
            newTask->RequesT = nextTask->RequesT;
            newTask->TaskArrival = timeArrival;

            // Lock before enque
            pthread_mutex_lock(&lockQue);
            Enqueue(DeliveryQue, *newTask);
            pthread_mutex_unlock(&lockQue);
            continue;
        }

        // Check if PaintingQue is empty
        if (!isEmpty(PaintingQue))
        {
            // Deque next Painting task
            *nextTask = Dequeue(PaintingQue);
            pthread_mutex_unlock(&lockQue);

            nextTask->Responsible = 'A';

            pthread_sleep(3);
            int tempGiftID = nextTask->GiftID;

            // Get timiFinished for old Task
            getSecond();
            timeFinished = time2;

            // Update old thread
            nextTask->TT = timeFinished - nextTask->TaskArrival;

            appendLogFile(nextTask);

            // Gift is Type 4
            if (nextTask->GiftType == 4)
            {
                pthread_mutex_lock(&lockGift4);
                packagingCanBeDone[tempGiftID]++;
                if (packagingCanBeDone[tempGiftID] == 2)
                {
                    pthread_mutex_unlock(&lockGift4);

                    pthread_mutex_lock(&lockID);
                    int tempTaskID = taskID;
                    taskID++;
                    pthread_mutex_unlock(&lockID);

                    getSecond();
                    timeArrival = time2;

                    // create packageging Task
                    newTask->GiftID = nextTask->GiftID;
                    newTask->GiftType = nextTask->GiftType;
                    newTask->RequesT = nextTask->RequesT;
                    newTask->TaskType = 'C';
                    newTask->ID = tempTaskID;
                    newTask->TaskArrival = timeArrival;

                    pthread_mutex_lock(&lockQue);
                    Enqueue(PackagingQue, *newTask);
                    pthread_mutex_unlock(&lockQue);
                    // free(nextTask);
                    continue;
                }
                else
                {
                    pthread_mutex_unlock(&lockGift4);
                    free(newTask);
                    continue;
                }
            }
            // Gift is Type 2
            else
            {
                pthread_mutex_lock(&lockID);
                int tempTaskID = taskID;
                taskID++;
                pthread_mutex_unlock(&lockID);

                getSecond();
                timeArrival = time2;

                // create packageging Task
                newTask->GiftID = nextTask->GiftID;
                newTask->GiftType = nextTask->GiftType;
                newTask->RequesT = nextTask->RequesT;
                newTask->TaskType = 'C';
                newTask->ID = tempTaskID;
                newTask->TaskArrival = timeArrival;

                pthread_mutex_lock(&lockQue);
                Enqueue(PackagingQue, *newTask);
                pthread_mutex_unlock(&lockQue);
                continue;
            }
        }
        else 
        {
            pthread_mutex_unlock(&lockQue);
        }
    }
}

void *ElfB(void *arg)
{
    while (TRUE)
    {
        Task *nextTask;
        Task *newTask = malloc(sizeof(Task));

        int timeFinished;
        int timeArrival;

        pthread_mutex_lock(&lockQue);
        if (!isEmpty(PackagingQue))
        {
            *nextTask = Dequeue(PackagingQue);
            pthread_mutex_unlock(&lockQue);

            nextTask->Responsible = 'B';

            pthread_sleep(1);

            getSecond();
            timeFinished = time2;

            // Old Task
            nextTask->TT = timeFinished - nextTask->TaskArrival;

            appendLogFile(nextTask);

            // Lock taskID before updating it
            pthread_mutex_lock(&lockID);
            int tempTaskID = taskID;
            taskID++;
            pthread_mutex_unlock(&lockID);

            getSecond();
            timeArrival = time2;

            // Create new Task
            newTask->ID = tempTaskID;
            newTask->GiftID = nextTask->GiftID;
            newTask->GiftType = nextTask->GiftType;
            newTask->TaskType = 'D';
            newTask->RequesT = nextTask->RequesT;
            newTask->TaskArrival = timeArrival;

            // Lock before enque
            pthread_mutex_lock(&lockQue);
            Enqueue(DeliveryQue, *newTask);
            pthread_mutex_unlock(&lockQue);
            continue;
        }


        // Check if PaintingQue is empty
        if (!isEmpty(AssemblyQue))
        {
            // Deque next Painting task
            *nextTask = Dequeue(AssemblyQue);
            pthread_mutex_unlock(&lockQue);

            nextTask->Responsible = 'B';
            pthread_sleep(2);
            int tempGiftID = nextTask->GiftID;

            // Get timiFinished for old Task
            getSecond();
            timeFinished = time2;

            // Update old thread
            nextTask->TT = timeFinished - nextTask->TaskArrival;

            appendLogFile(nextTask);

            // Gift is Type 5
            if (nextTask->GiftType == 5)
            {
                pthread_mutex_lock(&lockGift5);
                packagingCanBeDone[tempGiftID]++;
                if (packagingCanBeDone[tempGiftID] == 2)
                {
                    pthread_mutex_unlock(&lockGift5);

                    pthread_mutex_lock(&lockID);
                    int tempTaskID = taskID;
                    taskID++;
                    pthread_mutex_unlock(&lockID);

                    getSecond();
                    timeArrival = time2;

                    // create packageging Task
                    newTask->GiftID = nextTask->GiftID;
                    newTask->GiftType = nextTask->GiftType;
                    newTask->RequesT = nextTask->RequesT;
                    newTask->TaskType = 'C';
                    newTask->ID = tempTaskID;
                    newTask->TaskArrival = timeArrival;

                    pthread_mutex_lock(&lockQue);
                    Enqueue(PackagingQue, *newTask);
                    pthread_mutex_unlock(&lockQue);
                    // free(nextTask);
                    continue;
                }
                else
                {
                    pthread_mutex_unlock(&lockGift5);
                    free(newTask);
                    continue;
                }
            }
            // Gift is Type 3
            else
            {
                pthread_mutex_lock(&lockID);
                int tempTaskID = taskID;
                taskID++;
                pthread_mutex_unlock(&lockID);

                getSecond();
                timeArrival = time2;

                // create packageging Task
                newTask->GiftID = nextTask->GiftID;
                newTask->GiftType = nextTask->GiftType;
                newTask->RequesT = nextTask->RequesT;
                newTask->TaskType = 'C';
                newTask->ID = tempTaskID;
                newTask->TaskArrival = timeArrival;

                pthread_mutex_lock(&lockQue);
                Enqueue(PackagingQue, *newTask);
                pthread_mutex_unlock(&lockQue);
                continue;
            }
            
        }
        else
        {
            pthread_mutex_unlock(&lockQue);
        }
    }
}

// manages Santa's tasks
void *Santa(void *arg)
{
    while (TRUE)
    {
        Task *nextTask;
        Task *newTask = malloc(sizeof(Task));

        int timeFinished;
        int timeArrival;

        pthread_mutex_lock(&lockQue);
        if (!isEmpty(DeliveryQue))
        {
            *nextTask = Dequeue(DeliveryQue);
            pthread_mutex_unlock(&lockQue);

            nextTask->Responsible = 'S';

            pthread_sleep(1);

            getSecond();
            timeFinished = time2;

            // Update old Task
            nextTask->TT = timeFinished - nextTask->TaskArrival;

            appendLogFile(nextTask);
            free(newTask);

            continue;
        }

        // Check if Assembly is empty
        if (!isEmpty(QAQue))
        {
            // Deque next Painting task
            *nextTask = Dequeue(QAQue);
            pthread_mutex_unlock(&lockQue);
            
            nextTask->Responsible = 'S';

            pthread_sleep(1);
            
            getSecond();
            timeFinished = time2;

            // Update old Task
            nextTask->TT = timeFinished - nextTask->TaskArrival;

            appendLogFile(nextTask);

            int tempGiftID = nextTask->GiftID;
            int tempGiftType = nextTask->GiftType;

            if (tempGiftType == 4)
            {

                pthread_mutex_lock(&lockGift4);
            }

            if (tempGiftType == 5)
            {
                pthread_mutex_lock(&lockGift5);
            }

            packagingCanBeDone[tempGiftID]++;
            if (packagingCanBeDone[tempGiftID] == 2)
            {
                if (tempGiftType == 4)
                {
                    pthread_mutex_unlock(&lockGift4);
                }

                if (tempGiftType == 5)
                {
                    pthread_mutex_unlock(&lockGift5);
                }

                // Lock before
                pthread_mutex_lock(&lockID);
                int tempTaskID = taskID;
                taskID++;
                pthread_mutex_unlock(&lockID);

                getSecond();
                timeArrival = time2;

                // create packageging Task
                newTask->GiftID = nextTask->GiftID;
                newTask->GiftType = nextTask->GiftType;
                newTask->TaskType = 'C';
                newTask->ID = tempTaskID;
                newTask->RequesT = nextTask->RequesT;
                newTask->TaskArrival = timeArrival;

                pthread_mutex_lock(&lockQue);
                Enqueue(PackagingQue, *newTask);
                pthread_mutex_unlock(&lockQue);

                // free(nextTask);

                continue;
            }
            else
            {
                if (tempGiftType == 4)
                {
                    pthread_mutex_unlock(&lockGift4);
                }

                if (tempGiftType == 5)
                {
                    pthread_mutex_unlock(&lockGift5);
                }
                free(newTask);
                continue;
            }
        }
        else
        {
            pthread_mutex_unlock(&lockQue);
        }
    }
}

// the function that controls queues and output
void *ControlThread(void *arg)
{

    while (TRUE)
    {
        getSecond();
        int currTime = time2;

        // to randomly select gift type
        int giftType = rand() % 20;

        if (((currTime % n) == 0) && (currTime != 0))
        {
            printQueues(currTime);
        }

        // Type 1 Gift (only Chocolate)
        if ((giftType >= 0) && (giftType <= 7))
        {
            pthread_mutex_lock(&lockID);
            int tempTaskID = taskID;
            taskID++;
            // int tempTaskID = taskID;
            pthread_mutex_unlock(&lockID);

            // Create the Task
            Task *TaskChoclate = malloc(sizeof(Task));
            TaskChoclate->ID = tempTaskID;
            TaskChoclate->GiftID = giftID;
            TaskChoclate->TaskType = 'C';
            TaskChoclate->GiftType = 1;
            TaskChoclate->RequesT = currTime;
            TaskChoclate->TaskArrival = currTime;

            // Enter Critical Section
            pthread_mutex_lock(&lockQue);

            Enqueue(PackagingQue, *TaskChoclate);

            // Exit Critical section
            pthread_mutex_unlock(&lockQue);
        }

        // Type 2 Gift (Wooden Toy + Chocolate)
        else if ((giftType >= 8) && (giftType <= 11))
        {
            pthread_mutex_lock(&lockID);
            int tempTaskID = taskID;
            taskID++;
            pthread_mutex_unlock(&lockID);

            Task *TaskWoodenT = malloc(sizeof(Task));

            TaskWoodenT->ID = tempTaskID;
            TaskWoodenT->GiftID = giftID;
            TaskWoodenT->TaskType = 'P';
            TaskWoodenT->GiftType = 2;
            TaskWoodenT->RequesT = currTime;
            TaskWoodenT->TaskArrival = currTime;

            // Enter Critical Section
            pthread_mutex_lock(&lockQue);

            // Enqueue(PackagingQue, *TaskChoclate);
            Enqueue(PaintingQue, *TaskWoodenT);

            // Exit Critical section
            pthread_mutex_unlock(&lockQue);
        }

        // Type 3 Gift (Plastic Toy + Chocolate)
        else if ((giftType >= 12) && (giftType <= 15))
        {
            pthread_mutex_lock(&lockID);
            int tempTaskID = taskID;
            taskID++;
            pthread_mutex_unlock(&lockID);

            Task *TaskPlasticT = malloc(sizeof(Task));

            // taskID++;
            TaskPlasticT->ID = tempTaskID;
            TaskPlasticT->GiftID = giftID;
            TaskPlasticT->TaskType = 'A';
            TaskPlasticT->GiftType = 3;
            TaskPlasticT->RequesT = currTime;
            TaskPlasticT->TaskArrival = currTime;

            // Enter Critical Section
            pthread_mutex_lock(&lockQue);

            // Enqueue(PackagingQue, *TaskChoclate);
            Enqueue(AssemblyQue, *TaskPlasticT);
            // Exit Critical section
            pthread_mutex_unlock(&lockQue);
        }

        // Type 4 Gift (Wooden Toy + GameStation5 + Chocolate)
        else if (giftType == 16)
        {
            // Task *TaskChoclate = malloc(sizeof(Task));
            pthread_mutex_lock(&lockID);
            int tempTaskID = taskID;
            int tempTaskID_2 = tempTaskID + 1;
            taskID += 2;
            pthread_mutex_unlock(&lockID);

            Task *TaskWoodenT = malloc(sizeof(Task));
            Task *TaskGameS = malloc(sizeof(Task));

            TaskWoodenT->ID = tempTaskID;
            TaskWoodenT->GiftID = giftID;
            TaskWoodenT->TaskType = 'P';
            TaskWoodenT->GiftType = 4;
            TaskWoodenT->RequesT = currTime;
            TaskWoodenT->TaskArrival = currTime;

            TaskGameS->ID = tempTaskID_2;
            TaskGameS->GiftID = giftID;
            TaskGameS->TaskType = 'Q';
            TaskGameS->GiftType = 4;
            TaskGameS->RequesT = currTime;
            TaskGameS->RequesT = currTime;

            // Enter Critical Section
            pthread_mutex_lock(&lockQue);

            // Enqueue(PackagingQue, *TaskChoclate);
            Enqueue(PaintingQue, *TaskWoodenT);
            Enqueue(QAQue, *TaskGameS);
            // Exit Critical section
            pthread_mutex_unlock(&lockQue);
        }

        // Type 5 Gift (Plastic Toy + GameStation5 + Chocolate)
        else if (giftType == 17)
        {
            // Task *TaskChoclate = malloc(sizeof(Task));
            pthread_mutex_lock(&lockID);
            int tempTaskID = taskID;
            int tempTaskID_2 = tempTaskID + 1;
            taskID += 2;
            pthread_mutex_unlock(&lockID);

            Task *TaskPlasticT = malloc(sizeof(Task));
            Task *TaskGameS = malloc(sizeof(Task));

            // taskID;
            TaskPlasticT->ID = tempTaskID;
            TaskPlasticT->GiftID = giftID;
            TaskPlasticT->TaskType = 'A';
            TaskPlasticT->GiftType = 5;
            TaskPlasticT->RequesT = currTime;
            TaskPlasticT->TaskArrival = currTime;

            TaskGameS->ID = tempTaskID_2;
            TaskGameS->GiftID = giftID;
            TaskGameS->TaskType = 'Q';
            TaskGameS->GiftType = 5;
            TaskGameS->RequesT = currTime;
            TaskGameS->TaskArrival = currTime;

            // Enter Critical Section
            pthread_mutex_lock(&lockQue);

            // Enqueue(PackagingQue, *TaskChoclate);
            Enqueue(AssemblyQue, *TaskPlasticT);
            Enqueue(QAQue, *TaskGameS);
            // Exit Critical section
            pthread_mutex_unlock(&lockQue);
        }
        // No Gift is created
        else
        {
            continue;
        }
        pthread_sleep(1);
        giftID++;
    }
}

void appendLogFile(Task *task)
{
    char buffer[ARRAY_SIZE];

    int taskID = task->ID;
    int giftID = task->GiftID;
    int giftType = task->GiftType;
    char TaskType = task->TaskType;
    int requestTime = task->RequesT;
    int taskArrival = task->TaskArrival;
    int TT = task->TT;
    char responsible = task->Responsible;

    sprintf(buffer, "%-10d%-10d%-10d%-12c%-13d%-14d%-6d%-10c \n",
            taskID, giftID, giftType,
            TaskType, requestTime, taskArrival, TT, responsible);

    pthread_mutex_lock(&lockFile);
    logFile = fopen("events1.log", "a");
    fputs(buffer, logFile);
    fclose(logFile);
    pthread_mutex_unlock(&lockFile);
}

void initializeLogFile()
{
    char buffer[ARRAY_SIZE];

    char taskID[] = "TaskID";
    char giftID[] = "GiftID";
    char giftType[] = "GiftType";
    char TaskType[] = "TaskType";
    char requestTime[] = "RequestTime";
    char taskArrival[] = "TaskArrival";
    char TT[] = "TT";
    char responsible[] = "Responsible";

    sprintf(buffer, "%-10s%-10s%-10s%-12s%-14s%-13s%-6s%-10s \n",
            taskID, giftID, giftType,
            TaskType, requestTime, taskArrival, TT, responsible);

    pthread_mutex_lock(&lockFile);
    logFile = fopen("events1.log", "w");
    fputs(buffer, logFile);
    fclose(logFile);
    pthread_mutex_unlock(&lockFile);
}

void printQueues(int currTime)
{
    int id;
    char buffer[ARRAY_SIZE];
    NODE *elem;
    Task *p;

    printf("\n          Queue Status\n");
    printf("************************************************** \n");

    pthread_mutex_lock(&lockQue);
    // Painting Queue
    printf("At %d sec painting  : ", currTime);
    if ((PaintingQue->size) > 0)
    {
        int leng = PaintingQue->size;
        int j;
        id;

        elem = PaintingQue->head;
        p = &(elem->data);
        id = p->ID;
        printf(" %d", id);

        for (j = 1; j < leng; j++)
        {
            elem = elem->prev;
            p = &(elem->data);
            int id = p->ID;
            printf(", %d", id);
        }
    }

    printf("\n");

    // Assembly Queue
    printf("At %d sec assembly  : ", currTime);
    if ((AssemblyQue->size) > 0)
    {
        int leng = AssemblyQue->size;
        int j;
        id;

        elem = AssemblyQue->head;
        p = &(elem->data);
        id = p->ID;
        printf(" %d", id);

        for (j = 1; j < leng; j++)
        {
            elem = elem->prev;
            p = &(elem->data);
            int id = p->ID;
            printf(", %d", id);
        }
    }

    printf("\n");

    printf("At %d sec packaging  : ", currTime);
    if ((PackagingQue->size) > 0)
    {
        int leng = PackagingQue->size;
        int j;
        id;

        elem = PackagingQue->head;
        p = &(elem->data);
        id = p->ID;
        printf(" %d", id);

        for (j = 1; j < leng; j++)
        {
            elem = elem->prev;
            p = &(elem->data);
            int id = p->ID;
            printf(", %d", id);
        }
    }

    printf("\n");

    // Delivery Queue
    printf("At %d sec delivery  : ", currTime);
    if ((DeliveryQue->size) > 0)
    {
        int leng = DeliveryQue->size;
        int j;
        id;

        elem = DeliveryQue->head;
        p = &(elem->data);
        id = p->ID;
        printf(" %d", id);

        for (j = 1; j < leng; j++)
        {
            elem = elem->prev;
            p = &(elem->data);
            int id = p->ID;
            printf(", %d", id);
        }
    }

    printf("\n");

    // QA Queue
    printf("At %d sec QA  : ", currTime);
    if ((QAQue->size) > 0)
    {
        int leng = QAQue->size;
        int j;
        id;

        elem = QAQue->head;
        p = &(elem->data);
        id = p->ID;
        printf(" %d", id);

        for (j = 1; j < leng; j++)
        {
            elem = elem->prev;
            p = &(elem->data);
            int id = p->ID;
            printf(", %d", id);
        }
    }

    pthread_mutex_unlock(&lockQue);
    printf("\n");
    printf("\n");
    printf("\n");
}

void getSecond()
{
    gettimeofday(&tv, &tz);
    struct tm *currTime;
    currTime = localtime(&tv.tv_sec);

    struct timeval timeDiff = {tv.tv_sec - start_tv.tv_sec, tv.tv_usec - start_tv.tv_usec};

    // update time2
    time2 = timeDiff.tv_sec;
}