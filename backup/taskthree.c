#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "taskthree.h"

#define MAX_LINE_LENGTH 1024

// Helper function to unfold lines
void unfold_line(char *result, const char *start) {
    while (*start != '\0') {
        if (*start == '\n' && (*(start + 1) == ' ' || *(start + 1) == '\t')) {
            start += 2; // Skip the newline and the whitespace character
        } else {
            *result++ = *start++;
        }
    }
    *result = '\0';
}

// Function to parse email headers
void parse_email(FILE *fp) {
    char line[MAX_LINE_LENGTH], unfolded[MAX_LINE_LENGTH * 2];
    int hasSubject = 0, hasTo = 0;

    printf("Parsing email headers...\n");
    while (fgets(line, MAX_LINE_LENGTH, fp) != NULL) {
        // Convert line to lowercase for case-insensitive comparison
        for (int i = 0; line[i]; i++) {
            line[i] = tolower(line[i]);
        }

        unfold_line(unfolded, line);

        if (strncmp(unfolded, "from:", 5) == 0) {
            printf("From: %s", unfolded + 6);
        } else if (strncmp(unfolded, "to:", 3) == 0) {
            hasTo = 1;
            printf("To: %s", unfolded + 4);
        } else if (strncmp(unfolded, "date:", 5) == 0) {
            printf("Date: %s", unfolded + 6);
        } else if (strncmp(unfolded, "subject:", 8) == 0) {
            hasSubject = 1;
            printf("Subject: %s", unfolded + 9);
        } else if (unfolded[0] == '\0') {
            break; // Stop parsing headers if a blank line is encountered
        }
    }

    // Handle missing headers
    if (!hasSubject) {
        printf("Subject: <No subject>\n");
    }
    if (!hasTo) {
        printf("To:\n");
    }
}