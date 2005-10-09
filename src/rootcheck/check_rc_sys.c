/*   $OSSEC, check_rc_sys.c, v0.1, 2005/10/04, Daniel B. Cid$   */

/* Copyright (C) 2005 Daniel B. Cid <dcid@ossec.net>
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

 
#include <stdio.h>       
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include "headers/defs.h"
#include "headers/debug_op.h"

#include "rootcheck.h"

int _sys_errors;
int _sys_total;

FILE *_wx;
FILE *_ww;
FILE *_suid;

/** Prototypes **/
int read_sys_dir(char *dir_name);

int read_sys_file(char *file_name)
{
    struct stat statbuf;
    
    if(lstat(file_name, &statbuf) < 0)
    {
        return(-1);
    }
    
    /* If directory, read the directory */
    else if(S_ISDIR(statbuf.st_mode))
    {
        return(read_sys_dir(file_name));
    }
    
    /* If has OTHER write and exec permission, alert */
    if(((statbuf.st_mode & S_IWOTH) == S_IWOTH) && 
         (S_ISREG(statbuf.st_mode)))
    {
        if((statbuf.st_mode & S_IXUSR) == S_IXUSR)
        {
            if(_wx)
                fprintf(_wx, "%s\n",file_name);
                
            _sys_errors++;    
        }
        else
        {
            if(_ww)
                fprintf(_ww, "%s\n", file_name);
        }

        if(statbuf.st_uid == 0)
        {
            char op_msg[OS_MAXSTR +1];
            snprintf(op_msg, OS_MAXSTR, "File '%s' is owned by root and has"
                                        " written permission to anyone.",
                                        file_name);

            notify_rk(ALERT_SYSTEM_CRIT, op_msg);

        }
        _sys_errors++;
    }

    else if((statbuf.st_mode & S_ISUID) == S_ISUID)
    {
        if(_suid)
            fprintf(_suid,"%s\n", file_name);
    }

    return(0);
}

/* read_dir v0.1
 *
 */
int read_sys_dir(char *dir_name)
{
    DIR *dp;
    
	struct dirent *entry;
	
    
    if((dir_name == NULL)||(strlen(dir_name) > PATH_MAX))
    {
        merror("%s: Invalid directory given",ARGV0);
        return(-1);
    }
    
    /* Opening the directory given */
    dp = opendir(dir_name);
	if(!dp)
    {
        if((strcmp(dir_name, "") == 0)&&
           (dp = opendir("/"))) 
        {
            /* ok */
        }
        else
        {
            return(-1);
        }
    }

    while((entry = readdir(dp)) != NULL)
    {
        char f_name[PATH_MAX +2];

        /* Just ignore . and ..  */
        if((strcmp(entry->d_name,".") == 0) ||
           (strcmp(entry->d_name,"..") == 0))  
            continue;
        
        snprintf(f_name, PATH_MAX +1, "%s/%s",dir_name, entry->d_name);
        
        read_sys_file(f_name);

    }

    closedir(dp);
    
    return(0);
}


/*  check_rc_sys: v0.1
 *  Scan the whole filesystem looking for possible issues
 */
void check_rc_sys(char *basedir)
{
    char file_path[OS_MAXSTR +1];

    _sys_errors = 0;
    _sys_total = 0;
    
    snprintf(file_path, OS_MAXSTR, "%s", basedir);

    /* Opening output files */
    _wx = fopen("rootcheck-rw-rw-rw-.txt", "w");
    _ww = fopen("rootcheck-rwxrwxrwx.txt", "w");
    _suid=fopen("rootcheck-suid-files.txt", "w");
    
    read_sys_dir(file_path);

    if(_sys_errors == 0)
    {
        char op_msg[OS_MAXSTR +1];
        snprintf(op_msg, OS_MAXSTR, "No problem found on the system."
                                    " Analized %d files.", _sys_total);
        notify_rk(ALERT_OK, op_msg);
    }

    else
    {
        char op_msg[OS_MAXSTR +1];
        snprintf(op_msg, OS_MAXSTR, "Check the following files for more "
                                    "information:\n"
                                    "      rootcheck-rw-rw-rw-.txt\n"
                                    "      rootcheck-rwxrwxrwx.txt\n"
                                    "      rootcheck-suid-files.txt\n");
        
        notify_rk(ALERT_SYSTEM_ERROR, op_msg);
    }

    if(_wx)
    {
        fclose(_wx);
    }
    
    if(_ww)
    {
        fclose(_ww);
    }
    
    if(_suid)
    {
        fclose(_suid); 
    }
               
    return;
}

/* EOF */
