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
#include <fts.h>

#define DEBUG

/* if DARWIN has defined, it means this program will be run at MacOS;
   or it means the program will be run at Linux. */
#define DARWIN

/*
#define ENABLE_H_OPTION
*/

#ifdef ENABLE_H_OPTION
#include <libutil.h>    /* for humanize_number() */
#endif

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
    long user_id;
    char owner_name[255];
    long group_id;
    char group_name[255];
    int number_of_bytes;
    char time_buf[255];
    
    char last_access_time[255];         /* ls -u */
    char last_modi_time[255];           /* default ls */
    char last_change_time[255];         /* ls -c */
    time_t a_time;
    time_t m_time;
    time_t c_time;

    char path_name[255];

    long number_of_blocks;

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
struct file_info * sort_by_lexi ( struct file_info * pList );


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

int f_h_option;     /* modifies the -s and -l options, causing the 
                       size to be reported in bytes displayed in a
                       human readable format. Overrides -k */

int f_i_option;     /* for each file, print the file's file file 
                       serial number ( inode number ) */

int f_l_option;     /* long list */

int f_n_option;     /* the same as -l, except that the owner and 
                       group IDs are displayed numerically rather
                       than converting to a owner or group name */

int f_R_option;

int f_s_option;     /* display the number of file system blocks
                       actually used by each file, in units of 512
                       bytes or BLOCKSIZE ( see ENVIRONMENT) where
                       partial units are rounded up to the next 
                       integer value. If the output is to a terminal
                       a total sum for all the file sizes is output
                       on a line before the listing. */ 

int f_t_option;     /* sorted by time modified before sorting the 
                       operands by lexicographical order */

int f_u_option;     /* use time of last access, instead of last
                       modification of the file for sorting (-t) 
                       or printing (-l). */

void usage()
{
	printf("usage: ls [-AacdFfhiklnqRrSstuw1] [file ...]\n");
}


void record_stat( struct stat * statp, char * path_name )
{
    struct file_info * node_ptr = file_info_list_head;
    struct file_info * new_node = malloc (sizeof(struct file_info));
    struct passwd * password;
    struct group * group;

    /* 
        initialize the new node
    */

    new_node->file_type = ' ';
    new_node->next = NULL;
    
    /* 
       assign file stat info to file info structure 
    */
   
    /* get file's file serial number (inode number) */

    new_node->inode_number = statp->st_ino;

    /* get file type and permissons */
    
    strmode ( statp->st_mode, new_node->type_permission_info );
   
    /* get file type */
    
    if ( S_ISDIR ( statp->st_mode ) )
        new_node->file_type = '/';
    /* how to distinct an exec file???
    else if ( S_ISREG ( statp->st_mode ) )
        file_type = '*';
    */
    else if ( S_ISLNK ( statp->st_mode ) )
        new_node->file_type = '%';
    /* how to distinct a whiteout file ???
    else if ( )
    */
    else if ( S_ISSOCK ( statp->st_mode ) )
        new_node->file_type = '=';
    else if ( S_ISFIFO ( statp->st_mode ) )
        new_node->file_type = '|';

    /* get file number of links */
    
    new_node->number_of_links = statp->st_nlink;
    
    /* get file owner */
    
    new_node->user_id = statp->st_uid;
    password = getpwuid ( statp->st_uid );
    strcpy ( new_node->owner_name, password->pw_name );

    /* get file group owner */
    
    new_node->group_id = statp->st_gid;
    group = getgrgid ( statp->st_gid );
    strcpy ( new_node->group_name, group->gr_name );

    /* get number of bytes */
    
    new_node->number_of_bytes = statp->st_size;

    /* get last access time */
    
    strftime ( new_node->last_access_time,
               sizeof(new_node->last_access_time),
               "%b %d %R",
               localtime ( & statp->st_atime ) );

    new_node->a_time = statp->st_atime;

    /* get last modified time */

    strftime ( new_node->last_modi_time,
               sizeof(new_node->last_modi_time),
               "%b %d %R",
               localtime ( &statp->st_mtime ) );

    new_node->m_time = statp->st_mtime; 

    /* get last change time */
    
    strftime ( new_node->last_change_time,
               sizeof(new_node->last_change_time),
               "%b %d %R",
               localtime ( &statp->st_ctime ) );
    
    new_node->c_time = statp->st_ctime;

    /* get file path name */
    
    strcpy ( new_node->path_name, path_name );
   
    /* get number of file system blocks actually used */

    new_node->number_of_blocks = statp->st_blocks;

    /* 
        add new node into list 
    */
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
    
    new_node->user_id = stat_buf.st_uid;
    password = getpwuid ( stat_buf.st_uid );
    strcpy ( new_node->owner_name, password->pw_name );

    /* get file group owner */
    
    new_node->group_id = stat_buf.st_gid;
    group = getgrgid ( stat_buf.st_gid );
    strcpy ( new_node->group_name, group->gr_name );

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
   
    /* get number of file system blocks actually used */

    new_node->number_of_blocks = stat_buf.st_blocks;

    /* 
        add new node into list 
    */
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
    /* put out a newline as a separator when come up with a directory item */
    /* .... */

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
    else
    {
        /* default output without file whose names begin with a dot ('.') */ 
        if ( node_ptr->path_name[0] == '.' )
            return;
    }

    if ( f_i_option )
        printf ( "%ld\t", node_ptr->inode_number );

    if ( f_s_option )
    {
#ifdef ENABLE_H_OPTION
        if ( f_h_option )
        {
            char szbuf[5];
            if ( (humanize_number(szbuf,
                    sizeof(szbuf), 
                    (int64_t)node_ptr->number_of_blocks,
                    "",
                    HN_AUTOSCALE,
                    (HN_DECIMAL | HN_B | HN_NOSPACE))) == -1 )
            {
                fprintf ( stderr, "humanize_number()" );
                exit(1);
            }
            printf ( "%s\t", szbuf );
        }
        else
#endif
            printf ( "%ld\t", node_ptr->number_of_blocks );
    }
   
    /* long format flag specified */

    if ( f_l_option || f_n_option )
    {

        printf ( "%s\t", node_ptr->type_permission_info );
        
        printf ( "%2d\t",node_ptr->number_of_links );
        
        if ( f_l_option )
            printf ( "%s\t", node_ptr->owner_name );
        else
            printf ( "%ld\t", node_ptr->user_id );

        if ( f_l_option )
            printf ( "%s\t", node_ptr->group_name );
        else
            printf ( "%ld\t", node_ptr->group_id );

#ifdef ENABLE_H_OPTION       
        if ( f_h_option )
        {
            char szbuf[5];
            if ( (humanize_number(szbuf,
                    sizeof(szbuf), 
                    (int64_t)node_ptr->number_of_bytes,
                    "",
                    HN_AUTOSCALE,
                    (HN_DECIMAL | HN_B | HN_NOSPACE))) == -1 )
            {
                fprintf ( stderr, "humanize_number()" );
                exit(1);
            }
            printf ( "%s\t", szbuf );
        }
        else
#endif
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
/*
    if ( f_t_option )
    {
        file_info_list_head = sort_by_time_modi ( file_info_list_head );
    }
    else
    {
        file_info_list_head = sort_by_lexi ( file_info_list_head );
    }
*/
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

struct file_info * sort_by_lexi ( struct file_info * pList )
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
            if ( *ppTrail == NULL ||
                 ( ((pHead->path_name[0])-' ') < ((*ppTrail)->path_name[0]-' ') ) )
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
    FTS * ftsp;
    FTSENT *p, *chp, *cur;
	
    /* 
        parse options
    */

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
            case 'h':
                f_h_option = 1;
                break;
            case 'i':
                f_i_option = 1;
                break;
			case 'l':
				f_l_option = 1;
				break;
            case 'n':
                f_n_option = 1;
                break;
            case 'R':
                f_R_option = 1;
                break;
            case 's':
                f_s_option = 1;
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

	/* 
        parse file argument
    */

	/* 
        If no operands are given, the contents of the current 
        directory are displayed. 
    */
    char * curr_dir = ".";
	
    if ( argc == 0 )
	{
        if ( ( ftsp =
             fts_open ( &curr_dir, FTS_LOGICAL, NULL ) ) == NULL )
        {
            fprintf ( stderr, "fts_open error" );
            fts_close ( ftsp );
            exit(1);
        }
     
        while ( ( p = fts_read ( ftsp ) ) != NULL )
        {
            switch ( p->fts_info ) 
            {
                // directory 
                case FTS_D:

                    if ( p->fts_level != FTS_ROOTLEVEL && !f_R_option )
                        break;

                    printf ( "^^%s\n", p->fts_name );
                    record_stat ( p->fts_statp, p->fts_name );
                    
                    // get files contained in a directory
                    chp = fts_children ( ftsp, 0 );
                    
                    // loop directory's files
                    for ( cur = chp; cur; cur = cur->fts_link )
                    {
                        printf ( "\t@@ %s\n", cur->fts_name );
                        record_stat ( cur->fts_statp, cur->fts_name );
                    }
                    break;

                default:
                    break;
            }
        }
        
        fts_close ( ftsp );

        print_file_info_list();

        exit (0);

    }

	/* 
        loop file arguments 
    */

	while (argc-- > 0)
	{
#ifdef DEBUG
        printf ( "\n### processing argv : %s\n", *argv );
#endif

        /* RE-initialize head of file_info linked list */
        file_info_list_head = NULL;

        if ( ( ftsp =
             fts_open ( &*argv, FTS_LOGICAL, NULL ) ) == NULL )
        {
            // BUG, never executed!!!
            fprintf ( stderr, "fts_open error %s", strerror(errno) );
            argv++;
            continue;
        }

        while ( ( p = fts_read ( ftsp ) ) != NULL )
        {
            switch ( p->fts_info ) 
            {
                /* directory */
                case FTS_D:

                    if ( p->fts_level != FTS_ROOTLEVEL && !f_R_option )
                        break;

                    printf ( "^^%s\n", p->fts_name );
                    record_stat ( p->fts_statp, p->fts_name );

                    // get files contained in a directory
                    chp = fts_children ( ftsp, 0 );
                    
                    // loop directory's files
                    for ( cur = chp; cur; cur = cur->fts_link )
                    {
                        printf ( "\t@@ %s\n", cur->fts_name );
                        record_stat ( cur->fts_statp, cur->fts_name );
                    }
                    break;

                default:
                    break;
            }
        }
        
        fts_close ( ftsp );

        print_file_info_list();
		
		argv++;
	}

	exit(0);
}
