#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <mysql.h>

#define VERSION "mysql_udf_socket 1.0.0"

#define D_LEN 128
#define P_TCP "tcp"
#define P_UNIX "unix"
#define S_TIMEO 2

bool mysql_udf_socket_info_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    if (args->arg_count != 0)
    {
        strcpy(message, "no arguments allowed");
        return 1;
    }
    return 0;
}

char *mysql_udf_socket_info(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long int *length, char *is_null, char *error)
{
    strcpy(result, VERSION);
    *length = strlen(VERSION);
    return result;
}

bool mysql_udf_socket_send_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    if (args->arg_count != 2 || args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT)
    {
        strcpy(message, "wrong arguments");
        return 1;
    }
    if (strncmp(P_TCP, args->args[0], strlen(P_TCP)) != 0 && strncmp(P_UNIX, args->args[0], strlen(P_UNIX)) != 0)
    {
        strcpy(message, "unsupported protocol");
        return 1;
    }
    if (args->lengths[1] == 0)
    {
        strcpy(message, "empty message");
        return 1;
    }
    if (args->lengths[1] > D_LEN)
    {
        strcpy(message, "message too long");
        return 1;
    }
    return 0;
}

char *mysql_udf_socket_send(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
    int sockfd;
    char dsn[D_LEN];
    memcpy(dsn, args->args[0], args->lengths[0]);
    dsn[args->lengths[0]] = 0;
    char proto[4];
    char uri[121];
    sscanf(dsn, "%99[^:]://%99s", proto, uri);
    char message[D_LEN];
    memcpy(message, args->args[1], args->lengths[1]);
    message[args->lengths[1]] = 0;
    if (strcmp(proto, P_TCP) == 0)
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            strcpy(result, "ERROR|socket");
            *length = 12;
            return result;
        }
        char addr[15];
        int port = 80;
        sscanf(uri, "%99[^:]:%99d", addr, &port);
        struct sockaddr_in serv_addr;
        memset((char *)&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_addr.s_addr = inet_addr(addr);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        struct timeval timeout;
        timeout.tv_sec = S_TIMEO;
        timeout.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        {
            strcpy(result, "ERROR|socket");
            *length = 12;
            return result;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        {
            strcpy(result, "ERROR|socket");
            *length = 12;
            return result;
        }
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            strcpy(result, "ERROR|connecting");
            *length = 16;
            return result;
        }
        if (send(sockfd, message, strlen(message), 0) < 0)
        {
            strcpy(result, "ERROR|sending");
            *length = 13;
            return result;
        }
        memset(message, 0, sizeof(message));
        if (recv(sockfd, message, D_LEN, 0) < 0)
        {
            strcpy(result, "ERROR|receiving");
            *length = 15;
            return result;
        }
    }
    else
    {
        if (access(uri, R_OK | W_OK) == -1)
        {
            strcpy(result, "ERROR|opening");
            *length = 13;
            return result;
        }
        sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            strcpy(result, "ERROR|opening");
            *length = 13;
            return result;
        }
        struct sockaddr_un serv_addr;
        memset((char *)&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sun_family = AF_UNIX;
        strcpy(serv_addr.sun_path, uri);
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            strcpy(result, "ERROR|connecting");
            *length = 16;
            return result;
        }
        if (write(sockfd, message, strlen(message)) < 0)
        {
            strcpy(result, "ERROR|writing");
            *length = 13;
            return result;
        }
        memset(message, 0, sizeof(message));
        if (read(sockfd, message, D_LEN) < 0)
        {
            strcpy(result, "ERROR|reading");
            *length = 13;
            return result;
        }
    }
    close(sockfd);
    strcpy(result, message);
    *length = strlen(message);
    return result;
}
