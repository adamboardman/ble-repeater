#include "Debugging.h"

CircularBuffer<char> serialLogBuffer(3000);

#define SERIAL_LOG_BUFFER_LEN 160

void printAvailableLogging() {
    static char line[SERIAL_LOG_BUFFER_LEN];
    // check for any logging that may have happened during servicing an interrupt
    if (serialLogBuffer.full()) {
        const auto len = serialLogBuffer.consume_line(line, SERIAL_LOG_BUFFER_LEN-1);
        printf("Buffer Maxed - Ignoring %d bytes tail of partial line\n", len);
    }

    while (!serialLogBuffer.empty()) {
        if (const auto len = serialLogBuffer.consume_line(line, SERIAL_LOG_BUFFER_LEN-1); line[len-1]=='\n') {
            line[len-1] = '\0';
        } else {
            line[len] = '\0';
        }
        printf("%s\n",line);
    }
    serialLogBuffer.clear_if_empty(); //try to keep logging within only one block
}