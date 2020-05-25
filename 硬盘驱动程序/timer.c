#define IRQ0_FREQUENCY	   100

#define mil_seconds_per_intr (1000 / IRQ0_FREQUENCY)
uint32_t ticks;

static void ticks_to_sleep(uint32_t sleep_ticks){
  uint32_t start_tick = ticks;

  while(ticks - start_tick < sleep_ticks){
    thread_yield();
  }
}

void mtime_sleep(uint32_t m_seconds) {
  uint32_t sleep_ticks = DIV_ROUND_UP(m_seconds, mil_seconds_per_intr);
  ASSERT(sleep_ticks > 0);
  ticks_to_sleep(sleep_ticks);
}
