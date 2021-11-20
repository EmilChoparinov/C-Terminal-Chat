#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static char *checklist[] = {
    "--",    ";--",    ";",      "/*",         "*/",         "@@",
    "@",     "char",   "nchar",  "varchar",    "nvarchar",   "alter",
    "begin", "cast",   "create", "cursor",     "declare",    "delete",
    "drop",  "end",    "exec",   "execute",    "fetch",      "insert",
    "kill",  "select", "sys",    "sysobjects", "syscolumns", "table",
    "update"};

int sic_is_sql_ok(char *query) {
    unsigned long checklist_size = sizeof(checklist) / sizeof(checklist[0]);
    for (int i = 0; i < checklist_size; i++) {
        char *word = checklist[i];
        // check lower
        if (strstr(query, word) != NULL) {
            return 1;
        }

        // convert string to uppercase
        char *upper_word = (char *)malloc(strlen(word) + 1);
        strcpy(upper_word, word);
        unsigned long word_size = strlen(upper_word);
        for (int j = 0; j < word_size; j++) {
            word[j] = toupper(word[j]);
        }

        // check upper
        if (strstr(query, word) != NULL) {
            return 1;
        }
    }
    return 0;
}