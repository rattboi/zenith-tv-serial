#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <syslog.h>

#include "commands.h"

void write_command(int fd, const char* cmd) {
  ssize_t written;
  written = write(fd, cmd, strlen(cmd));
  // syslog(LOG_DEBUG, "Wrote command: '%s'; %zi of %zi bytes written.", cmd, written, strlen(cmd));
  printf("Wrote command: '%s'; %zi of %zi bytes written.\n", cmd, written, strlen(cmd));
}

struct Response {
  char partial_command;
  uint8_t television_id;
  char status[2];
  int data;
};

struct Response parse_response(char buf[]) {
  struct Response response;
  int scan_result;
  scan_result = sscanf(
    buf,
    " %c %d %2c%dx ",
    &response.partial_command,
    &response.television_id,
    response.status,
    &response.data);
  if (4 == scan_result) {
    printf("Command: %c\n", response.partial_command);
    printf("Television ID: %i\n", response.television_id);
    printf("Status: %.2s\n", response.status);
    printf("Data: %i", response.data);
  } else {
    perror("error parsing result string");
  }

  return response;
}

int timed_read(int fd, char *buf, ssize_t buf_size) {
  fd_set input;
  struct timeval timeout;
  int select_result;
  int read_count = 0;

  FD_ZERO(&input);
  FD_SET(fd, &input);

  timeout.tv_sec = 0;
  timeout.tv_usec = 50000;

  select_result = select(fd+1, &input, NULL, NULL, &timeout);
  if (select_result > 0) {
    printf("Time remaining: %i microseconds\n", timeout.tv_usec);
    printf("Is the FD ready? %i\n", FD_ISSET(fd, &input));
    memset(buf, 0, buf_size);
    read_count = read(fd, buf, buf_size-1);
    printf("Read %zi bytes of response: %s\n", read_count, buf);
  } else {
    printf("No response received!\n");
  }

  return 0;
}

int status_is_okay(struct Response response) {
  return response.status[0] == 'O'
      && response.status[1] == 'K';
}

COMMAND_STATUS verify_response(struct Response actual, struct Response expected) {
    if (status_is_okay(actual)
      && actual.partial_command == expected.partial_command
      && actual.television_id == expected.television_id
      && actual.data == expected.data) {
      return SUCCESS;
    } else {
      return FAILURE;
    }
}

COMMAND_STATUS write_and_verify_command(int fd, const char* cmd, struct Response expected) {
  char buf[32];
  struct Response actual;
  int read_result;

  write_command(fd, cmd);
  read_result = timed_read(fd, buf, 32);
  if (read_result) {
    actual = parse_response(buf);
    return verify_response(actual, expected);
  } else {
    return TIMEOUT;
  }
}

COMMAND_STATUS tv_power_on(int fd) {
  const char cmd[] = "ka 0 01\r";
  struct Response expected;

  expected.partial_command = 'a';
  expected.television_id = 0;
  expected.data = 1;

  return write_and_verify_command(fd, cmd, expected);
}
