#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <gtk/gtk.h>

int main()
{
    pid_t pid = fork();
    if(pid == 0)
    {
        if(execl("/root/code/tcp/chat/login", "login", NULL) == -1)
        {
            perror("");
        }
    }
    else
    {
        int status = -1;
        wait(&status);
        //printf("%d\n", WEXITSTATUS(status));
        if(WEXITSTATUS(status))
        {
            int sockfd = socket(PF_INET, SOCK_STREAM, 0);
            assert(sockfd != -1);

            struct sockaddr_in ser;
            ser.sin_family = AF_INET;
            ser.sin_port = htons(7000);
            ser.sin_addr.s_addr = inet_addr("127.0.0.1");

            int con = connect(sockfd, (struct sockaddr*)&ser, sizeof(ser));
            assert(con != -1);

            int epollfd = epoll_create(5);
            struct epoll_event in, ev,  events[5];
            in.data.fd = STDIN_FILENO;
            ev.data.fd = sockfd;
            in.events = EPOLLIN;
            ev.events = EPOLLIN | EPOLLET;
            epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
            epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &in);

            char buf[128] = {0};
            while(1)
            {
                int ewait = epoll_wait(epollfd, events, 5, 500);

                for(int i = 0; i < ewait; i++)
                {
                    if(events[i].data.fd == STDIN_FILENO)
                    {
                        //printf("suc\n");
                        fflush(stdout);
                        fgets(buf, 128, stdin);
                        ev.events = EPOLLOUT | EPOLLET;
                        //ev.data.fd = sockfd;
                        epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &ev);
                    }
                    else if(events[i].events & EPOLLOUT)
                    {
                        //printf("out\n");

                        ssize_t sen = send(sockfd, buf, strlen(buf), 0);
                        assert(sen != -1);
                        ev.events = EPOLLIN | EPOLLET;
                        ev.data.fd = sockfd;
                        epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &ev);
                    }

                    else if(events[i].events & EPOLLIN)
                    {
                        //printf("in\n");

                        memset(buf, 0, 128);
                        ssize_t rec = recv(sockfd, buf, sizeof(buf), 0);
                        assert(rec != -1);

                        printf("%s", buf);
                        fflush(stdout);

                        ev.data.fd = sockfd;
                        epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &ev);
                    }
                }
            }
            close(sockfd);
        }
        else
        {
            exit(0);
        }
    }
}
