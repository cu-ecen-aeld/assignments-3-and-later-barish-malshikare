
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>
#include <time.h>

#define PORT 9000
#define SOCKETDATA_FILE "/var/tmp/aesdsocketdata"
#define CLIENT_BUFFER_LEN 1024
FILE *tmp_file = NULL;
bool exit_main_loop = false;
typedef struct
{
    int client_fd;
    int file_fd;
    struct sockaddr_storage socket_addr;
} ThreadArgs;

typedef struct thread_Node
{
    pthread_t thread_id;
    SLIST_ENTRY(thread_Node) entry;
}thread_Node;

SLIST_HEAD(ThreadList, thread_Node) head = SLIST_HEAD_INITIALIZER(head);

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_list_mutex = PTHREAD_MUTEX_INITIALIZER;


bool create_daemon()
{
    pid_t pid;
    pid = fork();
    bool status = false;

    if (pid < 0)
    {
        syslog(LOG_ERR, "Fork failed");
        return status;
    }

    if (pid > 0)
    {
        // Parent process hence exit
        exit(EXIT_SUCCESS);
    } 
    
    return true;
}

void handle_sigint(int sig)
{
    printf("Received SIGINT (ctr+C)");
    exit_main_loop = true;
}

void handle_sigterm(int sig)
{
    printf("Received SIGTERM");
    exit_main_loop = true;
}

void initialize_sigaction()
{
    struct sigaction sig_int = {0};
    struct sigaction sig_term = {0};

    // Initialize sigaction
    sig_int.sa_handler = handle_sigint;
    sig_term.sa_handler = handle_sigterm;

    // Catch SIGINT
    if (sigaction(SIGINT, &sig_int, NULL) == -1)
    {
        syslog(LOG_ERR, "Error setting up signal handler SIGINT: %s \n", strerror(errno));
    }

    // Catch SIGTERM
    if (sigaction(SIGTERM, &sig_term, NULL) == -1)
    {
        syslog(LOG_ERR, "Error setting up signal handler SIGINT: %s \n", strerror(errno));
    }
}


int store_received_data(int client_fd, int file_fd)
{
    char *client_buffer = NULL;
    size_t total_received = 0;
    size_t current_size = CLIENT_BUFFER_LEN;
    size_t multiplication_factor = 1;

    // Dynamically allocate initial buffer
    client_buffer = (char *)calloc(current_size, sizeof(char));
    if (client_buffer == NULL)
    {
        syslog(LOG_ERR, "Client buffer allocation failed, returning with error");
        return -1;
    }

    while (true)
    {
        // Receive data from client
        ssize_t received_no_of_bytes = recv(client_fd, client_buffer + total_received, current_size - total_received - 1, 0);
        if (received_no_of_bytes <= 0)
        {
            break; // Connection closed or error
        }
        total_received += received_no_of_bytes;
        client_buffer[total_received] = '\0'; // Null-terminate the string

        // Check for newline
        if (strchr(client_buffer, '\n') != NULL)
        {
            break; // Newline found, exit the loop
        }

        // If we reach this point, we need to resize the buffer
        multiplication_factor <<= 1;
        size_t new_size = multiplication_factor * CLIENT_BUFFER_LEN;
        char *new_buffer = (char *)realloc(client_buffer, new_size);
        if (new_buffer == NULL)
        {
            syslog(LOG_ERR, "Reallocation of client buffer failed, returning with error");
            free(client_buffer);
            return -1;
        }
        
        client_buffer = new_buffer;
        current_size = new_size;
    }

    // Now we have the complete data, store it in the file
    syslog(LOG_INFO, "Writing received data to the sockedata file");
    // Lock the mutex before writing to the file
    pthread_mutex_lock(&file_mutex);
    if (write(file_fd, client_buffer, total_received) != -1)
    {
        syslog(LOG_INFO, "Syncing data to the disk");
        fdatasync(file_fd);
    }
    else
    {
        syslog(LOG_ERR, "Writing received data to the socketdata file failed");
        pthread_mutex_unlock(&file_mutex); //Unlock mutex before returning from function
        free(client_buffer);
        return -1;
    }
    // UnLock the mutex after writing to the file
    pthread_mutex_unlock(&file_mutex);
    free(client_buffer);
    return 0; // Return success
}

int return_data_to_client(int client_fd, int file_fd)
{
    char *send_buffer;
    size_t bytes_read;
    lseek(file_fd, 0, SEEK_SET);
    send_buffer = (char *)malloc(CLIENT_BUFFER_LEN);
    if (send_buffer == NULL)
    {
        syslog(LOG_INFO, "Client buffer was not allocated hence returning with error");
        return -1;
    }

    // Lock the mutex while reading from the file
    pthread_mutex_lock(&file_mutex);
    // Read and send data
    while ((bytes_read = read(file_fd, send_buffer, sizeof(send_buffer) - 1)) > 0)
    {
        send_buffer[bytes_read] = '\0';
        // Send to client
        if (send(client_fd, send_buffer, bytes_read, 0) == -1)
        {
            syslog(LOG_ERR, "Send to client failed: %s", strerror(errno));
            break;
        }
    }
    //Unlock the mutex after reading from file
    pthread_mutex_unlock(&file_mutex); 
    free(send_buffer);
    return 0;
}

int main(int argc, char **argv)
{
    int socket_fd, client_fd;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size;
    int file_fd, status;
    int option = 1;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    bool daemon_mode = false;

    // Check if the application to be run in daemon mode
    if ((argc >= 2) && (strcmp(argv[1], "-d") == 0))
    {
        daemon_mode = true;
    }
    
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        syslog(LOG_ERR, "Error occurred while creating a socket: %s\n", strerror(errno));
        exit(1);
    }
    // Set socket options
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof option) == -1)
    {
        // Set Socket operation has failed, log the details,
        syslog(LOG_ERR, "Error occured while setting a socket option: %s \n", strerror(errno));
        exit(1);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        // Bind operation has failed, log the details,
        syslog(LOG_ERR, "Error occured while binding a socket: %s \n", strerror(errno));        
        exit(1);
    }



    // Check if daemon to be created
    if (daemon_mode)
    {
        if (!create_daemon())
        {
            syslog(LOG_ERR, "Daemon creation failed, hence exiting");            
            exit(1);
        }
    }

    if (listen(socket_fd, 10) == -1)
    {
        // listen operation has failed, log the details,
        syslog(LOG_ERR, "Error occured during listen operation: %s \n", strerror(errno));
        
        exit(1);
    }

    file_fd = open(SOCKETDATA_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (file_fd == -1)
    {
        syslog(LOG_ERR, "Open/create of /var/tmp/aesdsocketdata failed");
        
        exit(1);
    }
    initialize_sigaction();
    client_addr_size = sizeof(client_addr); // Initialize client address size

    while (!exit_main_loop)
    {
        client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_size);
        if (client_fd == -1)
        {
            syslog(LOG_ERR, "Error occured during accept operation: %s \n", strerror(errno));
            continue;
        }
        
        char client_ip[INET_ADDRSTRLEN];
       if (client_addr.ss_family == AF_INET) {
            // IPv4 address
            struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
            if (inet_ntop(AF_INET, &s->sin_addr, client_ip, sizeof(client_ip)) == NULL) {
            }
   		}  


		syslog(LOG_INFO, "Accepted connection from %s", client_ip);
		if (store_received_data(client_fd, file_fd) == 0)
		{
		    return_data_to_client(client_fd, file_fd);
		}
		if (close(client_fd) == 0)
		{
		    syslog(LOG_INFO, "Closed connection from %s", client_ip);
		}
		else
		{
		    syslog(LOG_ERR, "Closing of connection from %s failed", client_ip);
		}
    }
    close(file_fd);    
}
