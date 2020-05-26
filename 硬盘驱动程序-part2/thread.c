struct task_struct* idle_thread;    // idle线程

static void idle(void* arg UNUSED) {
  while(1) {
      thread_block(TASK_BLOCKED);
      asm volatile ("sti; hlt":::"memory");
  }
}


void thread_yield(void) {
  struct task_struct* cur = running_thread();
  enum intr_status old_status = intr_disable();
  ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
  list_append(&thread_ready_list, &cur->general_tag);
  cur->status = TASK_READY;
  schedule();
  intr_set_status(old_status);
}
