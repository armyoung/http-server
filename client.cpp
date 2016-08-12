#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include <vector>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "log.h"
#include "protocol.h"

const char* ip = NULL;
int port       = 0;

int Connect(const char* ip, int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        LogErr("socket err %s", strerror(errno));
        return __LINE__;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family         = AF_INET;
    addr.sin_port           = htons(port);

    inet_pton(AF_INET, ip, &(addr.sin_addr.s_addr));

    int ret = connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (ret < 0)
    {
        LogErr("connect err %s", strerror(errno));
        return __LINE__;
    }

    return fd;
}

int Communicate(int fd, int times)
{
    int loop            = 0;
    char buffer[1024]   = {0};
    char response[1024] = {0};
    while (loop++ < times)
    {
        snprintf(buffer,
                 sizeof(buffer),
                 "server %s %d loop %d time %d",
                 ip,
                 port,
                 loop,
                 static_cast<int>(time(NULL)));

        Header header;
        header.size = strlen(buffer) + 1;
        size_t len  = write(fd, &header, sizeof(header));
        if (len < 0)
        {
            LogErr("write %d to_write %u err %s", fd, header.size, strerror(errno));
            continue;
        }

        len = write(fd, buffer, strlen(buffer) + 1);
        if (len < 0)
        {
            LogErr("write %d %s err %s", fd, buffer, strerror(errno));
            continue;
        }

        printf("write %s %d %d buffer %s len %zu\n", ip, port, fd, buffer, len);

        len = read(fd, response, sizeof(response));
        if (len < 0)
        {
            LogErr("read %d err %s", fd, strerror(errno));
            continue;
        }

        printf("read %s %d %d response %s len %zu\n", ip, port, fd, response, len);
    }
    return 0;
}

struct Arg
{
    int total;
    int index;
    std::vector<int>* handles;
};

void* Job(void* in)
{
    Arg* arg                  = reinterpret_cast<Arg*>(in);
    std::vector<int>& handles = *(arg->handles);
    int total                 = arg->total;
    int index                 = arg->index;

    for (int cursor = 0; cursor < 10; ++cursor)
    {
        for (size_t step = 0; step < handles.size(); ++step)
        {
            if (step % total != index)
            {
                continue;
            }

            Communicate(handles[step], 1);
        }
    }
}

void StartPusher(std::vector<int>& handles)
{
    pthread_t tids[100];
    Arg args[100];
    for (int index = 0; index < 100; ++index)
    {
        args[index].total   = 100;
        args[index].index   = index;
        args[index].handles = &handles;

        pthread_create(&tids[index], NULL, Job, &args[index]);
    }
    for (int index = 0; index < 100; ++index)
    {
        pthread_join(tids[index], NULL);
    }
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("%s ip port log\n", argv[0]);
        return 0;
    }

    int ret = Log::Instance()->Init(argv[3]);
    if (ret < 0)
    {
        return ret;
    }

    ip   = argv[1];
    port = atoi(argv[2]);

    std::vector<int> handles;
    while (handles.size() != 100)
    {
        int fd = Connect(ip, port);
        if (fd < 0)
        {
            return fd;
        }

        handles.push_back(fd);

        usleep(1);
    }

    StartPusher(handles);

    for (size_t index = 0; index < handles.size(); ++index)
    {
        close(handles[index]);
    }

    return 0;
}
