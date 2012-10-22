/* 
 * ls.c
 * List directory contents
 *
 * SYNOPSIS
 * ls [-AacdFfhiklnqRrSstuw1] [file ...]
 *
 * Author: BoYu (byu1@stevens.edu)
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
#include <ctype.h>

/* print debug info */
/*
#define DEBUG
*/

/*
#define LINUXLAB
*/

#ifdef LINUXLAB
    #include <bsd/string.h>
#endif

/*
#define ENABLE_H_OPTION
*/

#ifdef ENABLE_H_OPTION
    #include <libutil.h>    /* for humanize_number() */
#endif

/* if environment variable COLUMNS is not defined or can't find, use this. */
#define COLUMNS 5

/*
    data structures
*/

/*
    struct file_info represents a file info which got from struct stat
*/

struct file_info
{
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

    unsigned long long number_of_blocks;

    struct file_info * next;            /* link to next node */
};

/* 
    global variables
*/

struct file_info * file_info_list_head = NULL;  /* list head */

int g_print_count;  /* marked how many file_info node have been out put */


/*
    function prototypes
*/

void usage();
void record_stat( struct stat * statp, char * path_name );
int get_file_info_list_length ();
void print_with_proper_option(struct file_info * node_ptr);
void print_file_info_list();
struct file_info * sort_by_time_modi_desc ( struct file_info * pList );
struct file_info * sort_by_time_modi_asce ( struct file_info * pList );
struct file_info * sort_by_lexi ( struct file_info * pList );
struct file_info * sort_by_lexi_rev ( struct file_info * pList );
struct file_info * sort_by_size_desc ( struct file_info * pList );
struct file_info * sort_by_size_asce ( struct file_info * pList );


/* 
    flags 
*/

int f_A_option;     /* except . .. */

int f_a_option;     /* include directory entries whose names begin 
                       with a dot ('.') */

int f_C_option;     /* Force multi-column output; this is the default
                       when output is to a terminal.*/

int f_c_option;     /* use time when file status was last changed, 
                       instead of time of last modification of the
                       file for sorting (-t) or printing (-l) */

int f_d_option;     /* directories are listed as plain files ( not
                       searched recursively ) and symbolic links in
                       the argument list are not indirected through
                    */

int f_F_option;     /* mark file type */

int f_f_option;     /* output is not sorted */

int f_h_option;     /* modifies the -s and -l options, causing the 
                       size to be reported in bytes displayed in a
                       human readable format. Overrides -k */

int f_i_option;     /* for each file, print the file's file file 
                       serial number ( inode number ) */

int f_k_option;     /* modifies the -s option, causing the sizes
                       to be reported in kilobytes. The rightmost
                       of the -k and -h flags overrides the previous 
                       flag. see also -h. */

int f_l_option;     /* long list */

int f_n_option;     /* the same as -l, except that the owner and 
                       group IDs are displayed numerically rather
                       than converting to a owner or group name */

int f_q_option;     /* force printing of non-printable characters 
                       in file names as the character '?', this is
                       default when output is to a terminal */

int f_R_option;     /* Recursively list subdirectories encountered */

int f_r_option;     /* reverse the order of the sort to get reverse
                       lexicographical order or the smallest or oldest
                       entries first. */

int f_S_option;     /* sort by size, largest file first. */

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

int f_w_option;     /* force raw printing of non-printable characters,
                       this is the default when output is not to a
                       terminal. */

int f_x_option;     /* The same as -C, except that the multi-column output
                       is produced with entries sorted across, rather than
                       down, the columns. */

int f_1_option;     /* force output to be one entry per line.
                       This is the default when output is not
                       to a terminal. */

/*
    banner
*/

void usage()
{
	printf("usage: ls [-AaCcdFfhiklnqRrSstuwx1] [file ...]\n");
}

/*
    add a file with info into the file_info linked list
*/

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
       assign file stat info into file_info structure 
    */
   
    /* 
        get file's file serial number (inode number)
    */

    new_node->inode_number = statp->st_ino;

    /* 
        get file type and permissons
    */
    
    strmode ( statp->st_mode, new_node->type_permission_info );
   
    /* 
        get file type
    */
    
    if ( S_ISDIR ( statp->st_mode ) )
        new_node->file_type = '/';
    else if ( S_ISLNK ( statp->st_mode ) )
        new_node->file_type = '@';
    else if (!access ( path_name, X_OK ))  /* executable file */
        new_node->file_type = '*';
#ifdef S_ISWHT
    else if ( S_ISWHT (statp->st_mode) )
        new_node->file_type = '%';
#endif
    else if ( S_ISSOCK ( statp->st_mode ) )
        new_node->file_type = '=';
    else if ( S_ISFIFO ( statp->st_mode ) )
        new_node->file_type = '|';

    /* 
        get file number of links 
    */
    
    new_node->number_of_links = statp->st_nlink;
    
    /* 
        get file owner 
    */
    
    new_node->user_id = statp->st_uid;
    password = getpwuid ( statp->st_uid );
    strcpy ( new_node->owner_name, password->pw_name );

    /*
        get file group owner 
    */
    
    new_node->group_id = statp->st_gid;
    group = getgrgid ( statp->st_gid );
    strcpy ( new_node->group_name, group->gr_name );

    /* 
        get number of bytes 
    */
    
    new_node->number_of_bytes = statp->st_size;

    /*
        get last access time
    */
    
    strftime ( new_node->last_access_time,
               sizeof(new_node->last_access_time),
               "%b %d %R",
               localtime ( & statp->st_atime ) );

    new_node->a_time = statp->st_atime;

    /*
        get last modified time
    */

    strftime ( new_node->last_modi_time,
               sizeof(new_node->last_modi_time),
               "%b %d %R",
               localtime ( &statp->st_mtime ) );

    new_node->m_time = statp->st_mtime; 

    /*
        get last change time
    */
    
    strftime ( new_node->last_change_time,
               sizeof(new_node->last_change_time),
               "%b %d %R",
               localtime ( &statp->st_ctime ) );
    
    new_node->c_time = statp->st_ctime;

    /*
        get file path name
    */
   
    if ( isatty (1) || f_q_option )
    {
        /* output is to a terminal or -q is set, then force 
           printing of non-printable characters in file name
           as the character '?'
        */

        char str [255];
        char * ptr;
        strcpy ( str, path_name );
        ptr = &str[0];
        while ( *ptr != '\0' )
        {
            if ( isprint(*ptr) == 0 )
            {
                *ptr = '?'; 
            }
            ptr ++;
        }
        strcpy ( new_node->path_name, str );
    }
    else
        strcpy ( new_node->path_name, path_name );
   
    /*
        get number of file system blocks actually used
    */

    char * blocksize_str;
    char * blocksize_constant = "BLOCKSIZE";
    unsigned long long blocksize = 0;
    unsigned long long n = 0;

    blocksize_str = getenv ( blocksize_constant );
    
    if ( NULL == blocksize_str )
    {
        /* ENV BLOCKSIZE is not set, so use default value */
        new_node->number_of_blocks = statp->st_blocks;
    }
    else
    {
        blocksize = strtoll ( blocksize_str, NULL, 0 );
        n = blocksize / 512;
        if ( n == 1 )
            new_node->number_of_blocks = statp->st_blocks;
        else
            new_node->number_of_blocks = statp->st_blocks / n;
    }

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

/*
    get file_info list length
*/

int get_file_info_list_length ()
{
    int len = 0;
    struct file_info * ptr = file_info_list_head;

    while ( ptr != NULL )
    {
        len++;
        ptr = ptr->next;
    }
    return len;
}

/*
    put out one file_info node info    
*/

void print_with_proper_option(struct file_info * node_ptr)
{
    if ( ! f_d_option )
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
        else if ( ! f_a_option )
        {
            /* 
                default output doesn't print file
                whose names begin with a dot ('.') 
            */ 
            if ( node_ptr->path_name[0] == '.' )
                return;
        }
    }

    if ( f_i_option )
        printf ( "%10ld ", node_ptr->inode_number );

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
            printf ( "%10s ", szbuf );
        }
        else if ( f_k_option )
        {
            char szbuf[5];
            if ( (humanize_number(szbuf,
                    sizeof(szbuf), 
                    (int64_t)node_ptr->number_of_blocks,
                    "k",
                    HN_AUTOSCALE,
                    (HN_DECIMAL | HN_B | HN_NOSPACE))) == -1 )
            {
                fprintf ( stderr, "humanize_number()" );
                exit(1);
            }
            printf ( "%10s ", szbuf );
             
        }
        else
#endif
        {
            printf ( "%10lld ", node_ptr->number_of_blocks );
        }
    }
   
    /* long format flag is specified */

    if ( f_l_option || f_n_option )
    {

        printf ( "%s ", node_ptr->type_permission_info );
        
        printf ( "%10d ",node_ptr->number_of_links );
        
        if ( f_l_option )
            printf ( "%s ", node_ptr->owner_name );
        else
            printf ( "%ld ", node_ptr->user_id );

        if ( f_l_option )
            printf ( "%s ", node_ptr->group_name );
        else
            printf ( "%ld ", node_ptr->group_id );

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
            printf ( "%s ", szbuf );
        }
        else
#endif
            printf ( "%10d ", node_ptr->number_of_bytes );

        if ( f_c_option )
            printf ( "%s ", node_ptr->last_change_time );
        else if ( f_u_option )
            printf ( "%s ", node_ptr->last_access_time );
        else
            printf ( "%s ", node_ptr->last_modi_time );
       
        /* print path name */
        printf ( "%s", node_ptr->path_name );

        if ( f_F_option )
        {
            if ( node_ptr->file_type != ' ' )
                printf ( "%c ", node_ptr->file_type );
        }
       
        /* if the file is a symbolic link, the pathname of the 
           linked-to file is preceded by "->" */
        if ( node_ptr->file_type == '@' )
        {
            char link_path [100];
            
            int ret = readlink ( node_ptr->path_name, link_path,
                sizeof(link_path)/sizeof(link_path[0]) );
            if ( ret == -1 )
            {
                fprintf ( stderr, "readlink() error : %s\n",
                    strerror ( errno ) );
                exit(1);
            }
            printf ( "-> %s ", link_path ); 
        }
    
        /* list one entry per line to standard output */
        if ( isatty (1) || f_1_option || !isatty (1) )
            printf ( "\n" );
    }
    /* 
        short output format
    */
    else 
    {
        printf ( "%s", node_ptr->path_name );
        if ( f_F_option )
        {
            if ( node_ptr->file_type != ' ' )
                printf ( "%c", node_ptr->file_type );
        }  
        
        /* -x */
        if ( f_x_option )
        {
            char * columns_str = "COLUMNS";
            char * columns;
            int col = 0;
            
            /* get environment variable COLUMNS */
            columns = getenv ( columns_str );
            if ( NULL == columns )
            {
                col = COLUMNS;
            }
            else
            {
                col = atoi ( columns );
            }
            
            if ( (g_print_count-1) % col == 0 )
            {
                printf ( "\n" ); 
            }
            else
            {
                printf ( "\t" );
            }
        }
        else
        {
            /* list one entry per line to standard output */
            if ( isatty (1) || f_1_option || !isatty (1) )
                printf ( "\n" );
        }
    }
}

/*
    out put every node of file_info list 
*/

void print_file_info_list()
{
    /* 
        -w
    */
    if ( f_w_option || !isatty(1) )
    {
        /* */
    }

    /* 
        sort the file_info list if needed
    */

    if ( ! f_f_option )
    {
        if ( f_t_option )
        {
            if ( ! f_r_option )
                file_info_list_head = sort_by_time_modi_desc ( file_info_list_head );
            else
                file_info_list_head = sort_by_time_modi_asce ( file_info_list_head );
        }
        else if ( f_S_option )
        {
            if ( ! f_r_option )
                file_info_list_head = sort_by_size_desc ( file_info_list_head );
            else
                file_info_list_head = sort_by_size_asce ( file_info_list_head );
        }
        else
        {
            if ( ! f_r_option )
                file_info_list_head = sort_by_lexi ( file_info_list_head );
            else
                file_info_list_head = sort_by_lexi_rev ( file_info_list_head );                          
        }
    }

    /*
        get a total sum for all the file sizes ( blocks )
        -l -n -s
    */

    struct file_info * ptr = file_info_list_head;
    unsigned long long sum = 0;
    if ( ! f_d_option )
    { 
        if ( f_l_option || f_n_option || ( f_s_option && isatty (1) ) )
        {
            while ( ptr != NULL )
            {
                if ( f_A_option )
                {
                    /* ignore . and .. */
                    if ( ! ( strcmp ( ptr->path_name, "." ) &&
                             strcmp ( ptr->path_name, "..") ))
                    {
                        sum += 0;
                        ptr = ptr->next;
                    }
                    else
                    {
                        sum += ptr->number_of_blocks;  
                        ptr = ptr->next;
                    }

                }
                else if ( ! f_a_option )
                {
                    /* 
                        default output doesn't print file
                        whose names begin with a dot ('.') 
                    */ 
                    if ( ptr->path_name[0] == '.' )
                    {
                        sum += 0;
                        ptr = ptr->next;
                    }
                    else
                    {
                        sum += ptr->number_of_blocks;  
                        ptr = ptr->next;
                    }
                }
                else
                {
                    sum += ptr->number_of_blocks;  
                    ptr = ptr->next;
                }
            }

            printf ( "total %lld\n", sum );
        }
    }

    /*
        out put every node
    */

    struct file_info * node_ptr = file_info_list_head;
    
    /* --- process -C --- */
    if ( f_C_option )
    {
        /* 1. create a matrix */

        char * columns_str = "COLUMNS"; 
        char * columns;
        int col = 0;
        int row = 0;
        int file_info_list_len = get_file_info_list_length();
        columns = getenv ( columns_str );
        if ( NULL == columns )
        {
            col = COLUMNS;
        }
        else
        {
            col = atoi ( columns );
        }
        row = file_info_list_len/col;

        int c = 0, r = 0;
        struct file_info * matrix [row][col];
#ifdef DEBUG
        printf ( "\n### row = %d, col = %d\n", row, col );
#endif       
/*
        for ( r = 0; r <= row; r++ )
            for ( c = 0; c <= col; c++ )
                matrix[r][c] = NULL;
*/
        /* 2. fill the matrix */

        for ( c = 0; c <= col; c++ )
        {
            for ( r = 0; r <= row; r++ )
            {
                if ( node_ptr != NULL )
                {
                    matrix [r][c] = node_ptr;
#ifdef DEBUG
                    printf ( "## %s\n", node_ptr->path_name );
#endif            
                    node_ptr = node_ptr->next;
                }
            }
        }
         
        /* 3. output matrix column by column */

        for ( r = 0; r <= row; r++ )
        {
            for ( c = 0; c <= col; c++ )
            {
                if ( matrix[r][c] != NULL )
                {
                    struct file_info * p = matrix [r][c];
                    printf ( "%s ", p->path_name );
                }
            }
            printf ( "\n" );
        }

    }
    /* --- end of -C --- */
    else
    {
        g_print_count = 0;
        
        while ( node_ptr != NULL )
        {
            print_with_proper_option ( node_ptr );
            node_ptr = node_ptr->next;
            g_print_count ++;
        }

        if ( f_x_option )
        {
            /* after the last file, print a newline */
            printf ( "\n" );
        }
    }
}

/*
    sort methods
*/

struct file_info * sort_by_time_modi_desc ( struct file_info * pList )
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

struct file_info * sort_by_time_modi_asce ( struct file_info * pList )
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
            if ( *ppTrail == NULL || pHead->m_time < (*ppTrail)->m_time )
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
                 ( ((pHead->path_name[0])-' ') <
                   ((*ppTrail)->path_name[0]-' ') ) )
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

struct file_info * sort_by_lexi_rev ( struct file_info * pList )
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
                 ( ((pHead->path_name[0])-' ') >
                   ((*ppTrail)->path_name[0]-' ') ) )
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

struct file_info * sort_by_size_desc ( struct file_info * pList )
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
                 ( (pHead->number_of_bytes) > 
                   ((*ppTrail)->number_of_bytes) ) )
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

struct file_info * sort_by_size_asce ( struct file_info * pList )
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
                 ( (pHead->number_of_bytes) < 
                   ((*ppTrail)->number_of_bytes) ) )
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

/*
    program entry
*/

int main ( int argc, char ** argv )
{
	int ch;
    FTS * ftsp;
    FTSENT *p, *chp, *cur;
    char * curr_dir = ".";
    int stat_ret;
    struct stat stat_buf;
    DIR * dp;
	struct dirent * dirp;

    /*
        -A is always set for the super user
    */

    if ( !getuid() )
        f_A_option = 1;

    /*
        The  tzset()  function initializes the tzname variable 
        from the TZ environment variable. 
    */
	
    tzset();

    /* 
        parse options
    */

	while ( ( ch = getopt(argc, argv, "AaCcdFfhiklnqRrSstuwx1") ) != -1 )
	{
		switch (ch)
		{
            case 'A':
                f_A_option = 1;
                break;
            case 'a':
                f_a_option = 1;
                break;
            case 'C':
                f_C_option = 1;
                f_x_option = 0;
                f_l_option = 0;
                f_n_option = 0;
                break;
            case 'c':
                f_c_option = 1;
                f_u_option = 0;     /* override -u */
                break;
            case 'd':
                f_d_option = 1;
                break;
            case 'F':
                f_F_option = 1;
                break;
            case 'f':
                f_f_option = 1;
                break;
            case 'h':
                f_h_option = 1;
                f_k_option = 0;     /* override -k */
                break;
            case 'i':
                f_i_option = 1;
                break;
            case 'k':
                f_k_option = 1;
                f_h_option = 0;     /* override -h */
                break;
			case 'l':
				f_l_option = 1;
                f_1_option = 0;     /* override */
                f_C_option = 0;
                f_x_option = 0;
				break;
            case 'n':
                f_n_option = 1;
                f_C_option = 0;
                f_x_option = 0;
                break;
            case 'q':
                f_q_option = 1;
                f_w_option = 0;
                break;
            case 'R':
                f_R_option = 1;
                break;
            case 'r':
                f_r_option = 1;
                break;
            case 'S':
                f_S_option = 1;
                break;
            case 's':
                f_s_option = 1;
                break;
            case 't':
                f_t_option = 1;
                break;
			case 'u':
                f_u_option = 1;
                f_c_option = 0;     /* override -c */
                break;
            case 'w':
                f_w_option = 1;
                f_q_option = 0;
                break;
            case 'x':
                f_x_option = 1;
                f_C_option = 0;
                f_l_option = 0;
                f_n_option = 0;
                break;
            case '1':
                f_1_option = 1;
                f_l_option = 0;
                break;
            default:
				usage();
		}
	}

	argc -= optind;
	argv += optind;

	/* 
        parse file argument(s)
    */

	/* 
        If no operands are given, the contents of the current 
        directory ( "." ) are displayed. 
    */
	
    if ( argc == 0 )
	{
        /* -d */
        if ( f_d_option )
        {
            stat_ret = lstat ( curr_dir, &stat_buf );
            if ( stat_ret < 0 )
            {
                fprintf ( stderr, "stat error\n" );
                exit (1);
            }

            record_stat ( &stat_buf, curr_dir );
            print_file_info_list ();
            exit (0);    
        }
        
        /* non-recursive */
        if ( ! f_R_option )
        {
            /* read directory NAME, and list the files in it */	
            if ( ( dp = opendir(curr_dir) ) == NULL )
            {
                fprintf ( stderr, "can't open '%s'\n", curr_dir );
                exit(1);
            }

            /* enter directory */
            if ( chdir(curr_dir) == -1 )
            {
                fprintf ( stderr, "can't chdir to '%s': %s\n",
                    curr_dir, strerror(errno) );
                exit (1);
            }

            while ( ( dirp = readdir(dp) ) != NULL )
            {
                if ( lstat ( dirp->d_name, &stat_buf ) < 0 )
                {
                    fprintf ( stderr, "lstat() error" );
                    exit (1);
                }
                
                record_stat ( &stat_buf, dirp->d_name );
            }

            if ( closedir(dp) < 0 )
            {
                fprintf ( stderr, "can't close directory\n" );
                exit(1);
            }

            print_file_info_list();
            
            exit (0);
        }
        /* -R : recursive */
        else
        {
            if ( ( ftsp =
                 fts_open ( &curr_dir, FTS_LOGICAL, NULL ) ) == NULL )
            {
                fprintf ( stderr, "fts_open() error" );
                fts_close ( ftsp );
                exit(1);
            }
         
            while ( ( p = fts_read ( ftsp ) ) != NULL )
            {
                switch ( p->fts_info ) 
                {
                    /* directory */ 
                    case FTS_D:
#ifdef DEBUG
                        printf ( "^^%s\n", p->fts_name );
#endif
                        /* print directory path */                        
                        printf ( "%s:\n", p->fts_path ); 

                        // get files contained in a directory
                        chp = fts_children ( ftsp, 0 );
                        
                        // loop directory's files
                        for ( cur = chp; cur; cur = cur->fts_link )
                        {
#ifdef DEBUG
                            printf ( "\t@@ %s\n", cur->fts_name );
#endif            
                            record_stat ( cur->fts_statp, cur->fts_name );
                        }
                        
                        print_file_info_list();
                        
                        printf ( "\n" );

                        /* RE-initialize head of file_info linked list */
                        file_info_list_head = NULL;

                        break;

                    default:
                        break;
                }
            }
            
            if ( fts_close ( ftsp ) == -1 )
            {
                fprintf ( stderr, "fts_close() error.\n" );
                exit (1);
            }

            exit (0);
        }
    }

	/* 
        loop file arguments 
    */

    /* save current directory path */
    char save_current_dir [255];
    getcwd ( save_current_dir, sizeof(save_current_dir) / sizeof(save_current_dir[0]) ); 

	while (argc-- > 0)
	{
#ifdef DEBUG
        printf ( "\n## processing argv : %s\n", *argv );
#endif
        
        /* enter original working directory */
        if ( chdir(save_current_dir) == -1 )
        {
            fprintf ( stderr, "can't chdir to '%s': %s\n",
                save_current_dir, strerror(errno) );
            exit (1);
        }

        /* RE-initialize head of file_info linked list */
        file_info_list_head = NULL;

        stat_ret = lstat ( *argv, &stat_buf );
		if ( stat_ret < 0 )
		{
			fprintf ( stderr, "stat error for %s\n", *argv );
            argv++;
			continue;   /* to process the next argv */
		}

        /* argument is a file */
		if ( S_ISREG ( stat_buf.st_mode ) )
		{
            record_stat ( &stat_buf, *argv );
            print_file_info_list();
		}
        /* argument is a directory */
        else if ( S_ISDIR ( stat_buf.st_mode ) )
        {
            /* 
               -d means we need just print directory info
               (not recursively)
            */
            if ( f_d_option )
            {
                stat_ret = lstat ( *argv, &stat_buf );
                if ( stat_ret < 0 )
                {
                    fprintf ( stderr, "lstat error\n" );
                    argv++;
                    continue;
                }

                record_stat ( &stat_buf, *argv );
                print_file_info_list ();
                argv++; 
                continue;
            }

            /* non-recursive */
            if ( ! f_R_option )
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
                    if ( lstat ( dirp->d_name, &stat_buf ) < 0 )
                    {
                        fprintf ( stderr, "stat() error" );
                        exit (1);
                    }
                    
                    record_stat ( &stat_buf, dirp->d_name );
                }

                if ( closedir(dp) < 0 )
                {
                    fprintf ( stderr, "can't close directory\n" );
                    exit(1);
                }
                print_file_info_list();
            }
            /* -R : recursive */
            else
            {
                if ( ( ftsp =
                     fts_open ( &*argv, FTS_LOGICAL, NULL ) ) == NULL )
                {
                    // BUG, never executed!!!
                    fprintf ( stderr, "fts_open error %s", 
                        strerror(errno) );
                    argv++;
                    continue;
                }

                while ( ( p = fts_read ( ftsp ) ) != NULL )
                {
                    switch ( p->fts_info ) 
                    {
                        /* directory */
                        case FTS_D:

                            /* print directory path */                        
                            printf ( "%s:\n", p->fts_path ); 

                            // get files contained in a directory
                            chp = fts_children ( ftsp, 0 );
                            
                            // loop directory's files
                            for ( cur = chp; cur; cur = cur->fts_link )
                            {
                                record_stat ( cur->fts_statp, cur->fts_name );
                            }
                            
                            print_file_info_list();
                            
                            printf ( "\n" );

                            /* RE-initialize head of file_info linked list */
                            file_info_list_head = NULL;
                            
                            break;

                        default:
                            break;
                    }
                }
                
                if ( -1 == fts_close ( ftsp ) )
                {
                    fprintf ( stderr, "fts_close() error.\n" );
                    argv++;
                    continue;
                }

                print_file_info_list();
	        }	
        }

		argv++;

	} /* endof while ( argc-- > 0 ) */

    /* return with success */
	exit(0);

} /* end of main() */
