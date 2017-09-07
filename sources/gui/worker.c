#include "worker.h"

#include <stdlib.h>

#include <gtk/gtk.h>

#include "controller.h"
#include "log.h"

struct worker_param {
  gpointer data;
  worker_worker_t worker;
  worker_after_t after;
  const char *name;
};

int worker_after(gpointer data) {
  struct worker_param *param = (struct worker_param *)data;
  DEBUG("Worker after [%s]\n", param->name);
  if (param->after) {
    param->after(param->data);
  }
  // Unlock gui
  controller_lock(0);
  free(param);
  return 0;
}

gpointer worker_worker(gpointer data) {
  struct worker_param *param = (struct worker_param *)data;
  param->data = param->worker(param->data);
  gdk_threads_add_idle(worker_after, param);
  DEBUG("Worker end [%s]\n", param->name);
  return NULL;
}

void worker_run(const char *name, worker_worker_t worker, void *data,
    worker_after_t after) {
  struct worker_param *param = malloc(sizeof(struct worker_param));
  param->data = data;
  param->worker = worker;
  param->after = after;
  param->name = name;
  DEBUG("Launch worker [%s]\n", param->name);
  // Lock gui
  controller_lock(1);
  g_thread_new(name, worker_worker, param);
}
