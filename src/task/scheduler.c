#include "utils/linked_list.h"
#include "task/thread.h"
#include "task/scheduler.h"

static linked_list_t ready_tasks;
static linked_list_t blocking_tasks;

void init_scheduler() {
  ready_tasks = create_linked_list();
  blocking_tasks = create_linked_list();
}
