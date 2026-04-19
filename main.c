#include "rand_malloc.h"
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef enum
{
    LINE_STATUS_OK,
    LINE_STATUS_SKIP,
    LINE_STATUS_EOF
} LineStatus;

static void discardLine(int ch)
{
    while (ch != '\n' && ch != EOF)
    {
        ch = getchar();
    }
}

static int isAllowedLineChar(int ch)
{
    return isspace((unsigned char)ch) || (ch >= '0' && ch <= '7');
}

static int growLineBuffer(char **buffer, size_t *buffSize)
{
    size_t newSize = (*buffSize) * 2;
    char *tmp = realloc(*buffer, newSize);
    if (tmp == NULL)
    {
        return 0;
    }

    *buffer = tmp;
    *buffSize = newSize;
    return 1;
}

static int appendCharToLine(char **buffer, size_t *buffSize, size_t *length, int ch)
{
    if (*length + 1 >= *buffSize)
    {
        if (!growLineBuffer(buffer, buffSize))
        {
            return 0;
        }
    }

    (*buffer)[(*length)++] = (char)ch;
    return 1;
}

static void updateMaxDigits(size_t *maxNum, size_t numCnt)
{
    if (maxNum != NULL && *maxNum < numCnt)
    {
        *maxNum = numCnt;
    }
}

void freeLines(char **lines, size_t lineCount)
{
    if (lines == NULL)
    {
        return;
    }

    for (size_t i = 0; i < lineCount; ++i)
    {
        free(lines[i]);
    }

    free(lines);
}

char *getLine(LineStatus *status, size_t *maxNum)
{
    size_t buffSize = 1024;
    size_t length = 0;
    size_t numCnt = 0;
    char *buffer = (char *)malloc(sizeof(char) * buffSize);

    if (buffer == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for line, Aborting...\n");
        return NULL;
    }

    int ch;

    while ((ch = getchar()) != EOF)
    {
        if (!isAllowedLineChar(ch))
        {
            fprintf(stderr, "Error: only digits 0-7 and whitespace are allowed \n");
            free(buffer);
            if (status != NULL)
            {
                *status = LINE_STATUS_SKIP;
            }
            discardLine(ch);
            return NULL;
        }

        if (ch >= '0' && ch <= '7')
        {
            numCnt++;
        }

        if (!appendCharToLine(&buffer, &buffSize, &length, ch))
        {
            free(buffer);
            if (status != NULL)
            {
                *status = LINE_STATUS_SKIP;
            }
            discardLine(ch);
            return NULL;
        }

        if (ch == '\n')
            break;
    }

    if (length == 0 && ch == EOF)
    {
        free(buffer);
        if (status != NULL)
        {
            *status = LINE_STATUS_EOF;
        }
        return NULL;
    }

    buffer[length] = '\0';
    if (status != NULL)
    {
        *status = LINE_STATUS_OK;
    }

    updateMaxDigits(maxNum, numCnt);

    return buffer;
}

static char **allocateLinesBuffer(size_t capacity)
{
    return malloc(capacity * sizeof(char *));
}

static int growLinesBuffer(char ***lines, size_t *capacity)
{
    size_t newCapacity = (*capacity) * 2;
    char **tmp = realloc(*lines, newCapacity * sizeof(*tmp));
    if (tmp == NULL)
    {
        return 0;
    }

    *lines = tmp;
    *capacity = newCapacity;
    return 1;
}

static int handleGetLineFailure(LineStatus status, char **lines, size_t count)
{
    if (status == LINE_STATUS_SKIP)
    {
        return 1;
    }

    if (status == LINE_STATUS_EOF)
    {
        return 0;
    }

    freeLines(lines, count);
    return -1;
}

char **getLines(size_t *lineCount, size_t *maxNum)
{
    size_t capacity = 8;
    size_t count = 0;
    char **lines = allocateLinesBuffer(capacity);

    if (lineCount != NULL)
    {
        *lineCount = 0;
    }

    if (lines == NULL)
    {
        fprintf(stderr, "Failed to allocate lines buffer");
        return NULL;
    }

    for (;;)
    {
        LineStatus status = LINE_STATUS_OK;
        char *line = getLine(&status, maxNum);

        if (line == NULL)
        {
            int action = handleGetLineFailure(status, lines, count);
            if (action > 0)
            {
                continue;
            }

            if (action == 0)
            {
                break;
            }

            return NULL;
        }

        if (count == capacity)
        {
            if (!growLinesBuffer(&lines, &capacity))
            {
                free(line);
                freeLines(lines, count);
                return NULL;
            }
        }

        lines[count++] = line;
    }

    if (lineCount != NULL)
    {
        *lineCount = count;
    }

    if (count == 0)
    {
        free(lines);
        return NULL;
    }

    return lines;
}

void deleteWhitespaces(char *line)
{
    if (line == NULL)
    {
        return;
    }

    size_t read = 0;
    size_t write = 0;

    while (line[read] != '\0')
    {
        if (!isspace((unsigned char)line[read]))
        {
            line[write++] = line[read];
        }
        read++;
    }

    line[write] = '\0';
}

void trimLeadingZeros(char *line)
{
    if (line == NULL || line[0] == '\0')
    {
        return;
    }

    size_t start = 0;
    while (line[start] == '0' && line[start + 1] != '\0')
    {
        start++;
    }

    if (start > 0)
    {
        memmove(line, line + start, strlen(line + start) + 1);
    }
}

int padLineWithZeros(char **line, size_t targetLen)
{
    if (line == NULL || *line == NULL)
    {
        return 0;
    }

    size_t len = strlen(*line);
    size_t outputLen = targetLen + 1;

    if (len > outputLen)
    {
        return 0;
    }

    size_t pad = outputLen - len;
    char *tmp = malloc(outputLen + 1);
    if (tmp == NULL)
    {
        return 0;
    }

    memset(tmp, '0', pad);
    memcpy(tmp + pad, *line, len + 1);

    free(*line);
    *line = tmp;
    return 1;
}

int addTwoDigits(char **digit1, char **digit2)
{
    if (digit1 == NULL || *digit1 == NULL || digit2 == NULL || *digit2 == NULL)
    {
        return 0;
    }

    size_t digi1Len = strlen(*digit1);
    size_t digi2Len = strlen(*digit2);
    size_t maxLen = digi1Len > digi2Len ? digi1Len : digi2Len;

    if (!padLineWithZeros(digit1, maxLen) || !padLineWithZeros(digit2, maxLen))
    {
        fprintf(stderr, "Failed to pad\n");
        return 0;
    }

    int i = (int)maxLen;
    int carry = 0;

    while (i >= 0)
    {
        int num1 = (*digit1)[i] - '0';
        int num2 = (*digit2)[i] - '0';
        int sum = num1 + num2 + carry;

        (*digit2)[i] = (sum % 8) + '0';
        carry = sum / 8;
        i--;
    }

    return 1;
}

int addLines(char **lines, size_t lineCount)
{
    if (lineCount == 0 || lines == NULL || *lines == NULL)
    {
        return 0;
    }

    for (size_t i = 0; i < lineCount - 1; i++)
    {
        if (!addTwoDigits(&lines[i], &lines[i + 1]))
        {
            fprintf(stderr, "Could not add two digits\n");
            return 0;
        }
    }

    return 1;
}

char **duplicateLines(char **lines, size_t lineCount)
{
    if (lines == NULL || lineCount == 0)
    {
        return NULL;
    }

    char **copies = malloc(lineCount * sizeof(*copies));
    if (copies == NULL)
    {
        return NULL;
    }

    for (size_t i = 0; i < lineCount; i++)
    {
        size_t len = strlen(lines[i]);
        copies[i] = malloc(len + 1);
        if (copies[i] == NULL)
        {
            for (size_t j = 0; j < i; j++)
            {
                free(copies[j]);
            }
            free(copies);
            return NULL;
        }

        memcpy(copies[i], lines[i], len + 1);
    }

    return copies;
}

int formatLines(char **lines, size_t lineCount, char ***sanitizedLines)
{

    if (lineCount == 0 || sanitizedLines == NULL)
    {
        return 0;
    }

    for (size_t i = 0; i < lineCount; i++)
    {
        deleteWhitespaces(lines[i]);
    }

    *sanitizedLines = duplicateLines(lines, lineCount);
    if (*sanitizedLines == NULL)
    {
        return 0;
    }

    if (!addLines(lines, lineCount))
    {
        freeLines(*sanitizedLines, lineCount);
        *sanitizedLines = NULL;
        return 0;
    }

    for (size_t i = 0; i < lineCount; i++)
    {
        trimLeadingZeros(lines[i]);
    }

    return 1;
}

int main(int argc, char *argv[])
{

    size_t lineCount = 0;
    size_t maxNum = 0;
    char **lines = getLines(&lineCount, &maxNum);
    char **sanitizedLines = NULL;

    if (lines == NULL || lineCount < 1)
    {
        fprintf(stderr, "Program failed\n");
        return 1;
    }

    if (!formatLines(lines, lineCount, &sanitizedLines))
    {
        fprintf(stderr, "Program failed\n");
        freeLines(sanitizedLines, lineCount);
        freeLines(lines, lineCount);
        return 1;
    }

    printf("Sum:\n%s\n\n", lines[lineCount - 1]);
    printf("Input numbers:\n");
    for (size_t i = 0; i < lineCount; i++)
    {
        printf("%s\n", sanitizedLines[i]);
    }

    freeLines(sanitizedLines, lineCount);
    freeLines(lines, lineCount);

    return 0;
}
