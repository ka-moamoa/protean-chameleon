#ifndef TASK_H_
#define TASK_H_

#define TASK(name) static void *name(void *__task_shared_buffer)

#define NEXT_TASK(name) return name

// reads the value from the scratch buffer
#define __GET(x) ( (FRAM_data_t *)__task_shared_buffer)->x

// returns the address of the variable
#define __GET_ADDR(x) &( (FRAM_data_t *)__task_shared_buffer)->x

// writes the value to the scratch buffer
#define __SET(x,val) ( (FRAM_data_t *)__task_shared_buffer)->x = val

// task definition, buffer will be passed by the runtime
typedef void* (*task_t) (void *);

#endif /* TASK_H_ */
