#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <time.h>
#include <mysql/mysql.h>
#include <stdlib.h>

int main()
{
    MYSQL con;
    MYSQL_ROW row;
    mysql_init(&con);
    if(mysql_real_connect(&con, NULL, NULL, NULL, "user", 0, NULL, 0) == NULL)
    {
        perror("");
        exit(1);
    }

    int loginfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(loginfd != -1);

    struct sockaddr_in login_ser, login_cli;
    login_ser.sin_family = AF_INET;
    login_ser.sin_port = htons(6000);
    login_ser.sin_addr.s_addr = INADDR_ANY;

    int resLogin = bind(loginfd, (struct sockaddr*)&login_ser, sizeof(login_ser));
    assert(resLogin = -1);

    int lisLogin = listen(loginfd, 5);
    assert(lisLogin != -1);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd != -1);

    struct sockaddr_in ser, cli;
    ser.sin_family = AF_INET;
    ser.sin_port = htons(7000);
    ser.sin_addr.s_addr = INADDR_ANY;

    int res = bind(listenfd, (struct sockaddr*)&ser, sizeof(ser));
    assert(res = -1);

    int lis = listen(listenfd, 5);
    assert(lis != -1);

    int epollfd = epoll_create(10);
    struct epoll_event ev, events[20];
    ev.data.fd = loginfd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, loginfd, &ev);

    int firfd;
    int login_clifd = -1;
    int flag = 0;
    int Login_Flag[5] = {0};
    int tmpfd;
    char buf[128] = {0};
    int tmp[5] = {-1, -1, -1, -1, -1};
    int buffd[5] = {0};
    int j = 0;
    char *name[5] = {0};
    char *timebuf;
    char insert[128] = {"insert into user(username,password) values('"};

    while(1)
    {
        int ewait = epoll_wait(epollfd, events, 10, 500);
        //printf("%d\n", ewait);

        for(int i = 0; i < ewait; i++)
        {
            if(events[i].data.fd == loginfd)
            {
                socklen_t len = sizeof(login_cli);
                login_clifd = accept(loginfd, (struct sockaddr*)&login_cli, &len);
                assert(login_clifd != -1);

                printf("login:%s已连接\n", inet_ntoa(login_cli.sin_addr));

                ev.data.fd = login_clifd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, login_clifd, &ev);
            }
            else if(events[i].data.fd == listenfd)
            {
                socklen_t len = sizeof(cli);
                int clifd = accept(listenfd, (struct sockaddr*)&cli, &len);
                assert(clifd != -1);

                buffd[j] = clifd;
                j++;

                if(flag == 0)
                {
                    firfd = clifd;
                    flag = 1;
                }

                printf("ip:%s已连接\n", inet_ntoa(cli.sin_addr));
                //send(clifd, "注册输入0\n登录输入1\n", 28, 0);

                ev.data.fd = clifd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, clifd, &ev);
            }
            else if(events[i].events & EPOLLIN)
            {
                tmpfd = events[i].data.fd;
                if(tmpfd == login_clifd)
                {
                    memset(buf, 0, 128);
                    ssize_t rec = recv(tmpfd, buf, sizeof(buf), 0);
                    if(rec <= 0 || (strncmp(buf, "end", 3) == 0))
                    {
                        printf("login:%s已断开\n", inet_ntoa(login_cli.sin_addr));
                        close(login_clifd);
                        login_clifd = -1;
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, login_clifd, &ev);
                        ev.data.fd = listenfd;
                        ev.events = EPOLLIN;
                        ev.events = EPOLLIN | EPOLLET;
                        epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);
                        break;
                    }

                    char *np[2] = {0};
                    np[0] = strtok(buf, "\n");
                    np[1] = strtok(NULL, "\n");
                    char select[128] = {"select password from user where username='"};
                    strcat(select, np[0]);
                    strcat(select, "'");

                    int b = mysql_query(&con, select);
                    if(b == 0)
                    {
                        MYSQL_RES *res = mysql_use_result(&con);
                        if(res)
                        {
                            int sFlag = 0;
                            while((row = mysql_fetch_row(res)))
                            {
                                for(i = 0; i < mysql_num_fields(res); i++)
                                {
                                    if(strcmp(row[i], np[1]) == 0)
                                    {
                                        printf("successed\n");
                                        send(events[i].data.fd, "1\n", 2, 0);
                                        sFlag = 1;
                                        //mysql_close(&con);
                                        break;
                                    }
                                }
                                if(sFlag == 1)
                                {
                                    break;
                                }
                            }
                            if(sFlag == 0)
                            {
                                send(tmpfd, "0\n", 2, 0);
                                printf("failed\n");
                            }
                            mysql_free_result(res);
                        }
                        else
                        {
                            perror("3");
                        }
                    }
                    else
                    {
                        perror("4");
                    }
                }
                else
                {
                    int num = tmpfd - firfd;
                    memset(buf, 0, 128);

                    ssize_t rec = recv(tmpfd, buf, sizeof(buf), 0);
                    if(rec <= 0 || (strncmp(buf, "end", 3) == 0))
                    {
                        printf("ip:%s已断开\n", inet_ntoa(cli.sin_addr));
                        tmp[num] = -1;
                        Login_Flag[num] = -1;
                        buffd[num] = 0;
                        name[num] = 0;
                        close(tmpfd);
                        break;
                    }
                    else
                    {
                        printf("fa:%s", buf);
                        fflush(stdin);
                        /*for(int i = 0; i < j; i++)
                        {
                            if(tmpfd != buffd[i])
                            {
                                time_t timep;
                                time(&timep);
                                timebuf = ctime(&timep);*/

                        /*ssize_t sen = send(buffd[i], name[num], strlen(name[num]) - 1, 0);
                        assert(sen != -1);
                        sen = send(buffd[i], " ", 1, 0);

                        sen = send(buffd[i], timebuf, strlen(timebuf), 0);
                        assert(sen != -1);*/

                        //sen = send(buffd[i], "   ", 3, 0);
                        ///ssize_t sen = send(buffd[i], buf, strlen(buf), 0);
                        ///assert(sen != -1);
                        //printf("%s", name[num]);
                    }

                    ev.data.fd = tmpfd;
                    epoll_ctl(epollfd, EPOLL_CTL_MOD, tmpfd, &ev);
                }
            }
        }
        /*else if(events[i].events & EPOLLOUT)
        {
            printf("in\n");
            int tmpfd;
            tmpfd = events[i].data.fd;
            char buf2[128] = {0};
            fgets(buf2, 128, stdin);

            ssize_t sen = send(tmpfd, buf2, sizeof(buf2), 0);
            assert(sen != 0);

            ev.data.fd = tmpfd;
            ev.events = EPOLLIN | EPOLLET;
            epoll_ctl(epollfd, EPOLL_CTL_MOD, tmpfd, &ev);
        }*/
    }
}
