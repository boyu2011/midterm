/* 
 * ls.c
 * List directory contents
 *
 * SYNOPSIS
 * ls [-AacdFfhiklnqRrSstuw1] [file ...]
 *
 * Author: BoYu (boyu2011@gamil.com)
 *
 *
 * bug: how to deal with -c and -u override each other!!
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

#define DEBUG

/* if DARWIN has defined, it means this program will be run at MacOS;
   or it means the program will be run at Linux. */
#define DARWIN

/*
    data structures
*/

struct file_info
{
    struct stat stat;
    long inode_number;
    char type_permission_info[255];
    char file_type;
    int number_of_links;
    struct passwd * password;
    struct group * group;
    char * owner_name;
    char * group_name;
    int number_of_bytes;
    char time_buf[255];
    
    char last_access_time[255];         /* ls -u */
    char last_modi_time[255];           /* default ls */
    char last_change_time[255];         /* ls -c */
    time_t a_time;
    time_t m_time;
    time_t c_time;

    char path_name[255];

    struct file_info * next;
};

/* 
    global variables
*/

struct file_info * file_info_list_head = NULL;


/*
    function prototypes
*/

void usage();
void display(struct dirent * dirp);
struct file_info * sort_by_time_modi ( struct file_info * pList );


/* 
    flags 
*/

int f_A_option;     /* except . .. */

int f_a_option;     /* include directory entries whose names begin 
                       with a dot ('.') */

int f_c_option;     /* use time when file status was last changed, 
                       instead of time of last modification of the
                       file for sorting (-t) or printing (-l) */

int f_d_option;     /* directories are listed as plain files ( not
                       searched recursively ) and symbolic links in
                       the argument list are not indirected through
                    */

int f_F_option;     /* mark file type */

int f_i_option;     /* for each file, print the file's file file 
                       serial number ( inode number ) */

int f_l_option;     /* long list */

int f_t_option;     /* sorted by time modified before sorting the 
                       operands by lexicographical order */

int f_u_option;     /* use time of last access, instead of last
                       modification of the file for sorting (-t) 
                       or printing (-l). */

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

    /* 
        initialize the new node
    */

    new_node->file_type = ' ';
    new_node->next = NULL;
    
    /* 
        get file status 
    */
    
    if ( stat ( path_name, &stat_buf ) < 0 )
    {
        fprintf ( stderr, "stat() error" );
        return;
    }

    /* 
       assign file stat info to file info structure 
    */
   
    /* get file's file serial number (inode number) */

    new_node->inode_number = stat_buf.st_ino;

    /* get file type and permissons */
    
    strmode ( stat_buf.st_mode, new_node->type_permission_info );
   
    /* get file type */
    
    if ( S_ISDIR ( stat_buf.st_mode ) )
        new_node->file_type = '/';
    /* how to distinct an exec file???
    else if ( S_ISREG ( stat_buf.st_mode ) )
        file_type = '*';
    */
    else if ( S_ISLNK ( stat_buf.st_mode ) )
        new_node->file_type = '%';
    /* how to distinct a whiteout file ???
    else if ( )
    */
    else if ( S_ISSOCK ( stat_buf.st_mode ) )
        new_node->file_type = '=';
    else if ( S_ISFIFO ( stat_buf.st_mode ) )
        new_node->file_type = '|';

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

    /* get last access time */
    
    strftime ( new_node->last_access_time,
               sizeof(new_node->last_access_time),
               "%b %d %R",
               localtime ( & stat_buf.st_atime ) );

    new_node->a_time = stat_buf.st_atime;

    /* get last modified time */

    strftime ( new_node->last_modi_time,
               sizeof(new_node->last_modi_time),
               "%b %d %R",
               localtime ( &stat_buf.st_mtime ) );

    new_node->m_time = stat_buf.st_mtime; 

    /* get last change time */
    
    strftime ( new_node->last_change_time,
               sizeof(new_node->last_change_time),
               "%b %d %R",
               localtime ( &stat_buf.st_ctime ) );
    
    new_node->c_time = stat_buf.st_ctime;

    /* get file path name */
    strcpy ( new_node->path_name, path_name );
    
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
    /* long format flag specified */

    if ( f_l_option )
    {
        if ( f_A_option )
        {
            /* ignore . and .. */
            if ( ! ( strcmp ( node_ptr->path_name, "." ) &&
                     strcmp ( node_ptr->path_name, "..") ))
            {
                return;
            }
        }
        else if ( f_a_option )
        {
            //....
        }
        
        if ( f_i_option )
            printf ( "%ld\t", node_ptr->inode_number );

        printf ( "%s\t", node_ptr->type_permission_info );
        
        printf ( "%2d\t",node_ptr->number_of_links );
        
        printf ( "%s\t", node_ptr->owner_name );
        
        printf ( "%s\t", node_ptr->group_name );
        
        printf ( "%d\t", node_ptr->number_of_bytes );
       
        /* bug: how to deal with -c -u override */
        if ( f_c_option )
            printf ( "%s\t", node_ptr->last_change_time );
        else if ( f_u_option )
            printf ( "%s\t", node_ptr->last_access_time );
        else
            printf ( "%s\t", node_ptr->last_modi_time );
        
        printf ( "%s", node_ptr->path_name );
        if ( f_F_option )
        {
            if ( node_ptr->file_type != ' ' )
                printf ( "%c", node_ptr->file_type );
        }

        printf ( "\n" );
    }
    else 
    {
        if ( f_A_option )
        {
            /* ignore . and .. */
            if ( ! ( strcmp ( node_ptr->path_name, "." ) &&
                     strcmp ( node_ptr->path_name, "..") ))
            {
                return;
            }
        }
        else if ( f_a_option )
        {
            //....
        }

        if ( f_i_option )
            printf ( "%ld\t", node_ptr->inode_number );
        
        printf ( "%s", node_ptr->path_name );
        if ( f_F_option )
        {
            if ( node_ptr->file_type != ' ' )
                printf ( "%c", node_ptr->file_type );
        }        
        printf ( "\n" );
    }
}

void print_file_info_list()
{
    /* 
        sort the file_info list if needed
    */

    if ( f_t_option )
    {
        file_info_list_head = sort_by_time_modi ( file_info_list_head );
    }

    /*
        printing
    */

    struct file_info * node_ptr = file_info_list_head;

    if ( node_ptr == NULL )
    {
        return;
    }
    else
    {
        while ( node_ptr->next != NULL )
        {
            print_with_proper_option(node_ptr);
            
            node_ptr = node_ptr->next;
        }

        print_with_proper_option(node_ptr);
    }
}

struct file_info * sort_by_time_modi ( struct file_info * pList )
{
    /* build up the sorted array from the empty list */
    struct file_info * pSorted = NULL;

    /* take items off the input list one by one until empty */
    while ( pList != NULL )
    {
        /* remember the head */
        struct file_info * pHead = pList;
        /* trailing pointer for efficient splice */
        struct file_info ** ppTrail = &pSorted;

        /* pop head off list */
        pList = pList->next;

        /* splice head into sorted list at proper place */
        while (1)
        {
            /* does head belong here? */
            if ( *ppTrail == NULL || pHead->m_time > (*ppTrail)->m_time )
            {
                /* yes */
                pHead->next = *ppTrail;
                *ppTrail = pHead;
                break;
            }
            else
            {
                /* no - continue down the list */
                ppTrail = & (*ppTrail)->next;
            }
        }
    }

    return pSorted;
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
            case 'A':
                f_A_option = 1;
                break;
            case 'a':
                f_a_option = 1;
                break;
            case 'c':
                f_c_option = 1;
                break;
            case 'd':
                f_d_option = 1;
                break;
            case 'F':
                f_F_option = 1;
                break;
            case 'i':
                f_i_option = 1;
                break;
			case 'l':
				f_l_option = 1;
				break;
            case 't':
                f_t_option = 1;
                break;
			case 'u':
                f_u_option = 1;
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

        /* enter directory */
        if ( chdir(".") == -1 )
        {
            fprintf ( stderr, "can't chdir to '%s': %s\n",
                *argv, strerror(errno) );
            exit (1);
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
