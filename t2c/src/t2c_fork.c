/******************************************************************************
Copyright (C) 2007 The Linux Foundation. All rights reserved.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
******************************************************************************/

//#include "../include/t2c_util.h"
#include "../include/t2c_tet_support.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

// The stuff below came from TET 3.7-lite with several changes 
// (mostly simplifications).

///////////////////////////////////////////////////////////////////////////
pid_t t2c_child = 0;

///////////////////////////////////////////////////////////////////////////
// Kill the child and wait T2C_KILLWAIT seconds for it to die.
// t2c_killwait is the tet_killw function with minor changes.
#define T2C_KILLWAIT    10

static int 
t2c_killwait(pid_t child, unsigned int timeout);

///////////////////////////////////////////////////////////////////////////

static int 
t2c_def_pcf(pid_t returned_pid, int* status);

///////////////////////////////////////////////////////////////////////////
// Alarm-related stuff
struct alrmaction 
{
    unsigned int waittime;
    struct sigaction sa;
    sigset_t mask;
};

// Nonzero if an alarm has been raised. Alarm handlers increment this flag.
int t2c_alarm_flag;

static void 
t2c_catch_alarm(int);

static int 
t2c_set_alarm(struct alrmaction *, struct alrmaction *);

static int 
t2c_clr_alarm(struct alrmaction *);

///////////////////////////////////////////////////////////////////////////
int pcc_pipe_[2];   // for parent-child control data transfer

///////////////////////////////////////////////////////////////////////////
// Signal handlers
static void
sig_term(int sig)
{
    /* clean up on receipt of SIGTERM, but arrange for wait
       status still to show termination by SIGTERM */

    struct sigaction sa;

    if (t2c_child > 0)
    {
        t2c_killwait(t2c_child, T2C_KILLWAIT);
    }

    sa.sa_handler = SIG_DFL;
    sa.sa_flags = 0; 
    sigemptyset(&sa.sa_mask); 
    sigaction(SIGTERM, &sa, (struct sigaction *)NULL);
    kill(getpid(), SIGTERM);
}

///////////////////////////////////////////////////////////////////////////
// t2c_fork()
int 
t2c_fork(TChildFunc childfunc, TParentControlFunc pcf, int waittime,
    TUserStartup ustartup, TUserCleanup ucleanup)
{
    int rtval, err, status;
    pid_t   savchild, pid;
    char    buf[256];
    struct sigaction tsa, new_sa; 
    struct alrmaction new_aa, old_aa; 
    
    int ret_status = -1;
    
    int sig;
    int ch_sig[] = {SIGTERM, SIGALRM, SIGABRT};
    
    fflush(stdout);
    fflush(stderr);
    
    t2c_alarm_flag = 0;

    // Save old value of t2c_child in case of recursive calls
    // to t2c_fork().
    savchild = t2c_child;
    
    pid = fork();
    t2c_child = pid;
    
    int init_failed = 0;
    char* reason = NULL;

    switch (pid)
    {
    
    case -1:
        err = errno;
        sprintf(buf, "t2c_fork: fork() failed. errno: %d.", err);
        tet_infoline(buf);
        tet_result(TET_UNRESOLVED);

        t2c_child = savchild;
        return -1;

    case 0:
        /* child process */
        
        // Reset signal handlers to default ones.
        for (sig = 0; sig < sizeof(ch_sig)/sizeof(ch_sig[0]); ++sig)
        {
            if (sigaction(ch_sig[sig], (struct sigaction *) 0, &tsa) != -1 && 
                (tsa.sa_handler != SIG_IGN && tsa.sa_handler != SIG_DFL)) 
            {
    			tsa.sa_handler = SIG_DFL;
    		    sigaction(ch_sig[sig], &tsa, (struct sigaction *) 0);
    		}
        }
		
        /* change context to distinguish output from parent's */
        tet_setcontext();

        // call startup (if not NULL) & examine init_failed.
        if (ustartup)
        {
            ustartup(&init_failed, &reason);
        }
        
        if (init_failed)
        {
            if (reason)
            {
                tet_infoline(reason);
            }
            tet_result(TET_UNINITIATED);
            
            // Write no-PASS test status to the pipe.
            int st = 0;
            int pcc_num = write(pcc_pipe_[1], &st, sizeof(int));
            if (pcc_num != sizeof(int))
            {
                sprintf(buf, "t2c_fork: unable to write to the parent-child control pipe, errno: %d.", errno);
                tet_infoline(buf);
            }
        }
        else
        {        
            // OK, call child function
            childfunc();
        }
        
        // call cleanup (if not NULL).
        if (ucleanup)
        {
            ucleanup();
        }
        
        exit(EXIT_SUCCESS);
    }
    
    ////////////////////////////////////////////////////////////////////////
    // parent process

    /* if SIGTERM is set to default (e.g. if this t2c_fork() was
       called from a child), catch it so we can propagate t2c_killwait() */
    if (sigaction(SIGTERM, (struct sigaction *)NULL, &new_sa) != -1 &&
        new_sa.sa_handler == SIG_DFL)
    {
        new_sa.sa_handler = sig_term;
        sigaction(SIGTERM, &new_sa, (struct sigaction *)NULL);
    }
    
    tet_setblock();

    /* wait for the child */
    if (waittime > 0)
    {
        new_aa.waittime = waittime; 
        new_aa.sa.sa_handler = t2c_catch_alarm; 
        new_aa.sa.sa_flags = 0; 
        (void) sigemptyset(&new_aa.sa.sa_mask); 
        
        t2c_alarm_flag = 0; 
        if (t2c_set_alarm(&new_aa, &old_aa) == -1)
        {
            sprintf(buf, "t2c_fork: failed to set alarm. errno: %d.", errno);
            tet_infoline(buf);
            tet_result(TET_UNRESOLVED);
            
            t2c_child = savchild;
            
            return -1;
        }
    }
    
    rtval = waitpid(t2c_child, &status, WUNTRACED);
    err = errno; 

    if (waittime > 0)
    {
        (void) t2c_clr_alarm(&old_aa);
    }

    int bContinue = 1;
    if (rtval == -1)
    {
        if (t2c_alarm_flag > 0)
        {
            sprintf(buf, "Child process timed out.");
            tet_infoline(buf);
            
            tet_result(TET_UNRESOLVED);
            t2c_killwait(t2c_child, T2C_KILLWAIT);
                        
            t2c_child = savchild;
            bContinue = 0;    // We need to exit ASAP, but read the child status first.
        }
    }
    
    // Read test status submitted by the child.
    int pcc_num = read(pcc_pipe_[0], &ret_status, sizeof(int));
    if (pcc_num != sizeof(int))
    {
        sprintf(buf, "t2c_fork: unable to read from the parent-child control pipe, errno: %d.", errno);
        tet_infoline(buf);
        ret_status = -1; // something wrong happened
    }
    
    if (!bContinue) 
    {
        // If we returned -1 the result would be set to UNRESOLVED and this
        // kind of result overrides any user-defined result codes.
        return 0;
    }
    
    // Check if the child has successfully completed its work.
    TParentControlFunc real_pcf = (pcf != NULL) ? pcf : t2c_def_pcf;
    if (real_pcf(rtval, &status) == 0) // if something went wrong...
    {
        sprintf(buf, "Abnormal child process termination. errno: %d.", err);
        tet_infoline(buf);
            
        tet_result(TET_UNRESOLVED);
                    
        // Kill the child with all its descendants.
        t2c_killwait(t2c_child, T2C_KILLWAIT);

        t2c_child = savchild;
        return -1;
    }
    else
    {
        t2c_child = savchild;
        return ret_status;
    }
    
    return -1;
}

static int 
t2c_killwait(pid_t child, unsigned int timeout)
{
    pid_t pid;
    int sig = SIGTERM;
    int ret = -1;
    int err, count, status;
    struct alrmaction new_aa, old_aa; 
    char buf[256];

    //<>
    //fprintf(stderr, "[NB] Doomed to die: %d.\n", (int)child);
    //<>

    new_aa.waittime = timeout; 
    new_aa.sa.sa_handler = t2c_catch_alarm; 
    new_aa.sa.sa_flags = 0; 
    sigemptyset(&new_aa.sa.sa_mask); 

    for (count = 0; count < 2; ++count)
    {
        if (kill(child, sig) == -1 && errno != ESRCH)
        {
            err = errno;
            break;
        }

        t2c_alarm_flag = 0; 
        if (t2c_set_alarm(&new_aa, &old_aa) == -1)
        {
            sprintf(buf, "Failed to set alarm - errno %d.", errno);
            tet_infoline(buf);
            tet_result(TET_UNRESOLVED);
            
            return -1;
        }
        
        pid = waitpid(child, &status, 0);
        err = errno;
        t2c_clr_alarm(&old_aa);

        if (pid == child)
        {
            ret = 0;
            break;
        }
        if (pid == -1 && t2c_alarm_flag == 0 && errno != ECHILD)
        {
            break;
        }
        
        sig = SIGKILL; /* use a stronger signal the next time */
    }

    errno = err;
    return ret;
}

static int
t2c_set_alarm(struct alrmaction *new_aa, struct alrmaction *old_aa)
{
    sigset_t alrmset;

    if (new_aa->waittime == 0)
    {
        return -2;
    }
    
    if (sigaction(SIGALRM, &new_aa->sa, &old_aa->sa) == -1)
    {
        return -1;
    }

    /* SIGALRM is blocked between tet_sigsafe_start/end calls,
       so unblock it.  (This means the handler mustn't longjmp.) */
    sigemptyset(&alrmset);
    sigaddset(&alrmset, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &alrmset, &old_aa->mask);

    alarm(new_aa->waittime);

    return 0;
}

static int
t2c_clr_alarm(struct alrmaction *old_aa)
{
    alarm(0);
    sigprocmask(SIG_SETMASK, &old_aa->mask, (sigset_t *)0);
    if (sigaction(SIGALRM, &old_aa->sa, (struct sigaction *)0) == -1)
    {
        return -1;
    }
    return 0;
}

// a handler for SIGALRM signal.
static void 
t2c_catch_alarm(int sig)
{
    // no longjmp, just increment the counter.
    ++t2c_alarm_flag;
}

static int 
t2c_def_pcf(pid_t returned_pid, int* status)
{
    if ((returned_pid != -1) && (status != NULL))
    {
        char buf[256];
        
        if (WIFEXITED(*status))
        {
            return 1;
        }
        else if (WIFSIGNALED(*status))
        {
            *status = WTERMSIG(*status);
            sprintf(buf, "Child process was terminated by signal %d.", *status);
            tet_infoline(buf);
        }
        else if (WIFSTOPPED(*status))
        {
            *status = WSTOPSIG(*status);
            sprintf(buf, "Child process was stopped by signal %d.", *status);
            tet_infoline(buf);
            t2c_killwait(t2c_child, T2C_KILLWAIT);
        }
        else
        {
            sprintf(buf, "Child process returned bad wait status (%#x)", *status);
            tet_infoline(buf);
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////
// t2c_fork_dbg() - debug version of t2c_fork
int 
t2c_fork_dbg(TChildFunc childfunc,  TParentControlFunc pcf, int waittime,
         TUserStartup ustartup, TUserCleanup ucleanup)
{
    char buf[256];
    int ret_status = -1; // failure is assumed by default
    
    fflush(stdout);
    fflush(stderr);
    
    int init_failed = 0;
    char* reason = NULL;

    if (ustartup)
    {
        ustartup(&init_failed, &reason);
    }
    
    if (init_failed)
    {
        if (reason)
        {
            tet_infoline(reason);
        }
        tet_result(TET_UNINITIATED);
        
        // Write no-PASS test status to the pipe.
        int st = 0;
        int pcc_num = write(pcc_pipe_[1], &st, sizeof(int));
        if (pcc_num != sizeof(int))
        {
            sprintf(buf, "t2c_fork_dbg: unable to write to the parent-child control pipe, errno: %d.", errno);
            tet_infoline(buf);
        }
    }
    else
    {        
        // OK, call the "child" function
        childfunc();
    }
    
    if (ucleanup)
    {
        ucleanup();
    }
    
    // Read test status submitted by the "child" function.
    int pcc_num = read(pcc_pipe_[0], &ret_status, sizeof(int));
    if (pcc_num != sizeof(int))
    {
        sprintf(buf, "t2c_fork_dbg: unable to read from the parent-child control pipe, errno: %d.", errno);
        tet_infoline(buf);
        ret_status = -1; // something wrong happened
    }
    
    return ret_status;
}
