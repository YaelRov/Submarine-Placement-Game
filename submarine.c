#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>

#define FILE_NAME "board.txt"
#define LOG_FILE "log.txt"

typedef struct
{
    char **board;
    int size;
    int *remainingParts;
    pthread_mutex_t *mutex;
} ThreadArgs;

// Function prototypes
void initializeBoard(char **board, int size);
void placeSubmarines(char **board, int size, int submarineCells);
int isPlacementValid(char **board, int size, int x, int y, int length, int horizontal);
void writeBoardToFile(char **board, int size);
void printBoard(char **board, int size); // Print board function
void *searchSubmarineParts(void *args);

int main()
{
    int N;
    printf("Enter the size of the board (N x N): ");
    scanf("%d", &N);

    // Calculate the number of cells to be occupied by submarines
    int submarineCells = (int)(0.1 * N * N) + rand() % (int)(0.05 * N * N);

    // Allocate memory for the board
    char **board = (char **)malloc(N * sizeof(char *));
    for (int i = 0; i < N; i++)
    {
        board[i] = (char *)malloc(N * sizeof(char));
    }

    // Initialize and populate the board
    srand(time(NULL));
    initializeBoard(board, N);
    placeSubmarines(board, N, submarineCells);

    // Print the board before the game starts
    printf("Initial Board:\n");
    printBoard(board, N);

    // Write the board to a file
    writeBoardToFile(board, N);

    // Shared resources
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    int remainingParts = submarineCells;

    // Create threads
    int numThreads = 4; // Number of players
    pthread_t threads[numThreads];
    ThreadArgs args = {board, N, &remainingParts, &mutex};

    for (int i = 0; i < numThreads; i++)
    {
        pthread_create(&threads[i], NULL, searchSubmarineParts, &args);
    }

    // Wait for threads to finish
    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Print the board after the game ends
    printf("Final Board:\n");
    printBoard(board, N);

    printf("Game over!\n");

    // Free allocated memory
    for (int i = 0; i < N; i++)
    {
        free(board[i]);
    }
    free(board);

    return 0;
}

void initializeBoard(char **board, int size)
{
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            board[i][j] = '.';
        }
    }
}

void placeSubmarines(char **board, int size, int submarineCells)
{
    int placedCells = 0;

    while (placedCells < submarineCells)
    {
        int length = 2 + rand() % 3; // Submarine length between 2 and 4
        int horizontal = rand() % 2; // 0 for vertical, 1 for horizontal
        int x = rand() % size;
        int y = rand() % size;

        if (isPlacementValid(board, size, x, y, length, horizontal))
        {
            // Place the submarine
            for (int i = 0; i < length; i++)
            {
                if (horizontal)
                {
                    board[x][y + i] = 'X';
                }
                else
                {
                    board[x + i][y] = 'X';
                }
            }
            placedCells += length;
        }
    }
}

int isPlacementValid(char **board, int size, int x, int y, int length, int horizontal)
{
    if (horizontal)
    {
        if (y + length > size)
            return 0; // Out of bounds
        for (int i = 0; i < length; i++)
        {
            if (board[x][y + i] != '.')
                return 0; // Overlap
            // Check for adjacent cells around the submarine
            if (x > 0 && board[x - 1][y + i] != '.')
                return 0; // Above
            if (x < size - 1 && board[x + 1][y + i] != '.')
                return 0; // Below
            if (y + i > 0 && board[x][y + i - 1] != '.')
                return 0; // Left
            if (y + i < size - 1 && board[x][y + i + 1] != '.')
                return 0; // Right
        }
    }
    else
    {
        if (x + length > size)
            return 0; // Out of bounds
        for (int i = 0; i < length; i++)
        {
            if (board[x + i][y] != '.')
                return 0; // Overlap
            // Check for adjacent cells around the submarine
            if (y > 0 && board[x + i][y - 1] != '.')
                return 0; // Left
            if (y < size - 1 && board[x + i][y + 1] != '.')
                return 0; // Right
            if (x + i > 0 && board[x + i - 1][y] != '.')
                return 0; // Above
            if (x + i < size - 1 && board[x + i + 1][y] != '.')
                return 0; // Below
        }
    }
    return 1;
}

void writeBoardToFile(char **board, int size)
{
    int fd = open(FILE_NAME, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Calculate file size and extend
    size_t fileSize = size * (size + 1); // Each line + newline character
    if (ftruncate(fd, fileSize) == -1)
    {
        perror("Error truncating file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Map the file to memory
    char *map = mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        perror("Error mapping file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Write the board to the memory-mapped file
    char *ptr = map;
    for (int i = 0; i < size; i++)
    {
        memcpy(ptr, board[i], size);
        ptr += size;
        *ptr = '\n';
        ptr++;
    }

    // Unmap and close the file
    if (munmap(map, fileSize) == -1)
    {
        perror("Error unmapping file");
    }
    close(fd);
}

void printBoard(char **board, int size)
{
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            printf("%c ", board[i][j]);
        }
        printf("\n");
    }
}

void *searchSubmarineParts(void *args)
{
    ThreadArgs *data = (ThreadArgs *)args;

    // Open log file with mmap
    int fd = open(LOG_FILE, O_RDWR | O_CREAT | O_APPEND, 0666);
    if (fd == -1)
    {
        perror("Error opening log file");
        pthread_exit(NULL);
    }

    // Search for submarine parts
    while (1)
    {
        pthread_mutex_lock(data->mutex);

        if (*data->remainingParts < 0)
        {
            pthread_mutex_unlock(data->mutex);
            break;
        }

        int x = rand() % data->size;
        int y = rand() % data->size;

        if (data->board[x][y] == 'X')
        {
            data->board[x][y] = 'V';
            (*data->remainingParts)--;

            printf("Hit at (%d, %d)\n", x, y);
            dprintf(fd, "Hit at (%d, %d)\n", x, y);
        }
        else if (data->board[x][y] == '.')
        {
            printf("Miss at (%d, %d)\n", x, y);
            dprintf(fd, "Miss at (%d, %d)\n", x, y);
        }
        else if (data->board[x][y] == 'V')
        {
            printf("Skip at (%d, %d)\n", x, y);
            dprintf(fd, "Skip at (%d, %d)\n", x, y);
        }

        pthread_mutex_unlock(data->mutex);
        usleep(100000); // Simulate delay
    }

    close(fd);
    pthread_exit(NULL);
}
