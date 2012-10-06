/* 
 * ls.c
 * List directory contents
 *
 * SYNOPSIS
 * ls [-AacdFfhiklnqRrSstuw1] [file ...]
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

#define DEBUG_INFO

struct file_info
{
    struct stat stat;
    char type_permission_info[255];
    int number_of_links;
    struct passwd * password;
    struct group * group;
    char * owner_name;
    char * group_name;
    int number_of_bytes;
    char time_buf[255];
    char * path_name;

    struct file_info * next;
};

struct file_info * file_info_list_head = NULL;


void usage();
void display(struct dirent * dirp);

/* flags */
int f_l_option;     /* long list */
int f_A_option;     /* except . .. */

void usage()
{
	printf("usage: ls [-AacdFfhiklnqRrSstuw1] [file ...]\n");
}

void record_file_info(char * path_name)
{
    struct file_info * node_ptr = file_info_list_head;
    struct file_info * new_node = malloc (sizeof(struct file_info));
    struct stat stat_buf;
    struct passwd * password;
    struct group * group;

    new_node->next = NULL;
    
    /* get file status */
    if ( stat ( path_name, &stat_buf ) < 0 )
    {
        fprintf ( stderr, "stat() error" );
        return;
    }

    /* assign file stat info to file info structure */
    
    /* get file type and permissons */
    strmode ( stat_buf.st_mode, new_node->type_permission_info );
    /* get file number of links */
    new_node->number_of_links = stat_buf.st_nlink;
    /* get file owner */
    password = getpwuid ( stat_buf.st_uid );
    new_node->owner_name = password->pw_name; 
    /* get file group owner */
    group = getgrgid ( stat_buf.st_gid );
    new_node->group_name = group->gr_name;
    /* get number of bytes */
    new_node->number_of_bytes = stat_buf.st_size;
    /* get last modified time */
    /* !!! time has a few minute gap !!! */
    strftime ( new_node->time_buf,
        sizeof(new_node->time_buf),
        "%b %d %k:%m", 
        (const struct tm *) localtime((const time_t *) & stat_buf.st_mtimespec) );
    /* get file path name */
    new_node->path_name = path_name;
   
    /* add new node into list */
    if ( node_ptr == NULL )
        file_info_list_head = new_node;
    else
    {
        while ( node_ptr->next != NULL )
        {
             node_ptr = node_ptr->next;
        }
        node_ptr->next = new_node;
    }
}

void print_with_proper_option(struct file_info * node_ptr)
{
    if ( f_A_option )
    {
        /* ignore . and .. */
        if ( strcmp ( node_ptr->path_name, "." ) &&
             strcmp ( node_ptr->path_name, ".." ) )
        {
            if ( f_l_option )
            {
                printf ( "%s\t", node_ptr->type_permission_info );
                printf ( "%2d\t",node_ptr->number_of_links );
                printf ( "%s\t", node_ptr->owner_name );
                printf ( "%s\t", node_ptr->group_name );
                printf ( "%d\t", node_ptr->number_of_bytes );
                printf ( "%s\t", node_ptr->time_buf );
                printf ( "%s", node_ptr->path_name );
                printf ( "\n" );
            }
            else
            {
                printf ( "%s", node_ptr->path_name );
                printf ( "\n" );
            }
        }
   }
    else if ( f_l_option )
    {
        printf ( "%s\t", node_ptr->type_permission_info );
        printf ( "%2d\t",node_ptr->number_of_links );
        printf ( "%s\t", node_ptr->owner_name );
        printf ( "%s\t", node_ptr->group_name );
        printf ( "%d\t", node_ptr->number_of_bytes );
        printf ( "%s\t", node_ptr->time_buf );
        printf ( "%s", node_ptr->path_name );
        printf ( "\n" );
    }
    else 
    {
        printf ( "%s", node_ptr->path_name );
        printf ( "\n" );
    }
    
}

void print_file_info_list()
{
    struct file_info * node_ptr = file_info_list_head;
    
    if ( node_ptr == NULL )
#ifdef DEBUG_INFO
        printf ( "list is empty\n" );
#endif
    else
    {
        while ( node_ptr->next != NULL )
        {
            print_with_proper_option ( node_ptr );
            node_ptr = node_ptr->next;
        }
         
        print_with_proper_option ( node_ptr );
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
				f_l_option = 1;
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
            record_file_info ( dirp->d_name );
		}

		if ( closedir(dp) < 0 )
		{
			fprintf ( stderr, "can't close directory\n" );
			exit(1);
		}

        print_file_info_list();
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
            record_file_info ( *argv );
            print_file_info_list();
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
                record_file_info ( dirp->d_name );
			}
			printf ( "\n" );

			if ( closedir(dp) < 0 )
			{
				fprintf ( stderr, "can't close directory\n" );
				exit(1);
			}

            print_file_info_list();
		}
		
		argv++;
	}
	
	exit(0);
}
