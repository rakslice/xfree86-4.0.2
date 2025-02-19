#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "xf86_OSproc.h"
#include "xf86_ansic.h"


#define MODPROBE_PATH_FILE      "/proc/sys/kernel/modprobe"
#define MAX_PATH                1024


/* XFree86 #defines execl to be the xf86execl() function which does
 * a fork AND exec.  We don't want that.  We want the regular,
 * standard execl().
 */
#ifdef execl
#undef execl
#endif


/*
 * Load a Linux kernel module.
 * This is used by the DRI/DRM to load a DRM kernel module when
 * the X server starts.  It could be used for other purposes in the future.
 * Input:
 *    modName - name of the kernel module (Ex: "tdfx")
 * Return:
 *    0 for failure, 1 for success
 */
int xf86LoadKernelModule(const char *modName)
{
   char mpPath[MAX_PATH] = "";
   int fd = -1, status, n;
   pid_t pid;

   /* get the path to the modprobe program */
   fd = open(MODPROBE_PATH_FILE, O_RDONLY);
   if (fd >= 0) {
      int count = read(fd, mpPath, MAX_PATH - 1);
      if (count <= 0) {
         mpPath[0] = 0;
      }
      else if (mpPath[count - 1] == '\n') {
         mpPath[count - 1] = 0;  /* replaces \n with \0 */
      }
      close(fd);
      /* if this worked, mpPath will be "/sbin/modprobe" or similar. */
   }

   if (mpPath[0] == 0) {
      /* we failed to get the path from the system, use a default */
      xf86strcpy(mpPath, "/sbin/modprobe");
   }

   /* now fork/exec the modprobe command */
   switch (pid = fork()) {
   case 0:  /* child */
      n = execl(mpPath, "modprobe", modName, NULL);
      exit(EXIT_FAILURE);  /* if we get here the child's exec failed */
      break;
   case -1:  /* fork failed */
      return 0;
   default:  /* fork worked */
      {
         /* XXX we loop over waitpid() because it sometimes fails on
          * the first attempt.  Don't know why!
          */
         int count = 0, p;
         do {
            p = waitpid(pid, &status, 0);
         } while (p == -1 && count++ < 4);

         if (p == -1) {
            return 0;
         }

         if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            return 1;  /* success! */
         }
         else {
            return 0;
         }
      }
   }

   /* never get here */
   return 0;
}
