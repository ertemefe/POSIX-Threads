COMP 304: Operating Systems Project 2: Santa’s Workshop

Arda Aykan Poyraz - Efe Ertem



# Overview:

This project is a simulation which aims to create and distribute different types of gift packages to children. There are four types of gifts: chocolate, wooden toy, plastic toy, gamestation 5. There are several processings needed for these gifts before delivery and Santa can only deliver packages when all the processing is done for a package. Gift processings and durations are listed below:

Chocolate: packaging (1 sec) + delivery (1 sec)

Wooden Toy: assembly (2 sec) + packaging (1 sec) + delivery (1 sec)

Plastic Toy: painting (3 sec) + packaging (1 sec) + delivery (1 sec)

Gamestation 5: quality assurance (1 sec) + packaging (1 sec) + delivery (1 sec)

There exists three “people” working on these tasks:

Santa can do delivery and quality assurance.

Elf A can do assembly and packaging.

Elf B can do painting and packaging.

As there are three “people” which can work on these tasks simultaneously, this project is mainly focused on multi-threading and synchronization. 
# Essential functions and files:
`	`**Queue.c:**

`	`In the queue file we have added several fields in the Task: int GiftID, char TaskType, int RequesT, int TaskArrival, int TT, char Responsible.

`	`**Main Function:**

In the main function, initializations and handling of threads exist. First the threads are created with the methods “elfA, elfB, Santa, ControlThread” then the main thread sleeps for desired simulation time (120 seconds by default) and the execution of the threads that are created takes place. After the simulation time is over the threads are destroyed. Before threads start, we create a log file in order to write the task logs using the “initiliazeLogFile” method we created.

**Worker Functions:**

The principle of these functions are, before every change in the “Task” struct made (in terms of threads accessing the memory), we used mutexes to lock and unlock so that when a thread operates on a value, there will not be starvation. 

`	`**Elf functions:**

`	`Both elf functions are very similar, their difference is the painting/assembly task. In our implementation, first if there exists a packaging task in the package queue, if there is, the appropriate operations are made with locking and unlocking (you can see in the project2.c file), sleep for 1 second, dequeued from the queue and then a new task is created and enqueued, in this case delivery task. The same applies for the painting/assembly tasks. If the queue is occupied, the appropriate operations are made, sleep for 2 or 3 seconds, dequeued from the task and a new task is created (packaging task) and enqueued.

`	`**Santa:**

`	`Santa first checks the delivery queue, if it is occupied, appropriate changes are made with the same principle as mentioned before, sleeps, dequeues from the delivery queue. Then checks the quality assurance queue and the same procedure is followed but also creates a packaging task.

`	`**ControlThread:**

`	`This function creates the gifts according to the probabilities defined in the project pdf using the random number generator and enqueues the gifts to appropriate queues.

`	`**AppendLogFile:**

`	`The task is taken into consideration and appended with a user-friendly format in the log file we created. After each event completion, AppendLogFile is called. It also has separate mutex for multiple threads not trying to append their tasks at the same time. 

`	`**InitializeLogFie:**

`	`Creates an events.log file, which is later used by ApppenLogFile to append the tasks. Each part has its own unique events.log file. For part1 it is *events1.log*, for part2 it is *events2.log,* for part3 it is *events3.log*.

**PrintQueues:**

`	`Each task in each of the queues is taken into consideration and Queue Status (task in the particular queue) is printed on the screen for giving information about the queues. 

`	`**Mutex:**
**
`	`We mainly used pthread\_mutex\_locks. We have lockQue, lockID, lockGift4, lockGift5, lockFile and lockNzQue (this one is just for part3 New Zealand). lockQue and lockNzQue is used whenever a thread wants to enqueue or dequeue from any of the queues (in part3 two different queues).  lockID is used for incrementing the lockID since it is unique for each task. lockGift4 and lockGift5 is used for Type5 and Type4 tasks to check if the other task is done or not. First each task acquires the lock and checks the packageingCanBeDone array for its GiftID  (Gift id is unique for each Gift) increments the value if the value reaches 2 then that thread enqueues the Gift for packaging. This way Type4 and Type5 tasks communicate with each to ensure both of them are done before packaging.

**GetSecond:**

This function uses time.h library to return (currentTime - startTime) in seconds for updating fields related with time for a task. 
time.h library to return (currentTime - startTime) in seconds for updating fields related with time for a task. 
