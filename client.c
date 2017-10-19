// CONCURRENT CLIENT

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT 3769


void error( char * reason, int sock_fd )
{
    perror(reason);
    close(sock_fd);
    exit(1);
}

void send_connect( int sock_fd )
{
    if ( write(sock_fd, "CONNECT", 7) < 0 )
        error("write connect", sock_fd);
//    printf("wrote connect...\n");
}

void setup_connection( int sock_fd, struct sockaddr_in * server_addr )
{
    if ( connect(sock_fd, (struct sockaddr *) server_addr, sizeof(*server_addr)) < 0 )
        error("connect", sock_fd);
}

void sub_setup_serveraddr( struct hostent * server, struct sockaddr_in * server_addr )
{
    server_addr->sin_family = AF_INET;
    server_addr->sin_addr = *( (struct in_addr *) server->h_addr);
    server_addr->sin_port = htons(PORT);
}

void setup_server( int sock_fd, struct hostent * server, char * host, struct sockaddr_in * server_addr )
{
    if ( (server = gethostbyname(host)) == NULL )
        error("hostname", sock_fd);
    
    memset( server_addr, 0, sizeof(*server_addr) );
    sub_setup_serveraddr(server, server_addr);
}

void setup_socket( int * sock_fd )
{
    if ( (*sock_fd = socket(AF_INET, SOCK_STREAM, 0)) <0 )
        error("socket", *sock_fd);
}

void check_argv( int argc, char * argv[] )
{
    if ( argc != 3 )
        printf( "args format: %s <hostname> <directory>\n", argv[0] ), exit(1);
}

void initial_setup ( int * sock_fd, struct hostent * server, struct sockaddr_in * server_addr, int argc, char * argv[] )
{
    check_argv(argc, argv);
    setup_socket( sock_fd );
    setup_server( *sock_fd, server, argv[1], server_addr );
    setup_connection( *sock_fd, server_addr );
    send_connect( *sock_fd );
}

void check_write ( int sock_fd, int sent, int wrote )
{
    if ( sent != wrote )
        error("did not send all", sock_fd);
}

void request_directory ( int * sock_fd, char * directory )
{
    char directory_nterm[1024] = { '\0' };
    
    strcpy(directory_nterm, directory);
    directory_nterm[ strlen(directory) ] = '\n'; // make it a oneliner
    
//    printf("about to request the directory... <%s>\n", directory_nterm);
    
    int wrote = write(*sock_fd, directory_nterm, strlen(directory_nterm));
    
    check_write( *sock_fd, wrote, strlen(directory_nterm));
    
    
}

char read_a_char (int sock_fd)
{
    char c;
    read(sock_fd, &c, 1);
    return c;
}

void read_a_line ( int sock_fd, char * destination, int max_size )
{
    int len;
    char c = read_a_char( sock_fd );
    
    for (len = 0; c != '\n' ; len++, c = read_a_char( sock_fd ) )
        destination[len] = c;
    
//    printf("read a line : <%s>\n", destination);
}

void open_new_file ( int sock_fd, char * path, int * fd )
{
    if ( ((*fd) = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666 )) < 0 )
        error( path, sock_fd );
}

void read_whole_file ( int sock_fd, char * path, int file_size )
{
    char buf[BUFSIZ];
    int fd, red, left_to_read = file_size;
    
//    printf( "***left to read is %d \n", left_to_read );
    
    open_new_file( sock_fd, path, &fd );
    while ( left_to_read > 0 )
    {
//        printf("~~loop left to read: %d", left_to_read);
        memset(buf, 0, BUFSIZ);
        if (left_to_read >= BUFSIZ)
            red = read(sock_fd, buf, BUFSIZ);

        else
            red = read(sock_fd, buf, left_to_read);    // read from socket
        
        int wr = write(fd, buf, red);               // wirte what red to file
        check_write(sock_fd, wr, red);
        left_to_read -= red;
    }
    
//    printf( "*THEN left to read is %d \n", left_to_read );
    
    close(fd);
}

void make_directory ( int sock_fd, char * path )
{
    if ( mkdir(path, 0777) == -1 )
        error("mkdir", sock_fd);
    
}

int count_to_skip ( char * long_path )
{
    char * my_dir = strrchr(long_path, '/');
    int total_len = strlen(long_path);
    int last_dir_len = strlen(my_dir);
    
    return total_len-last_dir_len;
    
}

void clean_path ( char * destination, char * server_path, int skip )
{
    memset(destination, 0, sizeof(&destination));
    sprintf(destination, ".%s", server_path + skip);
}

void accept_directory ( int sock_fd, int skip )
{
    char server_path[1024] = {'\0'}, local_path[1024] = {'\0'};
    
    read_a_line(sock_fd, server_path, 1024);
    
    
    
    clean_path(local_path, server_path, skip);
    
    printf("-d %s \n", local_path);
    
    make_directory( sock_fd, local_path );
    
}

void accept_file ( int sock_fd, int skip )
{
    char server_path[1024] = {'\0'}, local_path[1024] = {'\0'}, char_size[64] = {'\0'};
    int file_size;
    read_a_line(sock_fd, server_path, 1024);
    clean_path(local_path, server_path, skip);
    
    printf("-f %s \n", local_path);
    
    read_a_line(sock_fd, char_size, 64 );
    file_size = atoi(char_size);
    
    read_whole_file( sock_fd, local_path, file_size );
}

int build_my_dir ( int sock_fd, int to_skip  )
{
    char bit = '\0';
    
    while ( read(sock_fd, &bit, 1) )
    {
        switch (bit)
        {
            case 'f':
                accept_file(sock_fd, to_skip);
                
                break;
                
            case 'd':
                accept_directory( sock_fd, to_skip );
                
                break;
                
            case 'e':
                return 1;
                break;
                
        }
    }
    return 0;
}

void create_my_dir ( int sock_fd, char * dir_name, int skip )
{
//    char * p = rindex(dir_name, '/');
    char my_directory[1024] = {'\0'};
    
    sprintf(my_directory, ".%s", dir_name + skip);
    
    make_directory( sock_fd, my_directory );
    
//    printf("created directory\n");
    
}

void remote_copy_client( int * sock_fd, struct sockaddr_in *server_addr, struct hostent *server, int argc, char * argv[] )
{
    initial_setup( sock_fd, server, server_addr, argc, argv);
    request_directory(sock_fd, argv[2]);
    
    int skip = count_to_skip(argv[2]);
    
    create_my_dir( *sock_fd, argv[2], skip );
    build_my_dir( *sock_fd, skip );
    
    close(*sock_fd);
}

int main(int argc, const char * argv[])
{
    int sock_fd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    
    remote_copy_client(&sock_fd, &server_addr, server, argc, argv);
    
    return 0;
}
