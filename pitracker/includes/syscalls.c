/* Support files for GNU libc.  Files in the system namespace go here.
   Files in the C namespace (ie those that do not start with an
   underscore) go in .c.  */

#include <_ansi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#include <reent.h>

#include <pi/debug.h>
void uart_putc ( unsigned int c );

unsigned int heap_end=0x0020000;
unsigned int prev_heap_end;

/* Forward prototypes.  */
int     _system     _PARAMS ((const char *));
int     _rename     _PARAMS ((const char *, const char *));
int     isatty      _PARAMS ((int));
clock_t _times      _PARAMS ((struct tms *));
int     _gettimeofday   _PARAMS ((struct timeval *, struct timezone *));
void    _raise      _PARAMS ((void));
int     _unlink     _PARAMS ((void));
int     _link       _PARAMS ((void));
int     _stat       _PARAMS ((const char *, struct stat *));
int     _fstat      _PARAMS ((int, struct stat *));
caddr_t _sbrk       _PARAMS ((int));
int     _getpid     _PARAMS ((int));
int     _kill       _PARAMS ((int, int));
void    _exit       _PARAMS ((int));
int     _close      _PARAMS ((int));
int     _open       _PARAMS ((const char *, int, ...));
int     _write      _PARAMS ((int, char *, int));
int     _lseek      _PARAMS ((int, int, int));
int     _read       _PARAMS ((int, char *, int));
void    initialise_monitor_handles _PARAMS ((void));

//static int
//remap_handle (int fh)
//{
    //return 0;
//}

void
initialise_monitor_handles (void)
{
    debug("initialise_monitor_handles is a stub\r\n");
}

//static int
//get_errno (void)
//{
    //return(0);
//}

//static int
//error (int result)
//{
  //errno = get_errno ();
  //return result;
//}


int
_read (int file,
       char * ptr,
       int len)
{
  debug("_read is a stub\r\n");
  return len;
}


int
_lseek (int file,
    int ptr,
    int dir)
{
  debug("_lseek is a stub\r\n");
    return 0;
}


int
_write (int    file,
    char * ptr,
    int    len)
{
    int r;
    for(r=0;r<len;r++) uart_putc(ptr[r]);
    return len;
}

//int write(int file, char*ptr, int len) { _write(file, ptr, len); }

int
_open (const char * path,
       int          flags,
       ...)
{
  debug("_open is a stub\r\n");
    return 0;
}


int
_close (int file)
{
  debug("_close is a stub\r\n");
    return 0;
}

void
_exit (int n)
{
  debug("_exit is a stub\r\n");
    while(1);
}

int
_kill (int n, int m)
{
  debug("_kill is a stub\r\n");
    return(0);
}

int
_getpid (int n)
{
  debug("_getpid is a stub\r\n");
  return 1;
  n = n;
}


caddr_t
_sbrk (int incr)
{
    prev_heap_end = heap_end;
    heap_end += incr;
    return (caddr_t) prev_heap_end;
}

int
_fstat (int file, struct stat * st)
{
  debug("_fstat is a stub\r\n");
  return 0;
}

int _stat (const char *fname, struct stat *st)
{
  debug("_stat is a stub\r\n");
  return 0;
}

int
_link (void)
{
  debug("_link is a stub\r\n");
  return -1;
}

int
_unlink (void)
{
  debug("_unlink is a stub\r\n");
  return -1;
}

void
_raise (void)
{
  debug("_raise is a stub\r\n");
  return;
}

int
_gettimeofday (struct timeval * tp, struct timezone * tzp)
{
    if(tp)
    {
        tp->tv_sec = 10;
        tp->tv_usec = 0;
    }
    if (tzp)
    {
        tzp->tz_minuteswest = 0;
        tzp->tz_dsttime = 0;
    }
    return 0;
}

clock_t
_times (struct tms * tp)
{
    clock_t timeval;

    timeval = 10;
    if (tp)
    {
        tp->tms_utime  = timeval;   /* user time */
        tp->tms_stime  = 0; /* system time */
        tp->tms_cutime = 0; /* user time, children */
        tp->tms_cstime = 0; /* system time, children */
    }
    return timeval;
};


int
_isatty (int fd)
{
  return 1;
  fd = fd;
}

int
_system (const char *s)
{
  if (s == NULL)
    return 0;
  errno = ENOSYS;
  return -1;
}

int
_rename (const char * oldpath, const char * newpath)
{
  errno = ENOSYS;
  return -1;
}

