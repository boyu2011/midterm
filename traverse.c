/*
    traverse a path recursively.
*/

#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

char g_path_name[255];

void do_path ()
{
    struct stat stat_buf;
    struct dirent * dirp;
    DIR * dp;
    char * ptr;

    if ( lstat ( g_path_name, &stat_buf ) < 0 )
    {
        fprintf ( stderr, "lstat error" );
        return;
    }

    if ( S_ISDIR(stat_buf.st_mode) == 0 )
    {
        printf ( "FILE --- %s\n", g_path_name );
        return;
    }

    printf ( "DIR --- %s\n", g_path_name );

    ptr = g_path_name + strlen ( g_path_name );
    *ptr++ = '/';
    *ptr = 0;
    
    if ( ( dp = opendir ( g_path_name ) ) == NULL )
    {
        fprintf ( stderr, "opendir() error" );
        return;
    }

    while ( ( dirp = readdir (dp) ) != NULL )
    {
        strcpy ( ptr, dirp->d_name );

        /* recursive */
        do_path ();
    
    }
    ptr[-1] = 0;

    if ( closedir ( dp ) < 0 )
    {
        fprintf ( stderr, "closedir() error" );
    }

    return;
}

int main ( int argc, char * argv[] )
{

    if ( argc != 2 )
    {
        fprintf ( stderr, "usage: traverse pathname" );
        exit (1);
    }

    strcpy ( g_path_name, argv[1] );

    do_path();

    exit(0);
}
