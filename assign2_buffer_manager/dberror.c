#include "dberror.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char *RC_message;

/* print a message to standard out describing the error */
void printError(RC error)
{
    if (RC_message != NULL)
        printf("EC (%i), \"%s\"\n", error, RC_message);
    else
        printf("EC (%i)\n", error);
}

/* return a string describing the error code */
char *errorMessage(RC error)
{
    char *message = (char *) malloc(256);

    switch (error)
    {
        case RC_OK:
            sprintf(message, "RC_OK: Operation successful");
            break;
        case RC_FILE_NOT_FOUND:
            sprintf(message, "RC_FILE_NOT_FOUND: File not found");
            break;
        case RC_FILE_HANDLE_NOT_INIT:
            sprintf(message, "RC_FILE_HANDLE_NOT_INIT: File handle not initialized");
            break;
        case RC_WRITE_FAILED:
            sprintf(message, "RC_WRITE_FAILED: Write operation failed");
            break;
        case RC_READ_NON_EXISTING_PAGE:
            sprintf(message, "RC_READ_NON_EXISTING_PAGE: Tried to read non-existing page");
            break;

        case RC_RM_COMPARE_VALUE_OF_DIFFERENT_DATATYPE:
            sprintf(message, "RC_RM_COMPARE_VALUE_OF_DIFFERENT_DATATYPE: Compare values of different datatype");
            break;
        case RC_RM_EXPR_RESULT_IS_NOT_BOOLEAN:
            sprintf(message, "RC_RM_EXPR_RESULT_IS_NOT_BOOLEAN: Expression result is not boolean");
            break;
        case RC_RM_BOOLEAN_EXPR_ARG_IS_NOT_BOOLEAN:
            sprintf(message, "RC_RM_BOOLEAN_EXPR_ARG_IS_NOT_BOOLEAN: Boolean expression arg is not boolean");
            break;
        case RC_RM_NO_MORE_TUPLES:
            sprintf(message, "RC_RM_NO_MORE_TUPLES: No more tuples");
            break;
        case RC_RM_NO_PRINT_FOR_DATATYPE:
            sprintf(message, "RC_RM_NO_PRINT_FOR_DATATYPE: No print for datatype");
            break;
        case RC_RM_UNKOWN_DATATYPE:
            sprintf(message, "RC_RM_UNKOWN_DATATYPE: Unknown datatype");
            break;

        case RC_IM_KEY_NOT_FOUND:
            sprintf(message, "RC_IM_KEY_NOT_FOUND: Key not found");
            break;
        case RC_IM_KEY_ALREADY_EXISTS:
            sprintf(message, "RC_IM_KEY_ALREADY_EXISTS: Key already exists");
            break;
        case RC_IM_N_TO_LAGE:
            sprintf(message, "RC_IM_N_TO_LAGE: N value too large");
            break;
        case RC_IM_NO_MORE_ENTRIES:
            sprintf(message, "RC_IM_NO_MORE_ENTRIES: No more entries");
            break;

        // ===== Buffer Manager specific codes =====
        case RC_INVALID:
            sprintf(message, "RC_INVALID: Invalid argument or NULL pointer");
            break;
        case RC_PINNED_PAGES:
            sprintf(message, "RC_PINNED_PAGES: Buffer cannot be shut down because pages are still pinned");
            break;
        case RC_PAGE_NOT_FOUND:
            sprintf(message, "RC_PAGE_NOT_FOUND: Requested page not found in buffer");
            break;
        case RC_NO_FREE_SLOT:
            sprintf(message, "RC_NO_FREE_SLOT: No free frame to replace (all pinned)");
            break;

        default:
            sprintf(message, "EC (%d): Unknown error", error);
            break;
    }

    return message;
}
