#include "rand_malloc.h"
#include <stddef.h>
#include <stdio.h>
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

char *getLine(LineStatus *status, size_t *maxNum)
{
    size_t buffSize = 1024;
    size_t length = 0;
    size_t numCnt = 0;
    char *buffer = (char *)malloc(sizeof(char) * buffSize);

    if (buffer == NULL)
    {
        if (status != NULL)
        {
            *status = LINE_STATUS_SKIP;
        }
        discardLine(getchar());
        return NULL;
    }

    int ch;

    while ((ch = getchar()) != EOF)
    {

        if (!(isspace((unsigned char)ch) || (ch >= '0' && ch <= '7')))
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

        if (length + 1 >= buffSize)
        {
            buffSize *= 2;
            char *tmp = realloc(buffer, buffSize);
            if (tmp == NULL)
            {
                free(buffer);
                if (status != NULL)
                {
                    *status = LINE_STATUS_SKIP;
                }
                discardLine(ch);
                return NULL;
            }

            buffer = tmp;
        }

        buffer[length++] = (char)ch;

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

    if (*maxNum < numCnt)
    {
        *maxNum = numCnt;
    }
    return buffer;
}

char **getLines(size_t *lineCount, size_t *maxNum)
{
    size_t capacity = 8;
    size_t count = 0;
    char **lines = malloc(capacity * sizeof(*lines));

    if (lineCount != NULL)
    {
        *lineCount = 0;
    }

    if (lines == NULL)
    {
        return NULL;
    }

    for (;;)
    {
        LineStatus status = LINE_STATUS_OK;
        char *line = getLine(&status, maxNum);

        if (line == NULL)
        {
            if (status == LINE_STATUS_SKIP)
            {
                continue;
            }
            break;
        }

        if (count == capacity)
        {
            capacity *= 2;
            char **tmp = realloc(lines, capacity * sizeof(*tmp));

            if (tmp == NULL)
            {
                free(line);
                for (size_t i = 0; i < count; ++i)
                {
                    free(lines[i]);
                }
                free(lines);
                return NULL;
            }

            lines = tmp;
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

int formatLines(char **lines, size_t lineCount)
{

    if (lineCount == 0)
    {
        return 0;
    }

    for (size_t i = 0; i < lineCount; i++)
    {
        deleteWhitespaces(lines[i]);
    }

    if (!addLines(lines, lineCount))
    {
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
    (void)argc;
    (void)argv;

    size_t lineCount = 0;
    size_t maxNum = 0;
    char **lines = getLines(&lineCount, &maxNum);

    if (lines == NULL || lineCount < 1)
    {
        fprintf(stderr, "Program failed\n");
        return 1;
    }

    if (!formatLines(lines, lineCount))
    {
        fprintf(stderr, "Program failed\n");
        freeLines(lines, lineCount);
        return 1;
    }

    printf("%s \n", lines[lineCount - 1]);

    freeLines(lines, lineCount);

    return 0;
}
