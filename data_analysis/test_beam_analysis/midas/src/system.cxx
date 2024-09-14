/********************************************************************\

  Name:         system.c
  Created by:   Stefan Ritt

  Contents:     All operating system dependent system services. This
                file containt routines which hide all system specific
                behaviour to higher levels. This is done by con-
                ditional compiling using the OS_xxx variable defined
                in MIDAS.H.

                Details about interprocess communication can be
                found in "UNIX distributed programming" by Chris
                Brown, Prentice Hall

  $Id$

\********************************************************************/

/**dox***************************************************************/
/** @file system.c
The Midas System file
*/

/** @defgroup msfunctionc  System Functions (ss_xxx)
 */

/**dox***************************************************************/
/** @addtogroup msfunctionc
 *
 *  @{  */

#undef NDEBUG // midas required assert() to be always enabled

#include <stdio.h>
#include <math.h>
#include <vector>
#include <atomic> // std::atomic_int & co
#include <thread>
#include <array>
#include <stdexcept>

#include "midas.h"
#include "msystem.h"

#ifndef HAVE_STRLCPY
#include "strlcpy.h"
#endif

#ifdef OS_UNIX
#include <sys/mount.h>
#endif

#ifdef LOCAL_ROUTINES
#include <signal.h>

/*------------------------------------------------------------------*/
/* globals */

#if defined(OS_UNIX)

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#if defined(OS_DARWIN)
#include <sys/posix_shm.h>
#endif

static int shm_trace = 0;
static int shm_count = 0;

static int use_sysv_shm = 0;
static int use_mmap_shm = 0;
static int use_posix_shm = 0;
static int use_posix1_shm = 0;
static int use_posix2_shm = 0;
static int use_posix3_shm = 0;
static int use_posix4_shm = 0;

#endif

static void check_shm_type(const char* shm_type)
{
#ifdef OS_UNIX
   char file_name[256];
   const int file_name_size = sizeof(file_name);
   char path[256];
   char buf[256];
   char* s;

   cm_get_path(path, sizeof(path));
   if (path[0] == 0) {
      if (!getcwd(path, sizeof(path)))
         path[0] = 0;
      strlcat(path, "/", sizeof(path));
   }

   strlcpy(file_name, path, file_name_size);
   strlcat(file_name, ".", file_name_size); /* dot file under UNIX */
   strlcat(file_name, "SHM_TYPE", file_name_size);
   strlcat(file_name, ".TXT", file_name_size);

   FILE* fp = fopen(file_name, "r");
   if (!fp) {
      fp = fopen(file_name, "w");
      if (!fp) {
         fprintf(stderr, "check_shm_type: Cannot write to config file \'%s\', errno %d (%s)", file_name, errno, strerror(errno));
         exit(1);
         // DOES NOT RETURN
      }

      fprintf(fp, "%s\n", shm_type);
      fclose(fp);

      fp = fopen(file_name, "r");
      if (!fp) {
         fprintf(stderr, "check_shm_type: Cannot open config file \'%s\', errno %d (%s)", file_name, errno, strerror(errno));
         exit(1);
         // DOES NOT RETURN
      }
   }

   if (!fgets(buf, sizeof(buf), fp))
      buf[0] = 0;

   fclose(fp);

   s = strchr(buf, '\n');
   if (s)
      *s = 0;

   //printf("check_shm_type: preferred %s got %s\n", shm_type, buf);

   if (strcmp(buf, "SYSV_SHM") == 0) {
      use_sysv_shm = 1;
      return;
   }

   if (strcmp(buf, "MMAP_SHM") == 0) {
      use_mmap_shm = 1;
      return;
   }

   if (strcmp(buf, "POSIX_SHM") == 0) {
      use_posix1_shm = 1;
      use_posix_shm = 1;
      return;
   }

   if (strcmp(buf, "POSIXv2_SHM") == 0) {
      use_posix2_shm = 1;
      use_posix_shm = 1;
      return;
   }

   if (strcmp(buf, "POSIXv3_SHM") == 0) {
      use_posix3_shm = 1;
      use_posix_shm = 1;
      return;
   }

   if (strcmp(buf, "POSIXv4_SHM") == 0) {
      use_posix4_shm = 1;
      use_posix_shm = 1;
      return;
   }

   fprintf(stderr, "check_shm_type: Config file \"%s\" specifies unknown or unsupported shared memory type \"%s\", supported types are: SYSV_SHM, MMAP_SHM, POSIX_SHM, POSIXv2_SHM, POSIXv3_SHM, POSIXv4_SHM, default/preferred type is \"%s\"\n", file_name, buf, shm_type);
   exit(1);
#endif
}

static void check_shm_host()
{
   char file_name[256];
   const int file_name_size = sizeof(file_name);
   char path[256];
   char buf[256];
   char hostname[256];
   char* s;
   FILE *fp;

   gethostname(hostname, sizeof(hostname));

   //printf("hostname [%s]\n", hostname);

   cm_get_path(path, sizeof(path));
   if (path[0] == 0) {
      if (!getcwd(path, sizeof(path)))
         path[0] = 0;
#if defined(OS_VMS)
#elif defined(OS_UNIX)
      strlcat(path, "/", sizeof(path));
#elif defined(OS_WINNT)
      strlcat(path, "\\", sizeof(path));
#endif
   }

   strlcpy(file_name, path, file_name_size);
#if defined (OS_UNIX)
   strlcat(file_name, ".", file_name_size); /* dot file under UNIX */
#endif
   strlcat(file_name, "SHM_HOST", file_name_size);
   strlcat(file_name, ".TXT", file_name_size);

   fp = fopen(file_name, "r");
   if (!fp) {
      fp = fopen(file_name, "w");
      if (!fp)
         cm_msg(MERROR, "check_shm_host", "Cannot write to \'%s\', errno %d (%s)", file_name, errno, strerror(errno));
      assert(fp != NULL);
      fprintf(fp, "%s\n", hostname);
      fclose(fp);
      return;
   }

   buf[0] = 0;

   if (!fgets(buf, sizeof(buf), fp))
      buf[0] = 0;

   fclose(fp);

   s = strchr(buf, '\n');
   if (s)
      *s = 0;

   if (strlen(buf) < 1)
      return; // success - provide user with a way to defeat this check

   if (strcmp(buf, hostname) == 0)
      return; // success!

   cm_msg(MERROR, "check_shm_host", "Error: Cannot connect to MIDAS shared memory - this computer hostname is \'%s\' while \'%s\' says that MIDAS shared memory for this experiment is located on computer \'%s\'. To connect to this experiment from this computer, use the mserver. Please see the MIDAS documentation for details.", hostname, file_name, buf);
   exit(1);
}

static int ss_shm_name(const char* name, std::string& mem_name, std::string& file_name, std::string& shm_name)
{
   check_shm_host();
#if defined(OS_DARWIN)
   check_shm_type("POSIXv3_SHM"); // uid + expt name + shm name
#elif defined(OS_UNIX)
   check_shm_type("POSIXv4_SHM"); // uid + expt name + shm name + expt directory
#endif

   mem_name = std::string("SM_") + name;

   /* append .SHM and preceed the path for the shared memory file name */

   std::string exptname = cm_get_experiment_name();
   std::string path = cm_get_path();

   //printf("shm name [%s], expt name [%s], path [%s]\n", name, exptname.c_str(), path.c_str());

   assert(path.length() > 0);
   assert(exptname.length() > 0);

   file_name = path;
#if defined (OS_UNIX)
   file_name += "."; /* dot file under UNIX */
#endif
   file_name += name;
   file_name += ".SHM";

#if defined(OS_UNIX)
   shm_name = "/";
   if (use_posix1_shm) {
      shm_name += file_name;
   } else if (use_posix2_shm) {
      shm_name += exptname;
      shm_name += "_";
      shm_name += name;
      shm_name += "_SHM";
   } else if (use_posix3_shm) {
      uid_t uid = getuid();
      char buf[16];
      sprintf(buf, "%d", uid);
      shm_name += buf;
      shm_name += "_";
      shm_name += exptname;
      shm_name += "_";
      shm_name += name;
   } else if (use_posix4_shm) {
      uid_t uid = getuid();
      char buf[16];
      sprintf(buf, "%d", uid);
      shm_name += buf;
      shm_name += "_";
      shm_name += exptname;
      shm_name += "_";
      shm_name += name;
      shm_name += "_";
      shm_name += cm_get_path();
   } else {
      fprintf(stderr, "check_shm_host: unsupported shared memory type, bye!\n");
      abort();
   }

   for (size_t i=1; i<shm_name.length(); i++)
      if (shm_name[i] == '/')
         shm_name[i] = '_';

   //printf("ss_shm_name: [%s] generated [%s]\n", name, shm_name.c_str());
#endif

   return SS_SUCCESS;
}

#if defined OS_UNIX
static int ss_shm_file_name_to_shmid(const char* file_name, int* shmid)
{
   int key, status;

   /* create a unique key from the file name */
   key = ftok(file_name, 'M');

   /* if file doesn't exist ... */
   if (key == -1)
      return SS_NO_MEMORY;

   status = shmget(key, 0, 0);
   if (status == -1)
      return SS_NO_MEMORY;

   (*shmid) = status;
   return SS_SUCCESS;
}
#endif

/*------------------------------------------------------------------*/
INT ss_shm_open(const char *name, INT size, void **adr, size_t *shm_size, HNDLE * handle, BOOL get_size)
/********************************************************************\

  Routine: ss_shm_open

  Purpose: Create a shared memory region which can be seen by several
     processes which know the name.

  Input:
    char *name              Name of the shared memory
    INT  size               Initial size of the shared memory in bytes
                            if .SHM file doesn't exist
    BOOL get_size           If TRUE and shared memory already exists, overwrite
                            "size" parameter with existing memory size

  Output:
    void  *adr              Address of opened shared memory
    HNDLE handle            Handle or key to the shared memory
    size_t shm_size         Size of shared memory to use with ss_shm_close() & co

  Function value:
    SS_SUCCESS              Successful completion
    SS_CREATED              Shared memory was created
    SS_FILE_ERROR           Paging file cannot be created
    SS_NO_MEMORY            Not enough memory
    SS_SIZE_MISMATCH        "size" differs from existing size and
                            get_size is FALSE
\********************************************************************/
{
   INT status;
   std::string mem_name;
   std::string file_name;
   std::string shm_name;

   ss_shm_name(name, mem_name, file_name, shm_name);

   if (shm_trace)
      printf("ss_shm_open(\"%s\",%d,%d), mem_name [%s], file_name [%s], shm_name [%s]\n", name, size, get_size, mem_name.c_str(), file_name.c_str(), shm_name.c_str());

#ifdef OS_WINNT

   status = SS_SUCCESS;

   {
      HANDLE hFile, hMap;
      char str[256], path[256], *p;
      DWORD file_size;

      /* make the memory name unique using the pathname. This is necessary
         because NT doesn't use ftok. So if different experiments are
         running in different directories, they should not see the same
         shared memory */
      cm_get_path(path, sizeof(path));
      strlcpy(str, path, sizeof(path));

      /* replace special chars by '*' */
      while (strpbrk(str, "\\: "))
         *strpbrk(str, "\\: ") = '*';
      strlcat(str, mem_name, sizeof(path));

      /* convert to uppercase */
      p = str;
      while (*p)
         *p++ = (char) toupper(*p);

      hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, str);
      if (hMap == 0) {
         hFile = CreateFile(file_name.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
         if (!hFile) {
            cm_msg(MERROR, "ss_shm_open", "CreateFile() failed");
            return SS_FILE_ERROR;
         }

         file_size = GetFileSize(hFile, NULL);
         if (get_size) {
            if (file_size != 0xFFFFFFFF && file_size > 0)
               size = file_size;
         } else {
            if (file_size != 0xFFFFFFFF && file_size > 0 && file_size != size) {
               cm_msg(MERROR, "ss_shm_open", "Requested size (%d) differs from existing size (%d)", size, file_size);
               return SS_SIZE_MISMATCH;
            }
         }

         hMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, size, str);

         if (!hMap) {
            status = GetLastError();
            cm_msg(MERROR, "ss_shm_open", "CreateFileMapping() failed, error %d", status);
            return SS_FILE_ERROR;
         }

         CloseHandle(hFile);
         status = SS_CREATED;
      }

      *adr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
      *handle = (HNDLE) hMap;
      *shm_size = size;

      if (adr == NULL) {
         cm_msg(MERROR, "ss_shm_open", "MapViewOfFile() failed");
         return SS_NO_MEMORY;
      }

      return status;
   }

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   status = SS_SUCCESS;

   {
      int addr[2];
      $DESCRIPTOR(memname_dsc, "dummy");
      $DESCRIPTOR(filename_dsc, "dummy");
      memname_dsc.dsc$w_length = strlen(mem_name);
      memname_dsc.dsc$a_pointer = mem_name;
      filename_dsc.dsc$w_length = file_name.length();
      filename_dsc.dsc$a_pointer = file_name.c_str();

      addr[0] = size;
      addr[1] = 0;

      status = ppl$create_shared_memory(&memname_dsc, addr, &PPL$M_NOUNI, &filename_dsc);

      if (status == PPL$_CREATED)
         status = SS_CREATED;
      else if (status != PPL$_NORMAL)
         status = SS_FILE_ERROR;

      *adr = (void *) addr[1];
      *handle = 0;              /* not used under VMS */
      *shm_size = addr[0];

      if (adr == NULL)
         return SS_NO_MEMORY;

      return status;
   }

#endif                          /* OS_VMS */
#ifdef OS_UNIX

   if (use_sysv_shm) {

      int key, shmid, fh;
      double file_size = 0;
      struct shmid_ds buf;

      status = SS_SUCCESS;

      /* create a unique key from the file name */
      key = ftok(file_name.c_str(), 'M');

      /* if file doesn't exist, create it */
      if (key == -1) {
         fh = open(file_name.c_str(), O_CREAT | O_TRUNC | O_BINARY | O_RDWR, 0644);
         if (fh > 0) {
            close(fh);
         }
         key = ftok(file_name.c_str(), 'M');

         if (key == -1) {
            cm_msg(MERROR, "ss_shm_open", "ftok() failed");
            return SS_FILE_ERROR;
         }

         status = SS_CREATED;

         /* delete any previously created memory */

         shmid = shmget(key, 0, 0);
         shmctl(shmid, IPC_RMID, &buf);
      } else {
         /* if file exists, retrieve its size */
         file_size = ss_file_size(file_name.c_str());
         if (file_size > 0) {
            if (get_size) {
               size = file_size;
            } else if (size != file_size) {
               cm_msg(MERROR, "ss_shm_open", "Existing file \'%s\' has size %.0f, different from requested size %d", file_name.c_str(), file_size, size);
               return SS_SIZE_MISMATCH;
            }
         }
      }

      if (shm_trace)
         printf("ss_shm_open(\"%s\",%d) get_size %d, file_name %s, size %.0f\n", name, size, get_size, file_name.c_str(), file_size);

      /* get the shared memory, create if not existing */
      shmid = shmget(key, size, 0);
      if (shmid == -1) {
         //cm_msg(MINFO, "ss_shm_open", "Creating shared memory segment, key: 0x%x, size: %d",key,size);
         shmid = shmget(key, size, IPC_CREAT | IPC_EXCL);
         if (shmid == -1 && errno == EEXIST) {
            cm_msg(MERROR, "ss_shm_open",
                   "Shared memory segment with key 0x%x already exists, please remove it manually: ipcrm -M 0x%x",
                   key, key);
            return SS_NO_MEMORY;
         }
         status = SS_CREATED;
      }

      if (shmid == -1) {
         cm_msg(MERROR, "ss_shm_open", "shmget(key=0x%x,size=%d) failed, errno %d (%s)", key, size, errno, strerror(errno));
         return SS_NO_MEMORY;
      }

      memset(&buf, 0, sizeof(buf));
      buf.shm_perm.uid = getuid();
      buf.shm_perm.gid = getgid();
      buf.shm_perm.mode = 0666;
      shmctl(shmid, IPC_SET, &buf);

      *adr = shmat(shmid, 0, 0);

      if ((*adr) == (void *) (-1)) {
         cm_msg(MERROR, "ss_shm_open", "shmat(shmid=%d) failed, errno %d (%s)", shmid, errno, strerror(errno));
         return SS_NO_MEMORY;
      }

      *handle = (HNDLE) shmid;
      *shm_size = size;

      /* if shared memory was created, try to load it from file */
      if (status == SS_CREATED && file_size > 0) {
         fh = open(file_name.c_str(), O_RDONLY, 0644);
         if (fh == -1)
            fh = open(file_name.c_str(), O_CREAT | O_RDWR, 0644);
         else {
            int rd = read(fh, *adr, size);
            if (rd != size)
               cm_msg(MERROR, "ss_shm_open", "File size mismatch shared memory \'%s\' size %d, file \'%s\' read %d, errno %d (%s)", name, size, file_name.c_str(), rd, errno, strerror(errno));
         }
         close(fh);
      }

      return status;
   }

   if (use_mmap_shm) {

      int ret;
      int fh, file_size;

      if (1) {
         static int once = 1;
         if (once && strstr(file_name.c_str(), "ODB")) {
            once = 0;
            cm_msg(MINFO, "ss_shm_open", "WARNING: This version of MIDAS system.c uses the experimental mmap() based implementation of MIDAS shared memory.");
         }
      }

      if (shm_trace)
         printf("ss_shm_open(\"%s\",%d) get_size %d, file_name %s\n", name, size, get_size, file_name.c_str());

      status = SS_SUCCESS;

      fh = open(file_name.c_str(), O_RDWR | O_BINARY | O_LARGEFILE, 0644);

      if (fh < 0) {
         if (errno == ENOENT) { // file does not exist
            fh = open(file_name.c_str(), O_CREAT | O_RDWR | O_BINARY | O_LARGEFILE, 0644);
         }

         if (fh < 0) {
            cm_msg(MERROR, "ss_shm_open", "Cannot create shared memory file \'%s\', errno %d (%s)", file_name.c_str(), errno, strerror(errno));
            return SS_FILE_ERROR;
         }

         ret = lseek(fh, size - 1, SEEK_SET);

         if (ret == (off_t) - 1) {
            cm_msg(MERROR, "ss_shm_open",
                   "Cannot create shared memory file \'%s\', size %d, lseek() errno %d (%s)",
                   file_name.c_str(), size, errno, strerror(errno));
            return SS_FILE_ERROR;
         }

         ret = 0;
         ret = write(fh, &ret, 1);
         assert(ret == 1);

         ret = lseek(fh, 0, SEEK_SET);
         assert(ret == 0);

         //cm_msg(MINFO, "ss_shm_open", "Created shared memory file \'%s\', size %d", file_name.c_str(), size);

         status = SS_CREATED;
      }

      /* if file exists, retrieve its size */
      file_size = (INT) ss_file_size(file_name.c_str());
      if (file_size < size) {
         cm_msg(MERROR, "ss_shm_open",
                "Shared memory file \'%s\' size %d is smaller than requested size %d. Please remove it and try again",
                file_name.c_str(), file_size, size);
         return SS_NO_MEMORY;
      }

      size = file_size;

      *adr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fh, 0);

      if ((*adr) == MAP_FAILED) {
         cm_msg(MERROR, "ss_shm_open", "mmap() failed, errno %d (%s)", errno, strerror(errno));
         return SS_NO_MEMORY;
      }

      *handle = ++shm_count;
      *shm_size = size;

      return status;
   }

   if (use_posix_shm) {

      int sh;
      int fh;
      int created = 0;
      double file_size = -1;

      fh = open(file_name.c_str(), O_RDONLY | O_BINARY | O_LARGEFILE, 0777);

      if (fh >= 0) {
         file_size = ss_file_size(file_name.c_str());
      }

      if (shm_trace)
         printf("ss_shm_open(\"%s\",%d) get_size %d, file_name %s, size %.0f\n", name, size, get_size, file_name.c_str(), file_size);

      if (file_size > 0) {
         if (get_size)
            size = file_size;

         if (file_size != size) {
            cm_msg(MERROR, "ss_shm_open", "Shared memory file \'%s\' size %.0f is different from requested size %d. Please backup and remove this file and try again", file_name.c_str(), file_size, size);
            if (fh >= 0)
               close(fh);
            return SS_NO_MEMORY;
         }
      }

      int mode = 0600; // 0777: full access for everybody (minus umask!), 0600: current user: read+write, others: no permission

      sh = shm_open(shm_name.c_str(), O_RDWR, mode);

      if (sh < 0) {
         // cannot open, try to create new one

         sh = shm_open(shm_name.c_str(), O_RDWR | O_CREAT, mode);

         //printf("ss_shm_open: name [%s], return %d, errno %d (%s)\n", shm_name, sh, errno, strerror(errno));

         if (sh < 0) {
#ifdef ENAMETOOLONG
            if (errno == ENAMETOOLONG) {
               fprintf(stderr, "ss_shm_open: Cannot create shared memory for \"%s\": shared memory object name \"%s\" is too long for shm_open(), please try to use shorter experiment name or shorter event buffer name or a shared memory type that uses shorter names, in this order: POSIXv3_SHM, POSIXv2_SHM or POSIX_SHM (as specified in config file .SHM_TYPE.TXT). Sorry, bye!\n", name, shm_name.c_str());
               exit(1);
            }
#endif
#ifdef EACCES
            if (errno == EACCES) {
               fprintf(stderr, "ss_shm_open: Cannot create shared memory for \"%s\" with shared memory object name \"%s\", shm_open() errno %d (%s), please inspect file permissions in \"ls -l /dev/shm\", and if this is a conflict with a different user using the same experiment name, please change shared memory type to the POSIXv4_SHM or POSIXv3_SHM (on MacOS) (as specified in config file .SHM_TYPE.TXT). Sorry, bye!\n", name, shm_name.c_str(), errno, strerror(errno));
               exit(1);
            }
#endif
            cm_msg(MERROR, "ss_shm_open", "Cannot create shared memory segment \'%s\', shm_open() errno %d (%s)", shm_name.c_str(), errno, strerror(errno));
            if (fh >= 0)
               close(fh);
            return SS_NO_MEMORY;
         }

         status = ftruncate(sh, size);
         if (status < 0) {
            cm_msg(MERROR, "ss_shm_open", "Cannot resize shared memory segment \'%s\', ftruncate(%d) errno %d (%s)", shm_name.c_str(), size, errno, strerror(errno));
            if (fh >= 0)
               close(fh);
            return SS_NO_MEMORY;
         }

         //cm_msg(MINFO, "ss_shm_open", "Created shared memory segment \'%s\', size %d", shm_name.c_str(), size);

         created = 1;
      }

      *adr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, sh, 0);

      if ((*adr) == MAP_FAILED) {
         cm_msg(MERROR, "ss_shm_open", "Cannot mmap() shared memory \'%s\', errno %d (%s)", shm_name.c_str(), errno, strerror(errno));
         close(sh);
         if (fh >= 0)
            close(fh);
         return SS_NO_MEMORY;
      }

      close(sh);

      /* if shared memory was created, try to load it from file */
      
      if (created && fh >= 0 && file_size > 0) {
         if (shm_trace)
            printf("ss_shm_open(\"%s\"), loading contents of file [%s], size %.0f\n", name, file_name.c_str(), file_size);

         status = read(fh, *adr, size);
         if (status != size) {
            cm_msg(MERROR, "ss_shm_open", "Cannot read \'%s\', read() returned %d instead of %d, errno %d (%s)", file_name.c_str(), status, size, errno, strerror(errno));
            close(fh);
            return SS_NO_MEMORY;
         }
      }
      
      close(fh);

      *handle = ++shm_count;
      *shm_size = size;

      if (created)
         return SS_CREATED;
      else
         return SS_SUCCESS;
   }

#endif /* OS_UNIX */

   return SS_FILE_ERROR;
}

/*------------------------------------------------------------------*/
INT ss_shm_close(const char *name, void *adr, size_t shm_size, HNDLE handle, INT destroy_flag)
/********************************************************************\

  Routine: ss_shm_close

  Purpose: Close a shared memory region.

  Input:
    char *name              Name of the shared memory
    void *adr               Base address of shared memory
    size_t shm_size         Size of shared memory shm_size returned by ss_shm_open()
    HNDLE handle            Handle of shared memeory
    BOOL destroy            Shared memory has to be destroyd and
          flushed to the mapping file.

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_INVALID_ADDRESS      Invalid base address
    SS_FILE_ERROR           Cannot write shared memory file
    SS_INVALID_HANDLE       Invalid shared memory handle

\********************************************************************/
{
   char mem_name[256], file_name[256], path[256];

   /*
      append a leading SM_ to the memory name to resolve name conflicts
      with mutex or semaphore names
    */
   sprintf(mem_name, "SM_%s", name);

   /* append .SHM and preceed the path for the shared memory file name */
   cm_get_path(path, sizeof(path));
   if (path[0] == 0) {
      if (!getcwd(path, sizeof(path)))
         path[0] = 0;
#if defined(OS_VMS)
#elif defined(OS_UNIX)
      strlcat(path, "/", sizeof(path));
#elif defined(OS_WINNT)
      strlcat(path, "\\", sizeof(path));
#endif
   }

   strlcpy(file_name, path, sizeof(file_name));
#if defined (OS_UNIX)
   strlcat(file_name, ".", sizeof(file_name));      /* dot file under UNIX */
#endif
   strlcat(file_name, name, sizeof(file_name));
   strlcat(file_name, ".SHM", sizeof(file_name));

   if (shm_trace)
      printf("ss_shm_close(\"%s\",%p,%.0f,%d,destroy_flag=%d), file_name [%s]\n", name, adr, (double)shm_size, handle, destroy_flag, file_name);

#ifdef OS_WINNT

   if (!UnmapViewOfFile(adr))
      return SS_INVALID_ADDRESS;

   CloseHandle((HANDLE) handle);

   return SS_SUCCESS;

#endif                          /* OS_WINNT */
#ifdef OS_VMS
/* outcommented because ppl$delete... makes privilege violation
  {
  int addr[2], flags, status;
  char mem_name[100];
  $DESCRIPTOR(memname_dsc, mem_name);

  strcpy(mem_name, "SM_");
  strcat(mem_name, name);
  memname_dsc.dsc$w_length = strlen(mem_name);

  flags = PPL$M_FLUSH | PPL$M_NOUNI;

  addr[0] = 0;
  addr[1] = adr;

  status = ppl$delete_shared_memory( &memname_dsc, addr, &flags);

  if (status == PPL$_NORMAL)
    return SS_SUCCESS;

  return SS_INVALID_ADDRESS;
  }
*/
   return SS_INVALID_ADDRESS;

#endif                          /* OS_VMS */
#ifdef OS_UNIX

   if (use_sysv_shm) {

      struct shmid_ds buf;

      /* get info about shared memory */
      memset(&buf, 0, sizeof(buf));
      if (shmctl(handle, IPC_STAT, &buf) < 0) {
         cm_msg(MERROR, "ss_shm_close", "shmctl(shmid=%d,IPC_STAT) failed, errno %d (%s)",
                handle, errno, strerror(errno));
         return SS_INVALID_HANDLE;
      }

      destroy_flag = (buf.shm_nattch == 1);

      if (shm_trace)
         printf("ss_shm_close(\"%s\"), destroy_flag %d, shmid %d, shm_nattach %d\n", name, destroy_flag, handle, (int)buf.shm_nattch);

      if (shmdt(adr) < 0) {
         cm_msg(MERROR, "ss_shm_close", "shmdt(shmid=%d) failed, errno %d (%s)", handle, errno, strerror(errno));
         return SS_INVALID_ADDRESS;
      }

      if (destroy_flag) {
         int status = ss_shm_delete(name);
         if (status != SS_SUCCESS)
            return status;
      }

      return SS_SUCCESS;
   }

   if (use_mmap_shm || use_posix_shm) {
      int status;

      if (shm_trace)
         printf("ss_shm_close(\"%s\"), destroy_flag %d\n", name, destroy_flag);

      status = munmap(adr, shm_size);
      if (status != 0) {
         cm_msg(MERROR, "ss_shm_close", "Cannot unmap shared memory \'%s\', munmap() errno %d (%s)", name, errno, strerror(errno));
         return SS_INVALID_ADDRESS;
      }

      if (destroy_flag) {
         status = ss_shm_delete(name);
         if (status != SS_SUCCESS)
            return status;
      }

      return SS_SUCCESS;
   }
#endif /* OS_UNIX */

   return SS_FILE_ERROR;
}

/*------------------------------------------------------------------*/
INT ss_shm_delete(const char *name)
/********************************************************************\

  Routine: ss_shm_delete

  Purpose: Delete shared memory segment from memory.

  Input:
    char *name              Name of the shared memory

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_MEMORY            Shared memory segment does not exist

\********************************************************************/
{
   int status;
   std::string mem_name;
   std::string file_name;
   std::string shm_name;

   status = ss_shm_name(name, mem_name, file_name, shm_name);

   if (shm_trace)
      printf("ss_shm_delete(\"%s\") file_name [%s] shm_name [%s]\n", name, file_name.c_str(), shm_name.c_str());

#ifdef OS_WINNT
   /* no shared memory segments to delete */
   return SS_SUCCESS;
#endif                          /* OS_WINNT */

#ifdef OS_VMS
   assert(!"not implemented!");
   return SS_NO_MEMORY;
#endif                          /* OS_VMS */

#ifdef OS_UNIX

   if (use_sysv_shm) {
      int shmid = -1;
      struct shmid_ds buf;

      status = ss_shm_file_name_to_shmid(file_name.c_str(), &shmid);

      if (shm_trace)
         printf("ss_shm_delete(\"%s\") file_name %s, shmid %d\n", name, file_name.c_str(), shmid);

      if (status != SS_SUCCESS)
         return status;

      status = shmctl(shmid, IPC_RMID, &buf);

      if (status == -1) {
         cm_msg(MERROR, "ss_shm_delete", "Cannot delete shared memory \'%s\', shmctl(IPC_RMID) failed, errno %d (%s)", name, errno, strerror(errno));
         return SS_FILE_ERROR;
      }

      return SS_SUCCESS;
   }

   if (use_mmap_shm) {
      /* no shared memory segments to delete */

      if (shm_trace)
         printf("ss_shm_delete(\"%s\") file_name %s (no-op)\n", name, file_name.c_str());

      return SS_SUCCESS;
   }

   if (use_posix_shm) {

      if (shm_trace)
         printf("ss_shm_delete(\"%s\") shm_name %s\n", name, shm_name.c_str());

      status = shm_unlink(shm_name.c_str());
      if (status < 0) {
         cm_msg(MERROR, "ss_shm_delete", "shm_unlink(%s) errno %d (%s)", shm_name.c_str(), errno, strerror(errno));
         return SS_NO_MEMORY;
      }

      return SS_SUCCESS;
   }

#endif /* OS_UNIX */

   return SS_FILE_ERROR;
}

/*------------------------------------------------------------------*/
INT ss_shm_protect(HNDLE handle, void *adr, size_t shm_size)
/********************************************************************\

  Routine: ss_shm_protect

  Purpose: Protect a shared memory region, disallow read and write
           access to it by this process

  Input:
    HNDLE handle            Handle of shared memeory
    void  *adr              Address of shared memory
    size_t shm_size         Size of shared memory

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_INVALID_ADDRESS      Invalid base address

\********************************************************************/
{
   if (shm_trace)
      printf("ss_shm_protect()   handle %d, adr %p, size %.0f\n", handle, adr, (double)shm_size);

#ifdef OS_WINNT

   if (!UnmapViewOfFile(adr))
      return SS_INVALID_ADDRESS;

#endif                          /* OS_WINNT */
#ifdef OS_UNIX

   if (use_sysv_shm) {

      if (shmdt(adr) < 0) {
         cm_msg(MERROR, "ss_shm_protect", "shmdt() failed");
         return SS_INVALID_ADDRESS;
      }
   }

   if (use_mmap_shm || use_posix_shm) {
      assert(shm_size > 0);

      int ret = mprotect(adr, shm_size, PROT_NONE);
      if (ret != 0) {
         cm_msg(MERROR, "ss_shm_protect", "Cannot mprotect(PROT_NONE): return value %d, errno %d (%s)", ret, errno, strerror(errno));
         return SS_INVALID_ADDRESS;
      }
   }

#endif // OS_UNIX

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_shm_unprotect(HNDLE handle, void **adr, size_t shm_size, BOOL read, BOOL write, const char* caller_name)
/********************************************************************\

  Routine: ss_shm_unprotect

  Purpose: Unprotect a shared memory region so that it can be accessed
           by this process

  Input:
    HNDLE handle            Handle or key to the shared memory, must
                            be obtained with ss_shm_open
    size_t shm_size         Size of shared memory shm_size returned by ss_shm_open()

  Output:
    void  *adr              Address of opened shared memory

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_MEMORY            Memory mapping failed

\********************************************************************/
{
   if (shm_trace)
      printf("ss_shm_unprotect() handle %d, adr %p, size %.0f, read %d, write %d, caller %s\n", handle, *adr, (double)shm_size, read, write, caller_name);
   
#ifdef OS_WINNT

   *adr = MapViewOfFile((HANDLE) handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

   if (*adr == NULL) {
      cm_msg(MERROR, "ss_shm_unprotect", "MapViewOfFile() failed");
      return SS_NO_MEMORY;
   }
#endif                          /* OS_WINNT */
#ifdef OS_UNIX

   if (use_sysv_shm) {

      *adr = shmat(handle, 0, 0);

      if ((*adr) == (void *) (-1)) {
         cm_msg(MERROR, "ss_shm_unprotect", "shmat() failed, errno = %d", errno);
         return SS_NO_MEMORY;
      }
   }

   if (use_mmap_shm || use_posix_shm) {
      assert(shm_size > 0);
      
      int mode = 0;
      if (read)
         mode |= PROT_READ;
      if (write)
         mode |= PROT_READ | PROT_WRITE;

      int ret = mprotect(*adr, shm_size, mode);
      if (ret != 0) {
         cm_msg(MERROR, "ss_shm_unprotect", "Cannot mprotect(%d): return value %d, errno %d (%s)", mode, ret, errno, strerror(errno));
         return SS_INVALID_ADDRESS;
      }
   }

#endif // OS_UNIX

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

typedef struct {
   std::string file_name;
   int fd;
   void *buf;
   int size;
} FL_PARAM;

INT ss_shm_flush_thread(void *p)
{
   FL_PARAM *param = (FL_PARAM *)p;

   //fprintf(stderr, "flush start!\n");

   uint32_t start = ss_time();

   /* write shared memory to file */
   ssize_t wr = write(param->fd, param->buf, param->size);
   if ((size_t)wr != (size_t)param->size) {
      cm_msg(MERROR, "ss_shm_flush", "Cannot write to file \'%s\', write() returned %d instead of %d, errno %d (%s)",
             param->file_name.c_str(), (int)wr, (int)param->size, errno, strerror(errno));
      close(param->fd);
      free(param->buf);
      param->buf = nullptr;
      return -1;
   }

   int ret = close(param->fd);
   if (ret < 0) {
      cm_msg(MERROR, "ss_shm_flush", "Cannot write to file \'%s\', close() errno %d (%s)",
             param->file_name.c_str(), errno, strerror(errno));
      free(param->buf);
      param->buf = nullptr;
      return -1;
   }

   free(param->buf);
   param->buf = nullptr;

   if (ss_time() - start > 4)
      cm_msg(MINFO, "ss_shm_flush", "Flushing shared memory took %d seconds", ss_time() - start);

   //fprintf(stderr, "flush end!\n");

   return 0;
}


INT ss_shm_flush(const char *name, const void *adr, size_t size, HNDLE handle, bool wait_for_thread)
/********************************************************************\

  Routine: ss_shm_flush

  Purpose: Flush a shared memory region to its disk file.

  Input:
    char *name              Name of the shared memory
    void *adr               Base address of shared memory
    INT  size               Size of shared memeory
    HNDLE handle            Handle of shared memory

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_INVALID_ADDRESS      Invalid base address

\********************************************************************/
{
   std::string mem_name;
   std::string file_name;
   std::string shm_name;

   ss_shm_name(name, mem_name, file_name, shm_name);

   if (shm_trace)
      printf("ss_shm_flush(\"%s\",%p,%.0f,%d), file_name [%s]\n", name, adr, (double)size, handle, file_name.c_str());

#ifdef OS_WINNT

   if (!FlushViewOfFile(adr, size))
      return SS_INVALID_ADDRESS;

   return SS_SUCCESS;

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   return SS_SUCCESS;

#endif                          /* OS_VMS */
#ifdef OS_UNIX

   if (use_sysv_shm || use_posix_shm) {

      assert(size > 0);

      int fd = open(file_name.c_str(), O_RDWR | O_CREAT, 0777);
      if (fd < 0) {
         cm_msg(MERROR, "ss_shm_flush", "Cannot write to file \'%s\', fopen() errno %d (%s)", file_name.c_str(), errno, strerror(errno));
         return SS_NO_MEMORY;
      }

      /* try to make a copy of the shared memory */
      void *buffer = malloc(size);
      if (buffer != nullptr) {
         memcpy(buffer, adr, size);
         static std::thread* thread = NULL; // THIS IS NOT THREAD SAFE!
         if (thread) { // reap the long finished thread from the previous flush
            thread->join();
            delete thread;
            thread = NULL;
         }
         static FL_PARAM param; // this is safe, thread is no longer running. K.O.
         param.file_name = file_name;
         param.fd = fd;
         param.buf = buffer;
         param.size = size;

         thread = new std::thread(ss_shm_flush_thread, &param);

         if (wait_for_thread) {
            //fprintf(stderr, "waiting for flush thread!\n");
            thread->join();
            delete thread;
            thread = NULL;
            //fprintf(stderr, "thread joined!\n");
         }

         // buffer gets freed in ss_shm_flush_thread, so we don't have to free() it here...
      } else {

         /* not enough memory for ODB copy buffer, so write directly */
         uint32_t start = ss_time();
         ssize_t wr = write(fd, adr, size);
         if ((size_t)wr != size) {
            cm_msg(MERROR, "ss_shm_flush", "Cannot write to file \'%s\', write() returned %d instead of %d, errno %d (%s)", file_name.c_str(), (int)wr, (int)size, errno, strerror(errno));
            close(fd);
            return SS_NO_MEMORY;
         }

         int ret = close(fd);
         if (ret < 0) {
            cm_msg(MERROR, "ss_shm_flush", "Cannot write to file \'%s\', close() errno %d (%s)",
                   file_name.c_str(), errno, strerror(errno));
            return SS_NO_MEMORY;
         }

         if (ss_time() - start > 4)
            cm_msg(MINFO, "ss_shm_flush", "Flushing shared memory took %d seconds", ss_time() - start);

      }

      return SS_SUCCESS;
   }

   if (use_mmap_shm) {

      assert(size > 0);

      if (shm_trace)
         printf("ss_shm_flush(\"%s\") size %.0f, mmap file_name [%s]\n", name, (double)size, file_name.c_str());

      int ret = msync((void *)adr, size, MS_ASYNC);
      if (ret != 0) {
         cm_msg(MERROR, "ss_shm_flush", "Cannot msync(MS_ASYNC): return value %d, errno %d (%s)", ret, errno, strerror(errno));
         return SS_INVALID_ADDRESS;
      }
      return SS_SUCCESS;
   }


#endif // OS_UNIX

   return SS_SUCCESS;
}

#endif                          /* LOCAL_ROUTINES */

/*------------------------------------------------------------------*/
static struct {
   char c;
   double d;
} test_align;

static struct {
   double d;
   char c;
} test_padding;

INT ss_get_struct_align()
/********************************************************************\

  Routine: ss_get_struct_align

  Purpose: Returns compiler alignment of structures. In C, structures
     can be byte aligned, word or even quadword aligned. This
     can usually be set with compiler switches. This routine
     tests this alignment during runtime and returns 1 for
     byte alignment, 2 for word alignment, 4 for dword alignment
     and 8 for quadword alignment.

  Input:
    <none>

  Output:
    <none>

  Function value:
    INT    Structure alignment

\********************************************************************/
{
   return (POINTER_T) (&test_align.d) - (POINTER_T) & test_align.c;
}

INT ss_get_struct_padding()
/********************************************************************\

 Routine: ss_get_struct_padding

 Purpose: Returns compiler padding of structures. Under some C
    compilers and architectures, C structures can be padded at the
    end to have a size of muliples of 4 or 8. This routine returns
    this number, like 8 if all structures are padded with 0-7 bytes
    to lie on an 8 byte boundary.

 Input:
 <none>

 Output:
 <none>

 Function value:
 INT    Structure alignment

 \********************************************************************/
{
   return (INT) sizeof(test_padding) - 8;
}

/********************************************************************\
*                                                                    *
*                  Process functions                                 *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/
INT ss_getpid(void)
/********************************************************************\

  Routine: ss_getpid

  Purpose: Return process ID of current process

  Input:
    none

  Output:
    none

  Function value:
    INT              Process ID

\********************************************************************/
{
#ifdef OS_WINNT

   return (int) GetCurrentProcessId();

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   return getpid();

#endif                          /* OS_VMS */
#ifdef OS_UNIX

   return getpid();

#endif                          /* OS_UNIX */
#ifdef OS_VXWORKS

   return 0;

#endif                          /* OS_VXWORKS */
#ifdef OS_MSDOS

   return 0;

#endif                          /* OS_MSDOS */
}

#ifdef LOCAL_ROUTINES

/********************************************************************   \

  Routine: ss_pid_exists

  Purpose: Check if given pid still exists

  Input:
    pid - process id returned by ss_getpid()

  Output:
    none

  Function value:
    BOOL              TRUE or FALSE

\********************************************************************/
BOOL ss_pid_exists(int pid)
{
#ifdef ESRCH
   /* Only enable this for systems that define ESRCH and hope that they also support kill(pid,0) */
   int status = kill(pid, 0);
   //printf("kill(%d,0) returned %d, errno %d\n", pid, status, errno);
   if ((status != 0) && (errno == ESRCH)) {
      return FALSE;
   }
#else
#warning Missing ESRCH for ss_pid_exists()
#endif
   return TRUE;
}

/********************************************************************\

  Routine: ss_kill

  Purpose: Kill given process, ensure it is not running anymore

  Input:
    pid - process id returned by ss_getpid()

  Output:
    none

  Function value:
    void - none

\********************************************************************/
void ss_kill(int pid)
{
#ifdef SIGKILL
   kill(pid, SIGKILL);
#else
#warning Missing SIGKILL for ss_kill()
#endif
}

#endif // LOCAL_ROUTINES

/*------------------------------------------------------------------*/

midas_thread_t ss_gettid(void)
/********************************************************************\

  Routine: ss_gettid

  Purpose: Return thread ID of current thread

  Input:
    none

  Output:
    none

  Function value:
    INT              thread ID

\********************************************************************/
{
#if defined OS_MSDOS

   return 0;

#elif defined OS_WINNT

   return GetCurrentThreadId();

#elif defined OS_VMS

   return ss_getpid();

#elif defined OS_DARWIN

   return pthread_self();

#elif defined OS_CYGWIN

   return pthread_self();

#elif defined OS_UNIX

   return pthread_self();
   //return syscall(SYS_gettid);

#elif defined OS_VXWORKS

   return ss_getpid();

#else
#error Do not know how to do ss_gettid()
#endif
}

std::string ss_tid_to_string(midas_thread_t thread_id)
{
#if defined OS_MSDOS

   return "0";

#elif defined OS_WINNT

#error Do not know how to do ss_tid_to_string()
   return "???";

#elif defined OS_VMS

   char buf[256];
   sprintf(buf, "%d", thread_id);
   return buf;

#elif defined OS_DARWIN

   char buf[256];
   sprintf(buf, "%p", thread_id);
   return buf;

#elif defined OS_CYGWIN

   char buf[256];
   sprintf(buf, "%p", thread_id);
   return buf;

#elif defined OS_UNIX

   char buf[256];
   sprintf(buf, "%lu", thread_id);
   return buf;

#elif defined OS_VXWORKS

   char buf[256];
   sprintf(buf, "%d", thread_id);
   return buf;

#else
#error Do not know how to do ss_tid_to_string()
#endif
}

/*------------------------------------------------------------------*/

#ifdef OS_UNIX
void catch_sigchld(int signo)
{
   int status;

   status = signo;              /* avoid compiler warning */
   wait(&status);
   return;
}
#endif

INT ss_spawnv(INT mode, const char *cmdname, const char* const argv[])
/********************************************************************\

  Routine: ss_spawnv

  Purpose: Spawn a subprocess or detached process

  Input:
    INT mode         One of the following modes:
           P_WAIT     Wait for the subprocess to compl.
           P_NOWAIT   Don't wait for subprocess to compl.
           P_DETACH   Create detached process.
    char cmdname     Program name to execute
    char *argv[]     Optional program arguments

  Output:
    none

  Function value:
    SS_SUCCESS       Successful completeion
    SS_INVALID_NAME  Command could not be executed;

\********************************************************************/
{
#ifdef OS_WINNT

   if (spawnvp(mode, cmdname, argv) < 0)
      return SS_INVALID_NAME;

   return SS_SUCCESS;

#endif                          /* OS_WINNT */

#ifdef OS_MSDOS

   spawnvp((int) mode, cmdname, argv);

   return SS_SUCCESS;

#endif                          /* OS_MSDOS */

#ifdef OS_VMS

   {
      char cmdstring[500], *pc;
      INT i, flags, status;
      va_list argptr;

      $DESCRIPTOR(cmdstring_dsc, "dummy");

      if (mode & P_DETACH) {
         cmdstring_dsc.dsc$w_length = strlen(cmdstring);
         cmdstring_dsc.dsc$a_pointer = cmdstring;

         status = sys$creprc(0, &cmdstring_dsc, 0, 0, 0, 0, 0, NULL, 4, 0, 0, PRC$M_DETACH);
      } else {
         flags = (mode & P_NOWAIT) ? 1 : 0;

         for (pc = argv[0] + strlen(argv[0]); *pc != ']' && pc != argv[0]; pc--);
         if (*pc == ']')
            pc++;

         strcpy(cmdstring, pc);

         if (strchr(cmdstring, ';'))
            *strchr(cmdstring, ';') = 0;

         strcat(cmdstring, " ");

         for (i = 1; argv[i] != NULL; i++) {
            strcat(cmdstring, argv[i]);
            strcat(cmdstring, " ");
         }

         cmdstring_dsc.dsc$w_length = strlen(cmdstring);
         cmdstring_dsc.dsc$a_pointer = cmdstring;

         status = lib$spawn(&cmdstring_dsc, 0, 0, &flags, NULL, 0, 0, 0, 0, 0, 0, 0, 0);
      }

      return BM_SUCCESS;
   }

#endif                          /* OS_VMS */
#ifdef OS_UNIX
   pid_t child_pid;

#ifdef OS_ULTRIX
   union wait *status;
#else
   int status;
#endif

#ifdef NO_FORK
   assert(!"support for fork() disabled by NO_FORK");
#else
   if ((child_pid = fork()) < 0)
      return (-1);
#endif

   if (child_pid == 0) {
      /* now we are in the child process ... */
      int error = execvp(cmdname, (char*const*)argv);
      fprintf(stderr, "ss_spawnv: Cannot execute command \"%s\": execvp() returned %d, errno %d (%s), aborting!\n", cmdname, error, errno, strerror(errno));
      // NB: this is the forked() process, if it returns back to the caller, we will have
      // a duplicate process for whoever called us. Very bad! So must abort. K.O.
      abort();
      // NOT REACHED
      return SS_SUCCESS;
   } else {
      /* still in parent process */
      if (mode == P_WAIT) {
#ifdef OS_ULTRIX
         waitpid(child_pid, status, WNOHANG);
#else
         waitpid(child_pid, &status, WNOHANG);
#endif

      } else {
         /* catch SIGCHLD signal to avoid <defunc> processes */
         signal(SIGCHLD, catch_sigchld);
      }
   }

   return SS_SUCCESS;

#endif                          /* OS_UNIX */
}

/*------------------------------------------------------------------*/
INT ss_shell(int sock)
/********************************************************************\

  Routine: ss_shell

  Purpose: Execute shell via socket (like telnetd)

  Input:
    int  sock        Socket

  Output:
    none

  Function value:
    SS_SUCCESS       Successful completeion

\********************************************************************/
{
#ifdef OS_WINNT

   HANDLE hChildStdinRd, hChildStdinWr, hChildStdinWrDup,
       hChildStdoutRd, hChildStdoutWr, hChildStderrRd, hChildStderrWr, hSaveStdin, hSaveStdout, hSaveStderr;

   SECURITY_ATTRIBUTES saAttr;
   PROCESS_INFORMATION piProcInfo;
   STARTUPINFO siStartInfo;
   char buffer[256], cmd[256];
   DWORD dwRead, dwWritten, dwAvail, i, i_cmd;
   fd_set readfds;
   struct timeval timeout;

   /* Set the bInheritHandle flag so pipe handles are inherited. */
   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   saAttr.bInheritHandle = TRUE;
   saAttr.lpSecurityDescriptor = NULL;

   /* Save the handle to the current STDOUT. */
   hSaveStdout = GetStdHandle(STD_OUTPUT_HANDLE);

   /* Create a pipe for the child's STDOUT. */
   if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
      return 0;

   /* Set a write handle to the pipe to be STDOUT. */
   if (!SetStdHandle(STD_OUTPUT_HANDLE, hChildStdoutWr))
      return 0;


   /* Save the handle to the current STDERR. */
   hSaveStderr = GetStdHandle(STD_ERROR_HANDLE);

   /* Create a pipe for the child's STDERR. */
   if (!CreatePipe(&hChildStderrRd, &hChildStderrWr, &saAttr, 0))
      return 0;

   /* Set a read handle to the pipe to be STDERR. */
   if (!SetStdHandle(STD_ERROR_HANDLE, hChildStderrWr))
      return 0;


   /* Save the handle to the current STDIN. */
   hSaveStdin = GetStdHandle(STD_INPUT_HANDLE);

   /* Create a pipe for the child's STDIN. */
   if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
      return 0;

   /* Set a read handle to the pipe to be STDIN. */
   if (!SetStdHandle(STD_INPUT_HANDLE, hChildStdinRd))
      return 0;

   /* Duplicate the write handle to the pipe so it is not inherited. */
   if (!DuplicateHandle(GetCurrentProcess(), hChildStdinWr, GetCurrentProcess(), &hChildStdinWrDup, 0, FALSE,   /* not inherited */
                        DUPLICATE_SAME_ACCESS))
      return 0;

   CloseHandle(hChildStdinWr);

   /* Now create the child process. */
   memset(&siStartInfo, 0, sizeof(siStartInfo));
   siStartInfo.cb = sizeof(STARTUPINFO);
   siStartInfo.lpReserved = NULL;
   siStartInfo.lpReserved2 = NULL;
   siStartInfo.cbReserved2 = 0;
   siStartInfo.lpDesktop = NULL;
   siStartInfo.dwFlags = 0;

   if (!CreateProcess(NULL, "cmd /Q",   /* command line */
                      NULL,     /* process security attributes */
                      NULL,     /* primary thread security attributes */
                      TRUE,     /* handles are inherited */
                      0,        /* creation flags */
                      NULL,     /* use parent's environment */
                      NULL,     /* use parent's current directory */
                      &siStartInfo,     /* STARTUPINFO pointer */
                      &piProcInfo))     /* receives PROCESS_INFORMATION */
      return 0;

   /* After process creation, restore the saved STDIN and STDOUT. */
   SetStdHandle(STD_INPUT_HANDLE, hSaveStdin);
   SetStdHandle(STD_OUTPUT_HANDLE, hSaveStdout);
   SetStdHandle(STD_ERROR_HANDLE, hSaveStderr);

   i_cmd = 0;

   do {
      /* query stderr */
      do {
         if (!PeekNamedPipe(hChildStderrRd, buffer, 256, &dwRead, &dwAvail, NULL))
            break;

         if (dwRead > 0) {
            ReadFile(hChildStderrRd, buffer, 256, &dwRead, NULL);
            send(sock, buffer, dwRead, 0);
         }
      } while (dwAvail > 0);

      /* query stdout */
      do {
         if (!PeekNamedPipe(hChildStdoutRd, buffer, 256, &dwRead, &dwAvail, NULL))
            break;
         if (dwRead > 0) {
            ReadFile(hChildStdoutRd, buffer, 256, &dwRead, NULL);
            send(sock, buffer, dwRead, 0);
         }
      } while (dwAvail > 0);


      /* check if subprocess still alive */
      if (!GetExitCodeProcess(piProcInfo.hProcess, &i))
         break;
      if (i != STILL_ACTIVE)
         break;

      /* query network socket */
      FD_ZERO(&readfds);
      FD_SET(sock, &readfds);
      timeout.tv_sec = 0;
      timeout.tv_usec = 100;
      select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

      if (FD_ISSET(sock, &readfds)) {
         i = recv(sock, cmd + i_cmd, 1, 0);
         if (i <= 0)
            break;

         /* backspace */
         if (cmd[i_cmd] == 8) {
            if (i_cmd > 0) {
               send(sock, "\b \b", 3, 0);
               i_cmd -= 1;
            }
         } else if (cmd[i_cmd] >= ' ' || cmd[i_cmd] == 13 || cmd[i_cmd] == 10) {
            send(sock, cmd + i_cmd, 1, 0);
            i_cmd += i;
         }
      }

      /* linefeed triggers new command */
      if (cmd[i_cmd - 1] == 10) {
         WriteFile(hChildStdinWrDup, cmd, i_cmd, &dwWritten, NULL);
         i_cmd = 0;
      }

   } while (TRUE);

   CloseHandle(hChildStdinWrDup);
   CloseHandle(hChildStdinRd);
   CloseHandle(hChildStderrRd);
   CloseHandle(hChildStdoutRd);

   return SS_SUCCESS;

#endif                          /* OS_WINNT */

#ifdef OS_UNIX
#ifndef NO_PTY
   pid_t pid;
   int i, p;
   char line[32], buffer[1024], shell[32];
   fd_set readfds;

#ifdef NO_FORK
   assert(!"support for forkpty() disabled by NO_FORK");
#else
   pid = forkpty(&p, line, NULL, NULL);
#endif
   if (pid < 0)
      return 0;
   else if (pid > 0) {
      /* parent process */

      do {
         FD_ZERO(&readfds);
         FD_SET(sock, &readfds);
         FD_SET(p, &readfds);

         select(FD_SETSIZE, &readfds, NULL, NULL, NULL);

         if (FD_ISSET(sock, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            i = recv(sock, buffer, sizeof(buffer), 0);
            if (i <= 0)
               break;
            if (write(p, buffer, i) != i)
               break;
         }

         if (FD_ISSET(p, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            i = read(p, buffer, sizeof(buffer));
            if (i <= 0)
               break;
            send(sock, buffer, i, 0);
         }

      } while (1);
   } else {
      /* child process */

      if (getenv("SHELL"))
         strlcpy(shell, getenv("SHELL"), sizeof(shell));
      else
         strcpy(shell, "/bin/sh");
      int error = execl(shell, shell, NULL);
      // NB: execl() does not return unless there is an error.
      fprintf(stderr, "ss_shell: Cannot execute command \"%s\": execl() returned %d, errno %d (%s), aborting!\n", shell, error, errno, strerror(errno));
      abort();
   }
#else
   send(sock, "not implemented\n", 17, 0);
#endif                          /* NO_PTY */

   return SS_SUCCESS;

#endif                          /* OS_UNIX */
}

/*------------------------------------------------------------------*/
static BOOL _daemon_flag;

INT ss_daemon_init(BOOL keep_stdout)
/********************************************************************\

  Routine: ss_daemon_init

  Purpose: Become a daemon

  Input:
    none

  Output:
    none

  Function value:
    SS_SUCCESS       Successful completeion
    SS_ABORT         fork() was not successful, or other problem

\********************************************************************/
{
#ifdef OS_UNIX

   /* only implemented for UNIX */
   int i, fd, pid;

#ifdef NO_FORK
   assert(!"support for fork() disabled by NO_FORK");
#else
   if ((pid = fork()) < 0)
      return SS_ABORT;
   else if (pid != 0)
      exit(0);                  /* parent finished */
#endif

   /* child continues here */

   _daemon_flag = TRUE;

   /* try and use up stdin, stdout and stderr, so other
      routines writing to stdout etc won't cause havoc. Copied from smbd */
   for (i = 0; i < 3; i++) {
      if (keep_stdout && ((i == 1) || (i == 2)))
         continue;

      close(i);
      fd = open("/dev/null", O_RDWR, 0);
      if (fd < 0)
         fd = open("/dev/null", O_WRONLY, 0);
      if (fd < 0) {
         cm_msg(MERROR, "ss_daemon_init", "Can't open /dev/null");
         return SS_ABORT;
      }
      if (fd != i) {
         cm_msg(MERROR, "ss_daemon_init", "Did not get file descriptor");
         return SS_ABORT;
      }
   }

   setsid();                    /* become session leader */

#endif

   return SS_SUCCESS;
}

#ifdef LOCAL_ROUTINES

/*------------------------------------------------------------------*/
BOOL ss_existpid(INT pid)
/********************************************************************\

  Routine: ss_existpid

  Purpose: Execute a Kill sig=0 which return success if pid found.

  Input:
    pid  : pid to check

  Output:
    none

  Function value:
    TRUE      PID found
    FALSE     PID not found

\********************************************************************/
{
#ifdef OS_UNIX
   /* only implemented for UNIX */
   return (kill(pid, 0) == 0 ? TRUE : FALSE);
#else
   cm_msg(MINFO, "ss_existpid", "implemented for UNIX only");
   return FALSE;
#endif
}

#endif // LOCAL_ROUTINES

/********************************************************************/
/**
Execute command in a separate process, close all open file descriptors
invoke ss_exec() and ignore pid.
\code
{ ...
  char cmd[256];
  sprintf(cmd,"%s %s %i %s/%s %1.3lf %d",lazy.commandAfter,
     lazy.backlabel, lazyst.nfiles, lazy.path, lazyst.backfile,
     lazyst.file_size/1000.0/1000.0, blockn);
  cm_msg(MINFO,"Lazy","Exec post file write script:%s",cmd);
  ss_system(cmd);
}
...
\endcode
@param command Command to execute.
@return SS_SUCCESS or ss_exec() return code
*/
INT ss_system(const char *command)
{
#ifdef OS_UNIX
   INT childpid;

   return ss_exec(command, &childpid);

#else

   system(command);
   return SS_SUCCESS;

#endif
}

/*------------------------------------------------------------------*/
INT ss_exec(const char *command, INT * pid)
/********************************************************************\

  Routine: ss_exec

  Purpose: Execute command in a separate process, close all open
           file descriptors, return the pid of the child process.

  Input:
    char * command    Command to execute
    INT  * pid        Returned PID of the spawned process.
  Output:
    none

  Function value:
    SS_SUCCESS       Successful completion
    SS_ABORT         fork() was not successful, or other problem

\********************************************************************/
{
#ifdef OS_UNIX

   /* only implemented for UNIX */
   int i, fd;

#ifdef NO_FORK
   assert(!"support for fork() disabled by NO_FORK");
#else
   *pid = fork();
#endif
   if (*pid < 0)
      return SS_ABORT;
   else if (*pid != 0) {
      /* avoid <defunc> parent processes */
      signal(SIGCHLD, catch_sigchld);
      return SS_SUCCESS;        /* parent returns */
   }

   /* child continues here... */

   /* close all open file descriptors */
   for (i = 0; i < 256; i++)
      close(i);

   /* try and use up stdin, stdout and stderr, so other
      routines writing to stdout etc won't cause havoc */
   for (i = 0; i < 3; i++) {
      fd = open("/dev/null", O_RDWR, 0);
      if (fd < 0)
         fd = open("/dev/null", O_WRONLY, 0);
      if (fd < 0) {
         cm_msg(MERROR, "ss_exec", "Can't open /dev/null");
         return SS_ABORT;
      }
      if (fd != i) {
         cm_msg(MERROR, "ss_exec", "Did not get file descriptor");
         return SS_ABORT;
      }
   }

   setsid();                    /* become session leader */
   /* chdir("/"); *//* change working directory (not on NFS!) */

   /* execute command */
   int error = execl("/bin/sh", "sh", "-c", command, NULL);
   // NB: execl() does not return unless there is an error. K.O.
   fprintf(stderr, "ss_shell: Cannot execute /bin/sh for command \"%s\": execl() returned %d, errno %d (%s), aborting!\n", command, error, errno, strerror(errno));
   abort();

#else

   system(command);

#endif

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/

std::string ss_replace_env_variables(const std::string& inputPath) {
   std::string result;
   size_t startPos = 0;
   size_t dollarPos;

   while ((dollarPos = inputPath.find('$', startPos)) != std::string::npos) {
      result.append(inputPath, startPos, dollarPos - startPos);

      size_t varEndPos = inputPath.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_", dollarPos + 1);
      size_t varLength = varEndPos - dollarPos - 1;
      std::string varName = inputPath.substr(dollarPos + 1, varLength);

      char* varValue = std::getenv(varName.c_str());
      if (varValue != nullptr) {
         result.append(varValue);
      }

      startPos = varEndPos;
   }

   result.append(inputPath.c_str(), startPos, std::string::npos);
   return result;
}

/*------------------------------------------------------------------*/
std::string ss_execs(const char *cmd)
/********************************************************************\

   Routine: ss_execs

   Purpose: Execute shell command and return result in a string

   Input:
      const char *command  Command to execute


   Function value:
      std::string          Result of shell commaand

\********************************************************************/
{
#ifdef OS_UNIX
   std::array<char, 256> buffer{};
   std::string result;
   std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);

   if (!pipe) {
      throw std::runtime_error("popen() failed!");
   }

   while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
      result += buffer.data();
   }

   return result;
#else
   fprintf(stderr, "ss_execs: Function not supported on this OS,  aborting!\n");
   abort();
#endif
}

/********************************************************************/
/**
Creates and returns a new thread of execution.

Note the difference when calling from vxWorks versus Linux and Windows.
The parameter pointer for a vxWorks call is a VX_TASK_SPAWN structure, whereas
for Linux and Windows it is a void pointer.
Early versions returned SS_SUCCESS or SS_NO_THREAD instead of thread ID.

Example for VxWorks
\code
...
VX_TASK_SPAWN tsWatch = {"Watchdog", 100, 0, 2000,  (int) pDevice, 0, 0, 0, 0, 0, 0, 0, 0 ,0};
midas_thread_t thread_id = ss_thread_create((void *) taskWatch, &tsWatch);
if (thread_id == 0) {
  printf("cannot spawn taskWatch\n");
}
...
\endcode
Example for Linux
\code
...
midas_thread_t thread_id = ss_thread_create((void *) taskWatch, pDevice);
if (thread_id == 0) {
  printf("cannot spawn taskWatch\n");
}
...
\endcode
@param (*thread_func) Thread function to create.
@param param a pointer to a VX_TASK_SPAWN structure for vxWorks and a void pointer
                for Unix and Windows
@return the new thread id or zero on error
*/
midas_thread_t ss_thread_create(INT(*thread_func) (void *), void *param)
{
#if defined(OS_WINNT)

   HANDLE status;
   DWORD thread_id;

   if (thread_func == NULL) {
      return 0;
   }

   status = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) thread_func, (LPVOID) param, 0, &thread_id);

   return status == NULL ? 0 : (midas_thread_t) thread_id;

#elif defined(OS_MSDOS)

   return 0;

#elif defined(OS_VMS)

   return 0;

#elif defined(OS_VXWORKS)

/* taskSpawn which could be considered as a thread under VxWorks
   requires several argument beside the thread args
   taskSpawn (taskname, priority, option, stacksize, entry_point
              , arg1, arg2, ... , arg9, arg10)
   all the arg will have to be retrieved from the param list.
   through a structure to be simpler  */

   INT status;
   VX_TASK_SPAWN *ts;

   ts = (VX_TASK_SPAWN *) param;
   status =
       taskSpawn(ts->name, ts->priority, ts->options, ts->stackSize,
                 (FUNCPTR) thread_func, ts->arg1, ts->arg2, ts->arg3,
                 ts->arg4, ts->arg5, ts->arg6, ts->arg7, ts->arg8, ts->arg9, ts->arg10);

   return status == ERROR ? 0 : status;

#elif defined(OS_UNIX)

   INT status;
   pthread_t thread_id;

   status = pthread_create(&thread_id, NULL, (void* (*)(void*))thread_func, param);

   return status != 0 ? 0 : thread_id;

#endif
}

/********************************************************************/
/**
Destroys the thread identified by the passed thread id.
The thread id is returned by ss_thread_create() on creation.

\code
...
midas_thread_t thread_id = ss_thread_create((void *) taskWatch, pDevice);
if (thread_id == 0) {
  printf("cannot spawn taskWatch\n");
}
...
ss_thread_kill(thread_id);
...
\endcode
@param thread_id the thread id of the thread to be killed.
@return SS_SUCCESS if no error, else SS_NO_THREAD
*/
INT ss_thread_kill(midas_thread_t thread_id)
{
#if defined(OS_WINNT)

   DWORD status;
   HANDLE th;

   th = OpenThread(THREAD_TERMINATE, FALSE, (DWORD)thread_id);
   if (th == 0)
      status = GetLastError();

   status = TerminateThread(th, 0);

   if (status == 0)
      status = GetLastError();

   return status != 0 ? SS_SUCCESS : SS_NO_THREAD;

#elif defined(OS_MSDOS)

   return 0;

#elif defined(OS_VMS)

   return 0;

#elif defined(OS_VXWORKS)

   INT status;
   status = taskDelete(thread_id);
   return status == OK ? 0 : ERROR;

#elif defined(OS_UNIX)

   INT status;
   status = pthread_kill(thread_id, SIGKILL);
   return status == 0 ? SS_SUCCESS : SS_NO_THREAD;

#endif
}

/*------------------------------------------------------------------*/

INT EXPRT ss_thread_set_name(std::string name)
{
#if defined(OS_DARWIN)

   pthread_setname_np(name.c_str());
   return SS_SUCCESS;

#elif defined(OS_UNIX)

   pthread_t thread = pthread_self();
   pthread_setname_np(thread, name.c_str());
   return SS_SUCCESS;

#else
   return 0;
#endif
}

std::string EXPRT ss_thread_get_name()
{
#if defined(OS_UNIX)
   char str[256];
   pthread_t thread = pthread_self();
   pthread_getname_np(thread, str, sizeof(str));
   return std::string(str);
#else
   return "";
#endif
}

/*------------------------------------------------------------------*/
static std::atomic_bool s_semaphore_trace{false};
static std::atomic_int  s_semaphore_nest_level{0}; // must be signed int!

INT ss_semaphore_create(const char *name, HNDLE * semaphore_handle)
/********************************************************************\

  Routine: ss_semaphore_create

  Purpose: Create a semaphore with a specific name

    Remark: Under VxWorks the specific semaphore handling is
            different than other OS. But VxWorks provides
            the POSIX-compatible semaphore interface.
            Under POSIX, no timeout is supported.
            So for the time being, we keep the pure VxWorks
            The semaphore type is a Binary instead of mutex
            as the binary is an optimized mutex.

  Input:
    char   *name            Name of the semaphore to create.
                            Special blank name "" creates a local semaphore for
                            syncronization between threads in multithreaded applications.

  Output:
    HNDLE  *semaphore_handle    Handle of the created semaphore

  Function value:
    SS_CREATED              semaphore was created
    SS_SUCCESS              semaphore existed already and was attached
    SS_NO_SEMAPHORE         Cannot create semaphore

\********************************************************************/
{
   char semaphore_name[256];

   /* Add a leading MX_ to the semaphore name */
   sprintf(semaphore_name, "MX_%s", name);

#ifdef OS_VXWORKS

   /* semBCreate is a Binary semaphore which is under VxWorks a optimized mutex
      refering to the programmer's Guide 5.3.1 */
   if ((*((SEM_ID *) mutex_handle) = semBCreate(SEM_Q_FIFO, SEM_EMPTY)) == NULL)
      return SS_NO_MUTEX;
   return SS_CREATED;

#endif                          /* OS_VXWORKS */

#ifdef OS_WINNT

   *semaphore_handle = (HNDLE) CreateMutex(NULL, FALSE, semaphore_name);

   if (*semaphore_handle == 0)
      return SS_NO_SEMAPHORE;

   return SS_CREATED;

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   /* VMS has to use lock manager... */

   {
      INT status;
      $DESCRIPTOR(semaphorename_dsc, "dummy");
      semaphorename_dsc.dsc$w_length = strlen(semaphore_name);
      semaphorename_dsc.dsc$a_pointer = semaphore_name;

      *semaphore_handle = (HNDLE) malloc(8);

      status = sys$enqw(0, LCK$K_NLMODE, *semaphore_handle, 0, &semaphorename_dsc, 0, 0, 0, 0, 0, 0);

      if (status != SS$_NORMAL) {
         free((void *) *semaphore_handle);
         *semaphore_handle = 0;
      }

      if (*semaphore_handle == 0)
         return SS_NO_SEMAPHORE;

      return SS_CREATED;
   }

#endif                          /* OS_VMS */
#ifdef OS_UNIX

   {
      INT key = IPC_PRIVATE;
      int status;
      struct semid_ds buf;

      if (name[0] != 0) {
         int fh;
         char path[256], file_name[256];

         /* Build the filename out of the path and the name of the semaphore */
         cm_get_path(path, sizeof(path));
         if (path[0] == 0) {
            if (!getcwd(path, sizeof(path)))
               path[0] = 0;
            strlcat(path, "/", sizeof(path));
         }

         strlcpy(file_name, path, sizeof(file_name));
         strlcat(file_name, ".", sizeof(file_name));
         strlcat(file_name, name, sizeof(file_name));
         strlcat(file_name, ".SHM", sizeof(file_name));

         /* create a unique key from the file name */
         key = ftok(file_name, 'M');
         if (key < 0) {
            fh = open(file_name, O_CREAT, 0644);
            close(fh);
            key = ftok(file_name, 'M');
            status = SS_CREATED;
         }
      }

#if (defined(OS_LINUX) && !defined(_SEM_SEMUN_UNDEFINED) && !defined(OS_CYGWIN)) || defined(OS_FREEBSD)
      union semun arg;
#else
      union semun {
         INT val;
         struct semid_ds *buf;
         ushort *array;
      } arg;
#endif

      status = SS_SUCCESS;

      /* create or get semaphore */
      *semaphore_handle = (HNDLE) semget(key, 1, 0);
      //printf("create1 key 0x%x, id %d, errno %d (%s)\n", key, *semaphore_handle, errno, strerror(errno));
      if (*semaphore_handle < 0) {
         *semaphore_handle = (HNDLE) semget(key, 1, IPC_CREAT);
         //printf("create2 key 0x%x, id %d, errno %d (%s)\n", key, *semaphore_handle, errno, strerror(errno));
         status = SS_CREATED;
      }

      if (*semaphore_handle < 0) {
         cm_msg(MERROR, "ss_semaphore_create", "Cannot create semaphore \'%s\', semget(0x%x) failed, errno %d (%s)", name, key, errno, strerror(errno));
         
         fprintf(stderr, "ss_semaphore_create: Cannot create semaphore \'%s\', semget(0x%x) failed, errno %d (%s)", name, key, errno, strerror(errno));
         abort(); // does not return
         return SS_NO_SEMAPHORE;
      }

      memset(&buf, 0, sizeof(buf));
      buf.sem_perm.uid = getuid();
      buf.sem_perm.gid = getgid();
      buf.sem_perm.mode = 0666;
      arg.buf = &buf;

      semctl(*semaphore_handle, 0, IPC_SET, arg);

      /* if semaphore was created, set value to one */
      if (key == IPC_PRIVATE || status == SS_CREATED) {
         arg.val = 1;
         if (semctl(*semaphore_handle, 0, SETVAL, arg) < 0)
            return SS_NO_SEMAPHORE;
      }

      if (s_semaphore_trace) {
         fprintf(stderr, "name %d %d %d %s\n", *semaphore_handle, (int)time(NULL), getpid(), name);
      }

      return SS_SUCCESS;
   }
#endif                          /* OS_UNIX */

#ifdef OS_MSDOS
   return SS_NO_SEMAPHORE;
#endif
}

/*------------------------------------------------------------------*/
INT ss_semaphore_wait_for(HNDLE semaphore_handle, INT timeout)
/********************************************************************\

  Routine: ss_semaphore_wait_for

  Purpose: Wait for a semaphore to get owned

  Input:
    HNDLE  *semaphore_handle    Handle of the semaphore
    INT    timeout          Timeout in ms, zero for no timeout

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_SEMAPHORE         Invalid semaphore handle
    SS_TIMEOUT              Timeout

\********************************************************************/
{
   INT status;

#ifdef OS_WINNT

   status = WaitForSingleObject((HANDLE) semaphore_handle, timeout == 0 ? INFINITE : timeout);
   if (status == WAIT_FAILED)
      return SS_NO_SEMAPHORE;
   if (status == WAIT_TIMEOUT)
      return SS_TIMEOUT;

   return SS_SUCCESS;
#endif                          /* OS_WINNT */
#ifdef OS_VMS
   status = sys$enqw(0, LCK$K_EXMODE, semaphore_handle, LCK$M_CONVERT, 0, 0, 0, 0, 0, 0, 0);
   if (status != SS$_NORMAL)
      return SS_NO_SEMAPHORE;
   return SS_SUCCESS;

#endif                          /* OS_VMS */
#ifdef OS_VXWORKS
   /* convert timeout in ticks (1/60) = 1000/60 ~ 1/16 = >>4 */
   status = semTake((SEM_ID) semaphore_handle, timeout == 0 ? WAIT_FOREVER : timeout >> 4);
   if (status == ERROR)
      return SS_NO_SEMAPHORE;
   return SS_SUCCESS;

#endif                          /* OS_VXWORKS */
#ifdef OS_UNIX
   {
      DWORD start_time;
      struct sembuf sb;

#if (defined(OS_LINUX) && !defined(_SEM_SEMUN_UNDEFINED) && !defined(OS_CYGWIN)) || defined(OS_FREEBSD)
      union semun arg;
#else
      union semun {
         INT val;
         struct semid_ds *buf;
         ushort *array;
      } arg;
#endif

      sb.sem_num = 0;
      sb.sem_op = -1;           /* decrement semaphore */
      sb.sem_flg = SEM_UNDO;

      memset(&arg, 0, sizeof(arg));

      start_time = ss_millitime();

      if (s_semaphore_trace) {
         fprintf(stderr, "waitlock %d %d %d nest %d\n", semaphore_handle, ss_millitime(), getpid(), int(s_semaphore_nest_level));
      }

      do {
#if defined(OS_DARWIN)
         status = semop(semaphore_handle, &sb, 1);
#elif defined(OS_LINUX)
         struct timespec ts;
         ts.tv_sec  = 1;
         ts.tv_nsec = 0;

         status = semtimedop(semaphore_handle, &sb, 1, &ts);
#else
         status = semop(semaphore_handle, &sb, 1);
#endif

         /* return on success */
         if (status == 0)
            break;

         /* retry if interrupted by a ss_wake signal */
         if (errno == EINTR || errno == EAGAIN) {
            /* return if timeout expired */
            if (timeout > 0 && (int) (ss_millitime() - start_time) > timeout)
               return SS_TIMEOUT;

            continue;
         }

         fprintf(stderr, "ss_semaphore_wait_for: semop/semtimedop(%d) returned %d, errno %d (%s)\n", semaphore_handle, status, errno, strerror(errno));
         return SS_NO_SEMAPHORE;
      } while (1);

      if (s_semaphore_trace) {
         s_semaphore_nest_level++;
         fprintf(stderr, "lock %d %d %d nest %d\n", semaphore_handle, ss_millitime(), getpid(), int( s_semaphore_nest_level));
      }

      return SS_SUCCESS;
   }
#endif                          /* OS_UNIX */

#ifdef OS_MSDOS
   return SS_NO_SEMAPHORE;
#endif
}

/*------------------------------------------------------------------*/
INT ss_semaphore_release(HNDLE semaphore_handle)
/********************************************************************\

  Routine: ss_semaphore_release

  Purpose: Release ownership of a semaphore

  Input:
    HNDLE  *semaphore_handle    Handle of the semaphore

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_SEMAPHORE         Invalid semaphore handle

\********************************************************************/
{
   INT status;

#ifdef OS_WINNT

   status = ReleaseMutex((HANDLE) semaphore_handle);

   if (status == FALSE)
      return SS_NO_SEMAPHORE;

   return SS_SUCCESS;

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   status = sys$enqw(0, LCK$K_NLMODE, semaphore_handle, LCK$M_CONVERT, 0, 0, 0, 0, 0, 0, 0);

   if (status != SS$_NORMAL)
      return SS_NO_SEMAPHORE;

   return SS_SUCCESS;

#endif                          /* OS_VMS */

#ifdef OS_VXWORKS

   if (semGive((SEM_ID) semaphore_handle) == ERROR)
      return SS_NO_SEMAPHORE;
   return SS_SUCCESS;
#endif                          /* OS_VXWORKS */

#ifdef OS_UNIX
   {
      struct sembuf sb;

      sb.sem_num = 0;
      sb.sem_op = 1;            /* increment semaphore */
      sb.sem_flg = SEM_UNDO;

      if (s_semaphore_trace) {
         fprintf(stderr, "unlock %d %d %d nest %d\n", semaphore_handle, ss_millitime(), getpid(), int(s_semaphore_nest_level));
         assert(s_semaphore_nest_level > 0);
         s_semaphore_nest_level--;
      }

      do {
         status = semop(semaphore_handle, &sb, 1);

         /* return on success */
         if (status == 0)
            break;

         /* retry if interrupted by a ss_wake signal */
         if (errno == EINTR)
            continue;

         fprintf(stderr, "ss_semaphore_release: semop/semtimedop(%d) returned %d, errno %d (%s)\n", semaphore_handle, status, errno, strerror(errno));
         return SS_NO_SEMAPHORE;
      } while (1);

      return SS_SUCCESS;
   }
#endif                          /* OS_UNIX */

#ifdef OS_MSDOS
   return SS_NO_SEMAPHORE;
#endif
}

/*------------------------------------------------------------------*/
INT ss_semaphore_delete(HNDLE semaphore_handle, INT destroy_flag)
/********************************************************************\

  Routine: ss_semaphore_delete

  Purpose: Delete a semaphore

  Input:
    HNDLE  *semaphore_handle    Handle of the semaphore

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_SEMAPHORE         Invalid semaphore handle

\********************************************************************/
{
#ifdef OS_WINNT

   if (CloseHandle((HANDLE) semaphore_handle) == FALSE)
      return SS_NO_SEMAPHORE;

   return SS_SUCCESS;

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   free((void *) semaphore_handle);
   return SS_SUCCESS;

#endif                          /* OS_VMS */

#ifdef OS_VXWORKS
   /* no code for VxWorks destroy yet */
   if (semDelete((SEM_ID) semaphore_handle) == ERROR)
      return SS_NO_SEMAPHORE;
   return SS_SUCCESS;
#endif                          /* OS_VXWORKS */

#ifdef OS_UNIX
#if (defined(OS_LINUX) && !defined(_SEM_SEMUN_UNDEFINED) && !defined(OS_CYGWIN)) || defined(OS_FREEBSD)
   union semun arg;
#else
   union semun {
      INT val;
      struct semid_ds *buf;
      ushort *array;
   } arg;
#endif

   memset(&arg, 0, sizeof(arg));

   if (destroy_flag) {
      int status = semctl(semaphore_handle, 0, IPC_RMID, arg);
      //printf("semctl(ID=%d, IPC_RMID) returned %d, errno %d (%s)\n", semaphore_handle, status, errno, strerror(errno));
      if (status < 0)
         return SS_NO_SEMAPHORE;
   }

   return SS_SUCCESS;

#endif                          /* OS_UNIX */

#ifdef OS_MSDOS
   return SS_NO_SEMAPHORE;
#endif
}

/*------------------------------------------------------------------*/

INT ss_mutex_create(MUTEX_T ** mutex, BOOL recursive)
/********************************************************************\

  Routine: ss_mutex_create

  Purpose: Create a mutex for inter-thread locking

  Output:
    MUTEX_T mutex           Address of pointer to mutex

  Function value:
    SS_CREATED              Mutex was created
    SS_NO_SEMAPHORE         Cannot create mutex

\********************************************************************/
{
#ifdef OS_VXWORKS

   /* semBCreate is a Binary semaphore which is under VxWorks a optimized mutex
      refering to the programmer's Guide 5.3.1 */
   if ((*((SEM_ID *) mutex_handle) = semBCreate(SEM_Q_FIFO, SEM_EMPTY)) == NULL)
      return SS_NO_MUTEX;
   return SS_CREATED;

#endif                          /* OS_VXWORKS */

#ifdef OS_WINNT

   *mutex = (MUTEX_T *)malloc(sizeof(HANDLE));
   **mutex = CreateMutex(NULL, FALSE, NULL);

   if (**mutex == 0)
      return SS_NO_MUTEX;

   return SS_CREATED;

#endif                          /* OS_WINNT */
#ifdef OS_UNIX

   {
      int status;
      pthread_mutexattr_t *attr;

      attr = (pthread_mutexattr_t*)malloc(sizeof(*attr));
      assert(attr);

      status = pthread_mutexattr_init(attr);
      if (status != 0) {
         fprintf(stderr, "ss_mutex_create: pthread_mutexattr_init() returned errno %d (%s)\n", status, strerror(status));
      }
      
      if (recursive) {
         status = pthread_mutexattr_settype(attr, PTHREAD_MUTEX_RECURSIVE);
         if (status != 0) {
            fprintf(stderr, "ss_mutex_create: pthread_mutexattr_settype() returned errno %d (%s)\n", status, strerror(status));
         }
      }

      *mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
      assert(*mutex);

      status = pthread_mutex_init(*mutex, attr);
      if (status != 0) {
         fprintf(stderr, "ss_mutex_create: pthread_mutex_init() returned errno %d (%s), aborting...\n", status, strerror(status));
         abort(); // does not return
         return SS_NO_MUTEX;
      }

      free(attr);
   
      if (recursive) {
         // test recursive locks
         
         status = pthread_mutex_trylock(*mutex);
         assert(status == 0);
         
         status = pthread_mutex_trylock(*mutex);
         assert(status == 0); // EBUSY if PTHREAD_MUTEX_RECURSIVE does not work
         
         status = pthread_mutex_unlock(*mutex);
         assert(status == 0);
         
         status = pthread_mutex_unlock(*mutex);
         assert(status == 0);
      }

      return SS_SUCCESS;
   }
#endif                          /* OS_UNIX */

#ifdef OS_MSDOS
   return SS_NO_SEMAPHORE;
#endif
}

/*------------------------------------------------------------------*/
INT ss_mutex_wait_for(MUTEX_T *mutex, INT timeout)
/********************************************************************\

  Routine: ss_mutex_wait_for

  Purpose: Wait for a mutex to get owned

  Input:
    MUTEX_T  *mutex         Pointer to mutex
    INT    timeout          Timeout in ms, zero for no timeout

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_MUTEX             Invalid mutex handle
    SS_TIMEOUT              Timeout

\********************************************************************/
{
   INT status;

#ifdef OS_WINNT

   status = WaitForSingleObject(*mutex, timeout == 0 ? INFINITE : timeout);

   if (status == WAIT_TIMEOUT) {
      return SS_TIMEOUT;
   }

   if (status == WAIT_FAILED) {
      fprintf(stderr, "ss_mutex_wait_for: WaitForSingleObject() failed, status = %d", status);
      abort(); // does not return
      return SS_NO_MUTEX;
   }

   return SS_SUCCESS;
#endif                          /* OS_WINNT */
#ifdef OS_VXWORKS
   /* convert timeout in ticks (1/60) = 1000/60 ~ 1/16 = >>4 */
   status = semTake((SEM_ID) mutex, timeout == 0 ? WAIT_FOREVER : timeout >> 4);
   if (status == ERROR)
      return SS_NO_MUTEX;
   return SS_SUCCESS;

#endif                          /* OS_VXWORKS */
#if defined(OS_UNIX)

#if defined(OS_DARWIN)

   if (timeout > 0) {
      // emulate pthread_mutex_timedlock under OS_DARWIN
      DWORD wait = 0;
      while (1) {
         status = pthread_mutex_trylock(mutex);
         if (status == 0) {
            return SS_SUCCESS;
         } else if (status == EBUSY) {
            ss_sleep(10);
            wait += 10;
         } else {
            fprintf(stderr, "ss_mutex_wait_for: fatal error: pthread_mutex_trylock() returned errno %d (%s), aborting...\n", status, strerror(status));
            abort(); // does not return
         }
         if (wait > timeout) {
            fprintf(stderr, "ss_mutex_wait_for: fatal error: timeout waiting for mutex, timeout was %d millisec, aborting...\n", timeout);
            abort(); // does not return
         }
      }
   } else {
      status = pthread_mutex_lock(mutex);
   }

   if (status != 0) {
      fprintf(stderr, "ss_mutex_wait_for: pthread_mutex_lock() returned errno %d (%s), aborting...\n", status, strerror(status));
      abort(); // does not return
   }

   return SS_SUCCESS;

#else // OS_DARWIN
   if (timeout > 0) {
      extern int pthread_mutex_timedlock (pthread_mutex_t *__restrict __mutex, __const struct timespec *__restrict __abstime) __THROW;
      struct timespec st;

      clock_gettime(CLOCK_REALTIME, &st);
      st.tv_sec += timeout / 1000;
      st.tv_nsec += (timeout % 1000) * 1000000;
      status = pthread_mutex_timedlock(mutex, &st);

      if (status == ETIMEDOUT) {
         fprintf(stderr, "ss_mutex_wait_for: fatal error: timeout waiting for mutex, timeout was %d millisec, aborting...\n", timeout);
         abort();
      }

      // Make linux timeout do same as MacOS timeout: abort() the program
      //if (status == ETIMEDOUT)
      //   return SS_TIMEOUT;
      //return SS_SUCCESS;
   } else {
      status = pthread_mutex_lock(mutex);
   }

   if (status != 0) {
      fprintf(stderr, "ss_mutex_wait_for: pthread_mutex_lock() returned errno %d (%s), aborting...\n", status, strerror(status));
      abort();
   }

   return SS_SUCCESS;
#endif

#endif /* OS_UNIX */

#ifdef OS_MSDOS
   return SS_NO_MUTEX;
#endif
}

/*------------------------------------------------------------------*/
INT ss_mutex_release(MUTEX_T *mutex)
/********************************************************************\

  Routine: ss_mutex_release

  Purpose: Release ownership of a mutex

  Input:
    MUTEX_T  *mutex         Pointer to mutex

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_MUTES             Invalid mutes handle

\********************************************************************/
{
   INT status;

#ifdef OS_WINNT

   status = ReleaseMutex(*mutex);
   if (status == FALSE)
      return SS_NO_SEMAPHORE;

   return SS_SUCCESS;

#endif                          /* OS_WINNT */
#ifdef OS_VXWORKS

   if (semGive((SEM_ID) mutes_handle) == ERROR)
      return SS_NO_MUTEX;
   return SS_SUCCESS;
#endif                          /* OS_VXWORKS */
#ifdef OS_UNIX

      status = pthread_mutex_unlock(mutex);
      if (status != 0) {
         fprintf(stderr, "ss_mutex_release: pthread_mutex_unlock() returned error %d (%s), aborting...\n", status, strerror(status));
         abort(); // does not return
         return SS_NO_MUTEX;
      }

      return SS_SUCCESS;
#endif                          /* OS_UNIX */

#ifdef OS_MSDOS
   return SS_NO_MUTEX;
#endif
}

/*------------------------------------------------------------------*/
INT ss_mutex_delete(MUTEX_T *mutex)
/********************************************************************\

  Routine: ss_mutex_delete

  Purpose: Delete a mutex

  Input:
    MUTEX_T  *mutex         Pointer to mutex

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_MUTEX             Invalid mutex handle

\********************************************************************/
{
#ifdef OS_WINNT

   if (CloseHandle(*mutex) == FALSE)
      return SS_NO_SEMAPHORE;

   free(mutex);

   return SS_SUCCESS;

#endif                          /* OS_WINNT */
#ifdef OS_VXWORKS
   /* no code for VxWorks destroy yet */
   if (semDelete((SEM_ID) mutex_handle) == ERROR)
      return SS_NO_MUTEX;
   return SS_SUCCESS;
#endif                          /* OS_VXWORKS */

#ifdef OS_UNIX
   {
      int status;

      status = pthread_mutex_destroy(mutex);
      if (status != 0) {
         fprintf(stderr, "ss_mutex_delete: pthread_mutex_destroy() returned errno %d (%s), aborting...\n", status, strerror(status));
         abort(); // do not return
         return SS_NO_MUTEX;
      }

      free(mutex);
      return SS_SUCCESS;
   }
#endif                          /* OS_UNIX */
}

/*------------------------------------------------------------------*/
bool ss_timed_mutex_wait_for_sec(std::timed_mutex& mutex, const char* mutex_name, double timeout_sec)
/********************************************************************\

  Routine: ss_timed_mutex_wait_for_sec

  Purpose: Lock C++11 timed mutex with a timeout

  Input:
    std::timed_mutex&  mutex         Pointer to mutex
    double             timeout_sec   Timeout in seconds, zero to wait forever

  Function value:
    true                    Successful completion
    false                   Timeout

\********************************************************************/
{
   if (timeout_sec <= 0) {
      mutex.lock();
      return true;
   }

   double starttime = ss_time_sec();
   double endtime = starttime + timeout_sec;

   // NB: per timed mutex try_lock_for(), one must always
   // look waiting for successful lock because it is permitted
   // to return "false" even if timeout did not yet expire. (cannot
   // tell permitted spurious failure from normal timeout). K.O.

   double locktime = starttime;

   while (1) {
      bool ok = mutex.try_lock_for(std::chrono::milliseconds(1000));

      if (ok) {
         //double now = ss_time_sec();
         //fprintf(stderr, "ss_timed_mutex_wait_for_sec: mutex %s locked in %.1f seconds. timeout %.1f seconds\n", mutex_name, now-starttime, timeout_sec);
         return true;
      }

      double now = ss_time_sec();

      if (mutex_name) {
         if (now-locktime < 0.2) {
            // mutex.try_lock_for() is permitted spuriously fail: return false before the 1 sec timeout expires, we should not print any messages about it. K.O.
            //fprintf(stderr, "ss_timed_mutex_wait_for_sec: short try_lock_for(1000). %.3f seconds\n", now-locktime);
         } else {
            fprintf(stderr, "ss_timed_mutex_wait_for_sec: long wait for mutex %s, %.1f seconds. %.1f seconds until timeout\n", mutex_name, now-starttime, endtime-now);
         }
      }

      if (now > endtime)
         return false;

      locktime = now;
   }
}

//
// thread-safe versions of tzset() and mktime().
//
// as of ubuntu 20.04, tzset() and mktime() are not thread safe,
// easy to see by source code inspection. there is no reeader lock
// in mktime() to protect global time zone data against modification
// by tzset() executing in another thread. (on stackoverflow people
// argue that as long as system time zone never changes, this violation of
// thread safety is benign).
//
// calling mktime() is quite expensive, easy to see by inspecting the source code:
// each call to mktime() will call tzset(), inside tzset(), "old_tz" is always
// reallocated by a free() and strdup() pair and a stat() syscall is made
// to check that file /etc/localtime did not change. These overheads can be turned off
// by setting setenv("TZ") to a time zone name (i.e. "UTC") or to the value "/etc/localtime".
//
// tzset() itself is thread-safe, it uses a lock to protect global
// time zone data against another tzset() running in a different thread.
// however this lock is not instrumented by the thread sanitizer and
// causes false positive data race warnings.
//
// in MIDAS, we choose this solution to avoid the thread sanitizer false positive
// warning about tzset() - introduce ss_tzset() to protect calls to tzset() with
// a mutex and introduce ss_mktime() to add same protection to tzset() called by mktime()
// internally. It also makes calls to ss_mktime() explicitely thread-safe.
//
// K.O. 2022-Mar-10.
//

static std::mutex gTzMutex;

void ss_tzset()
{
   std::lock_guard<std::mutex> lock(gTzMutex);
   //defeat tzset() error trap from msystem.h
   //#ifdef tzset
   //#undef tzset
   //#endif
   tzset();
}

time_t ss_mktime(struct tm* tms)
{
   std::lock_guard<std::mutex> lock(gTzMutex);
   //defeat mktime() error trap from msystem.h
   //#ifdef mktime
   //#undef mktime
   //#endif
   return mktime(tms);
}

/********************************************************************/
/**
Returns the actual time in milliseconds with an arbitrary
origin. This time may only be used to calculate relative times.

Overruns in the 32 bit value don't hurt since in a subtraction calculated
with 32 bit accuracy this overrun cancels (you may think about!)..
\code
...
DWORD start, stop:
start = ss_millitime();
  < do operations >
stop = ss_millitime();
printf("Operation took %1.3lf seconds\n",(stop-start)/1000.0);
...
\endcode
@return millisecond time stamp.
*/
DWORD ss_millitime()
{
#ifdef OS_WINNT

   return (int) GetTickCount();

#endif                          /* OS_WINNT */
#ifdef OS_MSDOS

   return clock() * 55;

#endif                          /* OS_MSDOS */
#ifdef OS_VMS

   {
      char time[8];
      DWORD lo, hi;

      sys$gettim(time);

      lo = *((DWORD *) time);
      hi = *((DWORD *) (time + 4));

/*  return *lo / 10000; */

      return lo / 10000 + hi * 429496.7296;

   }

#endif                          /* OS_VMS */
#ifdef OS_UNIX
   {
      struct timeval tv;

      gettimeofday(&tv, NULL);

      DWORD m = tv.tv_sec * 1000 + tv.tv_usec / 1000;
      //m += 0x137e0000; // adjust milltime for testing 32-bit wrap-around
      return m;
   }

#endif                          /* OS_UNIX */
#ifdef OS_VXWORKS
   {
      int count;
      static int ticks_per_msec = 0;

      if (ticks_per_msec == 0)
         ticks_per_msec = 1000 / sysClkRateGet();

      return tickGet() * ticks_per_msec;
   }
#endif                          /* OS_VXWORKS */
}

/********************************************************************/
/**
Returns the actual time in seconds since 1.1.1970 UTC.
\code
...
DWORD start, stop:
start = ss_time();
  ss_sleep(12000);
stop = ss_time();
printf("Operation took %1.3lf seconds\n",stop-start);
...
\endcode
@return Time in seconds
*/
DWORD ss_time()
{
   return (DWORD) time(NULL);
}

double ss_time_sec()
{
   struct timeval tv; 
   gettimeofday(&tv, NULL); 
   return tv.tv_sec*1.0 + tv.tv_usec/1000000.0; 
}

/*------------------------------------------------------------------*/
DWORD ss_settime(DWORD seconds)
/********************************************************************\

  Routine: ss_settime

  Purpose: Set local time. Used to synchronize different computers

   Input:
    INT    Time in seconds since 1.1.1970 UTC.

  Output:
    none

  Function value:

\********************************************************************/
{
#if defined(OS_WINNT)
   SYSTEMTIME st;
   struct tm ltm;

   ss_tzset();
   localtime_r((time_t *) & seconds, &ltm);

   st.wYear = ltm.tm_year + 1900;
   st.wMonth = ltm.tm_mon + 1;
   st.wDay = ltm.tm_mday;
   st.wHour = ltm.tm_hour;
   st.wMinute = ltm.tm_min;
   st.wSecond = ltm.tm_sec;
   st.wMilliseconds = 0;

   SetLocalTime(&st);

#elif defined(OS_DARWIN) && defined(CLOCK_REALTIME)

   struct timespec ltm;

   ltm.tv_sec = seconds;
   ltm.tv_nsec = 0;
   clock_settime(CLOCK_REALTIME, &ltm);

#elif defined(OS_CYGWIN) && defined(CLOCK_REALTIME)

   struct timespec ltm;

   ltm.tv_sec = seconds;
   ltm.tv_nsec = 0;
   clock_settime(CLOCK_REALTIME, &ltm);
   return SS_NO_DRIVER;

#elif defined(OS_UNIX) && defined(CLOCK_REALTIME)

   struct timespec ltm;

   ltm.tv_sec = seconds;
   ltm.tv_nsec = 0;
   clock_settime(CLOCK_REALTIME, &ltm);

#elif defined(OS_VXWORKS)

   struct timespec ltm;

   ltm.tv_sec = seconds;
   ltm.tv_nsec = 0;
   clock_settime(CLOCK_REALTIME, &ltm);

#else
#warning ss_settime() is not supported!
#endif
   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
std::string ss_asctime()
/********************************************************************\

  Routine: ss_asctime

  Purpose: Returns the local actual time as a string

  Input:
    none

  Output:
    none

  Function value:
    char   *     Time string

\********************************************************************/
{
   ss_tzset(); // required for localtime_t()
   time_t seconds = (time_t) ss_time();
   struct tm tms;
   localtime_r(&seconds, &tms);
   char str[32];
   asctime_r(&tms, str);
   /* strip new line */
   str[24] = 0;

   return str;
}

/*------------------------------------------------------------------*/
INT ss_timezone()
/********************************************************************\

  Routine: ss_timezone

  Purpose: Returns difference in seconds between coordinated universal
           time and local time.

  Input:
    none

  Output:
    none

  Function value:
    INT    Time difference in seconds

\********************************************************************/
{
#if defined(OS_DARWIN) || defined(OS_VXWORKS)
   return 0;
#else
   return (INT) timezone;       /* on Linux, comes from "#include <time.h>". */
#endif
}


/*------------------------------------------------------------------*/

#ifdef OS_UNIX
/* dummy function for signal() call */
void ss_cont(int signum)
{
}
#endif

/********************************************************************/
/**
Suspend the calling process for a certain time.

The function is similar to the sleep() function,
but has a resolution of one milliseconds. Under VxWorks the resolution
is 1/60 of a second. It uses the socket select() function with a time-out.
See examples in ss_time()
@param millisec Time in milliseconds to sleep. Zero means
                infinite (until another process calls ss_wake)
@return SS_SUCCESS
*/
INT ss_sleep(INT millisec)
{
   if (millisec == 0) {
#ifdef OS_WINNT
      SuspendThread(GetCurrentThread());
#endif
#ifdef OS_VMS
      sys$hiber();
#endif
#ifdef OS_UNIX
      signal(SIGCONT, ss_cont);
      pause();
#endif
      return SS_SUCCESS;
   }
#ifdef OS_WINNT
   Sleep(millisec);
#endif
#ifdef OS_UNIX
   struct timespec ts;
   int status;

   ts.tv_sec = millisec / 1000;
   ts.tv_nsec = (millisec % 1000) * 1E6;

   do {
      status = nanosleep(&ts, &ts);
      if ((int)ts.tv_sec < 0)
         break; // can be negative under OSX
   } while (status == -1 && errno == EINTR);
#endif

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
BOOL ss_kbhit()
/********************************************************************\

  Routine: ss_kbhit

  Purpose: Returns TRUE if a key is pressed

  Input:
    none

  Output:
    none

  Function value:
    FALSE                 No key has been pressed
    TRUE                  Key has been pressed

\********************************************************************/
{
#ifdef OS_MSDOS

   return kbhit();

#endif                          /* OS_MSDOS */
#ifdef OS_WINNT

   return kbhit();

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   return FALSE;

#endif                          /* OS_VMS */
#ifdef OS_UNIX

   int n;

   if (_daemon_flag)
      return 0;

   ioctl(0, FIONREAD, &n);
   return (n > 0);

#endif                          /* OS_UNIX */
#ifdef OS_VXWORKS

   int n;
   ioctl(0, FIONREAD, (long) &n);
   return (n > 0);

#endif                          /* OS_UNIX */
}


/*------------------------------------------------------------------*/
#ifdef LOCAL_ROUTINES

/*------------------------------------------------------------------*/
#ifdef OS_WINNT

static void (*UserCallback) (int);
static UINT _timer_id = 0;

VOID CALLBACK _timeCallback(UINT idEvent, UINT uReserved, DWORD dwUser, DWORD dwReserved1, DWORD dwReserved2)
{
   _timer_id = 0;
   if (UserCallback != NULL)
      UserCallback(0);
}

#endif                          /* OS_WINNT */

INT ss_alarm(INT millitime, void (*func) (int))
/********************************************************************\

  Routine: ss_alarm

  Purpose: Schedules an alarm. Call function referenced by *func
     after the specified seconds.

  Input:
    INT    millitime        Time in milliseconds
    void   (*func)()        Function to be called after the spe-
          cified time.

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion

\********************************************************************/
{
#ifdef OS_WINNT

   UserCallback = func;
   if (millitime > 0)
      _timer_id = timeSetEvent(millitime, 100, (LPTIMECALLBACK) _timeCallback, 0, TIME_ONESHOT);
   else {
      if (_timer_id)
         timeKillEvent(_timer_id);
      _timer_id = 0;
   }

   return SS_SUCCESS;

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   signal(SIGALRM, func);
   alarm(millitime / 1000);
   return SS_SUCCESS;

#endif                          /* OS_VMS */
#ifdef OS_UNIX

   signal(SIGALRM, func);
   alarm(millitime / 1000);
   return SS_SUCCESS;

#endif                          /* OS_UNIX */
}

/*------------------------------------------------------------------*/
void (*MidasExceptionHandler) (void);

#ifdef OS_WINNT

LONG MidasExceptionFilter(LPEXCEPTION_POINTERS pexcep)
{
   if (MidasExceptionHandler != NULL)
      MidasExceptionHandler();

   return EXCEPTION_CONTINUE_SEARCH;
}

INT MidasExceptionSignal(INT sig)
{
   if (MidasExceptionHandler != NULL)
      MidasExceptionHandler();

   raise(sig);

   return 0;
}

/*
INT _matherr(struct _exception *except)
{
  if (MidasExceptionHandler != NULL)
    MidasExceptionHandler();

  return 0;
}
*/

#endif                          /* OS_WINNT */

#ifdef OS_VMS

INT MidasExceptionFilter(INT * sigargs, INT * mechargs)
{
   if (MidasExceptionHandler != NULL)
      MidasExceptionHandler();

   return (SS$_RESIGNAL);
}

void MidasExceptionSignal(INT sig)
{
   if (MidasExceptionHandler != NULL)
      MidasExceptionHandler();

   kill(getpid(), sig);
}

#endif                          /* OS_VMS */

/*------------------------------------------------------------------*/
INT ss_exception_handler(void (*func) (void))
/********************************************************************\

  Routine: ss_exception_handler

  Purpose: Establish new exception handler which is called before
     the program is aborted due to a Ctrl-Break or an access
     violation. This handler may clean up things which may
     otherwise left in an undefined state.

  Input:
    void  (*func)()     Address of handler function
  Output:
    none

  Function value:
    BM_SUCCESS          Successful completion

\********************************************************************/
{
#ifdef OS_WINNT

   MidasExceptionHandler = func;
/*  SetUnhandledExceptionFilter(
    (LPTOP_LEVEL_EXCEPTION_FILTER) MidasExceptionFilter);

  signal(SIGINT, MidasExceptionSignal);
  signal(SIGILL, MidasExceptionSignal);
  signal(SIGFPE, MidasExceptionSignal);
  signal(SIGSEGV, MidasExceptionSignal);
  signal(SIGTERM, MidasExceptionSignal);
  signal(SIGBREAK, MidasExceptionSignal);
  signal(SIGABRT, MidasExceptionSignal); */

#elif defined (OS_VMS)

   MidasExceptionHandler = func;
   lib$establish(MidasExceptionFilter);

   signal(SIGINT, MidasExceptionSignal);
   signal(SIGILL, MidasExceptionSignal);
   signal(SIGQUIT, MidasExceptionSignal);
   signal(SIGFPE, MidasExceptionSignal);
   signal(SIGSEGV, MidasExceptionSignal);
   signal(SIGTERM, MidasExceptionSignal);

#else                           /* OS_VMS */
#endif

   return SS_SUCCESS;
}

#endif                          /* LOCAL_ROUTINES */

/*------------------------------------------------------------------*/
void *ss_ctrlc_handler(void (*func) (int))
/********************************************************************\

  Routine: ss_ctrlc_handler

  Purpose: Establish new exception handler which is called before
     the program is aborted due to a Ctrl-Break. This handler may
     clean up things which may otherwise left in an undefined state.

  Input:
    void  (*func)(int)     Address of handler function, if NULL
                           install default handler

  Output:
    none

  Function value:
    same as signal()

\********************************************************************/
{
#ifdef OS_WINNT

   if (func == NULL) {
      signal(SIGBREAK, SIG_DFL);
      return signal(SIGINT, SIG_DFL);
   } else {
      signal(SIGBREAK, func);
      return signal(SIGINT, func);
   }
   return NULL;

#endif                          /* OS_WINNT */
#ifdef OS_VMS

   return signal(SIGINT, func);

#endif                          /* OS_WINNT */

#ifdef OS_UNIX

   if (func == NULL) {
      signal(SIGTERM, SIG_DFL);
      return (void *) signal(SIGINT, SIG_DFL);
   } else {
      signal(SIGTERM, func);
      return (void *) signal(SIGINT, func);
   }

#endif                          /* OS_UNIX */
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                  Suspend/resume functions                          *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/
/* globals */

/*
   The suspend structure is used in a multithread environment
   (multi thread server) where each thread may resume another thread.
   Since all threads share the same global memory, the ports and
   sockets for suspending and resuming must be stored in a array
   which keeps one entry for each thread.
*/

typedef struct suspend_struct {
   midas_thread_t thread_id = 0;
   INT ipc_recv_port = 0;
   INT ipc_recv_socket = 0;
   INT ipc_send_socket = 0;
   struct sockaddr_in bind_addr;
} SUSPEND_STRUCT;

static std::vector<SUSPEND_STRUCT*> _ss_suspend_vector;

static midas_thread_t _ss_odb_thread = 0;
static SUSPEND_STRUCT* _ss_suspend_odb = NULL;

static midas_thread_t _ss_listen_thread = 0;
static int _ss_server_listen_socket = 0; // mserver listening for connections
static int _ss_client_listen_socket = 0; // normal midas program listening for rpc connections for run transitions, etc

static midas_thread_t _ss_client_thread = 0;
static RPC_SERVER_CONNECTION* _ss_client_connection = NULL; // client-side connection to the mserver

static midas_thread_t _ss_server_thread = 0;
static RPC_SERVER_ACCEPTION_LIST* _ss_server_acceptions = NULL; // server side RPC connections (run transitions, etc)

/*------------------------------------------------------------------*/
static bool ss_match_thread(midas_thread_t tid1, midas_thread_t tid2)
{
   if (tid1 == 0)
      return true;
   if (tid1 == tid2)
      return true;
   return false;
}

INT ss_suspend_set_rpc_thread(midas_thread_t thread_id)
{
   _ss_listen_thread = thread_id; // this thread handles listen()/accept() activity
   _ss_client_thread = thread_id; // this thread reads the mserver connection, handles ODB and event buffer notifications (db_watch->db_update_record_local(), bm_poll_event())
   _ss_server_thread = thread_id; // this thread reads and executes RPC requests
   _ss_odb_thread = thread_id; // this thread reads and dispatches ODB notifications (db_watch & co)
   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
static INT ss_suspend_init_struct(SUSPEND_STRUCT* psuspend)
/********************************************************************\

  Routine: ss_suspend_init_struct

  Purpose: Create sockets used in the suspend/resume mechanism.

  Input:
    SUSPEND_STRUCT* psuspend structure to initialize

  Function value:
    SS_SUCCESS              Successful completion
    SS_SOCKET_ERROR         Error in socket routines
    SS_NO_MEMORY            Not enough memory

\********************************************************************/
{
   INT status, sock;
   unsigned int size;
   struct sockaddr_in bind_addr;
   //int udp_bind_hostname = 0; // bind to localhost or bind to hostname or bind to INADDR_ANY?

   //printf("ss_suspend_init_struct: thread %s\n", ss_tid_to_string(psuspend->thread_id).c_str());

   assert(psuspend->thread_id != 0);

#ifdef OS_WINNT
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return SS_SOCKET_ERROR;
   }
#endif

  /*--------------- create UDP receive socket -------------------*/
   sock = socket(AF_INET, SOCK_DGRAM, 0);
   if (sock == -1)
      return SS_SOCKET_ERROR;

   /* let OS choose port for socket */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = 0;
   bind_addr.sin_port = 0;

   /* decide if UDP sockets are bound to localhost (they are only use for local communications)
      or to hostname (for compatibility with old clients - their hotlinks will not work) */
   {
      std::string path = cm_get_path();
      path += ".UDP_BIND_HOSTNAME";

      //cm_msg(MERROR, "ss_suspend_init_ipc", "check file [%s]", path.c_str());

      FILE *fp = fopen(path.c_str(), "r");
      if (fp) {
         cm_msg(MERROR, "ss_suspend_init_ipc", "Support for UDP_BIND_HOSTNAME was removed. Please delete file \"%s\"", path.c_str());
         //udp_bind_hostname = 1;
         fclose(fp);
         fp = NULL;
      }
   }

   //#ifdef OS_VXWORKS
   //{
   //   char local_host_name[HOST_NAME_LENGTH];
   //   INT host_addr;
   //
   //   gethostname(local_host_name, sizeof(local_host_name));
   //
   //host_addr = hostGetByName(local_host_name);
   //   memcpy((char *) &(bind_addr.sin_addr), &host_addr, 4);
   //}
   //#else
   //if (udp_bind_hostname) {
   //   char local_host_name[HOST_NAME_LENGTH];
   //   struct hostent *phe = gethostbyname(local_host_name);
   //   if (phe == NULL) {
   //      cm_msg(MERROR, "ss_suspend_init_ipc", "cannot get IP address for host name \'%s\'", local_host_name);
   //      return SS_SOCKET_ERROR;
   //   }
   //   memcpy((char *) &(bind_addr.sin_addr), phe->h_addr, phe->h_length);
   //} else {
   bind_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   //}
   //#endif

   status = bind(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
   if (status < 0)
      return SS_SOCKET_ERROR;

   /* find out which port OS has chosen */
   size = sizeof(bind_addr);
#ifdef OS_WINNT
   getsockname(sock, (struct sockaddr *) &bind_addr, (int *) &size);
#else
   getsockname(sock, (struct sockaddr *) &bind_addr, &size);
#endif

   // ipc receive socket must be set to non-blocking mode, see explanation
   // in ss_suspend_process_ipc(). K.O. July 2022.

   int flags = fcntl(sock, F_GETFL, 0);
   status = fcntl(sock, F_SETFL, flags | O_NONBLOCK);

   if (status < 0) {
      fprintf(stderr, "ss_suspend_init_struct: cannot set non-blocking mode of ipc receive socket, fcntl() returned %d, errno %d (%s)\n", status, errno, strerror(errno));
      return SS_SOCKET_ERROR;
   }

   psuspend->ipc_recv_socket = sock;
   psuspend->ipc_recv_port = ntohs(bind_addr.sin_port);

  /*--------------- create UDP send socket ----------------------*/
   sock = socket(AF_INET, SOCK_DGRAM, 0);

   if (sock == -1)
      return SS_SOCKET_ERROR;

   /* fill out bind struct pointing to local host */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = 0;

   //#ifdef OS_VXWORKS
   //{
   //   INT host_addr;
   //
   //   host_addr = hostGetByName(local_host_name);
   //memcpy((char *) &(bind_addr.sin_addr), &host_addr, 4);
   //}
   //#else
   //if (udp_bind_hostname) {
   //   // nothing
   //} else {
   bind_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   
   status = bind(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
   if (status < 0)
      return SS_SOCKET_ERROR;
   //}
   //#endif

   memcpy(&(psuspend->bind_addr), &bind_addr, sizeof(bind_addr));
   psuspend->ipc_send_socket = sock;

   //printf("ss_suspend_init_struct: thread %s, udp port %d\n", ss_tid_to_string(psuspend->thread_id).c_str(), psuspend->ipc_recv_port);

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
SUSPEND_STRUCT* ss_suspend_get_struct(midas_thread_t thread_id)
/********************************************************************\

  Routine: ss_suspend_get_struct

  Purpose: Return the suspend structure for this thread.

  Input:
    midas_thread_t thread_id thread is returned by ss_gettid()

  Function value:
    SUSPEND_STRUCT for the given thread

\********************************************************************/
{
   // find thread_id
   for (unsigned i=0; i<_ss_suspend_vector.size(); i++) {
      if (!_ss_suspend_vector[i])
         continue;
      if (_ss_suspend_vector[i]->thread_id == thread_id) {
         return _ss_suspend_vector[i];
      }
   }

   // create new one if not found
   SUSPEND_STRUCT *psuspend = new SUSPEND_STRUCT;
   psuspend->thread_id = thread_id;

   // place into empty slot
   for (unsigned i=0; i<_ss_suspend_vector.size(); i++) {
      if (!_ss_suspend_vector[i]) {
         _ss_suspend_vector[i] = psuspend;
         return psuspend;
      }
   }

   // add to vector if no empty slots
   _ss_suspend_vector.push_back(psuspend);

   return psuspend;
}

static void ss_suspend_close(SUSPEND_STRUCT* psuspend)
{
   if (psuspend->ipc_recv_socket) {
      closesocket(psuspend->ipc_recv_socket);
      psuspend->ipc_recv_socket = 0;
   }

   if (psuspend->ipc_send_socket) {
      closesocket(psuspend->ipc_send_socket);
      psuspend->ipc_send_socket = 0;
   }
   
   //printf("ss_suspend_close: free thread %s, udp port %d\n", ss_tid_to_string(psuspend->thread_id).c_str(), psuspend->ipc_recv_port);

   psuspend->thread_id = 0;
   psuspend->ipc_recv_port = 0;
}

/*------------------------------------------------------------------*/
INT ss_suspend_exit()
/********************************************************************\

  Routine: ss_suspend_exit

  Purpose: Closes the sockets used in the suspend/resume mechanism.
     Should be called before a thread exits.

  Input:
    none

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion

\********************************************************************/
{
   midas_thread_t thread_id = ss_gettid();
   
   for (unsigned i=0; i<_ss_suspend_vector.size(); i++) {
      if (!_ss_suspend_vector[i])
         continue;
      if (_ss_suspend_vector[i]->thread_id == thread_id) {
         SUSPEND_STRUCT* psuspend = _ss_suspend_vector[i];
         _ss_suspend_vector[i] = NULL;
         ss_suspend_close(psuspend);
         delete psuspend;
      }
   }

   if (_ss_suspend_odb) {
      bool last = true;
      for (unsigned i=0; i<_ss_suspend_vector.size(); i++) {
         if (_ss_suspend_vector[i]) {
            last = false;
            break;
         }
      }
      if (last) {
         SUSPEND_STRUCT* psuspend = _ss_suspend_odb;
         _ss_suspend_odb = NULL;
         ss_suspend_close(psuspend);
         delete psuspend;
      }
   }

   return SS_SUCCESS;
}

INT ss_suspend_set_server_listener(int listen_socket)
{
   // mserver listener socket
   _ss_server_listen_socket = listen_socket;
   return SS_SUCCESS;
}

INT ss_suspend_set_client_listener(int listen_socket)
{
   // midas program rpc listener socket (run transitions, etc)
   _ss_client_listen_socket = listen_socket;
   return SS_SUCCESS;
}

INT ss_suspend_set_client_connection(RPC_SERVER_CONNECTION* connection)
{
   // client side of the mserver connection
   _ss_client_connection = connection;
   return SS_SUCCESS;
}

INT ss_suspend_set_server_acceptions(RPC_SERVER_ACCEPTION_LIST* acceptions)
{
   // server side of the RPC connections (run transitions, etc)
   _ss_server_acceptions = acceptions;
   return SS_SUCCESS;
}

INT ss_suspend_init_odb_port()
/********************************************************************\

  Routine: ss_suspend_init_odb_port

  Purpose: Setup UDP port to receive ODB notifications (db_watch & co)

  Function value:
    SS_SUCCESS              Successful completion

\********************************************************************/
{
   if (!_ss_suspend_odb) {
      _ss_suspend_odb = new SUSPEND_STRUCT;
      _ss_suspend_odb->thread_id = ss_gettid();
      ss_suspend_init_struct(_ss_suspend_odb);
   }

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_suspend_get_odb_port(INT * port)
/********************************************************************\

  Routine: ss_suspend_get_odb_port

  Purpose: Return the UDP port number for receiving ODB notifications (db_watch & co)

  Input:
    none

  Output:
    INT    *port            UDP port number

  Function value:
    SS_SUCCESS              Successful completion

\********************************************************************/
{
   assert(_ss_suspend_odb);

   *port = _ss_suspend_odb->ipc_recv_port;

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_suspend_get_buffer_port(midas_thread_t thread_id, INT * port)
/********************************************************************\

  Routine: ss_suspend_get_buffer_port

  Purpose: Return the UDP port number which can be used to resume
     the calling thread inside a ss_suspend function. The port
     number can then be used by another process as a para-
     meter to the ss_resume function to resume the thread
     which called ss_suspend.

  Input:
    none

  Output:
    INT    *port            UDP port number

  Function value:
    SS_SUCCESS              Successful completion

\********************************************************************/
{
   SUSPEND_STRUCT* psuspend = ss_suspend_get_struct(thread_id);

   if (!psuspend->ipc_recv_port) {
      ss_suspend_init_struct(psuspend);
   }

   *port = psuspend->ipc_recv_port;

   return SS_SUCCESS;
}

static int ss_suspend_process_ipc(INT millisec, INT msg, int ipc_recv_socket)
{
   char buffer[80];
   buffer[0] = 0;
   /* receive IPC message */
   struct sockaddr from_addr;
   socklen_t from_addr_size = sizeof(struct sockaddr);

   // note: ipc_recv_socket must be set in non-blocking mode:
   // it looks as if we come here from ss_suspend() only if select() said
   // that our socket has data. but this is not true. after that select(),
   // ss_suspend() reads other sockets, calls other handlers, which may call
   // ss_suspend() recursively (i.e. via bm_receive_event() RPC call to "wait_for_more_data"
   // call to ss_suspend()). the recursively called ss_suspend() will
   // also so select() and call this function to read from this socket. then it eventually
   // returns, all the handlers return back to the original ss_suspend(), which
   // happily remembers that the original select() told us we have data. but this data
   // was already read by the recursively call ss_suspend(), so the socket is empty
   // and our recvfrom() will sleep forever. inside the mserver, this makes mserver
   // stop (very bad!). with the socket set to non-blocking mode
   // recvfrom() will never sleep and this problem is avoided. K.O. July 2022
   // see bug report https://bitbucket.org/tmidas/midas/issues/346/rpc-timeout-in-bm_receive_event

   // note2: in midas, there is never a situation where we wait for data
   // from the ipc sockets. these sockets are used for "event buffer has data" and "odb has new data"
   // notifications. we check them, but we do not wait for them. this setting
   // the socket to non-blocking mode is safe. K.O. July 2022.

   ssize_t size = recvfrom(ipc_recv_socket, buffer, sizeof(buffer), 0, &from_addr, &from_addr_size);

   if (size <= 0) {
      //fprintf(stderr, "ss_suspend_process_ipc: recvfrom() returned %zd, errno %d (%s)\n", size, errno, strerror(errno));
      // return 0 means we did not do anyting. K.O.
      return 0;
   }

   // NB: ss_suspend(MSG_BM) (and ss_suspend(MSG_ODB)) are needed to break
   // recursive calls to the event handler (and db_watch() handler) if these
   // handlers call ss_suspend() again. The rootana interactive ROOT graphics
   // mode does this. To prevent this recursion, event handlers must always
   // call ss_suspend() with MSG_BM (and MSG_ODB). K.O.
   
   /* return if received requested message */
   if (msg == MSG_BM && buffer[0] == 'B')
      return SS_SUCCESS;
   if (msg == MSG_ODB && buffer[0] == 'O')
      return SS_SUCCESS;
   
   // NB: do not need to check thread id, the mserver is single-threaded. K.O.
   int mserver_client_socket = 0;
   if (_ss_server_acceptions) {
      for (unsigned i = 0; i < _ss_server_acceptions->size(); i++) {
         if ((*_ss_server_acceptions)[i]->is_mserver) {
            mserver_client_socket = (*_ss_server_acceptions)[i]->send_sock;
         }
      }
   }
   
   time_t tstart = time(NULL);
   int return_status = 0;

   /* receive further messages to empty UDP queue */
   while (1) {
      char buffer_tmp[80];
      buffer_tmp[0] = 0;
      from_addr_size = sizeof(struct sockaddr);

      // note: ipc_recv_socket must be in non-blocking mode, see comments above. K.O.

      ssize_t size_tmp = recvfrom(ipc_recv_socket, buffer_tmp, sizeof(buffer_tmp), 0, &from_addr, &from_addr_size);

      if (size_tmp <= 0) {
         //fprintf(stderr, "ss_suspend_process_ipc: second recvfrom() returned %zd, errno %d (%s)\n", size, errno, strerror(errno));
         break;
      }
      
      /* stop the loop if received requested message */
      if (msg == MSG_BM && buffer_tmp[0] == 'B') {
         return_status = SS_SUCCESS;
         break;
      }
      if (msg == MSG_ODB && buffer_tmp[0] == 'O') {
         return_status = SS_SUCCESS;
         break;
      }
      
      /* don't forward same MSG_BM as above */
      if (buffer_tmp[0] != 'B' || strcmp(buffer_tmp, buffer) != 0) {
         cm_dispatch_ipc(buffer_tmp, size_tmp, mserver_client_socket);
      }
      
      if (millisec > 0) {
         time_t tnow = time(NULL);
         // make sure we do not loop for longer than our timeout
         if (tnow - tstart > 1 + millisec/1000) {
            //printf("ss_suspend - break out dt %d, %d loops\n", (int)(tnow-tstart), count);
            break;
         }
      }
   }
   
   /* call dispatcher */
   cm_dispatch_ipc(buffer, size, mserver_client_socket);
   
   return return_status;
}

static int ss_socket_check(int sock)
{
   // copied from the old rpc_server_receive()

   /* only check if TCP connection is broken */

   char test_buffer[256];
#ifdef OS_WINNT
   int n_received = recv(sock, test_buffer, sizeof(test_buffer), MSG_PEEK);
#else
   int n_received = recv(sock, test_buffer, sizeof(test_buffer), MSG_PEEK | MSG_DONTWAIT);
   
   /* check if we caught a signal */
   if ((n_received == -1) && (errno == EAGAIN))
      return SS_SUCCESS;
#endif
   
   if (n_received == -1) {
      cm_msg(MERROR, "ss_socket_check", "recv(%d,MSG_PEEK) returned %d, errno: %d (%s)", (int) sizeof(test_buffer), n_received, errno, strerror(errno));
   }
   
   if (n_received <= 0)
      return SS_ABORT;
   
   return SS_SUCCESS;
}

bool ss_event_socket_has_data()
{
   if (_ss_server_acceptions) {
      for (unsigned i = 0; i < _ss_server_acceptions->size(); i++) {
         /* event channel */
         int sock = (*_ss_server_acceptions)[i]->event_sock;

         if (!sock)
            continue;

         /* check for buffered event */
         int status = ss_socket_wait(sock, 1);

         if (status == SS_SUCCESS)
            return true;
      }
   }
   
   /* no event socket or no data in event socket */
   return false;
}

/*------------------------------------------------------------------*/
INT ss_suspend(INT millisec, INT msg)
/********************************************************************\

  Routine: ss_suspend

  Purpose: Suspend the calling thread for a specified time. If
     timeout (in millisec.) is negative, the thead is suspended
     indefinitely. It can only be resumed from another thread
     or process which calls ss_resume or by some data which
     arrives on the client or server sockets.

     If msg equals to one of MSG_BM, MSG_ODB, the function
     return whenever such a message is received. This is needed
     to break recursive calls to the event handler and db_watch() handler:

     Avoided recursion via ss_suspend(MSG_BM):

     ss_suspend(0) ->
     -> MSG_BM message arrives in the UDP socket
     -> ss_suspend_process_ipc()
     -> cm_dispatch_ipc()
     -> bm_push_event()
     -> bm_push_buffer()
     -> bm_read_buffer()
     -> bm_wait_for_more_events()
     -> ss_suspend(MSG_BM) <- event buffer code calls ss_suspend() with MSG_BM set
     -> MSG_BM arrives arrives in the UDP socket
     -> ss_suspend_process_ipc(MSG_BM)
     -> the newly arrived MSG_BM message is discarded,
        recursive call to cm_dispatch_ipc(), bm_push_buffer() & co avoided

     Incorrect recursion via the event handler where user called ss_suspend() without MSG_BM:

     analyzer ->
     -> cm_yield() in the main loop
     -> ss_suspend(0)
     -> MSG_BM message arrives in the UDP socket
     -> ss_suspend_process_ipc(0)
     -> cm_dispatch_ipc()
     -> bm_push_event()
     -> bm_push_buffer()
     -> bm_read_buffer()
     -> bm_dispatch_event()
     -> user event handler
     -> user event handler ROOT graphics main loop needs to sleep
     -> ss_suspend(0) <--- should be ss_suspend(MSG_BM)!!!     
     -> MSG_BM message arrives in the UDP socket
     -> ss_suspend_process_ipc(0) <- should be ss_suspend_process_ipc(MSG_BM)!!!
     -> cm_dispatch_ipc() <- without MSG_BM, calling cm_dispatch_ipc() again
     -> bm_push_event()
     -> bm_push_buffer()
     -> bm_read_buffer()
     -> bm_dispatch_event()
     -> user event handler <---- called recursively, very bad!

  Input:
    INT    millisec         Timeout in milliseconds
    INT    msg              Return from ss_suspend when msg (MSG_BM, MSG_ODB) is received.

  Output:
    none

  Function value:
    SS_SUCCESS              Requested message was received
    SS_TIMEOUT              Timeout expired
    SS_SERVER_RECV          Server channel got data
    SS_CLIENT_RECV          Client channel got data
    SS_ABORT (RPC_ABORT)    Connection lost
    SS_EXIT                 Connection closed

\********************************************************************/
{
   INT status, return_status;

   midas_thread_t thread_id = ss_gettid();

   SUSPEND_STRUCT* psuspend = ss_suspend_get_struct(thread_id);

   //printf("ss_suspend: thread %s\n", ss_tid_to_string(thread_id).c_str());

   return_status = SS_TIMEOUT;

   do {
      fd_set readfds;
      FD_ZERO(&readfds);

      if (ss_match_thread(_ss_listen_thread, thread_id)) {
         /* check listen sockets */
         if (_ss_server_listen_socket) {
            FD_SET(_ss_server_listen_socket, &readfds);
            //printf("ss_suspend: thread %s listen ss_server socket %d\n", ss_tid_to_string(thread_id).c_str(), _ss_server_listen_socket);
         }
         
         if (_ss_client_listen_socket) {
            FD_SET(_ss_client_listen_socket, &readfds);
            //printf("ss_suspend: thread %s listen ss_client socket %d\n", ss_tid_to_string(thread_id).c_str(), _ss_client_listen_socket);
         }
      }

      /* check server channels */
      if (ss_match_thread(_ss_server_thread, thread_id) && _ss_server_acceptions) {
         //printf("ss_suspend: thread %s server acceptions %d\n", ss_tid_to_string(thread_id).c_str(), _ss_server_num_acceptions);
         for (unsigned i = 0; i < _ss_server_acceptions->size(); i++) {
            /* RPC channel */
            int sock = (*_ss_server_acceptions)[i]->recv_sock;

            if (!sock)
               continue;
            
            ///* only watch the event tcp connection belonging to this thread */
            //if (_suspend_struct[idx].server_acception[i].tid != ss_gettid())
            //   continue;

            /* watch server socket if no data in cache */
            if (recv_tcp_check(sock) == 0)
               FD_SET(sock, &readfds);
            /* set timeout to zero if data in cache (-> just quick check IPC)
               and not called from inside bm_send_event (-> wait for IPC) */
            else if (msg == 0)
               millisec = 0;

            if (msg == 0 && msg != MSG_BM) {
               /* event channel */
               sock = (*_ss_server_acceptions)[i]->event_sock;

               if (!sock)
                  continue;

               /* check for buffered event */
               status = rpc_server_receive_event(0, NULL, BM_NO_WAIT);

               if (status == BM_ASYNC_RETURN) {
                  /* event buffer is full and rpc_server_receive_event() is holding on
                   * to an event it cannot get rid of. Do not read more events from
                   * the event socket, they have nowhere to go. K.O. */
               } else if (status == RPC_SUCCESS) {
                  FD_SET(sock, &readfds);
               }
            }
         }
      }

      /* watch for messages from the mserver */
      if (ss_match_thread(_ss_client_thread, thread_id)) {
         if (_ss_client_connection) {
            FD_SET(_ss_client_connection->recv_sock, &readfds);
         }
      }

      /* watch for UDP messages in the IPC socket: buffer and odb notifications */
      if (ss_match_thread(_ss_odb_thread, thread_id)) {
         if (_ss_suspend_odb && _ss_suspend_odb->ipc_recv_socket)
            FD_SET(_ss_suspend_odb->ipc_recv_socket, &readfds);
      }

      if (psuspend->ipc_recv_socket)
         FD_SET(psuspend->ipc_recv_socket, &readfds);

      struct timeval timeout;

      timeout.tv_sec = millisec / 1000;
      timeout.tv_usec = (millisec % 1000) * 1000;

      do {
         //printf("select millisec %d, tv_sec %d, tv_usec %d\n", millisec, (int)timeout.tv_sec, (int)timeout.tv_usec);

         if (millisec < 0)
            status = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);    /* blocking */
         else
            status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

         /* if an alarm signal was cought, restart select with reduced timeout */
         if (status == -1 && timeout.tv_sec >= WATCHDOG_INTERVAL / 1000)
            timeout.tv_sec -= WATCHDOG_INTERVAL / 1000;

      } while (status == -1);   /* dont return if an alarm signal was cought */

      /* check listener sockets */

      if (_ss_server_listen_socket && FD_ISSET(_ss_server_listen_socket, &readfds)) {
         //printf("ss_suspend: thread %s rpc_server_accept socket %d\n", ss_tid_to_string(thread_id).c_str(), _ss_server_listen_socket);
         status = rpc_server_accept(_ss_server_listen_socket);
         if (status == RPC_SHUTDOWN) {
            return status;
         }
      }

      if (_ss_client_listen_socket && FD_ISSET(_ss_client_listen_socket, &readfds)) {
         //printf("ss_suspend: thread %s rpc_client_accept socket %d\n", ss_tid_to_string(thread_id).c_str(), _ss_client_listen_socket);
         status = rpc_client_accept(_ss_client_listen_socket);
         if (status == RPC_SHUTDOWN) {
            return status;
         }
      }

      /* check server channels */
      if (_ss_server_acceptions) {
         for (unsigned i = 0; i < _ss_server_acceptions->size(); i++) {
            /* rpc channel */
            int sock = (*_ss_server_acceptions)[i]->recv_sock;

            if (!sock)
               continue;
            
            //printf("rpc index %d, socket %d, hostname \'%s\', progname \'%s\'\n", i, sock, _suspend_struct[idx].server_acception[i].host_name, _suspend_struct[idx].server_acception[i].prog_name);

            if (recv_tcp_check(sock) || FD_ISSET(sock, &readfds)) {
               //printf("ss_suspend: msg %d\n", msg);
               if (msg == MSG_BM) {
                  status = ss_socket_check(sock);
               } else {
                  //printf("ss_suspend: rpc_server_receive_rpc() call!\n");
                  status = rpc_server_receive_rpc(i, (*_ss_server_acceptions)[i]);
                  //printf("ss_suspend: rpc_server_receive_rpc() status %d\n", status);
               }
               (*_ss_server_acceptions)[i]->last_activity = ss_millitime();

               if (status == SS_ABORT || status == SS_EXIT || status == RPC_SHUTDOWN) {
                  return status;
               }
               
               return_status = SS_SERVER_RECV;
            }

            /* event channel */
            sock = (*_ss_server_acceptions)[i]->event_sock;

            if (!sock)
               continue;

            if (FD_ISSET(sock, &readfds)) {
               if (msg != 0) {
                  status = ss_socket_check(sock);
               } else {
                  //printf("ss_suspend: rpc_server_receive_event() call!\n");
                  status = rpc_server_receive_event(i, (*_ss_server_acceptions)[i], BM_NO_WAIT);
                  //printf("ss_suspend: rpc_server_receive_event() status %d\n", status);
               }
               (*_ss_server_acceptions)[i]->last_activity = ss_millitime();

               if (status == SS_ABORT || status == SS_EXIT || status == RPC_SHUTDOWN) {
                  return status;
               }
               
               return_status = SS_SERVER_RECV;
            }
         }
      }

      /* check for messages from the mserver */
      if (_ss_client_connection) {
         int sock = _ss_client_connection->recv_sock;

         if (FD_ISSET(sock, &readfds)) {
            status = rpc_client_dispatch(sock);

            if (status == SS_ABORT) {
               cm_msg(MINFO, "ss_suspend", "RPC connection to mserver at \'%s\' was broken", _ss_client_connection->host_name.c_str());

               /* close client connection if link broken */
               closesocket(_ss_client_connection->send_sock);
               closesocket(_ss_client_connection->recv_sock);
               closesocket(_ss_client_connection->event_sock);

               _ss_client_connection->send_sock = 0;
               _ss_client_connection->recv_sock = 0;
               _ss_client_connection->event_sock = 0;
            
               _ss_client_connection->clear();

               /* exit program after broken connection to MIDAS server */
               return SS_ABORT;
            }

            return_status = SS_CLIENT_RECV;
         }
      }

      /* check ODB IPC socket */
      if (_ss_suspend_odb && _ss_suspend_odb->ipc_recv_socket && FD_ISSET(_ss_suspend_odb->ipc_recv_socket, &readfds)) {
         status = ss_suspend_process_ipc(millisec, msg, _ss_suspend_odb->ipc_recv_socket);
         if (status) {
            return status;
         }
      }
      
      /* check per-thread IPC socket */
      if (psuspend && psuspend->ipc_recv_socket && FD_ISSET(psuspend->ipc_recv_socket, &readfds)) {
         status = ss_suspend_process_ipc(millisec, msg, psuspend->ipc_recv_socket);
         if (status) {
            return status;
         }
      }


   } while (millisec < 0);

   return return_status;
}

/*------------------------------------------------------------------*/
INT ss_resume(INT port, const char *message)
/********************************************************************\

  Routine: ss_resume

  Purpose: Resume another thread or process which called ss_suspend.
     The port has to be transfered (shared memory or so) from
     the thread or process which should be resumed. In that
     process it can be obtained via ss_suspend_get_port.

  Input:
    INT    port             UDP port number
    INT    msg              Mesage id & parameter transferred to
    INT    param              target process

  Output:
    none

  Function value:
    SS_SUCCESS              Successful completion
    SS_SOCKET_ERROR         Socket error

\********************************************************************/
{
   assert(_ss_suspend_odb);

   struct sockaddr_in bind_addr;

   memcpy(&bind_addr, &_ss_suspend_odb->bind_addr, sizeof(struct sockaddr_in));
   bind_addr.sin_port = htons((short) port);

   size_t message_size = strlen(message) + 1;

   ssize_t wr = sendto(_ss_suspend_odb->ipc_send_socket, message, message_size, 0, (struct sockaddr *) &bind_addr, sizeof(struct sockaddr_in));

   if (wr < 0) {
      return SS_SOCKET_ERROR;
   }

   if (((size_t)wr) != message_size) {
      return SS_SOCKET_ERROR;
   }

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                     Network functions                              *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/
int ss_socket_wait(int sock, INT millisec)
/********************************************************************\

  Routine: ss_socket_wait

  Purpose: Wait for data available to read from a socket

  Input:
    INT   sock               Socket which was previosly opened.
    INT   millisec           Timeout in ms

  Function value:
    SS_SUCCESS               Data is available
    SS_TIMEOUT               Timeout
    SS_SOCKET_ERROR          Error

\********************************************************************/
{
   INT status;
   fd_set readfds;
   struct timeval timeout;
   struct timeval timeout0;
   DWORD start_time = 0; // start_time is only used for BSD select() behaviour (MacOS)
   DWORD end_time = 0;

   FD_ZERO(&readfds);
   FD_SET(sock, &readfds);

   timeout.tv_sec = millisec / 1000;
   timeout.tv_usec = (millisec % 1000) * 1000;

   timeout0 = timeout;

   while (1) {
      status = select(sock+1, &readfds, NULL, NULL, &timeout);
      //printf("ss_socket_wait: millisec %d, tv_sec %d, tv_usec %d, isset %d, status %d, errno %d (%s)\n", millisec, timeout.tv_sec, timeout.tv_usec, FD_ISSET(sock, &readfds), status, errno, strerror(errno));

#ifndef OS_WINNT
      if (status<0 && errno==EINTR) { /* watchdog alarm signal */
         /* need to determine if select() updates "timeout" (Linux) or keeps original value (BSD) */
         if (timeout.tv_sec == timeout0.tv_sec) {
            DWORD now = ss_time();
            if (start_time == 0) {
               start_time = now;
               end_time = start_time + (millisec+999)/1000;
            }
            //printf("ss_socket_wait: EINTR: now %d, timeout %d, wait time %d\n", now, end_time, end_time - now);
            if (now > end_time)
               return SS_TIMEOUT;
         }
         continue;
      }
#endif
      if (status < 0) { /* select() syscall error */
         cm_msg(MERROR, "ss_socket_wait", "unexpected error, select() returned %d, errno: %d (%s)", status, errno, strerror(errno));
         return SS_SOCKET_ERROR;
      }
      if (status == 0) /* timeout */
         return SS_TIMEOUT;
      if (!FD_ISSET(sock, &readfds))
         return SS_TIMEOUT;
      return SS_SUCCESS;
   }
   /* NOT REACHED */
}

static bool gSocketTrace = false;

/*------------------------------------------------------------------*/
INT ss_socket_connect_tcp(const char* hostname, int tcp_port, int* sockp, std::string* error_msg_p)
{
   assert(sockp != NULL);
   assert(error_msg_p != NULL);
   *sockp = 0;

#ifdef OS_WINNT
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return RPC_NET_ERROR;
   }
#endif

   char portname[256];
   sprintf(portname, "%d", tcp_port);

   struct addrinfo *ainfo = NULL;

   int status = getaddrinfo(hostname, portname, NULL, &ainfo);

   if (status != 0) {
      *error_msg_p = msprintf("cannot resolve hostname \"%s\", getaddrinfo() error %d (%s)", hostname, status, gai_strerror(status));
      if (ainfo)
         freeaddrinfo(ainfo);
      return RPC_NET_ERROR;
   }

   // NOTE: ainfo must be freeed using freeaddrinfo(ainfo);

   int sock = 0;

   for (const struct addrinfo *r = ainfo; r != NULL; r = r->ai_next) {
      if (gSocketTrace) {
         fprintf(stderr, "ss_socket_connect_tcp: hostname [%s] port %d addrinfo: flags %d, family %d, socktype %d, protocol %d, canonname [%s]\n",
                 hostname,
                 tcp_port,
                 r->ai_flags,
                 r->ai_family,
                 r->ai_socktype,
                 r->ai_protocol,
                 r->ai_canonname);
      }

      // skip anything but TCP addresses
      if (r->ai_socktype != SOCK_STREAM) {
         continue;
      }
      
      // skip anything but TCP protocol 6
      if (r->ai_protocol != 6) {
         continue;
      }
      
      sock = ::socket(r->ai_family, r->ai_socktype, 0);
      
      if (sock <= 0) {
         *error_msg_p = msprintf("cannot create socket, errno %d (%s)", errno, strerror(errno));
         continue;
      }
      
      status = ::connect(sock, r->ai_addr, r->ai_addrlen);
      if (status != 0) {
         if (gSocketTrace) {
            fprintf(stderr, "ss_socket_connect_tcp: connect() status %d, errno %d (%s)\n", status, errno, strerror(errno));
         }
         *error_msg_p = msprintf("cannot connect to host \"%s\" port %d, errno %d (%s)", hostname, tcp_port, errno, strerror(errno));
         ::close(sock);
         sock = 0;
         continue;
      }
      // successfully connected
      break;
   }
   
   freeaddrinfo(ainfo);
   ainfo = NULL;

   if (sock == 0) {
      // error_msg is already set
      return RPC_NET_ERROR;
   }

   *sockp = sock;

   if (gSocketTrace) {
      fprintf(stderr, "ss_socket_connect_tcp: hostname [%s] port %d new socket %d\n", hostname, tcp_port, *sockp);
   }

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_socket_listen_tcp(bool listen_localhost, int tcp_port, int* sockp, int* tcp_port_p, std::string* error_msg_p)
{
   assert(sockp != NULL);
   assert(tcp_port_p != NULL);
   assert(error_msg_p != NULL);

   *sockp = 0;
   *tcp_port_p = 0;

#ifdef OS_WINNT
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return RPC_NET_ERROR;
   }
#endif

#ifdef AF_INET6
   bool use_inet6 = true;
#else
   bool use_inet6 = false;
#endif

   if (listen_localhost)
      use_inet6 = false;

   /* create a socket for listening */
   int lsock;
   if (use_inet6) {
#ifdef AF_INET6
      lsock = socket(AF_INET6, SOCK_STREAM, 0);
#endif
   } else {
      lsock = socket(AF_INET, SOCK_STREAM, 0);
   }

   if (lsock == -1) {
      *error_msg_p = msprintf("socket(AF_INET, SOCK_STREAM) failed, errno %d (%s)", errno, strerror(errno));
      return RPC_NET_ERROR;
   }

   /* reuse address, needed if previous server stopped (30s timeout!) */
   int flag = 1;
   int status = setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, (char *) &flag, sizeof(int));
   if (status < 0) {
      *error_msg_p = msprintf("setsockopt(SO_REUSEADDR) failed, errno %d (%s)", errno, strerror(errno));
      return RPC_NET_ERROR;
   }

#ifdef AF_INET6
#ifdef IPV6_V6ONLY
   if (use_inet6) {
      /* turn off IPV6_V6ONLY, see RFC 3493 */
      flag = 0;
      status = setsockopt(lsock, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &flag, sizeof(int));
      if (status < 0) {
         *error_msg_p = msprintf("setsockopt(IPPROTO_IPV6, IPV6_V6ONLY) failed, errno %d (%s)", errno, strerror(errno));
         return RPC_NET_ERROR;
      }
   }
#else
#warning strange: AF_INET6 is defined, but IPV6_V6ONLY is not defined
#endif
#endif

   if (use_inet6) {
#ifdef AF_INET6
      /* bind local node name and port to socket */
      struct sockaddr_in6 bind_addr6;
      memset(&bind_addr6, 0, sizeof(bind_addr6));
      bind_addr6.sin6_family = AF_INET6;
      
      if (listen_localhost) {
         bind_addr6.sin6_addr = in6addr_loopback;
      } else {
         bind_addr6.sin6_addr = in6addr_any;
      }

      if (tcp_port)
         bind_addr6.sin6_port = htons((short) tcp_port);
      else
         bind_addr6.sin6_port = htons(0); // OS will allocate a port number for us
      
      status = bind(lsock, (struct sockaddr *) &bind_addr6, sizeof(bind_addr6));
      if (status < 0) {
         *error_msg_p = msprintf("IPv6 bind() to port %d failed, errno %d (%s)", tcp_port, errno, strerror(errno));
         return RPC_NET_ERROR;
      }
#endif
   } else {
      /* bind local node name and port to socket */
      struct sockaddr_in bind_addr;
      memset(&bind_addr, 0, sizeof(bind_addr));
      bind_addr.sin_family = AF_INET;
      
      if (listen_localhost) {
         bind_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      } else {
         bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
      }
      
      if (tcp_port)
         bind_addr.sin_port = htons((short) tcp_port);
      else
         bind_addr.sin_port = htons(0); // OS will allocate a port number for us
      
      status = bind(lsock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
      if (status < 0) {
         *error_msg_p = msprintf("bind() to port %d failed, errno %d (%s)", tcp_port, errno, strerror(errno));
         return RPC_NET_ERROR;
      }
   }

   /* listen for connection */
#ifdef OS_MSDOS
   status = listen(lsock, 1);
#else
   status = listen(lsock, SOMAXCONN);
#endif
   if (status < 0) {
      *error_msg_p = msprintf("listen() failed, errno %d (%s)", errno, strerror(errno));
      return RPC_NET_ERROR;
   }

   if (use_inet6) {
#ifdef AF_INET6
      struct sockaddr_in6 addr;
      socklen_t sosize = sizeof(addr);
      status = getsockname(lsock, (struct sockaddr*)&addr, &sosize);
      if (status < 0) {
         *error_msg_p = msprintf("IPv6 getsockname() failed, errno %d (%s)", errno, strerror(errno));
         return RPC_NET_ERROR;
      }
      
      *tcp_port_p = ntohs(addr.sin6_port);
#endif
   } else {
      struct sockaddr_in addr;
      socklen_t sosize = sizeof(addr);
      status = getsockname(lsock, (struct sockaddr*)&addr, &sosize);
      if (status < 0) {
         *error_msg_p = msprintf("getsockname() failed, errno %d (%s)", errno, strerror(errno));
         return RPC_NET_ERROR;
      }
   
      *tcp_port_p = ntohs(addr.sin_port);
   }

   *sockp = lsock;

   if (gSocketTrace) {
      if (listen_localhost)
         fprintf(stderr, "ss_socket_listen_tcp: listening tcp port %d local connections only, new socket %d\n", *tcp_port_p, *sockp);
      else
         fprintf(stderr, "ss_socket_listen_tcp: listening tcp port %d all internet connections, socket %d\n", *tcp_port_p, *sockp);
   }

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_socket_close(int* sockp)
{
   assert(sockp != NULL);
   if (gSocketTrace) {
      fprintf(stderr, "ss_socket_close: %d\n", *sockp);
   }
   int err = close(*sockp);
   if (err) {
      cm_msg(MERROR, "ss_socket_close", "unexpected error, close() returned %d, errno: %d (%s)", err, errno, strerror(errno));
   }
   *sockp = 0;
   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_socket_get_peer_name(int sock, std::string* hostp, int* portp)
{
   char addr[64];

   unsigned size = sizeof(addr);
   int rv = getpeername(sock, (struct sockaddr *) &addr, &size);

   //printf("getpeername() returned %d, size %d, buffer %d\n", rv, size, (int)sizeof(addr));

   if (rv != 0) {
      cm_msg(MERROR, "ss_socket_get_peer_name", "Error: getpeername() returned %d, errno %d (%s)", rv, errno, strerror(errno));
      return SS_SOCKET_ERROR;
   }
   
   char hostname[256];
   char servname[16];

   int ret = getnameinfo((struct sockaddr*)&addr, size,
                         hostname, sizeof(hostname),
                         servname, sizeof(servname),
                         NI_NUMERICSERV);

   if (ret != 0) {
      cm_msg(MERROR, "ss_socket_get_peer_name", "Error: getnameinfo() error %d (%s)", ret, gai_strerror(ret));
      return SS_SOCKET_ERROR;
   }

   //printf("getnameinfo() returned %d, hostname [%s], servname[%s]\n", ret, hostname, servname);

   if (hostp)
      *hostp = hostname;

   if (portp)
      *portp = atoi(servname);

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT send_tcp(int sock, char *buffer, DWORD buffer_size, INT flags)
/********************************************************************\

  Routine: send_tcp

  Purpose: Send network data over TCP port. Break buffer in smaller
           parts if larger than maximum TCP buffer size (usually 64k).

  Input:
    INT   sock               Socket which was previosly opened.
    DWORD buffer_size        Size of the buffer in bytes.
    INT   flags              Flags passed to send()
                             0x10000 : do not send error message

  Output:
    char  *buffer            Network receive buffer.

  Function value:
    INT                     Same as send()

\********************************************************************/
{
   DWORD count;
   INT status;
   //int net_tcp_size = NET_TCP_SIZE;
   int net_tcp_size = 1024 * 1024;

   /* transfer fragments until complete buffer is transferred */

   for (count = 0; (INT) count < (INT) buffer_size - net_tcp_size;) {
      status = send(sock, buffer + count, net_tcp_size, flags & 0xFFFF);
      if (status != -1)
         count += status;
      else {
#ifdef OS_UNIX
         if (errno == EINTR)
            continue;
#endif
         if ((flags & 0x10000) == 0)
            cm_msg(MERROR, "send_tcp",
                   "send(socket=%d,size=%d) returned %d, errno: %d (%s)",
                   sock, net_tcp_size, status, errno, strerror(errno));
         return status;
      }
   }

   while (count < buffer_size) {
      status = send(sock, buffer + count, buffer_size - count, flags & 0xFFFF);
      if (status != -1)
         count += status;
      else {
#ifdef OS_UNIX
         if (errno == EINTR)
            continue;
#endif
         if ((flags & 0x10000) == 0)
            cm_msg(MERROR, "send_tcp",
                   "send(socket=%d,size=%d) returned %d, errno: %d (%s)",
                   sock, (int) (buffer_size - count), status, errno, strerror(errno));
         return status;
      }
   }

   return count;
}

/*------------------------------------------------------------------*/
INT ss_write_tcp(int sock, const char *buffer, size_t buffer_size)
/********************************************************************\

  Routine: write_tcp

  Purpose: Send network data over TCP port. Handle partial writes

  Input:
    INT   sock               Socket which was previosly opened.
    DWORD buffer_size        Size of the buffer in bytes.
    INT   flags              Flags passed to send()
                             0x10000 : do not send error message

  Output:
    char  *buffer            Network receive buffer.

  Function value:
    SS_SUCCESS               Everything was sent
    SS_SOCKET_ERROR          There was a socket error

\********************************************************************/
{
   size_t count = 0;

   while (count < buffer_size) {
      ssize_t wr = write(sock, buffer + count, buffer_size - count);

      if (wr == 0) {
         cm_msg(MERROR, "ss_write_tcp", "write(socket=%d,size=%d) returned zero, errno: %d (%s)", sock, (int) (buffer_size - count), errno, strerror(errno));
         return SS_SOCKET_ERROR;
      } else if (wr < 0) {
#ifdef OS_UNIX
         if (errno == EINTR)
            continue;
#endif
         cm_msg(MERROR, "ss_write_tcp", "write(socket=%d,size=%d) returned %d, errno: %d (%s)", sock, (int) (buffer_size - count), (int)wr, errno, strerror(errno));
         return SS_SOCKET_ERROR;
      }

      // good write
      count += wr;
   }

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT recv_string(int sock, char *buffer, DWORD buffer_size, INT millisec)
/********************************************************************\

  Routine: recv_string

  Purpose: Receive network data over TCP port. Since sockets are
     operated in stream mode, a single transmission via send
     may not transfer the full data. Therefore, one has to check
     at the receiver side if the full data is received. If not,
     one has to issue several recv() commands.

     The length of the data is determined by a trailing zero.

  Input:
    INT   sock               Socket which was previosly opened.
    DWORD buffer_size        Size of the buffer in bytes.
    INT   millisec           Timeout in ms

  Output:
    char  *buffer            Network receive buffer.

  Function value:
    INT                      String length

\********************************************************************/
{
   INT i, status;
   DWORD n;

   n = 0;
   memset(buffer, 0, buffer_size);

   do {
      if (millisec > 0) {
         status = ss_socket_wait(sock, millisec);
         if (status != SS_SUCCESS)
            break;
      }

      i = recv(sock, buffer + n, 1, 0);

      if (i <= 0)
         break;

      n++;

      if (n >= buffer_size)
         break;

   } while (buffer[n - 1] && buffer[n - 1] != 10);

   return n - 1;
}

/*------------------------------------------------------------------*/
INT recv_tcp(int sock, char *net_buffer, DWORD buffer_size, INT flags)
/********************************************************************\

  Routine: recv_tcp

  Purpose: Receive network data over TCP port. Since sockets are
     operated in stream mode, a single transmission via send
     may not transfer the full data. Therefore, one has to check
     at the receiver side if the full data is received. If not,
     one has to issue several recv() commands.

     The length of the data is determined by the data header,
     which consists of two DWORDs. The first is the command code
     (or function id), the second is the size of the following
     parameters in bytes. From that size recv_tcp() determines
     how much data to receive.

  Input:
    INT   sock               Socket which was previosly opened.
    char  *net_buffer        Buffer to store data to
    DWORD buffer_size        Size of the buffer in bytes.
    INT   flags              Flags passed to recv()

  Output:
    char  *buffer            Network receive buffer.

  Function value:
    INT                      Same as recv()

\********************************************************************/
{
   INT param_size, n_received, n;
   NET_COMMAND *nc;

   if (buffer_size < sizeof(NET_COMMAND_HEADER)) {
      cm_msg(MERROR, "recv_tcp", "parameters too large for network buffer");
      return -1;
   }

   /* first receive header */
   n_received = 0;
   do {
#ifdef OS_UNIX
      do {
         n = recv(sock, net_buffer + n_received, sizeof(NET_COMMAND_HEADER), flags);

         /* don't return if an alarm signal was cought */
      } while (n == -1 && errno == EINTR);
#else
      n = recv(sock, net_buffer + n_received, sizeof(NET_COMMAND_HEADER), flags);
#endif

      if (n == 0) {
         cm_msg(MERROR, "recv_tcp", "header: recv(%d) returned %d, n_received = %d, unexpected connection closure", (int)sizeof(NET_COMMAND_HEADER), n, n_received);
         return n;
      }

      if (n < 0) {
         cm_msg(MERROR, "recv_tcp", "header: recv(%d) returned %d, n_received = %d, errno: %d (%s)", (int)sizeof(NET_COMMAND_HEADER), n, n_received, errno, strerror(errno));
         return n;
      }

      n_received += n;

   } while (n_received < (int) sizeof(NET_COMMAND_HEADER));

   /* now receive parameters */

   nc = (NET_COMMAND *) net_buffer;
   param_size = nc->header.param_size;
   n_received = 0;

   if (param_size == 0)
      return sizeof(NET_COMMAND_HEADER);

   if (param_size > (INT)buffer_size) {
      cm_msg(MERROR, "recv_tcp", "param: receive buffer size %d is too small for received data size %d", buffer_size, param_size);
      return -1;
   }

   do {
#ifdef OS_UNIX
      do {
         n = recv(sock, net_buffer + sizeof(NET_COMMAND_HEADER) + n_received, param_size - n_received, flags);

         /* don't return if an alarm signal was cought */
      } while (n == -1 && errno == EINTR);
#else
      n = recv(sock, net_buffer + sizeof(NET_COMMAND_HEADER) + n_received, param_size - n_received, flags);
#endif

      if (n == 0) {
         cm_msg(MERROR, "recv_tcp", "param: recv() returned %d, n_received = %d, unexpected connection closure", n, n_received);
         return n;
      }

      if (n < 0) {
         cm_msg(MERROR, "recv_tcp", "param: recv() returned %d, n_received = %d, errno: %d (%s)", n, n_received, errno, strerror(errno));
         return n;
      }

      n_received += n;
   } while (n_received < param_size);

   return sizeof(NET_COMMAND_HEADER) + param_size;
}

/*------------------------------------------------------------------*/
INT recv_tcp2(int sock, char *net_buffer, int buffer_size, int timeout_ms)
/********************************************************************\

  Routine: recv_tcp2

  Purpose: Receive network data over TCP port. Since sockets are
     operated in stream mode, a single transmission via send
     may not transfer the full data. Therefore, one has to check
     at the receiver side if the full data is received. If not,
     one has to issue several recv() commands.

  Input:
    INT   sock               Socket which was previosly opened
    char* net_buffer         Buffer to store data
    int   buffer_size        Number of bytes to receive
    int   timeout_ms         Timeout in milliseconds

  Output:
    char* net_buffer         Network receive buffer

  Function value:
    number of bytes received (less than buffer_size if there was a timeout), or
     0 : timeout and nothing was received
    -1 : socket error

\********************************************************************/
{
   int n_received = 0;
   int flags = 0;
   int n;

   //printf("recv_tcp2: %p+%d bytes, timeout %d ms!\n", net_buffer + n_received, buffer_size - n_received, timeout_ms);

   while (n_received != buffer_size) {

      if (timeout_ms > 0) {
         int status = ss_socket_wait(sock, timeout_ms);
         if (status == SS_TIMEOUT)
            return n_received;
         if (status != SS_SUCCESS)
            return -1;
      }

      n = recv(sock, net_buffer + n_received, buffer_size - n_received, flags);

      //printf("recv_tcp2: %p+%d bytes, returned %d, errno %d (%s)\n", net_buffer + n_received, buffer_size - n_received, n, errno, strerror(errno));

#ifdef EINTR
      /* don't return if an alarm signal was cought */
      if (n == -1 && errno == EINTR)
         continue;
#endif

      if (n == 0) {
         // socket closed
         cm_msg(MERROR, "recv_tcp2", "unexpected connection closure");
         return -1;
      }

      if (n < 0) {
         // socket error
         cm_msg(MERROR, "recv_tcp2", "unexpected connection error, recv() errno %d (%s)", errno, strerror(errno));
         return -1;
      }

      n_received += n;
   }

   return n_received;
}


/*------------------------------------------------------------------*/
INT ss_recv_net_command(int sock, DWORD* routine_id, DWORD* param_size, char **param_ptr, int timeout_ms)
/********************************************************************\

  Routine: ss_recv_net_command

  Purpose: Receive MIDAS data packet from a TCP port. MIDAS data packet
     is defined by NET_COMMAND_HEADER
     which consists of two DWORDs. The first is the command code
     (or function id), the second is the size of the following
     parameters in bytes. From that size recv_tcp() determines
     how much data to receive.

  Input:
    int    sock              Socket which was previosly opened.
    DWORD* routine_id        routine_id from NET_COMMAND_HEADER
    DWORD* param_size        param_size from NET_COMMAND_HEADER, size of allocated data buffer
    char** param_ptr         pointer to allocated data buffer
    int    timeout_ms        timeout in milliseconds

  Function value:
    INT                      SS_SUCCESS, SS_NO_MEMORY, SS_SOCKET_ERROR

\********************************************************************/
{
   NET_COMMAND_HEADER ncbuf;
   size_t n;

   /* first receive header */
   n = recv_tcp2(sock, (char*)&ncbuf, sizeof(ncbuf), timeout_ms);

   if (n == 0) {
      cm_msg(MERROR, "ss_recv_net_command", "timeout receiving network command header");
      return SS_TIMEOUT;
   }

   if (n != sizeof(ncbuf)) {
      cm_msg(MERROR, "ss_recv_net_command", "error receiving network command header, see messages");
      return SS_SOCKET_ERROR;
   }

   // FIXME: where is the big-endian/little-endian conversion?
   *routine_id = ncbuf.routine_id;
   *param_size = ncbuf.param_size;

   if (*param_size == 0) {
      *param_ptr = NULL;
      return SS_SUCCESS;
   }

   *param_ptr = (char *)malloc(*param_size);

   if (*param_ptr == NULL) {
      cm_msg(MERROR, "ss_recv_net_command", "error allocating %d bytes for network command data", *param_size);
      return SS_NO_MEMORY;
   }

   /* first receive header */
   n = recv_tcp2(sock, *param_ptr, *param_size, timeout_ms);

   if (n == 0) {
      cm_msg(MERROR, "ss_recv_net_command", "timeout receiving network command data");
      free(*param_ptr);
      *param_ptr = NULL;
      return SS_TIMEOUT;
   }

   if (n != *param_size) {
      cm_msg(MERROR, "ss_recv_net_command", "error receiving network command data, see messages");
      free(*param_ptr);
      *param_ptr = NULL;
      return SS_SOCKET_ERROR;
   }

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
std::string ss_gethostname()
/********************************************************************\

  Routine: ss_gethostname

  Purpose: Get name of local machine using gethostname() syscall

  Input:
    int   buffer_size        Size of the buffer in bytes.

  Output:
    char  *buffer            receive buffer

  Function value:
    INT                      SS_SUCCESS or SS_IO_ERROR

\********************************************************************/
{
   char buf[256];
   memset(buf, 0, sizeof(buf));

   int status = gethostname(buf, sizeof(buf)-1);

   //printf("gethostname %d (%s)\n", status, buffer);

   if (status != 0) {
      cm_msg(MERROR, "ss_gethostname", "gethostname() errno %d (%s)", errno, strerror(errno));
      return "";
   }

   return buf;
}

/*------------------------------------------------------------------*/
INT ss_gethostname(char* buffer, int buffer_size)
/********************************************************************\

  Routine: ss_gethostname

  Purpose: Get name of local machine using gethostname() syscall

  Input:
    int   buffer_size        Size of the buffer in bytes.

  Output:
    char  *buffer            receive buffer

  Function value:
    INT                      SS_SUCCESS or SS_IO_ERROR

\********************************************************************/
{
   std::string h = ss_gethostname();

   if (h.length() == 0) {
      return SS_IO_ERROR;
   } else {
      strlcpy(buffer, h.c_str(), buffer_size);
      return SS_SUCCESS;
   }
}

/*------------------------------------------------------------------*/

std::string ss_getcwd()
{
   char *s = getcwd(NULL, 0);
   if (s) {
      std::string cwd = s;
      free(s);
      //printf("ss_getcwd: %s\n", cwd.c_str());
      return cwd;
   } else {
      return "/GETCWD-FAILED-ON-US";
   }
}

/*------------------------------------------------------------------*/

#ifdef OS_MSDOS
#ifdef sopen
/********************************************************************\
   under Turbo-C, sopen is defined as a macro instead a function.
   Since the PCTCP library uses sopen as a function call, we supply
   it here.
\********************************************************************/

#undef sopen

int sopen(const char *path, int access, int shflag, int mode)
{
   return open(path, (access) | (shflag), mode);
}

#endif
#endif

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                     Tape functions                                 *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/
INT ss_tape_open(char *path, INT oflag, INT * channel)
/********************************************************************\

  Routine: ss_tape_open

  Purpose: Open tape channel

  Input:
    char  *path             Name of tape
                            Under Windows NT, usually \\.\tape0
                            Under UNIX, usually /dev/tape
    INT   oflag             Open flags, same as open()

  Output:
    INT   *channel          Channel identifier

  Function value:
    SS_SUCCESS              Successful completion
    SS_NO_TAPE              No tape in device
    SS_DEV_BUSY             Device is used by someone else

\********************************************************************/
{
#ifdef OS_UNIX
   //cm_enable_watchdog(FALSE);

   *channel = open(path, oflag, 0644);

   //cm_enable_watchdog(TRUE);

   if (*channel < 0)
      cm_msg(MERROR, "ss_tape_open", "open() returned %d, errno %d (%s)", *channel, errno, strerror(errno));

   if (*channel < 0) {
      if (errno == EIO)
         return SS_NO_TAPE;
      if (errno == EBUSY)
         return SS_DEV_BUSY;
      return errno;
   }
#ifdef MTSETBLK
   {
   /* set variable block size */
   struct mtop arg;
   arg.mt_op = MTSETBLK;
   arg.mt_count = 0;

   ioctl(*channel, MTIOCTOP, &arg);
   }
#endif                          /* MTSETBLK */

#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;
   TAPE_GET_MEDIA_PARAMETERS m;

   *channel = (INT) CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);

   if (*channel == (INT) INVALID_HANDLE_VALUE) {
      status = GetLastError();
      if (status == ERROR_SHARING_VIOLATION) {
         cm_msg(MERROR, "ss_tape_open", "tape is used by other process");
         return SS_DEV_BUSY;
      }
      if (status == ERROR_FILE_NOT_FOUND) {
         cm_msg(MERROR, "ss_tape_open", "tape device \"%s\" doesn't exist", path);
         return SS_NO_TAPE;
      }

      cm_msg(MERROR, "ss_tape_open", "unknown error %d", status);
      return status;
   }

   status = GetTapeStatus((HANDLE) (*channel));
   if (status == ERROR_NO_MEDIA_IN_DRIVE || status == ERROR_BUS_RESET) {
      cm_msg(MERROR, "ss_tape_open", "no media in drive");
      return SS_NO_TAPE;
   }

   /* set block size */
   memset(&m, 0, sizeof(m));
   m.BlockSize = TAPE_BUFFER_SIZE;
   SetTapeParameters((HANDLE) (*channel), SET_TAPE_MEDIA_INFORMATION, &m);

#endif

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_close(INT channel)
/********************************************************************\

  Routine: ss_tape_close

  Purpose: Close tape channel

  Input:
    INT   channel           Channel identifier

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Low level error number

\********************************************************************/
{
   INT status;

#ifdef OS_UNIX

   status = close(channel);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_close", "close() returned %d, errno %d (%s)", status, errno, strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT

   if (!CloseHandle((HANDLE) channel)) {
      status = GetLastError();
      cm_msg(MERROR, "ss_tape_close", "unknown error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_status(char *path)
/********************************************************************\

  Routine: ss_tape_status

  Purpose: Print status information about tape

  Input:
    char  *path             Name of tape

  Output:
    <print>                 Tape information

  Function value:
    SS_SUCCESS              Successful completion

\********************************************************************/
{
#ifdef OS_UNIX
   int status;
   char str[256];
   /* let 'mt' do the job */
   sprintf(str, "mt -f %s status", path);
   status = system(str);
   if (status == -1)
      return SS_TAPE_ERROR;
   return SS_SUCCESS;
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status, channel;
   DWORD size;
   TAPE_GET_MEDIA_PARAMETERS m;
   TAPE_GET_DRIVE_PARAMETERS d;
   double x;

   channel = (INT) CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);

   if (channel == (INT) INVALID_HANDLE_VALUE) {
      status = GetLastError();
      if (status == ERROR_SHARING_VIOLATION) {
         cm_msg(MINFO, "ss_tape_status", "tape is used by other process");
         return SS_SUCCESS;
      }
      if (status == ERROR_FILE_NOT_FOUND) {
         cm_msg(MINFO, "ss_tape_status", "tape device \"%s\" doesn't exist", path);
         return SS_SUCCESS;
      }

      cm_msg(MINFO, "ss_tape_status", "unknown error %d", status);
      return status;
   }

   /* poll media changed messages */
   GetTapeParameters((HANDLE) channel, GET_TAPE_DRIVE_INFORMATION, &size, &d);
   GetTapeParameters((HANDLE) channel, GET_TAPE_DRIVE_INFORMATION, &size, &d);

   status = GetTapeStatus((HANDLE) channel);
   if (status == ERROR_NO_MEDIA_IN_DRIVE || status == ERROR_BUS_RESET) {
      cm_msg(MINFO, "ss_tape_status", "no media in drive");
      CloseHandle((HANDLE) channel);
      return SS_SUCCESS;
   }

   GetTapeParameters((HANDLE) channel, GET_TAPE_DRIVE_INFORMATION, &size, &d);
   GetTapeParameters((HANDLE) channel, GET_TAPE_MEDIA_INFORMATION, &size, &m);

   printf("Hardware error correction is %s\n", d.ECC ? "on" : "off");
   printf("Hardware compression is %s\n", d.Compression ? "on" : "off");
   printf("Tape %s write protected\n", m.WriteProtected ? "is" : "is not");

   if (d.FeaturesLow & TAPE_DRIVE_TAPE_REMAINING) {
      x = ((double) m.Remaining.LowPart + (double) m.Remaining.HighPart * 4.294967295E9)
          / 1000.0 / 1000.0;
      printf("Tape capacity remaining is %d MB\n", (int) x);
   } else
      printf("Tape capacity is not reported by tape\n");

   CloseHandle((HANDLE) channel);

#endif

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_write(INT channel, void *pdata, INT count)
/********************************************************************\

  Routine: ss_tape_write

  Purpose: Write count bytes to tape channel

  Input:
    INT   channel           Channel identifier
    void  *pdata            Address of data to write
    INT   count             number of bytes

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    SS_IO_ERROR             Physical IO error
    SS_TAPE_ERROR           Unknown tape error

\********************************************************************/
{
#ifdef OS_UNIX
   INT status;

   do {
      status = write(channel, pdata, count);
/*
    if (status != count)
      printf("count: %d - %d\n", count, status);
*/
   } while (status == -1 && errno == EINTR);

   if (status != count) {
      cm_msg(MERROR, "ss_tape_write", "write() returned %d, errno %d (%s)", status, errno, strerror(errno));

      if (errno == EIO)
         return SS_IO_ERROR;
      else
         return SS_TAPE_ERROR;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;
   DWORD written;

   WriteFile((HANDLE) channel, pdata, count, &written, NULL);
   if (written != (DWORD) count) {
      status = GetLastError();
      cm_msg(MERROR, "ss_tape_write", "error %d", status);

      return SS_IO_ERROR;
   }
#endif                          /* OS_WINNT */

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_read(INT channel, void *pdata, INT * count)
/********************************************************************\

  Routine: ss_tape_write

  Purpose: Read count bytes to tape channel

  Input:
    INT   channel           Channel identifier
    void  *pdata            Address of data
    INT   *count            Number of bytes to read

  Output:
    INT   *count            Number of read

  Function value:
    SS_SUCCESS              Successful operation
    <errno>                 Error code

\********************************************************************/
{
#ifdef OS_UNIX
   INT n, status;

   do {
      n = read(channel, pdata, *count);
   } while (n == -1 && errno == EINTR);

   if (n == -1) {
      if (errno == ENOSPC || errno == EIO)
         status = SS_END_OF_TAPE;
      else {
         if (n == 0 && errno == 0)
            status = SS_END_OF_FILE;
         else {
            cm_msg(MERROR, "ss_tape_read", "unexpected tape error: n=%d, errno=%d\n", n, errno);
            status = errno;
         }
      }
   } else
      status = SS_SUCCESS;
   *count = n;

   return status;

#elif defined(OS_WINNT)         /* OS_UNIX */

   INT status;
   DWORD read;

   if (!ReadFile((HANDLE) channel, pdata, *count, &read, NULL)) {
      status = GetLastError();
      if (status == ERROR_NO_DATA_DETECTED)
         status = SS_END_OF_TAPE;
      else if (status == ERROR_FILEMARK_DETECTED)
         status = SS_END_OF_FILE;
      else if (status == ERROR_MORE_DATA)
         status = SS_SUCCESS;
      else
         cm_msg(MERROR, "ss_tape_read", "unexpected tape error: n=%d, errno=%d\n", read, status);
   } else
      status = SS_SUCCESS;

   *count = read;
   return status;

#else                           /* OS_WINNT */

   return SS_SUCCESS;

#endif
}

/*------------------------------------------------------------------*/
INT ss_tape_write_eof(INT channel)
/********************************************************************\

  Routine: ss_tape_write_eof

  Purpose: Write end-of-file to tape channel

  Input:
    INT   *channel          Channel identifier

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Error number

\********************************************************************/
{
#ifdef MTIOCTOP
   struct mtop arg;
   INT status;

   arg.mt_op = MTWEOF;
   arg.mt_count = 1;

   //cm_enable_watchdog(FALSE);

   status = ioctl(channel, MTIOCTOP, &arg);

   //cm_enable_watchdog(TRUE);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_write_eof", "ioctl() failed, errno %d (%s)", errno, strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT

   TAPE_GET_DRIVE_PARAMETERS d;
   DWORD size;
   INT status;

   size = sizeof(TAPE_GET_DRIVE_PARAMETERS);
   GetTapeParameters((HANDLE) channel, GET_TAPE_DRIVE_INFORMATION, &size, &d);

   if (d.FeaturesHigh & TAPE_DRIVE_WRITE_FILEMARKS)
      status = WriteTapemark((HANDLE) channel, TAPE_FILEMARKS, 1, FALSE);
   else if (d.FeaturesHigh & TAPE_DRIVE_WRITE_LONG_FMKS)
      status = WriteTapemark((HANDLE) channel, TAPE_LONG_FILEMARKS, 1, FALSE);
   else if (d.FeaturesHigh & TAPE_DRIVE_WRITE_SHORT_FMKS)
      status = WriteTapemark((HANDLE) channel, TAPE_SHORT_FILEMARKS, 1, FALSE);
   else
      cm_msg(MERROR, "ss_tape_write_eof", "tape doesn't support writing of filemarks");

   if (status != NO_ERROR) {
      cm_msg(MERROR, "ss_tape_write_eof", "unknown error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_fskip(INT channel, INT count)
/********************************************************************\

  Routine: ss_tape_fskip

  Purpose: Skip count number of files on a tape

  Input:
    INT   *channel          Channel identifier
    INT   count             Number of files to skip

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Error number

\********************************************************************/
{
#ifdef MTIOCTOP
   struct mtop arg;
   INT status;

   if (count > 0)
      arg.mt_op = MTFSF;
   else
      arg.mt_op = MTBSF;
   arg.mt_count = abs(count);

   //cm_enable_watchdog(FALSE);

   status = ioctl(channel, MTIOCTOP, &arg);

   //cm_enable_watchdog(TRUE);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_fskip", "ioctl() failed, errno %d (%s)", errno, strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;

   status = SetTapePosition((HANDLE) channel, TAPE_SPACE_FILEMARKS, 0, (DWORD) count, 0, FALSE);

   if (status == ERROR_END_OF_MEDIA)
      return SS_END_OF_TAPE;

   if (status != NO_ERROR) {
      cm_msg(MERROR, "ss_tape_fskip", "error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return SS_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_rskip(INT channel, INT count)
/********************************************************************\

  Routine: ss_tape_rskip

  Purpose: Skip count number of records on a tape

  Input:
    INT   *channel          Channel identifier
    INT   count             Number of records to skip

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Error number

\********************************************************************/
{
#ifdef MTIOCTOP
   struct mtop arg;
   INT status;

   if (count > 0)
      arg.mt_op = MTFSR;
   else
      arg.mt_op = MTBSR;
   arg.mt_count = abs(count);

   //cm_enable_watchdog(FALSE);

   status = ioctl(channel, MTIOCTOP, &arg);

   //cm_enable_watchdog(TRUE);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_rskip", "ioctl() failed, errno %d (%s)", errno, strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;

   status = SetTapePosition((HANDLE) channel, TAPE_SPACE_RELATIVE_BLOCKS, 0, (DWORD) count, 0, FALSE);
   if (status != NO_ERROR) {
      cm_msg(MERROR, "ss_tape_rskip", "error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_rewind(INT channel)
/********************************************************************\

  Routine: ss_tape_rewind

  Purpose: Rewind tape

  Input:
    INT   channel           Channel identifier

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Error number

\********************************************************************/
{
#ifdef MTIOCTOP
   struct mtop arg;
   INT status;

   arg.mt_op = MTREW;
   arg.mt_count = 0;

   //cm_enable_watchdog(FALSE);

   status = ioctl(channel, MTIOCTOP, &arg);

   //cm_enable_watchdog(TRUE);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_rewind", "ioctl() failed, errno %d (%s)", errno, strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;

   status = SetTapePosition((HANDLE) channel, TAPE_REWIND, 0, 0, 0, FALSE);
   if (status != NO_ERROR) {
      cm_msg(MERROR, "ss_tape_rewind", "error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_spool(INT channel)
/********************************************************************\

  Routine: ss_tape_spool

  Purpose: Spool tape forward to end of recorded data

  Input:
    INT   channel           Channel identifier

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Error number

\********************************************************************/
{
#ifdef MTIOCTOP
   struct mtop arg;
   INT status;

#ifdef MTEOM
   arg.mt_op = MTEOM;
#else
   arg.mt_op = MTSEOD;
#endif
   arg.mt_count = 0;

   //cm_enable_watchdog(FALSE);

   status = ioctl(channel, MTIOCTOP, &arg);

   //cm_enable_watchdog(TRUE);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_rewind", "ioctl() failed, errno %d (%s)", errno, strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;

   status = SetTapePosition((HANDLE) channel, TAPE_SPACE_END_OF_DATA, 0, 0, 0, FALSE);
   if (status != NO_ERROR) {
      cm_msg(MERROR, "ss_tape_spool", "error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_mount(INT channel)
/********************************************************************\

  Routine: ss_tape_mount

  Purpose: Mount tape

  Input:
    INT   channel           Channel identifier

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Error number

\********************************************************************/
{
#ifdef MTIOCTOP
   struct mtop arg;
   INT status;

#ifdef MTLOAD
   arg.mt_op = MTLOAD;
#else
   arg.mt_op = MTNOP;
#endif
   arg.mt_count = 0;
   
   //cm_enable_watchdog(FALSE);

   status = ioctl(channel, MTIOCTOP, &arg);

   //cm_enable_watchdog(TRUE);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_mount", "ioctl() failed, errno %d (%s)", errno, strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;

   status = PrepareTape((HANDLE) channel, TAPE_LOAD, FALSE);
   if (status != NO_ERROR) {
      cm_msg(MERROR, "ss_tape_mount", "error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_unmount(INT channel)
/********************************************************************\

  Routine: ss_tape_unmount

  Purpose: Unmount tape

  Input:
    INT   channel           Channel identifier

  Output:
    <none>

  Function value:
    SS_SUCCESS              Successful completion
    errno                   Error number

\********************************************************************/
{
#ifdef MTIOCTOP
   struct mtop arg;
   INT status;

#ifdef MTOFFL
   arg.mt_op = MTOFFL;
#else
   arg.mt_op = MTUNLOAD;
#endif
   arg.mt_count = 0;

   //cm_enable_watchdog(FALSE);

   status = ioctl(channel, MTIOCTOP, &arg);

   //cm_enable_watchdog(TRUE);

   if (status < 0) {
      cm_msg(MERROR, "ss_tape_unmount", "ioctl() failed, errno %d (%s)", errno, strerror(errno));
      return errno;
   }
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   INT status;

   status = PrepareTape((HANDLE) channel, TAPE_UNLOAD, FALSE);
   if (status != NO_ERROR) {
      cm_msg(MERROR, "ss_tape_unmount", "error %d", status);
      return status;
   }
#endif                          /* OS_WINNT */

   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
INT ss_tape_get_blockn(INT channel)
/********************************************************************\
Routine: ss_tape_get_blockn
Purpose: Ask the tape channel for the present block number
Input:
INT   *channel          Channel identifier
Function value:
blockn:  >0 = block number, =0 option not available, <0 errno
\********************************************************************/
{
#if defined(OS_DARWIN)

   return 0;

#elif defined(OS_UNIX)

   INT status;
   struct mtpos arg;

   //cm_enable_watchdog(FALSE);
   status = ioctl(channel, MTIOCPOS, &arg);
   //cm_enable_watchdog(TRUE);
   if (status < 0) {
      if (errno == EIO)
         return 0;
      else {
         cm_msg(MERROR, "ss_tape_get_blockn", "ioctl() failed, errno %d (%s)", errno, strerror(errno));
         return -errno;
      }
   }
   return (arg.mt_blkno);

#elif defined(OS_WINNT)

   INT status;
   TAPE_GET_MEDIA_PARAMETERS media;
   unsigned long size;
   /* I'm not sure the partition count corresponds to the block count */
   status = GetTapeParameters((HANDLE) channel, GET_TAPE_MEDIA_INFORMATION, &size, &media);
   return (media.PartitionCount);

#endif
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                     Disk functions                                 *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/
double ss_disk_free(const char *path)
/********************************************************************\

  Routine: ss_disk_free

  Purpose: Return free disk space

  Input:
    char  *path             Name of a file in file system to check

  Output:

  Function value:
    doube                   Number of bytes free on disk

\********************************************************************/
{
#ifdef OS_UNIX
#if defined(OS_OSF1)
   struct statfs st;
   statfs(path, &st, sizeof(st));
   return (double) st.f_bavail * st.f_bsize;
#elif defined(OS_LINUX)
   struct statfs st;
   int status;
   status = statfs(path, &st);
   if (status != 0)
      return -1;
   return (double) st.f_bavail * st.f_bsize;
#elif defined(OS_SOLARIS)
   struct statvfs st;
   statvfs(path, &st);
   return (double) st.f_bavail * st.f_bsize;
#elif defined(OS_IRIX)
   struct statfs st;
   statfs(path, &st, sizeof(struct statfs), 0);
   return (double) st.f_bfree * st.f_bsize;
#else
   struct fs_data st;
   statfs(path, &st);
   return (double) st.fd_otsize * st.fd_bfree;
#endif

#elif defined(OS_WINNT)         /* OS_UNIX */
   DWORD SectorsPerCluster;
   DWORD BytesPerSector;
   DWORD NumberOfFreeClusters;
   DWORD TotalNumberOfClusters;
   char str[80];

   strcpy(str, path);
   if (strchr(str, ':') != NULL) {
      *(strchr(str, ':') + 1) = 0;
      strcat(str, DIR_SEPARATOR_STR);
      GetDiskFreeSpace(str, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);
   } else
      GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);

   return (double) NumberOfFreeClusters *SectorsPerCluster * BytesPerSector;
#else                           /* OS_WINNT */

   return 1e9;

#endif
}

#if defined(OS_ULTRIX) || defined(OS_WINNT)
int fnmatch(const char *pat, const char *str, const int flag)
{
   while (*str != '\0') {
      if (*pat == '*') {
         pat++;
         if ((str = strchr(str, *pat)) == NULL)
            return -1;
      }
      if (*pat == *str) {
         pat++;
         str++;
      } else
         return -1;
   }
   if (*pat == '\0')
      return 0;
   else
      return -1;
}
#endif

#ifdef OS_WINNT
HANDLE pffile;
LPWIN32_FIND_DATA lpfdata;
#endif

INT ss_file_find(const char *path, const char *pattern, char **plist)
{
   STRING_LIST list;

   int count = ss_file_find(path, pattern, &list);
   if (count <= 0)
      return count;

   size_t size = list.size();
   *plist = (char *) malloc(size*MAX_STRING_LENGTH);
   for (size_t i=0; i<size; i++) {
      //printf("file %d [%s]\n", (int)i, list[i].c_str());
      strlcpy((*plist)+i*MAX_STRING_LENGTH, list[i].c_str(), MAX_STRING_LENGTH);
   }

   return size;
}

INT ss_file_find(const char *path, const char *pattern, STRING_LIST *plist)
/********************************************************************\

  Routine: ss_file_find

  Purpose: Return list of files matching 'pattern' from the 'path' location

  Input:
    char  *path             Name of a file in file system to check
    char  *pattern          pattern string (wildcard allowed)

  Output:
    char  **plist           pointer to the lfile list

  Function value:
    int                     Number of files matching request

\********************************************************************/
{
   assert(plist);
   // Check if the directory exists
   if (access(path, F_OK) != 0) {
      return -1; // Return -1 files if directory doesn't exist
   }

#ifdef OS_UNIX
   DIR *dir_pointer;
   struct dirent *dp;

   plist->clear();
   if ((dir_pointer = opendir(path)) == NULL)
      return 0;
   for (dp = readdir(dir_pointer); dp != NULL; dp = readdir(dir_pointer)) {
      if (fnmatch(pattern, dp->d_name, 0) == 0 && (dp->d_type == DT_REG || dp->d_type == DT_LNK || dp->d_type == DT_UNKNOWN)) {
         plist->push_back(dp->d_name);
         seekdir(dir_pointer, telldir(dir_pointer));
      }
   }
   closedir(dir_pointer);
#endif
#ifdef OS_WINNT
   char str[255];
   int first;

   strcpy(str, path);
   strcat(str, "\\");
   strcat(str, pattern);
   first = 1;
   lpfdata = (WIN32_FIND_DATA *) malloc(sizeof(WIN32_FIND_DATA));
   *plist->clear();
   pffile = FindFirstFile(str, lpfdata);
   if (pffile == INVALID_HANDLE_VALUE)
      return 0;
   first = 0;
   plist->push_back(lpfdata->cFileName);
   i++;
   while (FindNextFile(pffile, lpfdata)) {
      plist->push_back(lpfdata->cFileName);
      i++;
   }
   free(lpfdata);
#endif
   return plist->size();
}

INT ss_dir_find(const char *path, const char *pattern, char** plist)
{
   STRING_LIST list;

   int count = ss_dir_find(path, pattern, &list);
   if (count <= 0)
      return count;

   size_t size = list.size();
   *plist = (char *) malloc(size*MAX_STRING_LENGTH);
   for (size_t i=0; i<size; i++) {
      //printf("file %d [%s]\n", (int)i, list[i].c_str());
      strlcpy((*plist)+i*MAX_STRING_LENGTH, list[i].c_str(), MAX_STRING_LENGTH);
   }

   return size;
}

INT ss_dir_find(const char *path, const char *pattern, STRING_LIST *plist)
/********************************************************************\

 Routine: ss_dir_find

 Purpose: Return list of direcories matching 'pattern' from the 'path' location

 Input:
 char  *path             Name of a file in file system to check
 char  *pattern          pattern string (wildcard allowed)

 Output:
 char  **plist           pointer to the lfile list

 Function value:
 int                     Number of files matching request

 \********************************************************************/
{
   assert(plist);
#ifdef OS_UNIX
   DIR *dir_pointer;
   struct dirent *dp;

   if ((dir_pointer = opendir(path)) == NULL)
      return 0;
   plist->clear();
   for (dp = readdir(dir_pointer); dp != NULL; dp = readdir(dir_pointer)) {
      if (fnmatch(pattern, dp->d_name, 0) == 0 && dp->d_type == DT_DIR) {
         plist->push_back(dp->d_name);
         seekdir(dir_pointer, telldir(dir_pointer));
      }
   }
   closedir(dir_pointer);
#endif
#ifdef OS_WINNT
   char str[255];
   int first;

   strcpy(str, path);
   strcat(str, "\\");
   strcat(str, pattern);
   first = 1;
   plist->clear();
   lpfdata = (WIN32_FIND_DATA *) malloc(sizeof(WIN32_FIND_DATA));
   pffile = FindFirstFile(str, lpfdata);
   if (pffile == INVALID_HANDLE_VALUE)
      return 0;
   first = 0;
   plist->push_back(lpfdata->cFileName);
   while (FindNextFile(pffile, lpfdata)) {
      plist->push_back(lpfdata->cFileName);
   }
   free(lpfdata);
#endif
   return plist->size();
}

INT ss_dirlink_find(const char *path, const char *pattern, char** plist)
{
   STRING_LIST list;

   int count = ss_dirlink_find(path, pattern, &list);
   if (count <= 0)
      return count;

   size_t size = list.size();
   *plist = (char *) malloc(size*MAX_STRING_LENGTH);
   for (size_t i=0; i<size; i++) {
      //printf("file %d [%s]\n", (int)i, list[i].c_str());
      strlcpy((*plist)+i*MAX_STRING_LENGTH, list[i].c_str(), MAX_STRING_LENGTH);
   }

   return size;
}

INT ss_dirlink_find(const char *path, const char *pattern, STRING_LIST *plist)
/********************************************************************\

 Routine: ss_dirlink_find

 Purpose: Return list of direcories and links matching 'pattern' from the 'path' location

 Input:
 char  *path             Name of a file in file system to check
 char  *pattern          pattern string (wildcard allowed)

 Output:
 char  **plist           pointer to the lfile list

 Function value:
 int                     Number of files matching request

 \********************************************************************/
{
   assert(plist);
#ifdef OS_UNIX
   DIR *dir_pointer;
   struct dirent *dp;

   if ((dir_pointer = opendir(path)) == NULL)
      return 0;
   plist->clear();
   for (dp = readdir(dir_pointer); dp != NULL; dp = readdir(dir_pointer)) {
      if (fnmatch(pattern, dp->d_name, 0) == 0) {
         /* must have a "/" at the end, otherwise also links to files are accepted */
         std::string full_path = std::string(path) + "/" + dp->d_name + "/";
         struct stat st;
         if (lstat(full_path.c_str(), &st) == 0 && (S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode))) {
            plist->push_back(dp->d_name);
         }
      }
   }
   closedir(dir_pointer);
#endif
#ifdef OS_WINNT
   char str[255];
   int first;

   strcpy(str, path);
   strcat(str, "\\");
   strcat(str, pattern);
   first = 1;
   plist->clear();
   lpfdata = (WIN32_FIND_DATA *) malloc(sizeof(WIN32_FIND_DATA));
   pffile = FindFirstFile(str, lpfdata);
   if (pffile == INVALID_HANDLE_VALUE)
      return 0;
   first = 0;
   plist->push_back(lpfdata->cFileName);
   while (FindNextFile(pffile, lpfdata)) {
      plist->push_back(lpfdata->cFileName);
   }
   free(lpfdata);
#endif
   return plist->size();
}

INT ss_file_remove(const char *path)
/********************************************************************\

  Routine: ss_file_remove

  Purpose: remove (delete) file given through the path

  Input:
    char  *path             Name of a file in file system to check

  Output:

  Function value:
    int                     function error 0= ok, -1 check errno

\********************************************************************/
{
   return remove(path);
}

double ss_file_size(const char *path)
/********************************************************************\

  Routine: ss_file_size

  Purpose: Return file size in bytes for the given path

  Input:
    char  *path             Name of a file in file system to check

  Output:

  Function value:
    double                     File size

\********************************************************************/
{
#ifdef _LARGEFILE64_SOURCE
   struct stat64 stat_buf;
   int status;

   /* allocate buffer with file size */
   status = stat64(path, &stat_buf);
   if (status != 0)
      return -1;
   return (double) stat_buf.st_size;
#else
   struct stat stat_buf;
   int status;

   /* allocate buffer with file size */
   status = stat(path, &stat_buf);
   if (status != 0)
      return -1;
   return (double) stat_buf.st_size;
#endif
}

time_t ss_file_time(const char *path)
/********************************************************************\

  Routine: ss_file_time

  Purpose: Return time of last file modification

  Input:
    char  *path             Name of a file in file system to check

  Output:

  Function value:
    time_t                  File modification time

\********************************************************************/
{
#ifdef _LARGEFILE64_SOURCE
   struct stat64 stat_buf;
   int status;

   /* allocate buffer with file size */
   status = stat64(path, &stat_buf);
   if (status != 0)
      return -1;
   return stat_buf.st_mtime;
#else
   struct stat stat_buf;
   int status;

   /* allocate buffer with file size */
   status = stat(path, &stat_buf);
   if (status != 0)
      return -1;
   return stat_buf.st_mtime;
#endif
}

double ss_disk_size(const char *path)
/********************************************************************\

  Routine: ss_disk_size

  Purpose: Return full disk space

  Input:
    char  *path             Name of a file in file system to check

  Output:

  Function value:
    doube                   Number of bytes free on disk

\********************************************************************/
{
#ifdef OS_UNIX
#if defined(OS_OSF1)
   struct statfs st;
   statfs(path, &st, sizeof(st));
   return (double) st.f_blocks * st.f_fsize;
#elif defined(OS_LINUX)
   int status;
   struct statfs st;
   status = statfs(path, &st);
   if (status != 0)
      return -1;
   return (double) st.f_blocks * st.f_bsize;
#elif defined(OS_SOLARIS)
   struct statvfs st;
   statvfs(path, &st);
   if (st.f_frsize > 0)
      return (double) st.f_blocks * st.f_frsize;
   else
      return (double) st.f_blocks * st.f_bsize;
#elif defined(OS_ULTRIX)
   struct fs_data st;
   statfs(path, &st);
   return (double) st.fd_btot * 1024;
#elif defined(OS_IRIX)
   struct statfs st;
   statfs(path, &st, sizeof(struct statfs), 0);
   return (double) st.f_blocks * st.f_bsize;
#else
#error ss_disk_size not defined for this OS
#endif
#endif                          /* OS_UNIX */

#ifdef OS_WINNT
   DWORD SectorsPerCluster;
   DWORD BytesPerSector;
   DWORD NumberOfFreeClusters;
   DWORD TotalNumberOfClusters;
   char str[80];

   strcpy(str, path);
   if (strchr(str, ':') != NULL) {
      *(strchr(str, ':') + 1) = 0;
      strcat(str, DIR_SEPARATOR_STR);
      GetDiskFreeSpace(str, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);
   } else
      GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);

   return (double) TotalNumberOfClusters *SectorsPerCluster * BytesPerSector;
#endif                          /* OS_WINNT */

   return 1e9;
}

int ss_file_exist(const char *path)
/********************************************************************\
 
 Routine: ss_file_exist
 
 Purpose: Check if a file exists
 
 Input:
 char  *path             Name of a file in file to check
 
 Output:
 
 Function value:
 int                     1: file exists
                         0: file does not exist
 
 \********************************************************************/
{
#ifdef OS_UNIX
   struct stat buf;
   
   int retval = stat(path, &buf);
   //printf("retval %d, errno %d (%s)\n", retval, errno, strerror(errno));
   if (retval < 0)
      return 0;
   if (S_ISDIR(buf.st_mode))
      return 0;
#endif
   
   int fd = open(path, O_RDONLY, 0);
   if (fd < 0)
      return 0;
   close(fd);
   return 1;
}

int ss_file_link_exist(const char *path)
/********************************************************************\

 Routine: ss_file_link_exist

 Purpose: Check if a symbolic link file exists

 Input:
 char  *path             Name of a file in file to check

 Output:

 Function value:
 int                     1: file exists
                         0: file does not exist

 \********************************************************************/
{
#ifdef OS_UNIX
   struct stat buf;

   int retval = lstat(path, &buf);
   if (retval < 0)
      return 0;
   if (S_ISLNK(buf.st_mode))
      return 1;
   return 0;
#endif

   return 0;
}

int ss_dir_exist(const char *path)
/********************************************************************\
 
 Routine: ss_dir_exist
 
 Purpose: Check if a directory exists
 
 Input:
 char  *path             Name of a file in file to check
 
 Output:
 
 Function value:
 int                     1: file exists
                         0: file does not exist
 
 \********************************************************************/
{
#ifdef OS_UNIX
   struct stat buf;
   
   int retval = stat(path, &buf);
   //printf("retval %d, errno %d (%s)\n", retval, errno, strerror(errno));
   if (retval < 0)
      return 0;
   if (!S_ISDIR(buf.st_mode))
      return 0;
#else
#warning ss_dir_exist() is not implemented!
#endif
   return 1;
}

int ss_file_copy(const char *src, const char *dst, bool append)
/********************************************************************\

 Routine: ss_file_copy

 Purpose: Copy file "src" to file "dst"

 Input:
     const char  *src        Source file name
     const char  *dst        Destination file name

 Output:

 Function value:
     int                     function error 0= ok, -1 check errno

 \********************************************************************/
{
   int fd_to, fd_from;
   char buf[4096];
   ssize_t nread;
   int saved_errno;

   fd_from = open(src, O_RDONLY);
   if (fd_from < 0)
      return -1;

   if (append)
      fd_to = open(dst, O_WRONLY | O_CREAT | O_EXCL | O_APPEND, 0666);
   else
      fd_to = open(dst, O_WRONLY | O_CREAT | O_EXCL, 0666);
   if (fd_to < 0)
      goto out_error;

   while (nread = read(fd_from, buf, sizeof(buf)), nread > 0) {
      char *out_ptr = buf;
      ssize_t nwritten;

      do {
         nwritten = write(fd_to, out_ptr, nread);

         if (nwritten >= 0) {
            nread -= nwritten;
            out_ptr += nwritten;
         } else if (errno != EINTR) {
            goto out_error;
         }
      } while (nread > 0);
   }

   if (nread == 0) {
      if (close(fd_to) < 0) {
         fd_to = -1;
         goto out_error;
      }
      close(fd_from);

      /* Success! */
      return 0;
   }

   out_error:
   saved_errno = errno;

   close(fd_from);
   if (fd_to >= 0)
      close(fd_to);

   errno = saved_errno;
   return -1;
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                  Screen  functions                                 *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/
void ss_clear_screen()
/********************************************************************\

  Routine: ss_clear_screen

  Purpose: Clear the screen

  Input:
    <none>

  Output:
    <none>

  Function value:
    <none>

\********************************************************************/
{
#ifdef OS_WINNT

   HANDLE hConsole;
   COORD coordScreen = { 0, 0 };        /* here's where we'll home the cursor */
   BOOL bSuccess;
   DWORD cCharsWritten;
   CONSOLE_SCREEN_BUFFER_INFO csbi;     /* to get buffer info */
   DWORD dwConSize;             /* number of character cells in the current buffer */

   hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

   /* get the number of character cells in the current buffer */
   bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
   dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

   /* fill the entire screen with blanks */
   bSuccess = FillConsoleOutputCharacter(hConsole, (TCHAR) ' ', dwConSize, coordScreen, &cCharsWritten);

   /* put the cursor at (0, 0) */
   bSuccess = SetConsoleCursorPosition(hConsole, coordScreen);
   return;

#endif                          /* OS_WINNT */
#if defined(OS_UNIX) || defined(OS_VXWORKS) || defined(OS_VMS)
   printf("\033[2J");
#endif
#ifdef OS_MSDOS
   clrscr();
#endif
}

/*------------------------------------------------------------------*/
void ss_set_screen_size(int x, int y)
/********************************************************************\

  Routine: ss_set_screen_size

  Purpose: Set the screen size in character cells

  Input:
    <none>

  Output:
    <none>

  Function value:
    <none>

\********************************************************************/
{
#ifdef OS_WINNT

   HANDLE hConsole;
   COORD coordSize;

   coordSize.X = (short) x;
   coordSize.Y = (short) y;
   hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleScreenBufferSize(hConsole, coordSize);

#else                           /* OS_WINNT */
#endif
}

/*------------------------------------------------------------------*/
void ss_printf(INT x, INT y, const char *format, ...)
/********************************************************************\

  Routine: ss_printf

  Purpose: Print string at given cursor position

  Input:
    INT   x,y               Cursor position, starting from zero,
          x=0 and y=0 left upper corner

    char  *format           Format string for printf
    ...                     Arguments for printf

  Output:
    <none>

  Function value:
    <none>

\********************************************************************/
{
   char str[256];
   va_list argptr;

   va_start(argptr, format);
   vsprintf(str, (char *) format, argptr);
   va_end(argptr);

#ifdef OS_WINNT
   {
      HANDLE hConsole;
      COORD dwWriteCoord;
      DWORD cCharsWritten;

      hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

      dwWriteCoord.X = (short) x;
      dwWriteCoord.Y = (short) y;

      WriteConsoleOutputCharacter(hConsole, str, strlen(str), dwWriteCoord, &cCharsWritten);
   }

#endif                          /* OS_WINNT */

#if defined(OS_UNIX) || defined(OS_VXWORKS) || defined(OS_VMS)
   printf("\033[%1d;%1dH", y + 1, x + 1);
   printf("%s", str);
   fflush(stdout);
#endif

#ifdef OS_MSDOS
   gotoxy(x + 1, y + 1);
   cputs(str);
#endif
}

/*------------------------------------------------------------------*/
char *ss_getpass(const char *prompt)
/********************************************************************\

  Routine: ss_getpass

  Purpose: Read password without echoing it at the screen

  Input:
    char   *prompt    Prompt string

  Output:
    <none>

  Function value:
    char*             Pointer to password

\********************************************************************/
{
   static char password[32];

   fprintf(stdout, "%s", prompt);
   fflush(stdout);
   memset(password, 0, sizeof(password));

#ifdef OS_UNIX
   return (char *) getpass("");
#elif defined(OS_WINNT)
   {
      HANDLE hConsole;
      DWORD nCharsRead;

      hConsole = GetStdHandle(STD_INPUT_HANDLE);
      SetConsoleMode(hConsole, ENABLE_LINE_INPUT);
      ReadConsole(hConsole, password, sizeof(password), &nCharsRead, NULL);
      SetConsoleMode(hConsole, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT);
      printf("\n");

      if (password[strlen(password) - 1] == '\r')
         password[strlen(password) - 1] = 0;

      return password;
   }
#elif defined(OS_MSDOS)
   {
      char c, *ptr;

      ptr = password;
      while ((c = getchar()) != EOF && c != '\n')
         *ptr++ = c;
      *ptr = 0;

      printf("\n");
      return password;
   }
#else
   {
      ss_gets(password, 32);
      return password;
   }
#endif
}

/*------------------------------------------------------------------*/
INT ss_getchar(BOOL reset)
/********************************************************************\

  Routine: ss_getchar

  Purpose: Read a single character

  Input:
    BOOL   reset            Reset terminal to standard mode

  Output:
    <none>

  Function value:
    int             0       for no character available
                    CH_xxs  for special character
                    n       ASCII code for normal character
                    -1      function not available on this OS

\********************************************************************/
{
#ifdef OS_UNIX

   static BOOL init = FALSE;
   static struct termios save_termios;
   struct termios buf;
   int i, fd;
   char c[3];

   if (_daemon_flag)
      return 0;

   fd = fileno(stdin);

   if (reset) {
      if (init)
         tcsetattr(fd, TCSAFLUSH, &save_termios);
      init = FALSE;
      return 0;
   }

   if (!init) {
      tcgetattr(fd, &save_termios);
      memcpy(&buf, &save_termios, sizeof(buf));

      buf.c_lflag &= ~(ECHO | ICANON | IEXTEN);

      buf.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON);

      buf.c_cflag &= ~(CSIZE | PARENB);
      buf.c_cflag |= CS8;
      /* buf.c_oflag &= ~(OPOST); */
      buf.c_cc[VMIN] = 0;
      buf.c_cc[VTIME] = 0;

      tcsetattr(fd, TCSAFLUSH, &buf);
      init = TRUE;
   }

   memset(c, 0, 3);
   i = read(fd, c, 1);

   if (i == 0)
      return 0;

   /* check if ESC */
   if (c[0] == 27) {
      i = read(fd, c, 2);
      if (i == 0)               /* return if only ESC */
         return 27;

      /* cursor keys return 2 chars, others 3 chars */
      if (c[1] < 65) {
         i = read(fd, c, 1);
      }

      /* convert ESC sequence to CH_xxx */
      switch (c[1]) {
      case 49:
         return CH_HOME;
      case 50:
         return CH_INSERT;
      case 51:
         return CH_DELETE;
      case 52:
         return CH_END;
      case 53:
         return CH_PUP;
      case 54:
         return CH_PDOWN;
      case 65:
         return CH_UP;
      case 66:
         return CH_DOWN;
      case 67:
         return CH_RIGHT;
      case 68:
         return CH_LEFT;
      }
   }

   /* BS/DEL -> BS */
   if (c[0] == 127)
      return CH_BS;

   return c[0];

#elif defined(OS_WINNT)

   static BOOL init = FALSE;
   static INT repeat_count = 0;
   static INT repeat_char;
   HANDLE hConsole;
   DWORD nCharsRead;
   INPUT_RECORD ir;
   OSVERSIONINFO vi;

   /* find out if we are under W95 */
   vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&vi);

   if (vi.dwPlatformId != VER_PLATFORM_WIN32_NT) {
      /* under W95, console doesn't work properly */
      int c;

      if (!kbhit())
         return 0;

      c = getch();
      if (c == 224) {
         c = getch();
         switch (c) {
         case 71:
            return CH_HOME;
         case 72:
            return CH_UP;
         case 73:
            return CH_PUP;
         case 75:
            return CH_LEFT;
         case 77:
            return CH_RIGHT;
         case 79:
            return CH_END;
         case 80:
            return CH_DOWN;
         case 81:
            return CH_PDOWN;
         case 82:
            return CH_INSERT;
         case 83:
            return CH_DELETE;
         }
      }
      return c;
   }

   hConsole = GetStdHandle(STD_INPUT_HANDLE);

   if (reset) {
      SetConsoleMode(hConsole, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT);
      init = FALSE;
      return 0;
   }

   if (!init) {
      SetConsoleMode(hConsole, ENABLE_PROCESSED_INPUT);
      init = TRUE;
   }

   if (repeat_count) {
      repeat_count--;
      return repeat_char;
   }

   PeekConsoleInput(hConsole, &ir, 1, &nCharsRead);

   if (nCharsRead == 0)
      return 0;

   ReadConsoleInput(hConsole, &ir, 1, &nCharsRead);

   if (ir.EventType != KEY_EVENT)
      return ss_getchar(0);

   if (!ir.Event.KeyEvent.bKeyDown)
      return ss_getchar(0);

   if (ir.Event.KeyEvent.wRepeatCount > 1) {
      repeat_count = ir.Event.KeyEvent.wRepeatCount - 1;
      repeat_char = ir.Event.KeyEvent.uChar.AsciiChar;
      return repeat_char;
   }

   if (ir.Event.KeyEvent.uChar.AsciiChar)
      return ir.Event.KeyEvent.uChar.AsciiChar;

   if (ir.Event.KeyEvent.dwControlKeyState & (ENHANCED_KEY)) {
      switch (ir.Event.KeyEvent.wVirtualKeyCode) {
      case 33:
         return CH_PUP;
      case 34:
         return CH_PDOWN;
      case 35:
         return CH_END;
      case 36:
         return CH_HOME;
      case 37:
         return CH_LEFT;
      case 38:
         return CH_UP;
      case 39:
         return CH_RIGHT;
      case 40:
         return CH_DOWN;
      case 45:
         return CH_INSERT;
      case 46:
         return CH_DELETE;
      }

      return ir.Event.KeyEvent.wVirtualKeyCode;
   }

   return ss_getchar(0);

#elif defined(OS_MSDOS)

   int c;

   if (!kbhit())
      return 0;

   c = getch();
   if (!c) {
      c = getch();
      switch (c) {
      case 71:
         return CH_HOME;
      case 72:
         return CH_UP;
      case 73:
         return CH_PUP;
      case 75:
         return CH_LEFT;
      case 77:
         return CH_RIGHT;
      case 79:
         return CH_END;
      case 80:
         return CH_DOWN;
      case 81:
         return CH_PDOWN;
      case 82:
         return CH_INSERT;
      case 83:
         return CH_DELETE;
      }
   }
   return c;

#else
   return -1;
#endif
}

/*------------------------------------------------------------------*/
char *ss_gets(char *string, int size)
/********************************************************************\

  Routine: ss_gets

  Purpose: Read a line from standard input. Strip trailing new line
           character. Return in a loop so that it cannot be interrupted
           by an alarm() signal (like under Sun Solaris)

  Input:
    INT    size             Size of string

  Output:
    BOOL   string           Return string

  Function value:
    char                    Return string

\********************************************************************/
{
   char *p;

   do {
      p = fgets(string, size, stdin);
   } while (p == NULL);


   if (strlen(p) > 0 && p[strlen(p) - 1] == '\n')
      p[strlen(p) - 1] = 0;

   return p;
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                  Direct IO functions                               *
*                                                                    *
\********************************************************************/

/*------------------------------------------------------------------*/
INT ss_directio_give_port(INT start, INT end)
{
#ifdef OS_WINNT

   /* under Windows NT, use DirectIO driver to open ports */

   OSVERSIONINFO vi;
   HANDLE hdio = 0;
   DWORD buffer[] = { 6, 0, 0, 0 };
   DWORD size;

   vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&vi);

   /* use DirectIO driver under NT to gain port access */
   if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
      hdio = CreateFile("\\\\.\\directio", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      if (hdio == INVALID_HANDLE_VALUE) {
         printf("hyt1331.c: Cannot access IO ports (No DirectIO driver installed)\n");
         return -1;
      }

      /* open ports */
      buffer[1] = start;
      buffer[2] = end;
      if (!DeviceIoControl(hdio, (DWORD) 0x9c406000, &buffer, sizeof(buffer), NULL, 0, &size, NULL))
         return -1;
   }

   return SS_SUCCESS;
#else
   return SS_SUCCESS;
#endif
}

/*------------------------------------------------------------------*/
INT ss_directio_lock_port(INT start, INT end)
{
#ifdef OS_WINNT

   /* under Windows NT, use DirectIO driver to lock ports */

   OSVERSIONINFO vi;
   HANDLE hdio;
   DWORD buffer[] = { 7, 0, 0, 0 };
   DWORD size;

   vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&vi);

   /* use DirectIO driver under NT to gain port access */
   if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
      hdio = CreateFile("\\\\.\\directio", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      if (hdio == INVALID_HANDLE_VALUE) {
         printf("hyt1331.c: Cannot access IO ports (No DirectIO driver installed)\n");
         return -1;
      }

      /* lock ports */
      buffer[1] = start;
      buffer[2] = end;
      if (!DeviceIoControl(hdio, (DWORD) 0x9c406000, &buffer, sizeof(buffer), NULL, 0, &size, NULL))
         return -1;
   }

   return SS_SUCCESS;
#else
   return SS_SUCCESS;
#endif
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                  Encryption                                        *
*                                                                    *
\********************************************************************/

#define bin_to_ascii(c) ((c)>=38?((c)-38+'a'):(c)>=12?((c)-12+'A'):(c)+'.')

char *ss_crypt(const char *buf, const char *salt)
/********************************************************************\

  Routine: ss_crypt

  Purpose: Simple fake of UNIX crypt(3) function, until we get
           a better one

  Input:
    char   *buf             Plain password
    char   *slalt           Two random characters
                            events. Can be used to skip events

  Output:
    <none>

  Function value:
    char*                   Encrypted password

\********************************************************************/
{
   int i, seed;
   static char enc_pw[13];

   memset(enc_pw, 0, sizeof(enc_pw));
   enc_pw[0] = salt[0];
   enc_pw[1] = salt[1];

   for (i = 0; i < 8 && buf[i]; i++)
      enc_pw[i + 2] = buf[i];
   for (; i < 8; i++)
      enc_pw[i + 2] = 0;

   seed = 123;
   for (i = 2; i < 13; i++) {
      seed = 5 * seed + 27 + enc_pw[i];
      enc_pw[i] = (char) bin_to_ascii(seed & 0x3F);
   }

   return enc_pw;
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                  NaN's                                             *
*                                                                    *
\********************************************************************/

double ss_nan()
{
   double nan;

   nan = 0;
   nan = 0 / nan;
   return nan;
}

#ifdef OS_WINNT
#include <float.h>
#ifndef isnan
#define isnan(x) _isnan(x)
#endif
#ifndef finite
#define finite(x) _finite(x)
#endif
#elif defined(OS_LINUX)
#include <math.h>
#endif

int ss_isnan(double x)
{
   return isnan(x);
}

int ss_isfin(double x)
{
#ifdef FP_INFINITE
   /* new-style finite() */
   return isfinite(x);
#else
   /* old-style finite() */
   return finite(x);
#endif
}

/*------------------------------------------------------------------*/
/********************************************************************\
*                                                                    *
*                  Stack Trace                                       *
*                                                                    *
\********************************************************************/

#ifndef NO_EXECINFO

#ifdef OS_LINUX
#include <execinfo.h>
#endif

#define N_STACK_HISTORY 500
char stack_history[N_STACK_HISTORY][80];
int stack_history_pointer = -1;

INT ss_stack_get(char ***string)
{
#ifdef OS_LINUX
#define MAX_STACK_DEPTH 16

   void *trace[MAX_STACK_DEPTH];
   int size;

   size = backtrace(trace, MAX_STACK_DEPTH);
   *string = backtrace_symbols(trace, size);
   return size;
#else
   return 0;
#endif
}

void ss_stack_print()
{
   char **string;
   int i, n;

   n = ss_stack_get(&string);
   for (i = 0; i < n; i++)
      printf("%s\n", string[i]);
   if (n > 0)
      free(string);
}

void ss_stack_history_entry(char *tag)
{
   char **string;
   int i, n;

   if (stack_history_pointer == -1) {
      stack_history_pointer++;
      memset(stack_history, 0, sizeof(stack_history));
   }
   strlcpy(stack_history[stack_history_pointer], tag, 80);
   stack_history_pointer = (stack_history_pointer + 1) % N_STACK_HISTORY;
   n = ss_stack_get(&string);
   for (i = 2; i < n; i++) {
      strlcpy(stack_history[stack_history_pointer], string[i], 80);
      stack_history_pointer = (stack_history_pointer + 1) % N_STACK_HISTORY;
   }
   free(string);

   strlcpy(stack_history[stack_history_pointer], "=========================", 80);
   stack_history_pointer = (stack_history_pointer + 1) % N_STACK_HISTORY;
}

void ss_stack_history_dump(char *filename)
{
   FILE *f;
   int i, j;

   f = fopen(filename, "wt");
   if (f != NULL) {
      j = stack_history_pointer;
      for (i = 0; i < N_STACK_HISTORY; i++) {
         if (strlen(stack_history[j]) > 0)
            fprintf(f, "%s\n", stack_history[j]);
         j = (j + 1) % N_STACK_HISTORY;
      }
      fclose(f);
      printf("Stack dump written to %s\n", filename);
   } else
      printf("Cannot open %s: errno=%d\n", filename, errno);
}

#endif

// Method to check if a given string is valid UTF-8.  Returns 1 if it is.
// This method was taken from stackoverflow user Christoph, specifically
// http://stackoverflow.com/questions/1031645/how-to-detect-utf-8-in-plain-c
bool ss_is_valid_utf8(const char * string)
{
   assert(string);

   // FIXME: this function over-reads the input array. K.O. May 2021

   const unsigned char * bytes = (const unsigned char *)string;
   while(*bytes) {
      if( (// ASCII
           // use bytes[0] <= 0x7F to allow ASCII control characters
           bytes[0] == 0x09 ||
           bytes[0] == 0x0A ||
           bytes[0] == 0x0D ||
           (0x20 <= bytes[0] && bytes[0] <= 0x7E)
           )
          ) {
         bytes += 1;
         continue;
      }
      
      if( (// non-overlong 2-byte
           (0xC2 <= bytes[0] && bytes[0] <= 0xDF) &&
           (0x80 <= bytes[1] && bytes[1] <= 0xBF)
           )
          ) {
         bytes += 2;
         continue;
      }
      
      if( (// excluding overlongs
           bytes[0] == 0xE0 &&
           (0xA0 <= bytes[1] && bytes[1] <= 0xBF) &&
           (0x80 <= bytes[2] && bytes[2] <= 0xBF)
           ) ||
          (// straight 3-byte
           ((0xE1 <= bytes[0] && bytes[0] <= 0xEC) ||
            bytes[0] == 0xEE ||
            bytes[0] == 0xEF) &&
           (0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
           (0x80 <= bytes[2] && bytes[2] <= 0xBF)
           ) ||
          (// excluding surrogates
           bytes[0] == 0xED &&
           (0x80 <= bytes[1] && bytes[1] <= 0x9F) &&
           (0x80 <= bytes[2] && bytes[2] <= 0xBF)
           )
          ) {
         bytes += 3;
         continue;
      }
      
      if( (// planes 1-3
           bytes[0] == 0xF0 &&
           (0x90 <= bytes[1] && bytes[1] <= 0xBF) &&
           (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
           (0x80 <= bytes[3] && bytes[3] <= 0xBF)
           ) ||
          (// planes 4-15
           (0xF1 <= bytes[0] && bytes[0] <= 0xF3) &&
           (0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
           (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
           (0x80 <= bytes[3] && bytes[3] <= 0xBF)
           ) ||
          (// plane 16
           bytes[0] == 0xF4 &&
           (0x80 <= bytes[1] && bytes[1] <= 0x8F) &&
           (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
           (0x80 <= bytes[3] && bytes[3] <= 0xBF)
           )
          ) {
         bytes += 4;
         continue;
      }
      
      //printf("ss_is_valid_utf8(): string [%s], not utf8 at offset %d, byte %d, [%s]\n", string, (int)((char*)bytes-(char*)string), (int)(0xFF&bytes[0]), bytes);
      //abort();
      
      return false;
   }

   return true;
}

bool ss_repair_utf8(char* string)
{
   assert(string);

   bool modified = false;

   //std::string original = string;

   // FIXME: this function over-reads the input array. K.O. May 2021

   unsigned char * bytes = (unsigned char *)string;
   while(*bytes) {
      if( (// ASCII
           // use bytes[0] <= 0x7F to allow ASCII control characters
           bytes[0] == 0x09 ||
           bytes[0] == 0x0A ||
           bytes[0] == 0x0D ||
           (0x20 <= bytes[0] && bytes[0] <= 0x7E)
           )
          ) {
         bytes += 1;
         continue;
      }
      
      if( (// non-overlong 2-byte
           (0xC2 <= bytes[0] && bytes[0] <= 0xDF) &&
           (0x80 <= bytes[1] && bytes[1] <= 0xBF)
           )
          ) {
         bytes += 2;
         continue;
      }
      
      if( (// excluding overlongs
           bytes[0] == 0xE0 &&
           (0xA0 <= bytes[1] && bytes[1] <= 0xBF) &&
           (0x80 <= bytes[2] && bytes[2] <= 0xBF)
           ) ||
          (// straight 3-byte
           ((0xE1 <= bytes[0] && bytes[0] <= 0xEC) ||
            bytes[0] == 0xEE ||
            bytes[0] == 0xEF) &&
           (0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
           (0x80 <= bytes[2] && bytes[2] <= 0xBF)
           ) ||
          (// excluding surrogates
           bytes[0] == 0xED &&
           (0x80 <= bytes[1] && bytes[1] <= 0x9F) &&
           (0x80 <= bytes[2] && bytes[2] <= 0xBF)
           )
          ) {
         bytes += 3;
         continue;
      }
      
      if( (// planes 1-3
           bytes[0] == 0xF0 &&
           (0x90 <= bytes[1] && bytes[1] <= 0xBF) &&
           (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
           (0x80 <= bytes[3] && bytes[3] <= 0xBF)
           ) ||
          (// planes 4-15
           (0xF1 <= bytes[0] && bytes[0] <= 0xF3) &&
           (0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
           (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
           (0x80 <= bytes[3] && bytes[3] <= 0xBF)
           ) ||
          (// plane 16
           bytes[0] == 0xF4 &&
           (0x80 <= bytes[1] && bytes[1] <= 0x8F) &&
           (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
           (0x80 <= bytes[3] && bytes[3] <= 0xBF)
           )
          ) {
         bytes += 4;
         continue;
      }

      if (bytes[0] == 0) // end of string
         break;

      bytes[0] = '?';
      bytes += 1;

      modified = true;
   }

   //if (modified) {
   //   printf("ss_repair_utf8(): invalid UTF8 string [%s] changed to [%s]\n", original.c_str(), string);
   //} else {
   //   //printf("ss_repair_utf8(): string [%s] is ok\n", string);
   //}
      
   return modified;
}

bool ss_repair_utf8(std::string& s)
{
   // C++11 std::string data() is same as c_str(), NUL-terminated.
   // C++17 std::string data() is not "const".
   // https://en.cppreference.com/w/cpp/string/basic_string/data
   return ss_repair_utf8((char*)s.data()); // FIXME: C++17 or newer, do not need to drop the "const". K.O. May 2021
}

std::chrono::time_point<std::chrono::high_resolution_clock> ss_us_start()
{
   return std::chrono::high_resolution_clock::now();
}

unsigned int ss_us_since(std::chrono::time_point<std::chrono::high_resolution_clock> start) {
   auto elapsed = std::chrono::high_resolution_clock::now() - start;
   return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}

/** @} *//* end of msfunctionc */
/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */