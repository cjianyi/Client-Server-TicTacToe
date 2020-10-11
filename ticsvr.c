#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int listenfd;
int port = 3000;
char board[9];
struct client {
    int fd;
    struct in_addr ipaddr;
    struct client *next;
    char s;
    int turn;
    int isplaying;
} *top = NULL;
int howmanyplaying = 0;
int howmany = 0;
static void addclient(int fd, struct in_addr addr, int shape);
static void removeclient(int fd);

int main(int argc, char **argv){
    int shape = 0;
    char bufp[500];
    struct client *p;
    int i, c, clientfd, len, num;
    int status = 0;
    struct sockaddr_in r;
    socklen_t socklen = sizeof(r);
    extern void showboardall();
    extern void showboardone(int fd);
    extern int allthree(int start, int offset);
    extern int isfull();
    extern int game_is_over();
    char buf2[80];
    char buf[500], buf1[500];
    int turn = 0;
    for (i = 0; i < 9; i++){
        board[i] = '1' + i;
    }

    while ((c = getopt(argc, argv, "p:")) != EOF) {
        switch (c) {
        case 'p':
            port = atoi(optarg);
            break;
        default:
            status = 1;
        }
    }
    if (status || optind < argc) {
        fprintf(stderr, "usage: %s [-p port]\n", argv[0]);
        return(1);
    }
    //start listen and bind
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket");
        exit(1);
    }
    memset(&r, '\0', sizeof r);
    r.sin_family = AF_INET;
    r.sin_addr.s_addr = INADDR_ANY;
    r.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *)&r, sizeof r)){
        perror("bind");
        exit(1);
    }
    if (listen(listenfd, 5)){
        perror("listen");
        exit(1);
    }
    //end of listen and bind
    while (1) {

        int maxfd = listenfd;
        fd_set fds;
        //initialize sockets
        FD_ZERO(&fds);
        FD_SET(listenfd, &fds);
        for (p = top; p; p = p->next) {
            FD_SET(p->fd, &fds);
            if (p->fd > maxfd){
                maxfd = p->fd;
            }
        }
        if (select(maxfd + 1, &fds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(1);
        }
        for (p = top; p; p = p->next){
            if (FD_ISSET(p->fd, &fds)){
                break;
            }
        }
        if (FD_ISSET(listenfd, &fds)){ //new connections
            if ((clientfd = accept(listenfd, (struct sockaddr *)&r, &socklen)) < 0){
                perror("accept");
                exit(1);
            }
            printf("connection from %s\n", inet_ntoa(r.sin_addr));
            addclient(clientfd, r.sin_addr, shape);
            showboardone(clientfd);
            if (howmany > 2){
                sprintf(bufp, "It is now %c's turn.\r\n", "xo"[turn]);
                if (write(clientfd, bufp, strlen(bufp)) < 0){
                    perror("write");
                }
             }else{
                printf("client from %s is now %c\n", inet_ntoa(r.sin_addr), "xo"[shape]);
                sprintf(bufp, "You now get to play! You are now %c.\r\n", "xo"[shape]);
                if (write(clientfd, bufp, strlen(bufp)) < 0){
                    perror("write");

                }

            }
            if (shape == 0){
                shape = 1;
            }else{
                shape = 0;
            }


            continue;

        }if (p){ //data from previous connections
             if ((len = read(p->fd, buf, sizeof buf - 1)) < 0) {
                perror("read");
             }else if (len == 0){
                 if (p->isplaying == 1){//If player disconnects
                     //new board
                         for (i = 0; i < 9; i++){
                             board[i] = '1' + i;
                         }
                         int temp1;
                         temp1 = p->s;
                         removeclient(p->fd);
                         char buf5[80];
                         sprintf(buf5, "%c has disconnected. Game restarting.\r\n",temp1);
                         turn = 0;
                         struct client *t;
                         for (t = top; t; t = t->next){
                             if (write(t->fd, buf5, strlen(buf5)) < 0){
                                     perror("write");

                                 }
                         }
                         struct client *k;
                         for (k = top; k; k = k->next){//initialize new player's shape
                             if (k->isplaying == 0){
                                 k->isplaying = 1;
                                 if (temp1 == 120){
                                     k->s = 'x';
                                 }else{
                                     k->s = 'o';
                                 }
                                 if (shape == 0){
                                     shape = 1;
                                 }else{
                                     shape = 0;
                                 }

                                 break;
                             }
                         }
                         for (k = top; k; k = k->next){
                             if (k ->isplaying == 1){
                                 int temp3;
                                 char buf6[80];
                                 temp3 = k->s;
                                 sprintf(buf6, "You now get to play! You are now %c.\r\n", temp3);
                                 printf("%s is now %c\n", inet_ntoa(k->ipaddr), k->s);
                                 if (write(k->fd, buf6, strlen(buf6)) < 0){
                                     perror("write");

                                 }
                             }
                         }

                         showboardall();
                  }else{// If watcher disconnects
                      char buf7[80];
                      sprintf(buf7, "%s has disconnected.\r\n", inet_ntoa(p->ipaddr));
                      struct client *t;
                      for (t = top; t; t = t->next){
                      if (write(t->fd, buf7, strlen(buf7)) < 0){
                          perror("write");
                      }
                      removeclient(p->fd);
                  }

               }

             }else{
                     buf[len]= '\0';
                     for (i = 0; i < len; i++) {
                            if (isascii(buf[i]) && !isspace(buf[i])) {
                            c = buf[i];
                            break;
                            }
                     }
                     if (48 <= c && c <= 57 && strlen(buf) == 2){
                          num = atoi(buf);
                          if (p->s == "xo"[turn] && p->isplaying == 1){
                             if (48 <= board[num - 1] && board[num - 1] <= 57){
                                 board[num - 1] = p->s;
                                 sprintf(buf2, "%c makes move %d\r\n", "xo"[turn], num);
                                 printf("%s\n", buf2);
                                 struct client *t;
                                 for (t = top; t; t = t->next){
                                     if (write(t->fd, buf2, strlen(buf2)) < 0){
                                         perror("write");
                                         exit(1);
                                     }
                                 }
                                 showboardall();
                                 if (game_is_over() == 32){//Draw game
                                      for (i = 0; i < 9; i++){
                                            board[i] = '1' + i;
                                      }
                                      struct client *t;
                                      printf("Game's over. It is a draw.\n");
                                      for (t = top; t; t = t->next){
                                          char gameover[80] = "Game's over. It is a draw.\r\n";
                                          if (write(t->fd, gameover, strlen(gameover)) < 0){
                                              perror("write");
                                          }
                                          if (t->isplaying == 1){
                                              if (t->s == 'x'){
                                                  t->s = 'o';
                                              }else{
                                                  t->s = 'x';
                                              }
                                              char buf4[80];
                                              sprintf(buf4, "Let's play again! You are now %c. It is now x's turn.\r\n", t->s);
                                              if (write(t->fd, buf4, strlen(buf4)) < 0){
                                                  perror("write");
                                              }
                                          }
                                      }
                                      showboardall();
                                      turn = 0;
                                      continue;
                                 }
                                 if (game_is_over() == 120){// X wins
                                     for (i = 0; i < 9; i++){
                                          board[i] = '1' + i;
                                     }
                                     struct client *t;
                                      printf("Game's over. x is the winner!\n");
                                      for (t = top; t; t = t->next){
                                          char gameover[80] = "Game's over. x is the winner!\r\n";
                                          if (write(t->fd, gameover, strlen(gameover)) < 0){
                                              perror("write");
                                          }
                                          if (t->isplaying == 1){
                                              if (t->s == 'x'){
                                                  t->s = 'o';
                                              }else{
                                                  t->s = 'x';
                                              }
                                              char buf4[80];
                                              sprintf(buf4, "Let's play again! You are now %c. It is now x's turn.\r\n", t->s);
                                              if (write(t->fd, buf4, strlen(buf4)) < 0){
                                                  perror("write");
                                                  exit(1);
                                              }
                                          }
                                      }
                                      showboardall();
                                      turn = 0;
                                      continue;
                                 }
                                 if (game_is_over() == 111){// O wins
                                     for (i = 0; i < 9; i++){
                                          board[i] = '1' + i;
                                     }
                                     struct client *t;
                                      printf("Game's over. o is the winner!\n");
                                      for (t = top; t; t = t->next){
                                          char gameover[80] = "Game's over. o is the winner!\r\n";
                                          if (write(t->fd, gameover, strlen(gameover)) < 0){
                                              perror("write");
                                          }
                                          if (t->isplaying == 1){
                                              if (t->s == 'x'){
                                                  t->s = 'o';
                                              }else{
                                                  t->s = 'x';
                                              }
                                              char buf4[80];
                                              sprintf(buf4, "Let's play again! You are now %c. It is now x's turn.\r\n", t->s);
                                              if (write(t->fd, buf4, strlen(buf4)) < 0){
                                                  perror("write");
                                                  exit(1);
                                              }
                                          }
                                      }
                                      showboardall();
                                      turn = 0;
                                      continue;
                                 }

                                 if (turn == 0){
                                     turn = 1;

                                 }else{
                                     turn = 0;
                                 }
                                 struct client *u;
                                 char buf3[60];
                                 sprintf(buf3, "It is now %c's turn\r\n", "xo"[turn]);
                                 for (u = top; u; u = u->next){
                                     if (write(u->fd, buf3, strlen(buf3)) < 0){
                                         perror("write");
                                         exit(1);
                                      }
                                 }


                             }else{//space is taken
                                 char taken[80] = "That space is taken\r\n";
                                 if (write(p->fd, taken, strlen(taken)) < 0){
                                     perror("write");
                                 }
                                 continue;
                             }
                         }else{// Not player's turn
                             char notturn[80] = "It's not your turn\r\n";
                             if (write(p->fd, notturn, strlen(notturn)) < 0){
                                 perror("write");
                                 exit(1);
                             }
                         }
                     }else{// Chat message
                        sprintf(buf1, "Chat message: %s\r\n", buf);
                        if (strlen(buf1) > 200){
                            char buf9[80] = "Message too long, please write a shorter message.\r\n";
                            if (write(p->fd, buf9, strlen(buf9)) < 0){
                                perror("write");
                                exit(1);
                            }
                            continue;
                        }
                        printf("Chat message: %s\n", buf);
                        struct client *t;
                        for (t = top; t; t = t->next){
                            if (write(t->fd, buf1, strlen(buf1)) < 0){
                                perror("write");
                                exit(1);
                            }
                        }
                     }
            }
       }//end of if p

    }//end of while loop
    return(0);

}//end of main

int game_is_over()  /* returns winner, or ' ' for draw, or 0 for not over */
{
    int i, c;
    extern int allthree(int start, int offset);
    extern int isfull();

    for (i = 0; i < 3; i++)
        if ((c = allthree(i, 3)) || (c = allthree(i * 3, 1)))
            return(c);
    if ((c = allthree(0, 4)) || (c = allthree(2, 2)))
        return(c);
    if (isfull())
        return(' ');
    return(0);
}


int isfull()
{
    int i;
    for (i = 0; i < 9; i++)
        if (board[i] < 'a')
            return(0);
    return(1);
}
int allthree(int start, int offset)
{
    if (board[start] > '9' && board[start] == board[start + offset]
            && board[start] == board[start + offset * 2])
        return(board[start]);
    return(0);
}

static void removeclient(int fd)
{
    struct client **p;
    for (p = &top; *p && (*p)->fd != fd; p = &(*p)->next)
        ;
    if (*p) {
        struct client *t = (*p)->next;
        printf("Removing client %s\n", inet_ntoa((*p)->ipaddr));
        fflush(stdout);
        free(*p);
        *p = t;
        howmany--;
    } else {
        fflush(stderr);
    }
}


void showboardone(int fd)
{
    char buf[100], *bufp, *boardp;
    int col, row;
    for (bufp = buf, col = 0, boardp = board; col < 3; col++) {
        for (row = 0; row < 3; row++, bufp += 4){
            sprintf(bufp, " %c |", *boardp++);
        }
        bufp -= 2;  // kill last " |"
        strcpy(bufp, "\r\n---+---+---\r\n");
        bufp = strchr(bufp, '\0');
    }
    if (write(fd, buf, bufp - buf) != bufp-buf){
        perror("write");
        exit(1);
    }
}


void showboardall()
{
    struct client *p;
    char buf[100], *bufp, *boardp;
    int col, row;
    for (bufp = buf, col = 0, boardp = board; col < 3; col++) {
        for (row = 0; row < 3; row++, bufp += 4){
            sprintf(bufp, " %c |", *boardp++);
        }
        bufp -= 2;  // kill last " |"
        strcpy(bufp, "\r\n---+---+---\r\n");
        bufp = strchr(bufp, '\0');
    }
    for (p = top; p; p = p->next){
        if (write(p->fd, buf, bufp - buf) != bufp-buf){
            perror("write");
            exit(1);
        }
    }
}



static void addclient(int fd, struct in_addr addr, int s)
{
    struct client *p = malloc(sizeof(struct client));
    if (!p) {
        fprintf(stderr, "out of memory!\n");  /* highly unlikely to happen */
        exit(1);
    }
    fflush(stdout);
    p->fd = fd;
    p->s = "xo"[s];
    p->ipaddr = addr;
    p->next = top;
    top = p;
    if (howmany == 0){
        p->turn = 1;
    }else{
        p->turn = 0;
    }
    if (howmany < 2){
        p->isplaying = 1;
        howmanyplaying++;

    }else{
        p->isplaying = 0;
    }
    howmany++;
}
