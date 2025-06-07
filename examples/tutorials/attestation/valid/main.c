#include "libtock/tock.h"
#include <stdio.h>
#include <stdlib.h>

#include <libtock/kernel/ipc.h>

#include <u8g2-tock.h>
#include <u8g2.h>

#define LOG_WIDTH 32

u8g2_t u8g2;

bool   started          = false;
bool   log_done         = false;
size_t screen_service   = -1; 
char   log_buf[LOG_WIDTH] __attribute__((aligned(LOG_WIDTH)));

const char SCREEN_SERVICE_NAME[] = "org.tockos.tutorials.attestation.screen";

static void ipc_callback(__attribute__ ((unused)) int   pid,
                         __attribute__ ((unused)) int   len,
                         __attribute__ ((unused)) int   arg2,
                         __attribute__ ((unused)) void *ud) {
  started = true;
}

static void log_done_callback(int pid, int len, int arg2, void *ud) {
  log_done = true; 
}

void wait_for_start(void) {
  ipc_register_service_callback("org.tockos.tutorials.attestation.valid", ipc_callback,
                                NULL);
  yield_for(&started);
}

int setup_logging() {
  returncode_t ret;
  
  ret = ipc_discover(SCREEN_SERVICE_NAME, &screen_service);
  if (ret != RETURNCODE_SUCCESS) {
    printf("Encryption oracle service not found.\n");
    return ret;
  }

  ipc_register_client_callback(screen_service, log_done_callback, NULL);
  ipc_share(screen_service, log_buf, LOG_WIDTH);

  return 0;
}

int log_to_screen(const char *message) {
  returncode_t ret;

  uint16_t len = strnlen(message, sizeof(log_buf));
  memcpy(log_buf, message, len);

  printf("App printing %s...\n", log_buf);

  // Start the logging process.
  ret = ipc_notify_service(screen_service);
  if (ret != RETURNCODE_SUCCESS) {
    printf("Failed to request a log to screen.\n");
    return ret;
  }

  // Wait for the log to complete.
  yield_for(&log_done);
  log_done = false;
  
  return 0;
}

int main(void) {
  returncode_t ret;

  // Wait to receive the signal to start from the app selector.
  wait_for_start();

  // Set up logging service.
  setup_logging();

  // Try logging something to the screen.
  log_to_screen("Yippee! 1");
  log_to_screen("Yippee! 2");
  log_to_screen("Yippee! 3");
  log_to_screen("Yippee! 4");
  log_to_screen("Yippee! 5");

  while (1) {
    yield();
  }
}
