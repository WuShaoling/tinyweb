#include "include/csapp.h"

// 处理一个HTTP事务
void doit(int fd);

// 读取HTTP请求头信息
void read_request_hdrs(rio_t *rp);

// 处理 uri
int parse_uri(char *uri, char *filename, char *cgi_args);

// 处理静态请求
void serve_static(int fd, char *filename, int file_size);

// 获得文件类型
void get_file_type(char *filename, char *file_type);

// 处理动态请求
void serve_dynamic(int fd, char *filename, char *cgi_args);

// 返回错误信息到客户端
void client_error(int fd, char *cause, char *err_num, char *short_msg, char *long_msg);

int main(int argc, char **argv) {
    struct sockaddr_in client_addr;

    // set listen port
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    int port = atoi(argv[1]);

    int listen_fd = Open_listenfd(port);
    while (1) {
        int client_len = sizeof(client_addr);
        int conn_fd = Accept(listen_fd, (SA *) &client_addr, &client_len);
        doit(conn_fd);
        Close(conn_fd);
    }
}

// 处理一个HTTP事务
void doit(int fd) {
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], filename[MAXLINE], cgi_args[MAXLINE];
    rio_t rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {
        client_error(fd, method, "501", "Not Implemented", "Tiny does not implement this method");
        return;
    }
    read_request_hdrs(&rio);

    /* Parse URI from GET request */
    int is_static = parse_uri(uri, filename, cgi_args);
    if (stat(filename, &sbuf) < 0) {
        client_error(fd, filename, "404", "Not found", "Tiny couldn't find this file");
        return;
    }

    if (is_static) { /* Serve static content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            client_error(fd, filename, "403", "Forbidden",
                         "Tiny couldn't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size);
    } else { /* Serve dynamic content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            client_error(fd, filename, "403", "Forbidden",
                         "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgi_args);
    }
}

/*
 * 读取HTTP请求头信息
 */
void read_request_hdrs(rio_t *rp) {
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}

/*
 * 处理uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
int parse_uri(char *uri, char *filename, char *cgi_args) {
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {  /* Static content */
        strcpy(cgi_args, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (uri[strlen(uri) - 1] == '/')
            strcat(filename, "index.html");
        return 1;
    } else {  /* Dynamic content */
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgi_args, ptr + 1);
            *ptr = '\0';
        } else
            strcpy(cgi_args, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

// 处理静态请求
void serve_static(int fd, char *filename, int file_size) {
    char filetype[MAXLINE], buf[MAXBUF];

    /* Send response headers to client */
    get_file_type(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, file_size);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));

    /* Send response body to client */
    int src_fd = Open(filename, O_RDONLY, 0);
    char *srcp = Mmap(0, file_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    Close(src_fd);
    Rio_writen(fd, srcp, file_size);
    Munmap(srcp, file_size);
}

// 获得文件类型
void get_file_type(char *filename, char *file_type) {
    if (strstr(filename, ".html"))
        strcpy(file_type, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(file_type, "image/gif");
    else if (strstr(filename, ".jpg"))
        strcpy(file_type, "image/jpeg");
    else if (strstr(filename, ".css"))
        strcpy(file_type, "text/css");
    else if (strstr(filename, ".png"))
        strcpy(file_type, "image/png");
    else if (strstr(filename, ".js"))
        strcpy(file_type, "application/x-javascript");
    else if (strstr(filename, ".woff2"))
        strcpy(file_type, "font/woff2");
    else
        strcpy(file_type, "text/plain");
}

// 处理动态请求 run a CGI program on behalf of the client
void serve_dynamic(int fd, char *filename, char *cgi_args) {
    char buf[MAXLINE], *emptylist[] = {NULL};

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) { /* child */
        /* Real server would set all CGI vars here */
        setenv("QUERY_STRING", cgi_args, 1);
        Dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */
        Execve(filename, emptylist, environ); /* Run CGI program */
    }
    Wait(NULL); /* Parent waits for and reaps child */
}

// 返回错误信息到客户端
void client_error(int fd, char *cause, char *err_num, char *short_msg, char *long_msg) {
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, err_num, short_msg);
    sprintf(body, "%s<p>%s: %s\r\n", body, long_msg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", err_num, short_msg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int) strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
