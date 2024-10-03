#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>

void write_to_file(const char *filename, const char *content) {
    // Open the file for writing
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        syslog(LOG_ERR, "Failed to open file: %s", filename);
        return;
    }

    // Write the content to the file
    fprintf(file, "%s\n", content);
    fclose(file);

    // Log the message to syslog
    syslog(LOG_DEBUG, "Writing \"%s\" to %s", content, filename);
}

int main(int argc, char *argv[]) {
    // Open connection to syslog
    openlog("writer", LOG_PID | LOG_CONS, LOG_USER);

    // Check command-line arguments
    if (argc != 3) {
        syslog(LOG_ERR, "Usage: %s <filename> <string>", argv[0]);
        fprintf(stderr, "Usage: %s <filename> <string>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];
    const char *content = argv[2];

    // Write to file and log
    write_to_file(filename, content);

    // Close connection to syslog
    closelog();

    return 0;
}

