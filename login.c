#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <gtk/gtk.h>
#include <assert.h>
#include <arpa/inet.h>

struct User
{
    GtkWidget *username_entry;
    GtkWidget *password_entry;
    GtkWidget *gtk;
    //int fd;
}*user;

GtkWidget *error_window = NULL;

void Close(GtkWidget *window, gpointer data)
{
    gtk_main_quit();
    exit(0);
}

void CloseErr(GtkWidget *button, gpointer data)
{
    gtk_widget_destroy(error_window);
}

void Error(int num)
{
    error_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(error_window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(error_window), 500, 100);
    gtk_window_set_resizable(GTK_WINDOW(error_window), FALSE);

    GtkWidget *error_label;
    if(num == 1)
    {
        error_label = gtk_label_new("请检查网络连接");
    }
    else
    {
        error_label = gtk_label_new("用户名或者密码错误");
    }
    GtkWidget *error_button = gtk_button_new_with_label("确认");
    GtkWidget *fixed = gtk_fixed_new();
    gtk_widget_set_size_request(error_button, 200, 30);
    gtk_fixed_put(GTK_FIXED(fixed), error_button, 150, 20);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_box_pack_start(GTK_BOX(vbox), error_label, TRUE, FALSE, 30);
    gtk_box_pack_start(GTK_BOX(vbox), fixed, FALSE, FALSE, 5);
    gtk_container_add(GTK_CONTAINER(error_window), vbox);
    gtk_widget_show_all(error_window);
    g_signal_connect(error_button, "button_press_event", G_CALLBACK(CloseErr), error_window);
    g_signal_connect(error_button, "activate", G_CALLBACK(CloseErr), error_window);
    printf("error\n");
}

void ButtonClick(GtkWidget *button, gpointer data)
{
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd != -1);

    struct sockaddr_in ser;
    ser.sin_family = AF_INET;
    ser.sin_port = htons(6000);
    ser.sin_addr.s_addr = inet_addr("127.0.0.1");

    int con = connect(sockfd, (struct sockaddr*)&ser, sizeof(ser));
    if(con == -1)
    {
        Error(1);
    }
    else
    {
        const char *n = gtk_entry_get_text(GTK_ENTRY(user->username_entry));
        const char *p = gtk_entry_get_text(GTK_ENTRY(user->password_entry));
        char np[64] = {0};
        strcpy(np, n);
        strcat(np, "\n");
        strcat(np, p);
        strcat(np, "\n");

        ssize_t sen = send(sockfd, np, strlen(np), 0);
        assert(sen != -1);

        char buf[3] = {0};
        ssize_t rec = recv(sockfd, buf, 2, 0);
        if(rec == -1)
        {
            Error(1);
        }
        else
        {
            if(buf[0] == '1')
            {
                printf("-------\n");
                gtk_main_quit();
                close(sockfd);
                exit(1);
            }
            else
            {
                Error(2);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    user = (struct User*)malloc(sizeof(struct User));

    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "WeChat");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

    user->gtk = window;
    g_signal_connect(window, "destroy", G_CALLBACK(Close), NULL);

    GtkWidget *username_label = gtk_label_new("账号");
    GtkWidget *password_label = gtk_label_new("密码");
    user->username_entry = gtk_entry_new();
    user->password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(user->password_entry), FALSE);

    GtkWidget *button = gtk_button_new_with_label("登录");
    GtkWidget *fixed = gtk_fixed_new();
    gtk_widget_set_size_request(button, 200, 30);
    gtk_fixed_put(GTK_FIXED(fixed), button, 60, 50);
    g_signal_connect(button, "button_press_event", G_CALLBACK(ButtonClick), NULL);
    g_signal_connect(user->password_entry, "activate", G_CALLBACK(ButtonClick), NULL);

    GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_box_pack_start(GTK_BOX(hbox1), username_label, TRUE, FALSE, 30);
    gtk_box_pack_start(GTK_BOX(hbox1), user->username_entry, TRUE, FALSE, 30);
    gtk_box_pack_start(GTK_BOX(hbox2), password_label, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), user->password_entry, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox3), fixed, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox3, FALSE, FALSE, 5);
    //gtk_container_add(GTK_CONTAINER(window), button);

    gtk_container_add(GTK_CONTAINER(window), vbox);

    gtk_widget_show_all(window);
    gtk_main();
}
