#ifndef __WORKER_H__
#define __WORKER_H__

typedef void *(*worker_worker_t) (void *data);
typedef void (*worker_after_t) (void *data);

void worker_run(const char *name, worker_worker_t worker, void *data,
    worker_after_t after);

#endif//__WORKER_H__
