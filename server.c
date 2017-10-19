// CONCURRENT SERVER

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>

#define PORT 3769
#define BUFSIZE 10000

void go_thru_directory ( char * dir_name, int c_fd );

void error ( char * reason )
{
    perror(reason);
    exit(1);
}

void setup_bind ( int sock, struct sockaddr_in * servername )
{
    (*servername).sin_port = htons(PORT);
    (*servername).sin_addr.s_addr = htonl(INADDR_ANY);
    if ( bind(sock, (struct sockaddr *) servername, sizeof(*servername)) < 0 )
        error("bind");
}

void setup_listen ( int sock )
{
    if ( listen( sock, 1) < 0 )
        error("listen");
}

void setup_socket ( int * sock )
{
    if ( ((*sock) = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
        error("socket");
}

void initial_setup ( int * sock, struct sockaddr_in * servername )
{
    setup_socket( sock );
    setup_bind( *sock, servername );
    setup_listen( *sock );
    
}

void do_accept ( int sock, struct sockaddr_in * clientname, size_t * size, int * new )
{
    if ( (*new = accept(sock, (struct sockaddr *) clientname, (socklen_t *) size)) < 0)
        error("accept");
//    printf("accept OK...\n");
}

void open_file ( int * ifd, char * path )
{
    if ( ( (*ifd) = open( path, O_RDONLY ) ) < 0 )
        perror("openfile");
}

//void readNwrite ( int c_fd, int ifd )
//{
//    int red;
//    char buffer[BUFSIZ];
//
//    while ( (red = read(ifd, buffer, BUFSIZ)) > 0 )
//        write(c_fd, buffer, red);
//}

// int do_read ( char buffer[1024], int c_fd )
// {
//     int red;
//     memset(buffer, 0, BUFSIZE);

//     if ( (red = read( c_fd, buffer, BUFSIZE-1)) < -1 )
//         error("client read");

//     return (strncmp(buffer, "DONE", 4) == 0 || red == 0) ? 0 : 1;
// }

//void do_read ( int c_fd, char * buffer, int bufsize )
//{
//    if ( read( c_fd, buffer, bufsize-1 ) < 0 )
//        error( "client read" );
//}

void writ_a_char ( int c_fd, char c )
{
    write(c_fd, &c, 1);
}

void writ_a_line ( int c_fd, char * line )
{
    write(c_fd, line, strlen(line));
}

char read_a_char ( int c_fd )
{
    char c;
    read(c_fd, &c, 1);
    return c;
}

void read_a_line ( int c_fd, char * destination, int max_size )
{
    int len;
    char c = read_a_char( c_fd );
    
    for (len = 0; c != '\n' ; len++, c = read_a_char( c_fd ) )
        destination[len] = c;
    

}

void send_file_contents ( int file_fd, int c_fd )
{
    int red;
    char buffer[BUFSIZ];
    while ( (red = read(file_fd, buffer, BUFSIZ )) > 0 )
        write( c_fd, buffer, red );
}

void setup_file_write ( int c_fd, char path_nterm[1024], char file_size[64], char * path, off_t size )
{
    sprintf(path_nterm, "%s\n", path);
    sprintf(file_size, "%lld\n", size);
    
    writ_a_char(c_fd, 'f');        // send FILE flag
    writ_a_line( c_fd, path_nterm ); // write path to client
    writ_a_line( c_fd, file_size);  // write size of the file to client
    
}

void send_file ( char * path, off_t size, int c_fd )
{
    int file_fd;
    char path_nterm[1024] = {'\0'}, file_size[64] = {'\0'};
    
    open_file(&file_fd, path);
    printf("-f %s \n", path);
    
    if (file_fd != -1)
    {
        setup_file_write(c_fd, path_nterm, file_size, path, size);
        send_file_contents(file_fd, c_fd); // send file
    }
}

void send_folder ( char * path, int c_fd )
{
    char path_nterm[1024] = {'\0'};
    
    sprintf( path_nterm, "%s\n", path );
    
    printf("-d %s \n", path);
    
    writ_a_char(c_fd, 'd');         // send DIR flag
    writ_a_line( c_fd, path_nterm );
    
    go_thru_directory(path, c_fd );
    
}

void fork_on_element_type ( char * dir_name, DIR * my_dir, struct dirent * dir_element, struct stat my_stat, char * path, int c_fd )
{
    if ( strncmp(dir_element->d_name, ".", 1) == 0 );
    
    else if ( my_stat.st_mode & S_IFREG )
    {
//        printf("- it's a file \n");
        send_file( path, my_stat.st_size, c_fd );
    }
    else if ( my_stat.st_mode & S_IFDIR )
    {
//        printf("- it's a directory \n");
        send_folder( path, c_fd );
    }
}

void deal_with_element ( char * dir_name, DIR * my_dir, struct dirent * dir_element, struct stat my_stat, int c_fd )
{
    char path[1024];
    
//    printf("dir element is:  %s \n", dir_element->d_name);
    
    sprintf(path, "%s/%s", dir_name, dir_element->d_name);
    
    if ( stat( path, &my_stat ) != -1 )
        fork_on_element_type( dir_name, my_dir, dir_element, my_stat, path, c_fd );
    else
        error( path );
}

void go_thru_directory ( char * dir_name, int c_fd )
{
    DIR * my_dir;
    struct dirent * dir_element;
    struct stat my_stat;
    
    my_dir = opendir( dir_name ); // GOT TO CHECK RETURN VALUE
    
//    printf("going through directory %s\n", dir_name);
    
    while ( (dir_element = readdir( my_dir )) != NULL )
        deal_with_element( dir_name, my_dir, dir_element, my_stat, c_fd );
}

void work_child ( int c_fd, int sock )
{
    char dir_name[1024] = {'\0'};
    
    read_a_line(c_fd, dir_name, 1024);
    printf("Request for %s\n", dir_name);
    go_thru_directory( dir_name, c_fd );
    
}


void do_fork ( int c_fd, char * envp[], int sock )
{
    //    int ifd;
    switch (fork())
    {
        case -1:
            error("fork");
            
        case 0:
            close(sock);
            
            work_child( c_fd, sock );
            
            writ_a_char(c_fd, 'e'); // signal DONE
            close( c_fd );
            exit(0);
            
        default:
            close(c_fd);
            break;
    }
}

int read_connect( int new_client )
{
    char buf[10] = {'\0'};
    
    if ( read(new_client, buf, 7) < 0 )
        error("first read");
    
    if ( strncmp(buf, "CONNECT", 7) == 0 )
        return 1;
    
    return 0;
}

int main(int argc, const char * argv[], char * envp[])
{
    int sock;
    struct sockaddr_in servername, clientname;
    size_t size;
    
    initial_setup( &sock, &servername );
    
    while (1)
    {
        int c_fd;
        do_accept( sock, &clientname, &size, &c_fd );
        
        if ( read_connect( c_fd ) )
            do_fork( c_fd, envp, sock );
        
        close( c_fd );
    }
    
    return 0;
}
