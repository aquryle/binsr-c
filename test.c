#include <stdio.h>
#include <string.h>

int main(void)
{
    char buffer[256];
    size_t length;

    printf("Input: ");
    if (fgets(buffer, 256, stdin) == NULL) {
        return 1;
    }
    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[--length] = '\0';
    }

    printf("Output: %s\n", buffer);
    printf("Length: %zu\n", length);
    return 0;
}
