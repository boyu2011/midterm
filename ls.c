/* 
 * ls.c
 * List directory contents
 *
 * SYNOPSIS
 * ls [-AacdFfhiklnqRrSstuw1] [file ...]
 *
 *
 * Author: BoYu (boyu2011@gamil.com)
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void usage();
void display(struct dirent * dirp);

/* flags */
int f_longform;


void usage()
{
	printf("usage: ls [-AacdFfhiklnqRrSstuw1] [file ...]\n");
}

/* show one directory entry to the screen */

void display(struct dirent * dirp)
{
    /* -l option has set */
	if ( f_longform )
	{	
	    struct stat stat_buf; 
        char file_type;
        struct passwd * password;
        struct group * group;
        char * owner_name;
        char * group_name;
        char time_buf [255];
        char perm_buf [255];

        /* get file status */
        if ( stat ( dirp->d_name, &stat_buf ) < 0 )
        {
            fprintf ( stderr, "stat() error" );
            return;
        }

        /* get file type */
        if (S_ISREG(stat_buf.st_mode))
            file_type = '-';
        else if (S_ISDIR(stat_buf.st_mode))
            file_type ='d';
        else if (S_ISCHR(stat_buf.st_mode))
            file_type = 'c';
        else if (S_ISBLK(stat_buf.st_mode))
            file_type = 'b';
        else if (S_ISFIFO(stat_buf.st_mode))
            file_type = 'p';
        else if (S_ISLNK(stat_buf.st_mode))
            file_type = 'l';
        else if (S_ISSOCK(stat_buf.st_mode))
            file_type = 's';

        /* get file type and permissons */
        strmode ( stat_buf.st_mode, perm_buf );
    
        /* get file owner */
        password = getpwuid ( stat_buf.st_uid );
        owner_name = password->pw_name;
     
        /* get file group owner */
        group = getgrgid ( stat_buf.st_gid );
        group_name = group->gr_name;

        /* get last modified time */
        /* !!! time has a few minute gap !!! */
        strftime ( time_buf, sizeof(time_buf), "%b %d %k:%m", 
            (const struct tm *) localtime((const time_t *) & stat_buf.st_mtimespec) );

        /* output file info to the screen */
        printf ( "%s ", perm_buf );
        printf ( "%2d ", stat_buf.st_nlink );
        printf ( "%s  ", owner_name );
        printf ( "%s ", group_name );
        printf ( "%d\t", (int) stat_buf.st_size );
        printf ( "%s\t", time_buf );
        printf ( "%s\n", dirp->d_name );
	}
    /* no option has set */
	else
	{
		printf ( "%s\t", dirp->d_name );
	}
}

int main ( int argc, char ** argv )
{
	int ch;
	DIR * dp;
	struct dirent * dirp;
	int stat_ret;
	struct stat stat_buf; 

	/* parse options */

	while ( ( ch = getopt(argc, argv, "AacdFfhiklnqRrSstuw1") ) != -1 )
	{
		switch (ch)
		{
			case 'l':
				f_longform = 1;
				break;
			default:
				usage();
		}
	}

	argc -= optind;
	argv += optind;

	/* parse file argument */

	/* If no operands are given, the contents of the current directory
	   are displayed. */
	if ( argc == 0 )
	{
		if ( ( dp = opendir(".") ) == NULL )
		{
			fprintf ( stderr, "can't open '%s'\n", "." );
			exit(1);
		}

		while ( ( dirp = readdir(dp) ) != NULL )
		{
			display(dirp);
		}
		printf ( "\n" );

		if ( closedir(dp) < 0 )
		{
			fprintf ( stderr, "can't close directory\n" );
			exit(1);
		}
	}

	/* loop file arguments */
	while (argc-- > 0)
	{
		stat_ret = stat ( *argv, &stat_buf );
		if ( stat_ret < 0 )
		{
			fprintf ( stderr, "stat error\n" );
			continue;
		}
	
        /* argument is a file */
		if ( S_ISREG ( stat_buf.st_mode ) )
		{
			printf ( "%s\n", *argv );
		}
        /* argument is a directory */
		else if ( S_ISDIR ( stat_buf.st_mode ) )
		{
			/* read directory NAME, and list the files in it */	
			if ( ( dp = opendir(*argv) ) == NULL )
			{
				fprintf ( stderr, "can't open '%s'\n", *argv );
				exit(1);
			}

            /* enter directory */
            if ( chdir(*argv) == -1 )
            {
                fprintf ( stderr, "can't chdir to '%s': %s\n",
                    *argv, strerror(errno) );
                exit (1);
            }

			while ( ( dirp = readdir(dp) ) != NULL )
			{
				display(dirp);
			}
			printf ( "\n" );

			if ( closedir(dp) < 0 )
			{
				fprintf ( stderr, "can't close directory\n" );
				exit(1);
			}
		}
		
		argv++;
	}
	
	exit(0);
}
