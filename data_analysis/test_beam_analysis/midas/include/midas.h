/********************************************************************\

  Name:         MIDAS.H
  Created by:   Stefan Ritt

  Contents:     Type definitions and function declarations needed
                for MIDAS applications


  $Id$

\********************************************************************/

#ifndef _MIDAS_H_
#define _MIDAS_H_

/*------------------------------------------------------------------*/

/**dox***************************************************************/
/** @file midas.h
The main include file
*/

/** @defgroup mdefineh Public Defines
 */
/** @defgroup mmacroh Public Macros
 */
/** @defgroup mdeferrorh Public Error definitions
 */
/** @defgroup msectionh Public Structure Declarations
 */

/* has to be changed whenever binary ODB format changes */
#define DATABASE_VERSION 3

/* MIDAS version number which will be incremented for every release */
#define MIDAS_VERSION "2.1"

// turn deprecated warning off until we replaced all sprintf with snprintf
#ifdef __clang__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

/*------------------------------------------------------------------*/

/* find out on which operating system we are running */

#if defined( VAX ) || defined( __VMS )
#define OS_VMS
#endif

#if defined( _MSC_VER )
#ifndef OS_WINNT
#define OS_WINNT
#endif
#endif

#if defined( __MSDOS__ )
#define OS_MSDOS
#endif

#if defined ( vxw )
#ifndef OS_VXWORKS
#define OS_VXWORKS
#undef OS_UNIX
#endif
#endif

#if defined ( __linux__ )
#ifndef OS_LINUX
#define OS_LINUX
#endif
#endif

#if defined ( __APPLE__ )
#ifndef OS_DARWIN
#define OS_LINUX
#define OS_DARWIN
#endif
#endif

#if defined(OS_LINUX) || defined(OS_OSF1) || defined(OS_ULTRIX) || defined(OS_FREEBSD) || defined(OS_SOLARIS) || defined(OS_IRIX) || defined(OS_DARWIN)
#define OS_UNIX
#endif

#if !defined(OS_IRIX) && !defined(OS_VMS) && !defined(OS_MSDOS) && !defined(OS_UNIX) && !defined(OS_VXWORKS) && !defined(OS_WINNT)
#error MIDAS cannot be used on this operating system
#endif

/*------------------------------------------------------------------*/

/* Define basic data types */

#ifndef MIDAS_TYPE_DEFINED
#define MIDAS_TYPE_DEFINED

typedef unsigned char BYTE;
typedef unsigned short int WORD;
#ifndef OS_WINNT // Windows defines already DWORD
typedef unsigned int DWORD;
#endif

#ifndef OS_WINNT
#ifndef OS_VXWORKS
typedef DWORD BOOL;
#endif
#endif

#endif                          /* MIDAS_TYPE_DEFINED */

/*
  Definitions depending on integer types:

  Note that the alpha chip uses 32 bit integers by default.
  Therefore always use 'INT' instead 'int'.
*/
#if defined(OS_MSDOS)
typedef long int INT;
#elif defined( OS_WINNT )

/* INT predefined in windows.h */
#ifndef _INC_WINDOWS
#include <windows.h>
#endif

#undef DB_TRUNCATED

#else
typedef int INT;
#endif

typedef INT HNDLE;

/* define integer types with explicit widths */
#ifndef NO_INT_TYPES_DEFINE
typedef unsigned char      UINT8;
typedef char               INT8;
typedef unsigned short     UINT16;
typedef short              INT16;
typedef unsigned int       UINT32;
typedef int                INT32;
typedef unsigned long long UINT64;
typedef long long          INT64;
#endif

/* Include vxWorks for BOOL definition */
#ifdef OS_VXWORKS
#ifndef __INCvxWorksh
#include <vxWorks.h>
#endif
#endif

/*
   Conversion from pointer to interger and back.

   On 64-bit systems, pointers are long ints
   On 32-bit systems, pointers are int

   Never use direct casting, since the code might then
   not be portable between 32-bit and 64-bit systems

*/
#if defined(__alpha) || defined(_LP64)
#define POINTER_T     long int
#else
#define POINTER_T     int
#endif

/* define old PTYPE for compatibility with old code */
#define PTYPE POINTER_T

/* need system-dependant thread type */
#if defined(OS_WINNT)
typedef HANDLE midas_thread_t;
#elif defined(OS_UNIX)
#include <pthread.h>
typedef pthread_t midas_thread_t;
#else
typedef INT midas_thread_t;
#endif

#define TRUE  1
#define FALSE 0

/* directory separator */
#if defined(OS_MSDOS) || defined(OS_WINNT)
#define DIR_SEPARATOR     '\\'
#define DIR_SEPARATOR_STR "\\"
#elif defined(OS_VMS)
#define DIR_SEPARATOR     ']'
#define DIR_SEPARATOR_STR "]"
#else
#define DIR_SEPARATOR     '/'
#define DIR_SEPARATOR_STR "/"
#endif

/* inline functions */
#if defined( _MSC_VER )
#define INLINE __inline
#elif defined(__GNUC__)
#ifndef INLINE
#define INLINE __inline__
#endif
#else
#define INLINE
#endif

/* large file (>2GB) support */
#ifndef _LARGEFILE64_SOURCE
#define O_LARGEFILE 0
#endif

/* disable "deprecated" warning */
#if defined( _MSC_VER )
#pragma warning( disable: 4996)
#endif

#if defined __GNUC__
#define MATTRPRINTF(a, b) __attribute__ ((format (printf, a, b)))
#else
#define MATTRPRINTF(a, b)
#endif

/* mutex definitions */
#if defined(OS_WINNT)
#ifndef MUTEX_T_DEFINED
typedef HANDLE MUTEX_T;
#define MUTEX_T_DEFINED
#endif
#elif defined(OS_LINUX)
#ifndef MUTEX_T_DEFINED
typedef pthread_mutex_t MUTEX_T;
#define MUTEX_T_DEFINED
#endif
#else
#ifndef MUTEX_T_DEFINED
typedef INT MUTEX_T;
#define MUTEX_T_DEFINED
#endif
#endif

/* OSX brings its own strlcpy/stlcat */
#ifdef OS_DARWIN
#ifndef HAVE_STRLCPY
#define HAVE_STRLCPY 1
#endif
#endif

#include <mutex>
#include <atomic>
#include <vector>
#include <string>
typedef std::vector<std::string> STRING_LIST;

class MJsonNode; // forward declaration from mjson.h

/*------------------------------------------------------------------*/

/* Definition of implementation specific constants */

#define DEFAULT_MAX_EVENT_SIZE (4*1024*1024) /**< default maximum event size 4MiB, actual maximum event size is set by ODB /Experiment/MAX_EVENT_SIZE */
#define DEFAULT_BUFFER_SIZE   (32*1024*1024) /**< default event buffer size 32MiB, actual event buffer size is set by ODB /Experiment/Buffer sizes/SYSTEM */

#define MIN_WRITE_CACHE_SIZE      (10000000) /**< default and minimum write cache size, in bytes */
#define MAX_WRITE_CACHE_SIZE_DIV         (3) /**< maximum write cache size as fraction of buffer size */
#define MAX_WRITE_CACHE_EVENT_SIZE_DIV  (10) /**< maximum event size cached in write cache as fraction of write cache size */

#ifdef OS_WINNT
#define TAPE_BUFFER_SIZE       0x100000      /**< buffer size for taping data */
#else
#define TAPE_BUFFER_SIZE       0x8000        /**< buffer size for taping data */
#endif
#define NET_TCP_SIZE           0xFFFF        /**< maximum TCP transfer size   */
#define OPT_TCP_SIZE           8192          /**< optimal TCP buffer size     */

#define EVENT_BUFFER_NAME      "SYSTEM"      /**< buffer name for commands    */
#define DEFAULT_ODB_SIZE       0x100000      /**< online database 1M          */

#define NAME_LENGTH            32            /**< length of names, mult.of 8! */
#define HOST_NAME_LENGTH       256           /**< length of TCP/IP names      */
#define MAX_CLIENTS            64            /**< client processes per buf/db */
#define MAX_EVENT_REQUESTS     10            /**< event requests per client   */
#define MAX_OPEN_RECORDS       256           /**< number of open DB records   */
#define MAX_ODB_PATH           256           /**< length of path in ODB       */
#define BANKLIST_MAX           4096          /**< max # of banks in event     */
#define STRING_BANKLIST_MAX    BANKLIST_MAX * 4   /**< for bk_list()          */
#define TRANSITION_ERROR_STRING_LENGTH 256   /**< length of transition error string */


#define MIDAS_TCP_PORT 1175     /* port under which server is listening */

/**
Timeouts [ms] */
#define DEFAULT_RPC_TIMEOUT    10000
#define WATCHDOG_INTERVAL      1000

#define DEFAULT_WATCHDOG_TIMEOUT 10000    /**< Watchdog */

#define USE_HIDDEN_EQ                     /**< Use hidden equipment in status page */

/*------------------------------------------------------------------*/

/* Enumeration definitions */

/**dox***************************************************************/
/** @addtogroup mdefineh
 *
 *  @{  */

/**
System states */
#define STATE_STOPPED 1      /**< MIDAS run stopped                  */
#define STATE_PAUSED  2      /**< MIDAS run paused                   */
#define STATE_RUNNING 3      /**< MIDAS run running                  */

/**
Data format */
#define FORMAT_MIDAS  1       /**< MIDAS banks                        */
#define FORMAT_YBOS   2       /**< YBOS  banks                        */
#define FORMAT_ASCII  3       /**< ASCII format                       */
#define FORMAT_FIXED  4       /**< Fixed length binary records        */
#define FORMAT_DUMP   5       /**< Dump (detailed ASCII) format       */
#define FORMAT_HBOOK  6       /**< CERN hbook (rz) format             */
#define FORMAT_ROOT   7       /**< CERN ROOT format                   */

/**
Event Sampling type */
#define GET_ALL   (1<<0)      /**< get all events (consume)             */
#define GET_NONBLOCKING (1<<1)/**< get as much as possible without blocking producer */
#define GET_RECENT (1<<2)     /**< get recent event (not older than 1 s)*/

/**
MIDAS Data Type Definitions                             min      max    */
#define TID_BYTE      1       /**< DEPRECATED, use TID_UINT8 instead    */
#define TID_UINT8     1       /**< unsigned byte         0       255    */
#define TID_SBYTE     2       /**< DEPRECATED, use TID_INT8 instead     */
#define TID_INT8      2       /**< signed byte         -128      127    */
#define TID_CHAR      3       /**< single character      0       255    */
#define TID_WORD      4       /**< DEPRECATED, use TID_UINT16 instead   */
#define TID_UINT16    4       /**< two bytes             0      65535   */
#define TID_SHORT     5       /**< DEPRECATED, use TID_INT16 instead    */
#define TID_INT16     5       /**< signed word        -32768    32767   */
#define TID_DWORD     6       /**< DEPRECATED, use TID_UINT32 instead   */
#define TID_UINT32    6       /**< four bytes            0      2^32-1  */
#define TID_INT       7       /**< DEPRECATED, use TID_INT32 instead    */
#define TID_INT32     7       /**< signed dword        -2^31    2^31-1  */
#define TID_BOOL      8       /**< four bytes bool       0        1     */
#define TID_FLOAT     9       /**< 4 Byte float format                  */
#define TID_FLOAT32   9       /**< 4 Byte float format                  */
#define TID_DOUBLE   10       /**< 8 Byte float format                  */
#define TID_FLOAT64  10       /**< 8 Byte float format                  */
#define TID_BITFIELD 11       /**< 32 Bits Bitfield      0  111... (32) */
#define TID_STRING   12       /**< zero terminated string               */
#define TID_ARRAY    13       /**< array with unknown contents          */
#define TID_STRUCT   14       /**< structure with fixed length          */
#define TID_KEY      15       /**< key in online database               */
#define TID_LINK     16       /**< link in online database              */
#define TID_INT64    17       /**< 8 bytes int          -2^63   2^63-1  */
#define TID_UINT64   18       /**< 8 bytes unsigned int  0      2^64-1  */
#define TID_QWORD    18       /**< 8 bytes unsigned int  0      2^64-1  */
#define TID_LAST     19       /**< end of TID list indicator            */

/**
Transition flags */
#define TR_SYNC          1
#define TR_ASYNC         2
#define TR_DETACH        4
#define TR_MTHREAD       8

/**
Synchronous / Asynchronous flags */
#define BM_WAIT          0
#define BM_NO_WAIT       1

/**
Access modes */
#define MODE_READ      (1<<0)
#define MODE_WRITE     (1<<1)
#define MODE_DELETE    (1<<2)
#define MODE_EXCLUSIVE (1<<3)
#define MODE_ALLOC     (1<<6)
#define MODE_WATCH     (1<<7)

/**
RPC options */
//#define RPC_OTIMEOUT       1
//#define RPC_OTRANSPORT     2
//#define RPC_OCONVERT_FLAG  3
//#define RPC_OHW_TYPE       4
//#define RPC_OSERVER_TYPE   5
//#define RPC_OSERVER_NAME   6
//#define RPC_CONVERT_FLAGS  7
//#define RPC_ODB_HANDLE     8
//#define RPC_CLIENT_HANDLE  9
//#define RPC_SEND_SOCK      10
//#define RPC_WATCHDOG_TIMEOUT 11
//#define RPC_NODELAY        12

/* special RPC handles for rpc_get_timeout() and rpc_set_timeout() */
#define RPC_HNDLE_MSERVER -1
#define RPC_HNDLE_CONNECT -2

#define RPC_NO_REPLY 0x80000000l

/**
Watchdog flags */
#define WF_WATCH_ME   (1<<0)    /* see cm_set_watchdog_flags   */
#define WF_CALL_WD    (1<<1)

/**
Transitions values */
#define TR_START      (1<<0)  /**< Start transition  */
#define TR_STOP       (1<<1)  /**< Stop transition  */
#define TR_PAUSE      (1<<2)  /**< Pause transition */
#define TR_RESUME     (1<<3)  /**< Resume transition  */
#define TR_STARTABORT (1<<4)  /**< Start aborted transition  */
#define TR_DEFERRED   (1<<12)

/**
Equipment types */
#define EQ_PERIODIC    (1<<0)   /**< Periodic Event */
#define EQ_POLLED      (1<<1)   /**< Polling Event */
#define EQ_INTERRUPT   (1<<2)   /**< Interrupt Event */
#define EQ_MULTITHREAD (1<<3)   /**< Multithread Event readout */
#define EQ_SLOW        (1<<4)   /**< Slow Control Event */
#define EQ_MANUAL_TRIG (1<<5)   /**< Manual triggered Event */
#define EQ_FRAGMENTED  (1<<6)   /**< Fragmented Event */
#define EQ_EB          (1<<7)   /**< Event run through the event builder */
#define EQ_USER        (1<<8)   /**< Polling handled in user part */

/**
Read - On flags */
#define RO_RUNNING    (1<<0)   /**< While running */
#define RO_STOPPED    (1<<1)   /**< Before stopping the run */
#define RO_PAUSED     (1<<2)   /**< ??? */
#define RO_BOR        (1<<3)   /**< At the Begin of run */
#define RO_EOR        (1<<4)   /**< At the End of run */
#define RO_PAUSE      (1<<5)   /**< Before pausing the run */
#define RO_RESUME     (1<<6)   /**< Before resuming the run */

#define RO_TRANSITIONS (RO_BOR|RO_EOR|RO_PAUSE|RO_RESUME)      /**< At all transitions */
#define RO_NONTRANS    (RO_RUNNING|RO_STOPPED|RO_PAUSED)       /**< Always except on tranistion */
#define RO_ALWAYS      (0xFF)                                  /**< Always and during transitions */

#define RO_ODB        (1<<8)   /**< Copy event data to ODB */

/**dox***************************************************************/
          /** @} *//* end of mdefineh */

/**
special characters */
#define CH_BS             8
#define CH_TAB            9
#define CH_CR            13

#define CH_EXT 0x100

#define CH_HOME   (CH_EXT+0)
#define CH_INSERT (CH_EXT+1)
#define CH_DELETE (CH_EXT+2)
#define CH_END    (CH_EXT+3)
#define CH_PUP    (CH_EXT+4)
#define CH_PDOWN  (CH_EXT+5)
#define CH_UP     (CH_EXT+6)
#define CH_DOWN   (CH_EXT+7)
#define CH_RIGHT  (CH_EXT+8)
#define CH_LEFT   (CH_EXT+9)

/* event sources in equipment */
/**
Code the LAM crate and LAM station into a bitwise register.
@param c Crate number
@param s Slot number
*/
#define LAM_SOURCE(c, s)         (c<<24 | ((s) & 0xFFFFFF))

/**
Code the Station number bitwise for the LAM source.
@param s Slot number
*/
#define LAM_STATION(s)           (1<<(s-1))

/**
Convert the coded LAM crate to Crate number.
@param c coded crate
*/
#define LAM_SOURCE_CRATE(c)      (c>>24)

/**
Convert the coded LAM station to Station number.
@param s Slot number
*/
#define LAM_SOURCE_STATION(s)    ((s) & 0xFFFFFF)

/**
CNAF commands */
#define CNAF        0x1         /* normal read/write                */
#define CNAF_nQ     0x2         /* Repeat read until noQ            */

#define CNAF_INHIBIT_SET         0x100
#define CNAF_INHIBIT_CLEAR       0x101
#define CNAF_CRATE_CLEAR         0x102
#define CNAF_CRATE_ZINIT         0x103
#define CNAF_TEST                0x110

/**dox***************************************************************/
/** @addtogroup mmacroh
 *
 *  @{
 */

/**
MAX */
#ifndef MAX
#define MAX(a,b)            (((a)>(b))?(a):(b))
#endif

/**
MIN */
#ifndef MIN
#define MIN(a,b)            (((a)<(b))?(a):(b))
#endif

/*------------------------------------------------------------------*/

/**
Align macro for data alignment on 8-byte boundary */
#define ALIGN8(x)  (((x)+7) & ~7)

/**
Align macro for variable data alignment */
#define VALIGN(adr,align)  (((POINTER_T) (adr)+(align)-1) & ~((align)-1))

/**dox***************************************************************/
          /** @} *//* end of mmacroh */

/**dox***************************************************************/
/** @addtogroup mdefineh
 *
 *  @{  */
/*
* Bit flags */
#define EVENTID_ALL        -1   /* any event id                   */
#define TRIGGER_ALL        -1   /* any type of trigger            */

/**
System message types */
#define MT_ERROR           (1<<0)     /**< - */
#define MT_INFO            (1<<1)     /**< - */
#define MT_DEBUG           (1<<2)     /**< - */
#define MT_USER            (1<<3)     /**< - */
#define MT_LOG             (1<<4)     /**< - */
#define MT_TALK            (1<<5)     /**< - */
#define MT_CALL            (1<<6)     /**< - */
#define MT_ALL              0xFF      /**< - */

#define MT_ERROR_STR       "ERROR"
#define MT_INFO_STR        "INFO"
#define MT_DEBUG_STR       "DEBUG"
#define MT_USER_STR        "USER"
#define MT_LOG_STR         "LOG"
#define MT_TALK_STR        "TALK"
#define MT_CALL_STR        "CALL"

#define MERROR             MT_ERROR, __FILE__, __LINE__ /**< - */
#define MINFO              MT_INFO,  __FILE__, __LINE__ /**< - */
#define MDEBUG             MT_DEBUG, __FILE__, __LINE__ /**< - */
#define MUSER              MT_USER,  __FILE__, __LINE__ /**< produced by interactive user */
#define MLOG               MT_LOG,   __FILE__, __LINE__ /**< info message which is only logged */
#define MTALK              MT_TALK,  __FILE__, __LINE__ /**< info message for speech system */
#define MCALL              MT_CALL,  __FILE__, __LINE__ /**< info message for telephone call */

/**dox***************************************************************/
          /** @} *//* end of mdefineh */


/**dox***************************************************************/
/** @addtogroup mdeferrorh
 *
 *  @{
 */

/**dox***************************************************************/
/**
 @defgroup err21 Status and error codes
 @{ */
#define SUCCESS                       1 /**< Success */
#define CM_SUCCESS                    1 /**< Same  */
#define CM_SET_ERROR                102 /**< set  */
#define CM_NO_CLIENT                103 /**< nobody */
#define CM_DB_ERROR                 104 /**< db access error */
#define CM_UNDEF_EXP                105 /**< - */
#define CM_VERSION_MISMATCH         106 /**< - */
#define CM_SHUTDOWN                 107 /**< - */
#define CM_WRONG_PASSWORD           108 /**< - */
#define CM_UNDEF_ENVIRON            109 /**< - */
#define CM_DEFERRED_TRANSITION      110 /**< - */
#define CM_TRANSITION_IN_PROGRESS   111 /**< - */
#define CM_TIMEOUT                  112 /**< - */
#define CM_INVALID_TRANSITION       113 /**< - */
#define CM_TOO_MANY_REQUESTS        114 /**< - */
#define CM_TRUNCATED                115 /**< - */
#define CM_TRANSITION_CANCELED      116 /**< - */
/**dox***************************************************************/
          /** @} *//* end of err21 */

/**dox***************************************************************/
/**
 @defgroup err22 Buffer Manager error codes
 @{ */
#define BM_SUCCESS                    1   /**< - */
#define BM_CREATED                  202   /**< - */
#define BM_NO_MEMORY                203   /**< - */
#define BM_INVALID_NAME             204   /**< - */
#define BM_INVALID_HANDLE           205   /**< - */
#define BM_NO_SLOT                  206   /**< - */
#define BM_NO_SEMAPHORE             207   /**< - */
#define BM_NOT_FOUND                208   /**< - */
#define BM_ASYNC_RETURN             209   /**< - */
#define BM_TRUNCATED                210   /**< - */
#define BM_MULTIPLE_HOSTS           211   /**< - */
#define BM_MEMSIZE_MISMATCH         212   /**< - */
#define BM_CONFLICT                 213   /**< - */
#define BM_EXIT                     214   /**< - */
#define BM_INVALID_PARAM            215   /**< - */
#define BM_MORE_EVENTS              216   /**< - */
#define BM_INVALID_MIXING           217   /**< - */
#define BM_NO_SHM                   218   /**< - */
#define BM_CORRUPTED                219   /**< - */
#define BM_INVALID_SIZE             220   /**< - */
/**dox***************************************************************/
          /** @} *//* end of group 22 */

/**dox***************************************************************/
/**  @defgroup err23 Online Database error codes
@{ */
#define DB_SUCCESS                    1   /**< - */
#define DB_CREATED                  302   /**< - */
#define DB_NO_MEMORY                303   /**< - */
#define DB_INVALID_NAME             304   /**< - */
#define DB_INVALID_HANDLE           305   /**< - */
#define DB_NO_SLOT                  306   /**< - */
#define DB_NO_SEMAPHORE             307   /**< - */
#define DB_MEMSIZE_MISMATCH         308   /**< - */
#define DB_INVALID_PARAM            309   /**< - */
#define DB_FULL                     310   /**< - */
#define DB_KEY_EXIST                311   /**< - */
#define DB_NO_KEY                   312   /**< - */
#define DB_KEY_CREATED              313   /**< - */
#define DB_TRUNCATED                314   /**< - */
#define DB_TYPE_MISMATCH            315   /**< - */
#define DB_NO_MORE_SUBKEYS          316   /**< - */
#define DB_FILE_ERROR               317   /**< - */
#define DB_NO_ACCESS                318   /**< - */
#define DB_STRUCT_SIZE_MISMATCH     319   /**< - */
#define DB_OPEN_RECORD              320   /**< - */
#define DB_OUT_OF_RANGE             321   /**< - */
#define DB_INVALID_LINK             322   /**< - */
#define DB_CORRUPTED                323   /**< - */
#define DB_STRUCT_MISMATCH          324   /**< - */
#define DB_TIMEOUT                  325   /**< - */
#define DB_VERSION_MISMATCH         326   /**< - */
/**dox***************************************************************/
          /** @} *//* end of group 23 */

/**dox***************************************************************/
/**  @defgroup err24 System Services error code
@{ */
#define SS_SUCCESS                    1   /**< - */
#define SS_CREATED                  402   /**< - */
#define SS_NO_MEMORY                403   /**< - */
#define SS_INVALID_NAME             404   /**< - */
#define SS_INVALID_HANDLE           405   /**< - */
#define SS_INVALID_ADDRESS          406   /**< - */
#define SS_FILE_ERROR               407   /**< - */
#define SS_NO_SEMAPHORE             408   /**< - */
#define SS_NO_PROCESS               409   /**< - */
#define SS_NO_THREAD                410   /**< - */
#define SS_SOCKET_ERROR             411   /**< - */
#define SS_TIMEOUT                  412   /**< - */
#define SS_SERVER_RECV              413   /**< - */
#define SS_CLIENT_RECV              414   /**< - */
#define SS_ABORT                    415   /**< - */
#define SS_EXIT                     416   /**< - */
#define SS_NO_TAPE                  417   /**< - */
#define SS_DEV_BUSY                 418   /**< - */
#define SS_IO_ERROR                 419   /**< - */
#define SS_TAPE_ERROR               420   /**< - */
#define SS_NO_DRIVER                421   /**< - */
#define SS_END_OF_TAPE              422   /**< - */
#define SS_END_OF_FILE              423   /**< - */
#define SS_FILE_EXISTS              424   /**< - */
#define SS_NO_SPACE                 425   /**< - */
#define SS_INVALID_FORMAT           426   /**< - */
#define SS_NO_ROOT                  427   /**< - */
#define SS_SIZE_MISMATCH            428   /**< - */
#define SS_NO_MUTEX                 429   /**< - */
/**dox***************************************************************/
          /** @} *//* end of group 24 */

/**dox***************************************************************/
/**  @defgroup err25 Remote Procedure Calls error codes
@{ */
#define RPC_SUCCESS                   1   /**< - */
#define RPC_ABORT              SS_ABORT   /**< - */
#define RPC_NO_CONNECTION           502   /**< - */
#define RPC_NET_ERROR               503   /**< - */
#define RPC_TIMEOUT                 504   /**< - */
#define RPC_EXCEED_BUFFER           505   /**< - */
#define RPC_NOT_REGISTERED          506   /**< - */
#define RPC_CONNCLOSED              507   /**< - */
#define RPC_INVALID_ID              508   /**< - */
#define RPC_SHUTDOWN                509   /**< - */
#define RPC_NO_MEMORY               510   /**< - */
#define RPC_DOUBLE_DEFINED          511   /**< - */
#define RPC_MUTEX_TIMEOUT           512   /**< - */
/**dox***************************************************************/
          /** @} *//* end of group 25 */

/**dox***************************************************************/
/**  @defgroup err26 Other errors
@{ */
#define FE_SUCCESS                    1   /**< - */
#define FE_ERR_ODB                  602   /**< - */
#define FE_ERR_HW                   603   /**< - */
#define FE_ERR_DISABLED             604   /**< - */
#define FE_ERR_DRIVER               605   /**< - */
#define FE_PARTIALLY_DISABLED       606   /**< - */
#define FE_NOT_YET_READ             607   /**< - */

/**
History error code */
#define HS_SUCCESS                    1   /**< - */
#define HS_FILE_ERROR               702   /**< - */
#define HS_NO_MEMORY                703   /**< - */
#define HS_TRUNCATED                704   /**< - */
#define HS_WRONG_INDEX              705   /**< - */
#define HS_UNDEFINED_EVENT          706   /**< - */
#define HS_UNDEFINED_VAR            707   /**< - */

/**
FTP error code */
#define FTP_SUCCESS                   1   /**< - */
#define FTP_NET_ERROR               802   /**< - */
#define FTP_FILE_ERROR              803   /**< - */
#define FTP_RESPONSE_ERROR          804   /**< - */
#define FTP_INVALID_ARG             805   /**< - */

/**
ELog error code */
#define EL_SUCCESS                    1   /**< - */
#define EL_FILE_ERROR               902   /**< - */
#define EL_NO_MESSAGE               903   /**< - */
#define EL_TRUNCATED                904   /**< - */
#define EL_FIRST_MSG                905   /**< - */
#define EL_LAST_MSG                 906   /**< - */

/**
Alarm error code */
#define AL_SUCCESS                    1   /**< - */
#define AL_INVALID_NAME            1002   /**< - */
#define AL_ERROR_ODB               1003   /**< - */
#define AL_RESET                   1004   /**< - */
#define AL_TRIGGERED               1005   /**< - */

/**
Slow control device driver commands */
#define CMD_INIT                      1 /* misc. commands must be below 20 !! */
#define CMD_EXIT                      2
#define CMD_START                     3
#define CMD_STOP                      4
#define CMD_IDLE                      5
#define CMD_GET_THRESHOLD             6
#define CMD_GET_THRESHOLD_CURRENT     7
#define CMD_GET_THRESHOLD_ZERO        8
#define CMD_SET_LABEL                 9
#define CMD_GET_LABEL                10
#define CMD_OPEN                     11
#define CMD_CLOSE                    12
#define CMD_MISC_LAST                12 /* update this if you add new commands */

#define CMD_SET_FIRST                CMD_MISC_LAST+1 /* set commands */
#define CMD_SET                      CMD_SET_FIRST   // = 13
#define CMD_SET_VOLTAGE_LIMIT        CMD_SET_FIRST+1
#define CMD_SET_CURRENT_LIMIT        CMD_SET_FIRST+2
#define CMD_SET_RAMPUP               CMD_SET_FIRST+3
#define CMD_SET_RAMPDOWN             CMD_SET_FIRST+4
#define CMD_SET_TRIP_TIME            CMD_SET_FIRST+5
#define CMD_SET_CHSTATE              CMD_SET_FIRST+6
#define CMD_SET_LAST                 CMD_SET_FIRST+6 /* update this if you add new commands */

#define CMD_GET_FIRST                CMD_SET_LAST+1  /* multithreaded get commands */
#define CMD_GET                      CMD_GET_FIRST   // = 20
#define CMD_GET_CURRENT              CMD_GET_FIRST+1
#define CMD_GET_TRIP                 CMD_GET_FIRST+2
#define CMD_GET_STATUS               CMD_GET_FIRST+3
#define CMD_GET_TEMPERATURE          CMD_GET_FIRST+4
#define CMD_GET_DEMAND               CMD_GET_FIRST+5
#define CMD_GET_LAST                 CMD_GET_FIRST+5 /* update this if you add new commands ! */

#define CMD_GET_DIRECT               CMD_GET_LAST+1  /* direct get commands */
#define CMD_GET_VOLTAGE_LIMIT        CMD_GET_DIRECT  // = 25
#define CMD_GET_CURRENT_LIMIT        CMD_GET_DIRECT+2
#define CMD_GET_RAMPUP               CMD_GET_DIRECT+3
#define CMD_GET_RAMPDOWN             CMD_GET_DIRECT+4
#define CMD_GET_TRIP_TIME            CMD_GET_DIRECT+5
#define CMD_GET_CHSTATE              CMD_GET_DIRECT+6
#define CMD_GET_CRATEMAP             CMD_GET_DIRECT+7
#define CMD_GET_DEMAND_DIRECT        CMD_GET_DIRECT+8
#define CMD_GET_DIRECT_LAST          CMD_GET_DIRECT+8 /* update this if you add new commands ! */

#define CMD_ENABLE_COMMAND       (1<<14)  /* these two commands can be used to enable/disable */
#define CMD_DISABLE_COMMAND      (1<<15)  /* one of the other commands                        */

/**
Slow control bus driver commands */
#define CMD_WRITE                   100
#define CMD_READ                    101
#define CMD_PUTS                    102
#define CMD_GETS                    103
#define CMD_DEBUG                   104
#define CMD_NAME                    105

/**
Commands for interrupt events */
#define CMD_INTERRUPT_ENABLE        100
#define CMD_INTERRUPT_DISABLE       101
#define CMD_INTERRUPT_ATTACH        102
#define CMD_INTERRUPT_DETACH        103

/**
macros for bus driver access */
#define BD_GETS(s,z,p,t)   info->bd(CMD_GETS, info->bd_info, s, z, p, t)
#define BD_READS(s,z,t)    info->bd(CMD_READ, info->bd_info, s, z, t)
#define BD_PUTS(s)         info->bd(CMD_PUTS, info->bd_info, s)
#define BD_WRITES(s)       info->bd(CMD_WRITE, info->bd_info, s)

/**dox***************************************************************/
          /** @} *//* end of 26 */

/**dox***************************************************************/
          /** @} *//* end of mdeferrorh */


#define ANA_CONTINUE                  1
#define ANA_SKIP                      0


/*---- Buffer manager structures -----------------------------------*/

/**dox***************************************************************/
/** @addtogroup msectionh
 *  @{  */

/**
Event header */
typedef struct {
   short int event_id;           /**< event ID starting from one      */
   short int trigger_mask;       /**< hardware trigger mask           */
   DWORD serial_number;          /**< serial number starting from one */
   DWORD time_stamp;             /**< time of production of event     */
   DWORD data_size;              /**< size of event in bytes w/o header */
} EVENT_HEADER;

/** @} */

/**
TRIGGER_MASK
Extract or set the trigger mask field pointed by the argument.
@param e pointer to the midas event (pevent)
*/
#define TRIGGER_MASK(e)    ((((EVENT_HEADER *) e)-1)->trigger_mask)

/**
EVENT_ID
Extract or set the event ID field pointed by the argument..
@param e pointer to the midas event (pevent)
*/
#define EVENT_ID(e)        ((((EVENT_HEADER *) e)-1)->event_id)

/**
SERIAL_NUMBER
Extract or set/reset the serial number field pointed by the argument.
@param e pointer to the midas event (pevent)
*/
#define SERIAL_NUMBER(e)   ((((EVENT_HEADER *) e)-1)->serial_number)

/**
TIME_STAMP
Extract or set/reset the time stamp field pointed by the argument.
@param e pointer to the midas event (pevent)
*/
#define TIME_STAMP(e)      ((((EVENT_HEADER *) e)-1)->time_stamp)

/**
DATA_SIZE
Extract or set/reset the data size field pointed by the argument.
@param e pointer to the midas event (pevent)
*/
#define DATA_SIZE(e)      ((((EVENT_HEADER *) e)-1)->data_size)

#define EVENT_SOURCE(e,o)  (* (INT*) (e+o))

/**
system event IDs */
#define EVENTID_BOR      ((short int) 0x8000)  /**< Begin-of-run      */
#define EVENTID_EOR      ((short int) 0x8001)  /**< End-of-run        */
#define EVENTID_MESSAGE  ((short int) 0x8002)  /**< Message events    */

/**
fragmented events */
#define EVENTID_FRAG1    ((short int) 0xC000)  /* first fragment */
#define EVENTID_FRAG     ((short int) 0xD000)  /* added to real event-id */

/**
magic number used in trigger_mask for BOR event */
#define MIDAS_MAGIC      0x494d            /**< 'MI' */

/** @addtogroup msectionh
 *  @{  */

/**
   Handler for event requests */
typedef void (EVENT_HANDLER)(HNDLE buffer_handler, HNDLE request_id, EVENT_HEADER* event_header, void* event_data);

/**
   Handler for rpc requests */
typedef INT (RPC_HANDLER)(INT index, void *prpc_param[]);

/**
Buffer structure */
typedef struct {
   INT id;                       /**< request id                      */
   BOOL valid;                   /**< indicating a valid entry        */
   short int event_id;           /**< event ID                        */
   short int trigger_mask;       /**< trigger mask                    */
   INT sampling_type;            /**< GET_ALL, GET_NONBLOCKING, GET_RECENT */
} EVENT_REQUEST;

typedef struct {
   char name[NAME_LENGTH];            /**< name of client             */
   INT pid;                           /**< process ID                 */
   INT unused0;                       /**< was thread ID              */
   INT unused;                        /**< was thread handle          */
   INT port;                          /**< UDP port for wake up       */
   INT read_pointer;                  /**< read pointer to buffer     */
   INT max_request_index;             /**< index of last request      */
   INT num_received_events;           /**< no of received events      */
   INT num_sent_events;               /**< no of sent events          */
   INT unused1;                       /**< was num_waiting_events     */
   float unused2;                     /**< was data_rate              */
   BOOL read_wait;                    /**< wait for read - flag       */
   INT write_wait;                    /**< wait for write # bytes     */
   BOOL wake_up;                      /**< client got a wake-up msg   */
   BOOL all_flag;                     /**< at least one GET_ALL request */
   DWORD last_activity;               /**< time of last activity      */
   DWORD watchdog_timeout;            /**< timeout in ms              */

   EVENT_REQUEST event_request[MAX_EVENT_REQUESTS];

} BUFFER_CLIENT;

typedef struct {
   char name[NAME_LENGTH];            /**< name of buffer             */
   INT num_clients;                   /**< no of active clients       */
   INT max_client_index;              /**< index of last client + 1   */
   INT size;                          /**< size of data area in bytes */
   INT read_pointer;                  /**< read pointer               */
   INT write_pointer;                 /**< write pointer              */
   INT num_in_events;                 /**< no of received events      */
   INT num_out_events;                /**< no of distributed events   */

   BUFFER_CLIENT client[MAX_CLIENTS]; /**< entries for clients        */

} BUFFER_HEADER;

/* Per-process buffer access structure (descriptor) */

/*
 * BUFFER locking rules:
 * locking order: read_cache_mutex -> write_cache_mutex -> buffer_mutex -> buffer semaphore
 * lock read_cache_mutex to access read_cache_xxx, ok to check read_cache_size is not 0
 * lock write_cache_mutex to access write_cache_xxx, ok to check write_cache_size is not 0
 * lock buffer_mutex to access all other data in BUFFER
 * lock buffer semaphore to access data in shared memory pointed to by buffer_header
 * to avoid deadlocks:
 * do not call ODB functions while holding any any of these locks.
 * ok to call cm_msg()
 */

struct BUFFER
{
   std::atomic_bool attached{false};  /**< TRUE if buffer is attached   */
   std::timed_mutex buffer_mutex;     /**< buffer mutex                 */
   INT client_index = 0;              /**< index to CLIENT str. in buf. */
   char client_name[NAME_LENGTH];     /**< name of client               */
   char buffer_name[NAME_LENGTH];     /**< name of buffer               */
   BUFFER_HEADER *buffer_header = NULL; /**< pointer to buffer header   */
   std::timed_mutex read_cache_mutex; /**< cache read mutex             */
   std::atomic<size_t> read_cache_size{0}; /**< cache size in bytes     */
   char *read_cache = NULL;           /**< cache for burst read         */
   size_t read_cache_rp = 0;          /**< cache read pointer           */
   size_t read_cache_wp = 0;          /**< cache write pointer          */
   std::timed_mutex write_cache_mutex; /**< cache write mutex           */
   std::atomic<size_t> write_cache_size{0}; /**< cache size in bytes    */
   char *write_cache = NULL;          /**< cache for burst read         */
   size_t write_cache_rp = 0;         /**< cache read pointer           */
   size_t write_cache_wp = 0;         /**< cache write pointer          */
   HNDLE semaphore = 0;               /**< semaphore handle             */
   INT shm_handle = 0;                /**< handle to shared memory      */
   size_t shm_size = 0;               /**< size of shared memory        */
   BOOL callback = false;             /**< callback defined for this buffer */
   BOOL locked = false;               /**< buffer is currently locked by us */
   BOOL get_all_flag = false;         /**< this is a get_all reader     */

   /* buffer statistics */
   int count_lock = 0;                /**< count how many times we locked the buffer */
   int count_sent = 0;                /**< count how many events we sent */
   double bytes_sent = 0;             /**< count how many bytes we sent */
   int count_write_wait = 0;          /**< count how many times we waited for free space */
   DWORD time_write_wait = 0;         /**< count for how long we waited for free space, in units of ss_millitime() */
   int last_count_lock = 0;           /**< avoid writing statistics to odb if lock count did not change */
   DWORD wait_start_time = 0;         /**< time when we started the wait */
   int wait_client_index = 0;         /**< waiting for which client */
   int max_requested_space = 0;       /**< waiting for this many bytes of free space */
   int count_read = 0;                /**< count how many events we read */
   double bytes_read = 0;             /**< count how many bytes we read */
   int client_count_write_wait[MAX_CLIENTS]; /**< per-client count_write_wait */
   DWORD client_time_write_wait[MAX_CLIENTS]; /**< per-client time_write_wait */
};

typedef struct {
   DWORD type;                        /**< TID_xxx type                      */
   INT num_values;                    /**< number of values                  */
   char name[NAME_LENGTH];            /**< name of variable                  */
   INT data;                          /**< Address of variable (offset)      */
   INT total_size;                    /**< Total size of data block          */
   INT item_size;                     /**< Size of single data item          */
   WORD access_mode;                  /**< Access mode                       */
   WORD notify_count;                 /**< Notify counter                    */
   INT next_key;                      /**< Address of next key               */
   INT parent_keylist;                /**< keylist to which this key belongs */
   INT last_written;                  /**< Time of last write action  */
} KEY;

typedef struct {
   INT parent;                        /**< Address of parent key      */
   INT num_keys;                      /**< number of keys             */
   INT first_key;                     /**< Address of first key       */
} KEYLIST;

/** @} */

/*---- Equipment ---------------------------------------------------*/

#define DF_INPUT              (1<<0)  /**< channel is input           */
#define DF_OUTPUT             (1<<1)  /**< channel is output          */
#define DF_PRIO_DEVICE        (1<<2)  /**< get demand values from device instead of ODB */
#define DF_READ_ONLY          (1<<3)  /**< never write demand values to device */
#define DF_MULTITHREAD        (1<<4)  //*< access device with a dedicated thread */
#define DF_HW_RAMP            (1<<5)  //*< high voltage device can do hardware ramping */
#define DF_LABELS_FROM_DEVICE (1<<6)  //*< pull HV channel names from device */
#define DF_REPORT_TEMP        (1<<7)  //*< report temperature from HV cards */
#define DF_REPORT_STATUS      (1<<8)  //*< report status word from HV channels */
#define DF_REPORT_CHSTATE     (1<<9)  //*< report channel state word from HV channels */
#define DF_REPORT_CRATEMAP    (1<<10) //*< reports an integer encoding size and occupancy of HV crate */
#define DF_QUICKSTART         (1<<11) //*< do not read channels initially during init to speed up startup */
#define DF_POLL_DEMAND        (1<<12) //*< continously read demand value from device */
#define DF_PRIORITY_READ      (1<<13) //*< read channel with priority after setting */

/** @addtogroup msectionh
 *  @{  */

typedef struct {
   char name[NAME_LENGTH];            /**< Driver name                       */
   INT(*bd) (INT cmd, ...);           /**< Device driver entry point         */
   void *bd_info;                     /**< Private info for bus driver       */
} BUS_DRIVER;

typedef struct {
   float variable[CMD_GET_LAST+1];    /**< Array for various values          */
   char  label[NAME_LENGTH];          /**< Array for channel labels          */
   int   n_read;                      /**< Number of reads                   */
} DD_MT_CHANNEL;

typedef struct {
   INT n_channels;                    /**< Number of channels                */
   midas_thread_t thread_id;          /**< Thread ID                         */
   INT status;                        /**< Status passed from device thread  */
   DD_MT_CHANNEL *channel;            /**< One data set for each channel     */

} DD_MT_BUFFER;

typedef struct {
   WORD event_id;                     /**< Event ID associated with equipm.  */
   WORD trigger_mask;                 /**< Trigger mask                      */
   char buffer[NAME_LENGTH];          /**< Event buffer to send events into  */
   INT eq_type;                       /**< One of EQ_xxx                     */
   INT source;                        /**< Event source (LAM/IRQ)            */
   char format[8];                    /**< Data format to produce            */
   BOOL enabled;                      /**< Enable flag                       */
   INT read_on;                       /**< Combination of Read-On flags RO_xxx */
   INT period;                        /**< Readout interval/Polling time in ms */
   double event_limit;                /**< Stop run when limit is reached    */
   DWORD num_subevents;               /**< Number of events in super event */
   INT history;                       /**< Log history                       */
   char frontend_host[NAME_LENGTH];   /**< Host on which FE is running       */
   char frontend_name[NAME_LENGTH];   /**< Frontend name                     */
   char frontend_file_name[256];      /**< Source file used for user FE      */
   char status[256];                  /**< Current status of equipment       */
   char status_color[NAME_LENGTH];    /**< Color or class to be used by mhttpd for status */
   BOOL hidden;                       /**< Hidden flag                       */
   INT  write_cache_size;             /**< Event buffer write cache size     */
} EQUIPMENT_INFO;

/** @} */

#define EQUIPMENT_COMMON_STR "\
Event ID = WORD : 0\n\
Trigger mask = WORD : 0\n\
Buffer = STRING : [32] SYSTEM\n\
Type = INT : 0\n\
Source = INT : 0\n\
Format = STRING : [8] FIXED\n\
Enabled = BOOL : 0\n\
Read on = INT : 0\n\
Period = INT : 0\n\
Event limit = DOUBLE : 0\n\
Num subevents = DWORD : 0\n\
Log history = INT : 0\n\
Frontend host = STRING : [32] \n\
Frontend name = STRING : [32] \n\
Frontend file name = STRING : [256] \n\
Status = STRING : [256] \n\
Status color = STRING : [32] \n\
Hidden = BOOL : 0\n\
Write cache size = INT : 100000\n\
"

/** @addtogroup msectionh
 *  @{  */

typedef struct {
   double events_sent;
   double events_per_sec;
   double kbytes_per_sec;
} EQUIPMENT_STATS;

/** @} */

#define EQUIPMENT_STATISTICS_STR "\
Events sent = DOUBLE : 0\n\
Events per sec. = DOUBLE : 0\n\
kBytes per sec. = DOUBLE : 0\n\
"

typedef struct eqpmnt *PEQUIPMENT;

/** @addtogroup msectionh
 *  @{  */

typedef struct {
   char name[NAME_LENGTH];            /**< Driver name                       */
   INT(*dd) (INT cmd, ...);           /**< Device driver entry point         */
   INT channels;                      /**< Number of channels                */
   INT(*bd) (INT cmd, ...);           /**< Bus driver entry point            */
   DWORD flags;                       /**< Combination of DF_xx              */
   BOOL enabled;                      /**< Enable flag                       */
   void *dd_info;                     /**< Private info for device driver    */
   DD_MT_BUFFER *mt_buffer;           /**< pointer to multithread buffer     */
   INT stop_thread;                   /**< flag used to stop the thread      */
   MUTEX_T *mutex;                    /**< mutex for buffer                  */
   EQUIPMENT_INFO *pequipment;        /**< pointer to equipment              */
   std::string *pequipment_name;      /**< name of equipment                 */
   BOOL priority_read;                /**< read channel with priority after set */
} DEVICE_DRIVER;

typedef struct eqpmnt {
   char name[NAME_LENGTH];              /**< Equipment name                            */
   EQUIPMENT_INFO info;                 /**< From above                                */
    INT(*readout) (char *, INT);        /**< Pointer to user readout routine           */
    INT(*cd) (INT cmd, PEQUIPMENT);     /**< Class driver routine                      */
   DEVICE_DRIVER *driver;               /**< Device driver list                        */
   void *event_descrip;                 /**< Init string for fixed events or bank list */
   void *cd_info;                       /**< private data for class driver             */
   INT status;                          /**< One of FE_xxx                             */
   DWORD last_called;                   /**< Last time event was read                  */
   DWORD last_idle;                     /**< Last time idle func was called            */
   DWORD poll_count;                    /**< Needed to poll 'period'                   */
   INT format;                          /**< FORMAT_xxx                                */
   HNDLE buffer_handle;                 /**< MIDAS buffer handle                       */
   HNDLE hkey_variables;                /**< Key to variables subtree in ODB           */
   DWORD serial_number;                 /**< event serial number                       */
   DWORD subevent_number;               /**< subevent number                           */
   DWORD odb_out;                       /**< # updates FE -> ODB                       */
   DWORD odb_in;                        /**< # updated ODB -> FE                       */
   DWORD bytes_sent;                    /**< number of bytes sent                      */
   DWORD events_sent;                   /**< number of events sent                     */
   DWORD events_collected;              /**< number of collected events for EQ_USER    */
   EQUIPMENT_STATS stats;
} EQUIPMENT;

/** @} */

INT device_driver(DEVICE_DRIVER *device_driver, INT cmd, ...);

/*---- Banks -------------------------------------------------------*/

#define BANK_FORMAT_VERSION             1  /**< - */
#define BANK_FORMAT_32BIT           (1<<4) /**< - */
#define BANK_FORMAT_64BIT_ALIGNED   (1<<5) /**< - */

/** @addtogroup msectionh
 *  @{  */

typedef struct {
   DWORD data_size;                    /**< Size in bytes */
   DWORD flags;                        /**< internal flag */
} BANK_HEADER;

typedef struct {
   char name[4];                       /**< - */
   WORD type;                          /**< - */
   WORD data_size;                     /**< - */
} BANK;

typedef struct {
   char name[4];                       /**< - */
   DWORD type;                         /**< - */
   DWORD data_size;                    /**< - */
} BANK32;

typedef struct {
   char name[4];                       /**< - */
   DWORD type;                         /**< - */
   DWORD data_size;                    /**< - */
   DWORD reserved;                     /**< - */
} BANK32A;

typedef struct {
   char name[NAME_LENGTH];             /**< - */
   DWORD type;                         /**< - */
   DWORD n_data;                       /**< - */
} TAG;

typedef struct {
   char name[9];                       /**< - */
   WORD type;                          /**< - */
   DWORD size;                         /**< - */
   char **init_str;                    /**< - */
   BOOL output_flag;                   /**< - */
   void *addr;                         /**< - */
   DWORD n_data;                       /**< - */
   HNDLE def_key;                      /**< - */
} BANK_LIST;

/** @} */

/*---- Analyzer request --------------------------------------------*/

typedef struct {
   char name[NAME_LENGTH];            /**< Module name                       */
   char author[NAME_LENGTH];          /**< Author                            */
    INT(*analyzer) (EVENT_HEADER *, void *);
                                           /**< Pointer to user analyzer routine  */
    INT(*bor) (INT run_number);       /**< Pointer to begin-of-run routine   */
    INT(*eor) (INT run_number);       /**< Pointer to end-of-run routine     */
    INT(*init) (void);                /**< Pointer to init routine           */
    INT(*exit) (void);                /**< Pointer to exit routine           */
   void *parameters;                  /**< Pointer to parameter structure    */
   INT param_size;                    /**< Size of parameter structure       */
   const char **init_str;             /**< Parameter init string             */
   BOOL enabled;                      /**< Enabled flag                      */
   void *histo_folder;
} ANA_MODULE;

typedef struct {
   INT event_id;                      /**< Event ID associated with equipm.  */
   INT trigger_mask;                  /**< Trigger mask                      */
   INT sampling_type;                 /**< GET_ALL/GET_NONBLOCKING/GET_RECENT*/
   char buffer[NAME_LENGTH];          /**< Event buffer to send events into  */
   BOOL enabled;                      /**< Enable flag                       */
   char client_name[NAME_LENGTH];     /**< Analyzer name                     */
   char host[NAME_LENGTH];            /**< Host on which analyzer is running */
} AR_INFO;

typedef struct {
   double events_received;
   double events_per_sec;
   double events_written;
} AR_STATS;

typedef struct {
   char event_name[NAME_LENGTH];      /**< Event name                        */
   AR_INFO ar_info;                   /**< From above                        */
    INT(*analyzer) (EVENT_HEADER *, void *);/**< Pointer to user analyzer routine  */
   ANA_MODULE **ana_module;           /**< List of analyzer modules          */
   BANK_LIST *bank_list;              /**< List of banks for event           */
   INT rwnt_buffer_size;              /**< Size in events of RW N-tuple buf  */
   BOOL use_tests;                    /**< Use tests for this event          */
   char **init_string;
   INT status;                        /**< One of FE_xxx                     */
   HNDLE buffer_handle;               /**< MIDAS buffer handle               */
   HNDLE request_id;                  /**< Event request handle              */
   HNDLE hkey_variables;              /**< Key to variables subtree in ODB   */
   HNDLE hkey_common;                 /**< Key to common subtree             */
   void *addr;                        /**< Buffer for CWNT filling           */
   struct {
      DWORD run;
      DWORD serial;
      DWORD time;
   } number;                          /**< Buffer for event number for CWNT  */
   DWORD events_received;             /**< number of events sent             */
   DWORD events_written;              /**< number of events written          */
   AR_STATS ar_stats;

} ANALYZE_REQUEST;

/* output file information, maps to /<analyzer>/Output */
typedef struct {
   char filename[256];
   BOOL rwnt;
   BOOL histo_dump;
   char histo_dump_filename[256];
   BOOL clear_histos;
   char last_histo_filename[256];
   BOOL events_to_odb;
   char global_memory_name[8];
} ANA_OUTPUT_INFO;

#define ANA_OUTPUT_INFO_STR "\
Filename = STRING : [256] run%05d.asc\n\
RWNT = BOOL : 0\n\
Histo Dump = BOOL : 0\n\
Histo Dump Filename = STRING : [256] his%05d.rz\n\
Clear histos = BOOL : 1\n\
Last Histo Filename = STRING : [256] last.rz\n\
Events to ODB = BOOL : 1\n\
Global Memory Name = STRING : [8] ONLN\n\
"

/*---- Tests -------------------------------------------------------*/

typedef struct {
   char name[80];
   BOOL registered;
   DWORD count;
   DWORD previous_count;
   BOOL value;
} ANA_TEST;

#define SET_TEST(t, v) { if (!t.registered) test_register(&t); t.value = (v); }
#define TEST(t) (t.value)

#ifdef DEFINE_TESTS
#define DEF_TEST(t) ANA_TEST t = { #t, 0, 0, FALSE };
#else
#define DEF_TEST(t) extern ANA_TEST t;
#endif

/*---- History structures ------------------------------------------*/

#define RT_DATA (*((DWORD *) "HSDA"))
#define RT_DEF  (*((DWORD *) "HSDF"))

typedef struct {
   DWORD record_type;           /* RT_DATA or RT_DEF */
   DWORD event_id;
   DWORD time;
   DWORD def_offset;            /* place of definition */
   DWORD data_size;             /* data following this header in bytes */
} HIST_RECORD;

typedef struct {
   DWORD event_id;
   char event_name[NAME_LENGTH];
   DWORD def_offset;
} DEF_RECORD;

typedef struct {
   DWORD event_id;
   DWORD time;
   DWORD offset;
} INDEX_RECORD;

typedef struct history_struct {
   DWORD event_id = 0;
   std::string event_name;
   DWORD n_tag = 0;
   TAG *tag = NULL;
   std::string hist_fn;
   std::string index_fn;
   std::string def_fn;
   int hist_fh  = 0;
   int index_fh = 0;
   int def_fh   = 0;
   DWORD base_time  = 0;
   DWORD def_offset = 0;
} HISTORY;

/*---- ODB runinfo -------------------------------------------------*/

typedef struct {
   INT state;                         /**< Current run condition  */
   INT online_mode;                   /**< Mode of operation online/offline */
   INT run_number;                    /**< Current processing run number      */
   INT transition_in_progress;        /**< Intermediate state during transition */
   INT start_abort;                   /**< Set if run start was aborted */
   INT requested_transition;          /**< Deferred transition request */
   char start_time[32];               /**< ASCII of the last start time */
   DWORD start_time_binary;           /**< Bin of the last start time */
   char stop_time[32];                /**< ASCII of the last stop time */
   DWORD stop_time_binary;            /**< ASCII of the last stop time */
} RUNINFO;

#define RUNINFO_STR(_name) const char *_name[] = {\
"[.]",\
"State = INT : 1",\
"Online Mode = INT : 1",\
"Run number = INT : 0",\
"Transition in progress = INT : 0",\
"Start abort = INT : 0",\
"Requested transition = INT : 0",\
"Start time = STRING : [32] Tue Sep 09 15:04:42 1997",\
"Start time binary = DWORD : 0",\
"Stop time = STRING : [32] Tue Sep 09 15:04:42 1997",\
"Stop time binary = DWORD : 0",\
"",\
NULL }

/*---- Alarm system ------------------------------------------------*/

/********************************************************************/
/**
Program information structure */
typedef struct {
   BOOL required;
   INT watchdog_timeout;
   DWORD check_interval;
   char start_command[256];
   BOOL auto_start;
   BOOL auto_stop;
   BOOL auto_restart;
   char alarm_class[32];
   DWORD first_failed;
} PROGRAM_INFO;

#define AT_INTERNAL   1 /**< - */
#define AT_PROGRAM    2 /**< - */
#define AT_EVALUATED  3 /**< - */
#define AT_PERIODIC   4 /**< - */
#define AT_LAST       4 /**< - */

#define PROGRAM_INFO_STR(_name) const char *_name[] = {\
"[.]",\
"Required = BOOL : n",\
"Watchdog timeout = INT : 10000",\
"Check interval = DWORD : 180000",\
"Start command = STRING : [256] ",\
"Auto start = BOOL : n",\
"Auto stop = BOOL : n",\
"Auto restart = BOOL : n",\
"Alarm class = STRING : [32] ",\
"First failed = DWORD : 0",\
"",\
NULL }

/**
Alarm class structure */
typedef struct {
   BOOL write_system_message;
   BOOL write_elog_message;
   INT system_message_interval;
   DWORD system_message_last;
   char execute_command[256];
   INT execute_interval;
   DWORD execute_last;
   BOOL stop_run;
   char display_bgcolor[32];
   char display_fgcolor[32];
} ALARM_CLASS;

#define ALARM_CLASS_STR(_name) const char *_name[] = {\
"[.]",\
"Write system message = BOOL : y",\
"Write Elog message = BOOL : n",\
"System message interval = INT : 60",\
"System message last = DWORD : 0",\
"Execute command = STRING : [256] ",\
"Execute interval = INT : 0",\
"Execute last = DWORD : 0",\
"Stop run = BOOL : n",\
"Display BGColor = STRING : [32] red",\
"Display FGColor = STRING : [32] black",\
"",\
NULL }

/**
Alarm structure */
typedef struct {
   BOOL active;
   INT triggered;
   INT type;
   INT check_interval;
   DWORD checked_last;
   char time_triggered_first[32];
   char time_triggered_last[32];
   char condition[256];
   char alarm_class[32];
   char alarm_message[80];
} ALARM;

#define ALARM_ODB_STR(_name) const char *_name[] = {\
"[.]",\
"Active = BOOL : n",\
"Triggered = INT : 0",\
"Type = INT : 3",\
"Check interval = INT : 60",\
"Checked last = DWORD : 0",\
"Time triggered first = STRING : [32] ",\
"Time triggered last = STRING : [32] ",\
"Condition = STRING : [256] /Runinfo/Run number > 100",\
"Alarm Class = STRING : [32] Alarm",\
"Alarm Message = STRING : [80] Run number became too large",\
"",\
NULL }

#define ALARM_PERIODIC_STR(_name) const char *_name[] = {\
"[.]",\
"Active = BOOL : n",\
"Triggered = INT : 0",\
"Type = INT : 4",\
"Check interval = INT : 28800",\
"Checked last = DWORD : 0",\
"Time triggered first = STRING : [32] ",\
"Time triggered last = STRING : [32] ",\
"Condition = STRING : [256] ",\
"Alarm Class = STRING : [32] Warning",\
"Alarm Message = STRING : [80] Please do your shift checks",\
"",\
NULL }

/*---- malloc/free routines for debugging --------------------------*/

#ifdef _MEM_DBG
#define M_MALLOC(x)   dbg_malloc((x), __FILE__, __LINE__)
#define M_CALLOC(x,y) dbg_calloc((x), (y), __FILE__, __LINE__)
#define M_FREE(x)     dbg_free  ((x), __FILE__, __LINE__)
#else
#define M_MALLOC(x) malloc(x)
#define M_CALLOC(x,y) calloc(x,y)
#define M_FREE(x) free(x)
#endif

void *dbg_malloc(unsigned int size, char *file, int line);
void *dbg_calloc(unsigned int size, unsigned int count, char *file, int line);
void dbg_free(void *adr, char *file, int line);

/*---- CERN libray -------------------------------------------------*/

#ifdef extname
#define PAWC_NAME pawc_
#else
#define PAWC_NAME PAWC
#endif

#define PAWC_DEFINE(size) \
INT PAWC_NAME[size/4];    \
INT pawc_size = size

/* bug in ROOT include files */
#undef GetCurrentTime

/*---- RPC ---------------------------------------------------------*/

/**
flags */
#define RPC_IN       (1 << 0)
#define RPC_OUT      (1 << 1)
#define RPC_POINTER  (1 << 2)
#define RPC_FIXARRAY (1 << 3)
#define RPC_VARARRAY (1 << 4)
#define RPC_OUTGOING (1 << 5)

/**
function list */
typedef struct {
   WORD tid;
   WORD flags;
   INT n;
} RPC_PARAM;

#define MAX_RPC_PARAMS 20

typedef struct {
   INT id;
   const char *name;
   RPC_PARAM param[MAX_RPC_PARAMS];
   RPC_HANDLER *dispatch;
} RPC_LIST;

/**
IDs allow for users */
#define RPC_MIN_ID    1
#define RPC_MAX_ID 9999

/**
Data conversion flags */
#define CF_ENDIAN          (1<<0)
#define CF_IEEE2VAX        (1<<1)
#define CF_VAX2IEEE        (1<<2)
//#define CF_ASCII           (1<<3)

#define CBYTE(_i)        (* ((BYTE *)       prpc_param[_i]))
#define CPBYTE(_i)       (  ((BYTE *)       prpc_param[_i]))

#define CSHORT(_i)       (* ((short *)      prpc_param[_i]))
#define CPSHORT(_i)      (  ((short *)      prpc_param[_i]))

#define CINT(_i)         (* ((INT *)        prpc_param[_i]))
#define CPINT(_i)        (  ((INT *)        prpc_param[_i]))

#define CWORD(_i)        (* ((WORD *)       prpc_param[_i]))
#define CPWORD(_i)       (  ((WORD *)       prpc_param[_i]))

#define CLONG(_i)        (* ((long *)       prpc_param[_i]))
#define CPLONG(_i)       (  ((long *)       prpc_param[_i]))

#define CDWORD(_i)       (* ((DWORD *)      prpc_param[_i]))
#define CPDWORD(_i)      (  ((DWORD *)      prpc_param[_i]))

#define CHNDLE(_i)       (* ((HNDLE *)      prpc_param[_i]))
#define CPHNDLE(_i)      (  ((HNDLE *)      prpc_param[_i]))

#define CBOOL(_i)        (* ((BOOL *)       prpc_param[_i]))
#define CPBOOL(_i)       (  ((BOOL *)       prpc_param[_i]))

#define CFLOAT(_i)       (* ((float *)      prpc_param[_i]))
#define CPFLOAT(_i)      (  ((float *)      prpc_param[_i]))

#define CDOUBLE(_i)      (* ((double *)     prpc_param[_i]))
#define CPDOUBLE(_i)     (  ((double *)     prpc_param[_i]))

#define CSTRING(_i)      (  ((char *)       prpc_param[_i]))
#define CARRAY(_i)       (  ((void *)       prpc_param[_i]))

#define CBYTE(_i)        (* ((BYTE *)       prpc_param[_i]))
#define CPBYTE(_i)       (  ((BYTE *)       prpc_param[_i]))

#define CSHORT(_i)       (* ((short *)      prpc_param[_i]))
#define CPSHORT(_i)      (  ((short *)      prpc_param[_i]))

#define CINT(_i)         (* ((INT *)        prpc_param[_i]))
#define CPINT(_i)        (  ((INT *)        prpc_param[_i]))

#define CWORD(_i)        (* ((WORD *)       prpc_param[_i]))
#define CPWORD(_i)       (  ((WORD *)       prpc_param[_i]))

#define CLONG(_i)        (* ((long *)       prpc_param[_i]))
#define CPLONG(_i)       (  ((long *)       prpc_param[_i]))

#define CDWORD(_i)       (* ((DWORD *)      prpc_param[_i]))
#define CPDWORD(_i)      (  ((DWORD *)      prpc_param[_i]))

#define CHNDLE(_i)       (* ((HNDLE *)      prpc_param[_i]))
#define CPHNDLE(_i)      (  ((HNDLE *)      prpc_param[_i]))

#define CBOOL(_i)        (* ((BOOL *)       prpc_param[_i]))
#define CPBOOL(_i)       (  ((BOOL *)       prpc_param[_i]))

#define CFLOAT(_i)       (* ((float *)      prpc_param[_i]))
#define CPFLOAT(_i)      (  ((float *)      prpc_param[_i]))

#define CDOUBLE(_i)      (* ((double *)     prpc_param[_i]))
#define CPDOUBLE(_i)     (  ((double *)     prpc_param[_i]))

#define CSTRING(_i)      (  ((char *)       prpc_param[_i]))
#define CARRAY(_i)       (  ((void *)       prpc_param[_i]))



#define cBYTE            (* ((BYTE *)       prpc_param[--n_param]))
#define cPBYTE           (  ((BYTE *)       prpc_param[--n_param]))

#define cSHORT           (* ((short *)      prpc_param[--n_param]))
#define cPSHORT          (  ((short *)      prpc_param[--n_param]))

#define cINT             (* ((INT *)        prpc_param[--n_param]))
#define cPINT            (  ((INT *)        prpc_param[--n_param]))

#define cWORD            (* ((WORD *)       prpc_param[--n_param]))
#define cPWORD           (  ((WORD *)       prpc_param[--n_param]))

#define cLONG            (* ((long *)       prpc_param[--n_param]))
#define cPLONG           (  ((long *)       prpc_param[--n_param]))

#define cDWORD           (* ((DWORD *)      prpc_param[--n_param]))
#define cPDWORD          (  ((DWORD *)      prpc_param[--n_param]))

#define cHNDLE           (* ((HNDLE *)      prpc_param[--n_param]))
#define cPHNDLE          (  ((HNDLE *)      prpc_param[--n_param]))

#define cBOOL            (* ((BOOL *)       prpc_param[--n_param]))
#define cPBOOL           (  ((BOOL *)       prpc_param[--n_param]))

#define cFLOAT           (* ((float *)      prpc_param[--n_param]))
#define cPFLOAT          (  ((float *)      prpc_param[--n_param]))

#define cDOUBLE          (* ((double *)     prpc_param[--n_param]))
#define cPDOUBLE         (  ((double *)     prpc_param[--n_param]))

#define cSTRING          (  ((char *)       prpc_param[--n_param]))
#define cARRAY           (  ((void *)       prpc_param[--n_param]))

/**
flags for db_json_save() */
#define JS_LEVEL_0        0
#define JS_LEVEL_1        1
#define JS_MUST_BE_SUBDIR 1
#define JSFLAG_SAVE_KEYS         (1<<1)
#define JSFLAG_FOLLOW_LINKS      (1<<2)
#define JSFLAG_RECURSE           (1<<3)
#define JSFLAG_LOWERCASE         (1<<4)
#define JSFLAG_OMIT_NAMES        (1<<5)
#define JSFLAG_OMIT_LAST_WRITTEN (1<<6)
#define JSFLAG_OMIT_OLD          (1<<7)

/*---- Function declarations ---------------------------------------*/

/* make functions under WinNT dll exportable */
#if defined(OS_WINNT) && defined(MIDAS_DLL)
#define EXPRT __declspec(dllexport)
#else
#define EXPRT
#endif

   /*---- common routines ----*/
   std::string cm_get_error(INT code);
   const char* EXPRT cm_get_version(void);
   const char* EXPRT cm_get_revision(void);
   [[deprecated("Use std::string cm_get_experiment_name();")]]
   INT EXPRT cm_get_experiment_name(char *name, int name_size);
   std::string cm_get_experiment_name();
   INT EXPRT cm_get_environment(char *host_name, int host_name_size, char *exp_name, int exp_name_size);
   INT EXPRT cm_get_environment(std::string *host_name, std::string *exp_name);
   INT EXPRT cm_list_experiments_local(STRING_LIST* exp_names);
   INT EXPRT cm_list_experiments_remote(const char *host_name, STRING_LIST* exp_names);
   [[deprecated("Use std::string cm_get_exptab_filename();")]]
   INT EXPRT cm_get_exptab_filename(char* filename, int filename_size);
   std::string cm_get_exptab_filename();
   [[deprecated("Please use the std::string version of this function")]]
   INT EXPRT cm_get_exptab(const char* exp_name, char* expdir, int expdir_size, char* expuser, int expuser_size);
   INT EXPRT cm_get_exptab(const char* exp_name, std::string* expdir, std::string* expuser);
   INT EXPRT cm_select_experiment_local(std::string *exp_name);
   INT EXPRT cm_select_experiment_remote(const char *host_name, std::string *exp_name);
   INT EXPRT cm_set_experiment_local(const char* exp_name);
   INT EXPRT cm_connect_experiment(const char *host_name, const char *exp_name,
                                   const char *client_name, void (*func) (char *));
   INT EXPRT cm_connect_experiment1(const char *host_name, const char *exp_name,
                                    const char *client_name,
                                    void (*func) (char *), INT odb_size,
                                    DWORD watchdog_timeout);
   INT EXPRT cm_disconnect_experiment(void);
   INT EXPRT cm_register_transition(INT transition, INT(*func) (INT, char *),
                                    int sequence_number);
   INT EXPRT cm_deregister_transition(INT transition);
   INT EXPRT cm_set_transition_sequence(INT transition, INT sequence_number);
   INT EXPRT cm_set_client_run_state(INT state);
   INT EXPRT cm_query_transition(int *transition, int *run_number, int *trans_time);
   INT EXPRT cm_register_deferred_transition(INT transition, BOOL(*func) (INT, BOOL));
   INT EXPRT cm_check_deferred_transition(void);
   INT EXPRT cm_transition(INT transition, INT run_number, char *error, INT strsize, INT async_flag, INT debug_flag);
   INT EXPRT cm_transition_cleanup();
   std::string EXPRT cm_transition_name(int transition);
   INT EXPRT cm_register_server(void);
   INT EXPRT cm_register_function(INT id, INT(*func) (INT, void **));
   INT EXPRT cm_connect_client(const char *client_name, HNDLE * hConn);
   INT EXPRT cm_disconnect_client(HNDLE hConn, BOOL bShutdown);
   INT EXPRT cm_set_experiment_database(HNDLE hDB, HNDLE hKeyClient);
   INT EXPRT cm_get_experiment_database(HNDLE * hDB, HNDLE * hKeyClient);
   INT EXPRT cm_set_experiment_semaphore(INT semaphore_alarm, INT semaphore_elog, INT semaphore_history, INT semaphore_msg);
   INT EXPRT cm_get_experiment_semaphore(INT * semaphore_alarm, INT * semaphore_elog, INT * semaphore_history, INT * semaphore_msg);
   INT EXPRT cm_set_client_info(HNDLE hDB, HNDLE * hKeyClient,
                                const char *host_name, char *client_name,
                                INT computer_id, const char *password, DWORD watchdog_timeout);
#define HAVE_CM_GET_CLIENT_NAME 1
   std::string EXPRT cm_get_client_name();
   INT EXPRT cm_check_client(HNDLE hDB, HNDLE hKeyClient);
   INT EXPRT cm_set_watchdog_params(BOOL call_watchdog, DWORD timeout);
   INT EXPRT cm_get_watchdog_params(BOOL * call_watchdog, DWORD * timeout);
   INT EXPRT cm_get_watchdog_info(HNDLE hDB, const char *client_name, DWORD * timeout, DWORD * last);
   INT EXPRT cm_watchdog_thread(void*unused);
   INT EXPRT cm_start_watchdog_thread(void);
   INT EXPRT cm_stop_watchdog_thread(void);
   INT EXPRT cm_shutdown(const char *name, BOOL bUnique);
   INT EXPRT cm_exist(const char *name, BOOL bUnique);
   INT EXPRT cm_cleanup(const char *client_name, BOOL ignore_timeout);
   INT EXPRT cm_yield(INT millisec);
#define HAVE_CM_PERIODIC_TASKS 1
   INT EXPRT cm_periodic_tasks(void);
   INT EXPRT cm_execute(const char *command, char *result, INT buf_size);
   INT EXPRT cm_synchronize(DWORD * sec);
   [[deprecated("Please use the std::string version of this function")]]
   INT EXPRT cm_asctime(char *str, INT buf_size);
   std::string EXPRT cm_asctime();
   INT EXPRT cm_time(DWORD * t);
   BOOL EXPRT cm_is_ctrlc_pressed(void);
   void EXPRT cm_ack_ctrlc_pressed(void);
   INT EXPRT cm_exec_script(const char* odb_path_to_script);
   int EXPRT cm_write_event_to_odb(HNDLE hDB, HNDLE hKey, const EVENT_HEADER* pevent, INT format);

   INT EXPRT cm_set_msg_print(INT system_mask, INT user_mask, int (*func) (const char *));
   INT EXPRT cm_msg(INT message_type, const char *filename, INT line, const char *routine, const char *format, ...) MATTRPRINTF(5,6);
   INT EXPRT cm_msg1(INT message_type, const char *filename, INT line, const char *facility, const char *routine, const char *format, ...) MATTRPRINTF(6,7);
   INT EXPRT cm_msg_flush_buffer(void);
   INT EXPRT cm_msg_register(EVENT_HANDLER* func);
   INT EXPRT cm_msg_retrieve(INT n_message, char *message, INT buf_size);
   INT EXPRT cm_msg_retrieve2(const char *facility, time_t t, int min_messages, char** messages, int* num_messages);
   INT EXPRT cm_msg_facilities(STRING_LIST *list);
   void EXPRT cm_msg_get_logfile(const char *facility, time_t t, std::string* filename, std::string* linkname, std::string* linktarget);
   INT EXPRT cm_msg_open_buffer(void);
   INT EXPRT cm_msg_close_buffer(void);
   INT EXPRT cm_msg_early_init(void);

   BOOL EXPRT equal_ustring(const char *str1, const char *str2);
   BOOL EXPRT ends_with_ustring(const char *str, const char *suffix);
   bool EXPRT ends_with_char(const std::string& s, char c);
   BOOL EXPRT strmatch(char* pattern, char*str);
   void EXPRT strarrayindex(char* odbpath, int* index1, int* index2);
   std::string msprintf(const char *format, ...) MATTRPRINTF(1,2);

   std::string cm_expand_env(const char* str);

   /*---- buffer manager ----*/
   INT EXPRT bm_open_buffer(const char *buffer_name, INT buffer_size, INT * buffer_handle);
   INT EXPRT bm_get_buffer_handle(const char *buffer_name, INT * buffer_handle);
   INT EXPRT bm_close_buffer(INT buffer_handle);
   INT EXPRT bm_close_all_buffers(void);
   INT EXPRT bm_init_buffer_counters(INT buffer_handle);
   INT EXPRT bm_get_buffer_info(INT buffer_handle, BUFFER_HEADER * buffer_header);
   INT EXPRT bm_get_buffer_level(INT buffer_handle, INT * n_bytes);
   INT EXPRT bm_set_cache_size(INT buffer_handle, size_t read_size, size_t write_size);
   INT EXPRT bm_compose_event(EVENT_HEADER * event_header, short int event_id, short int trigger_mask, DWORD data_size, DWORD serial);
   INT EXPRT bm_compose_event_threadsafe(EVENT_HEADER *event_header, short int event_id, short int trigger_mask, DWORD data_size, DWORD *serial);
   INT EXPRT bm_match_event(short int event_id, short int trigger_mask, const EVENT_HEADER *pevent);
   INT EXPRT bm_request_event(INT buffer_handle,
                              short int event_id,
                              short int trigger_mask,
                              INT sampling_type,
                              INT * request_id,
                              EVENT_HANDLER *func
                              );
   INT EXPRT bm_add_event_request(INT buffer_handle,
                                  short int event_id,
                                  short int trigger_mask,
                                  INT sampling_type,
                                  EVENT_HANDLER *func,
                                  INT request_id);
   INT EXPRT bm_remove_event_request(INT buffer_handle, INT request_id);
   INT EXPRT bm_delete_request(INT request_id);
   INT EXPRT bm_send_event(INT buffer_handle, const EVENT_HEADER* event, int unused, int timeout_msec);
#define HAVE_BM_SEND_EVENT_VEC 1
   INT EXPRT bm_send_event_vec(INT buffer_handle, const std::vector<char>& event, int timeout_msec);
   INT EXPRT bm_send_event_vec(INT buffer_handle, const std::vector<std::vector<char>>& event, int timeout_msec);
   INT EXPRT bm_send_event_sg(INT buffer_handle, int sg_n, const char* const sg_ptr[], const size_t sg_len[], int timeout_msec);
   INT EXPRT bm_receive_event(INT buffer_handle, void *destination, INT * buf_size, int timeout_msec);
#define HAVE_BM_RECEIVE_EVENT_VEC 1
   INT EXPRT bm_receive_event_vec(INT buffer_handle, std::vector<char> *event, int timeout_msec);
#define HAVE_BM_RECEIVE_EVENT_ALLOC 1
   INT EXPRT bm_receive_event_alloc(INT buffer_handle, EVENT_HEADER** ppevent, int timeout_msec);
   INT EXPRT bm_skip_event(INT buffer_handle);
   INT EXPRT bm_flush_cache(INT buffer_handle, int timeout_msec);
   INT EXPRT bm_poll_event(void);
   INT EXPRT bm_empty_buffers(void);
   INT EXPRT bm_check_buffers(void);
   INT EXPRT bm_write_statistics_to_odb(void);

   /** @addtogroup odbfunctionc */
   /** @{ */

   /*---- online database functions -----*/
   INT EXPRT db_open_database(const char *database_name, INT database_size, HNDLE * hdb, const char *client_name);
   INT EXPRT db_close_database(HNDLE database_handle);
   INT EXPRT db_close_all_databases(void);
   INT EXPRT db_protect_database(HNDLE database_handle);

   INT EXPRT db_create_key(HNDLE hdb, HNDLE key_handle, const char *key_name, DWORD type);
   INT EXPRT db_create_link(HNDLE hdb, HNDLE key_handle, const char *link_name, const char *destination);
   INT EXPRT db_set_value(HNDLE hdb, HNDLE hKeyRoot, const char *key_name, const void *data, INT size, INT num_values, DWORD type);
   INT EXPRT db_set_value_index(HNDLE hDB, HNDLE hKeyRoot, const char *key_name, const void *data, INT data_size, INT index, DWORD type, BOOL truncate);
   INT EXPRT db_get_value(HNDLE hdb, HNDLE hKeyRoot, const char *key_name, void *data, INT * size, DWORD type, BOOL create);
   INT EXPRT db_resize_string(HNDLE hDB, HNDLE hKeyRoot, const char *key_name, int num_values, int max_string_size);
#define HAVE_DB_GET_VALUE_STRING_CREATE_STRING_LENGTH 1
   INT EXPRT db_get_value_string(HNDLE hdb, HNDLE hKeyRoot, const char *key_name, int index, std::string* s, BOOL create = FALSE, int create_string_length = 0);
   INT EXPRT db_set_value_string(HNDLE hDB, HNDLE hKeyRoot, const char *key_name, const std::string* s);
   INT EXPRT db_find_key(HNDLE hdb, HNDLE hkey, const char *name, HNDLE * hsubkey);
   INT EXPRT db_find_link(HNDLE hDB, HNDLE hKey, const char *key_name, HNDLE * subhKey);
   INT EXPRT db_find_key1(HNDLE hdb, HNDLE hkey, const char *name, HNDLE * hsubkey);
   INT EXPRT db_find_link1(HNDLE hDB, HNDLE hKey, const char *key_name, HNDLE * subhKey);
   INT EXPRT db_find_keys(HNDLE hDB, HNDLE hKey, char *odbpath, std::vector<HNDLE> &hKeyVector);
   INT EXPRT db_get_parent(HNDLE hDB, HNDLE hKey, HNDLE * parenthKey);
   INT EXPRT db_scan_tree(HNDLE hDB, HNDLE hKey, int level, INT(*callback) (HNDLE, HNDLE, KEY *, INT, void *), void *info);
   INT EXPRT db_scan_tree_link(HNDLE hDB, HNDLE hKey, int level, void (*callback) (HNDLE, HNDLE, KEY *, INT, void *), void *info);
   [[deprecated("Please use the std::string version of this function")]]
   INT EXPRT db_get_path(HNDLE hDB, HNDLE hKey, char *path, INT buf_size);
   std::string EXPRT db_get_path(HNDLE hDB, HNDLE hKey);
   INT EXPRT db_delete_key(HNDLE database_handle, HNDLE key_handle, BOOL follow_links);
   INT EXPRT db_enum_key(HNDLE hdb, HNDLE key_handle, INT index, HNDLE * subkey_handle);
   INT EXPRT db_enum_link(HNDLE hdb, HNDLE key_handle, INT index, HNDLE * subkey_handle);
   INT EXPRT db_get_next_link(HNDLE hdb, HNDLE key_handle, HNDLE * subkey_handle);
   INT EXPRT db_get_key(HNDLE hdb, HNDLE key_handle, KEY * key);
   INT EXPRT db_get_link(HNDLE hdb, HNDLE key_handle, KEY * key);
   INT EXPRT db_get_key_info(HNDLE hDB, HNDLE hKey, char *name, INT name_size, INT * type, INT * num_values, INT * item_size);
   INT EXPRT db_get_key_time(HNDLE hdb, HNDLE key_handle, DWORD * delta);
   INT EXPRT db_rename_key(HNDLE hDB, HNDLE hKey, const char *name);
   INT EXPRT db_reorder_key(HNDLE hDB, HNDLE hKey, INT index);
   INT EXPRT db_get_data(HNDLE hdb, HNDLE key_handle, void *data, INT * buf_size, DWORD type);
   INT EXPRT db_get_link_data(HNDLE hdb, HNDLE key_handle, void *data, INT * buf_size, DWORD type);
   INT EXPRT db_get_data1(HNDLE hDB, HNDLE hKey, void *data, INT * buf_size, DWORD type, INT * num_values);
   INT EXPRT db_get_data_index(HNDLE hDB, HNDLE hKey, void *data, INT * buf_size, INT index, DWORD type);
   INT EXPRT db_set_data(HNDLE hdb, HNDLE hKey, const void *data, INT buf_size, INT num_values, DWORD type);
   INT EXPRT db_set_data1(HNDLE hdb, HNDLE hKey, const void *data, INT buf_size, INT num_values, DWORD type);
   INT EXPRT db_notify_clients_array(HNDLE hdb, HNDLE hKey[], INT n);
   INT EXPRT db_set_link_data(HNDLE hDB, HNDLE hKey, const void *data, INT buf_size, INT num_values, DWORD type);
   INT EXPRT db_set_data_index(HNDLE hDB, HNDLE hKey, const void *data, INT size, INT index, DWORD type);
   INT EXPRT db_set_link_data_index(HNDLE hDB, HNDLE hKey, const void *data, INT size, INT index, DWORD type);
   INT EXPRT db_set_data_index1(HNDLE hDB, HNDLE hKey, const void *data, INT size, INT index, DWORD type, BOOL bNotify);
   INT EXPRT db_set_num_values(HNDLE hDB, HNDLE hKey, INT num_values);
   INT EXPRT db_merge_data(HNDLE hDB, HNDLE hKeyRoot, const char *name, void *data, INT data_size, INT num_values, INT type);
   INT EXPRT db_set_mode(HNDLE hdb, HNDLE key_handle, WORD mode, BOOL recurse);
   INT EXPRT db_create_record(HNDLE hdb, HNDLE hkey, const char *name, const char *init_str);
   INT EXPRT db_check_record(HNDLE hDB, HNDLE hKey, const char *key_name, const char *rec_str, BOOL correct);
   INT EXPRT db_open_record(HNDLE hdb, HNDLE hkey, void *ptr, INT rec_size, WORD access, void (*dispatcher) (INT, INT, void *), void *info);
   INT EXPRT db_open_record1(HNDLE hdb, HNDLE hkey, void *ptr, INT rec_size, WORD access, void (*dispatcher) (INT, INT, void *), void *info, const char *rec_str);
   INT EXPRT db_close_record(HNDLE hdb, HNDLE hkey);
   INT EXPRT db_get_record(HNDLE hdb, HNDLE hKey, void *data, INT * buf_size, INT align);
   INT EXPRT db_get_record1(HNDLE hdb, HNDLE hKey, void *data, INT * buf_size, INT align, const char *rec_str);
   INT EXPRT db_get_record2(HNDLE hdb, HNDLE hKey, void *data, INT * buf_size, INT align, const char *rec_str, BOOL correct);
   INT EXPRT db_get_record_size(HNDLE hdb, HNDLE hKey, INT align, INT * buf_size);
   INT EXPRT db_set_record(HNDLE hdb, HNDLE hKey, void *data, INT buf_size, INT align);
   INT EXPRT db_set_record2(HNDLE hdb, HNDLE hKey, void *data, INT buf_size, INT align, const char *rec_str);
   INT EXPRT db_send_changed_records(void);
   INT EXPRT db_get_open_records(HNDLE hDB, HNDLE hKey, char *str, INT buf_size, BOOL fix);

   INT EXPRT db_add_open_record(HNDLE hDB, HNDLE hKey, WORD access_mode);
   INT EXPRT db_remove_open_record(HNDLE hDB, HNDLE hKey, BOOL lock);

   INT EXPRT db_watch(HNDLE hDB, HNDLE hKey, void (*dispatcher) (INT, INT, INT, void *info), void *info);
   INT EXPRT db_unwatch(HNDLE hDB, HNDLE hKey);
   INT EXPRT db_unwatch_all(void);
   
   INT EXPRT db_load(HNDLE hdb, HNDLE key_handle, const char *filename, BOOL bRemote);
   INT EXPRT db_save(HNDLE hdb, HNDLE key_handle, const char *filename, BOOL bRemote);
   INT EXPRT db_copy(HNDLE hDB, HNDLE hKey, char *buffer, INT * buffer_size, const char *path);
   INT EXPRT db_paste(HNDLE hDB, HNDLE hKeyRoot, const char *buffer);
   INT EXPRT db_paste_xml(HNDLE hDB, HNDLE hKeyRoot, const char *buffer);
   INT EXPRT db_save_struct(HNDLE hDB, HNDLE hKey, const char *file_name, const char *struct_name, BOOL append);
   INT EXPRT db_save_string(HNDLE hDB, HNDLE hKey, const char *file_name, const char *string_name, BOOL append);
   INT EXPRT db_save_xml(HNDLE hDB, HNDLE hKey, const char *file_name);
   INT EXPRT db_copy_xml(HNDLE hDB, HNDLE hKey, char *buffer, int *buffer_size, bool header);

   INT EXPRT db_save_json(HNDLE hDB, HNDLE hKey, const char *file_name, int flags=JSFLAG_SAVE_KEYS|JSFLAG_RECURSE);
   INT EXPRT db_load_json(HNDLE hdb, HNDLE key_handle, const char *filename);
   void EXPRT json_write(char **buffer, int* buffer_size, int* buffer_end, int level, const char* s, int quoted);
   int EXPRT json_write_anything(HNDLE hDB, HNDLE hKey, char **buffer, int *buffer_size, int *buffer_end, int level, int must_be_subdir, int flags, time_t timestamp);

   /* db_copy_json() is obsolete, use db_copy_json_save, _values and _ls instead */
   INT EXPRT db_copy_json_obsolete(HNDLE hDB, HNDLE hKey, char **buffer, int *buffer_size, int *buffer_end, int save_keys, int follow_links, int recurse);

   /* json encoder using the "ODB save" encoding, for use with "ODB load" and db_paste_json() */
   INT EXPRT db_copy_json_save(HNDLE hDB, HNDLE hKey, char **buffer, int* buffer_size, int* buffer_end);
   /* json encoder using the "ls" format, for getting the contents of a single ODB subdirectory */
   INT EXPRT db_copy_json_ls(HNDLE hDB, HNDLE hKey, char **buffer, int* buffer_size, int* buffer_end);
   /* json encoder using the "get_values" format, for resolving links and normalized ODB path names (converted to lower-case) */
  INT EXPRT db_copy_json_values(HNDLE hDB, HNDLE hKey, char **buffer, int* buffer_size, int* buffer_end, int omit_names, int omit_last_written, time_t omit_old_timestamp, int preserve_case);
   /* json encoder for an ODB array */
   INT EXPRT db_copy_json_array(HNDLE hDB, HNDLE hKey, char **buffer, int *buffer_size, int *buffer_end);
   /* json encoder for a single element of an ODB array */
   INT EXPRT db_copy_json_index(HNDLE hDB, HNDLE hKey, int index, char **buffer, int *buffer_size, int *buffer_end);

   INT EXPRT db_paste_json(HNDLE hDB, HNDLE hKey, const char *buffer);
   INT EXPRT db_paste_json_node(HNDLE hDB, HNDLE hKey, int index, const MJsonNode* json_node);

   MJsonNode* EXPRT db_sor(HNDLE hDB, const char* path); // show open records
   MJsonNode* EXPRT db_scl(HNDLE hDB); // show clients

   [[deprecated("Please use the std::string version of this function")]]
   INT EXPRT db_sprintf(char *string, const void *data, INT data_size, INT index, DWORD type);
   [[deprecated("Please use the std::string version of this function")]]
   INT EXPRT db_sprintff(char *string, const char *format, const void *data, INT data_size, INT index, DWORD type);
   [[deprecated("Please use the std::string version of this function")]]
   INT EXPRT db_sprintfh(char *string, const void *data, INT data_size, INT index, DWORD type);
   std::string EXPRT db_sprintf(const void *data, INT data_size, INT index, DWORD type);
   std::string EXPRT db_sprintff(const char *format, const void *data, INT data_size, INT index, DWORD type);
   std::string EXPRT db_sprintfh(const void *data, INT data_size, INT index, DWORD type);
   INT EXPRT db_sscanf(const char *string, void *data, INT * data_size, INT index, DWORD type);
   INT db_get_watchdog_info(HNDLE hDB, const char *client_name, DWORD * timeout, DWORD * last);
   INT EXPRT db_update_last_activity(DWORD millitime);
   INT EXPRT db_delete_client_info(HNDLE hDB, int pid);

   [[deprecated("Please use the std::string version of this function")]]
   char EXPRT *strcomb(const char **list);
   std::string EXPRT strcomb1(const char **list);

   /** @} */

   /*---- Bank routines ----*/
   void EXPRT bk_init(void *pbh);
   void EXPRT bk_init32(void *event);
   void EXPRT bk_init32a(void *event);
   BOOL EXPRT bk_is32(const void *event);
   BOOL EXPRT bk_is32a(const void *event);
   INT EXPRT bk_size(const void *pbh);
   void EXPRT bk_create(void *pbh, const char *name, WORD type, void **pdata);
   INT EXPRT bk_delete(void *event, const char *name);
   INT EXPRT bk_close(void *pbh, void *pdata);
   INT EXPRT bk_list(const void *pbh, char *bklist);
   INT EXPRT bk_locate(const void *pbh, const char *name, void *pdata);
   INT EXPRT bk_iterate(const void *pbh, BANK ** pbk, void *pdata);
   INT EXPRT bk_iterate32(const void *pbh, BANK32 ** pbk, void *pdata);
   INT EXPRT bk_iterate32a(const void *pbh, BANK32A ** pbk, void *pdata);
   INT EXPRT bk_copy(char * pevent, char * psrce, const char * bkname);
   INT EXPRT bk_swap(void *event, BOOL force);
   INT EXPRT bk_find(const BANK_HEADER * pbkh, const char *name, DWORD * bklen, DWORD * bktype, void **pdata);

   /*---- RPC routines ----*/
   INT EXPRT rpc_clear_allowed_hosts(void);
   INT EXPRT rpc_add_allowed_host(const char* hostname);
   INT EXPRT rpc_check_allowed_host(const char* hostname);

   INT EXPRT rpc_register_functions(const RPC_LIST * new_list, RPC_HANDLER func);
   INT EXPRT rpc_register_function(INT id, RPC_HANDLER func);
   INT EXPRT rpc_get_hw_type();
   INT EXPRT rpc_get_timeout(HNDLE hConn);
   INT EXPRT rpc_set_timeout(HNDLE hConn, int timeout_msec, int* old_timeout_msec = NULL);
   INT EXPRT rpc_set_name(const char *name);
   std::string rpc_get_name();
   bool EXPRT rpc_is_remote(void);
   bool EXPRT rpc_is_connected(void);
   std::string rpc_get_mserver_hostname();
   bool EXPRT rpc_is_mserver(void);
   INT EXPRT rpc_set_debug(void (*func) (const char *), INT mode);
   void EXPRT rpc_debug_printf(const char *format, ...);

   INT EXPRT rpc_register_server(int port, int *plsock, int *pport);
   INT EXPRT rpc_register_client(const char *name, RPC_LIST * list);
   INT EXPRT rpc_server_loop(void);
   INT EXPRT rpc_server_shutdown(void);
   INT EXPRT rpc_client_call(HNDLE hConn, DWORD routine_id, ...);
   INT EXPRT rpc_call(DWORD routine_id, ...);
   INT EXPRT rpc_tid_size(INT id);
   const char EXPRT *rpc_tid_name(INT id);
   const char EXPRT *rpc_tid_name_old(INT id);
   INT EXPRT rpc_name_tid(const char* name); // inverse of rpc_tid_name()
   INT EXPRT rpc_server_connect(const char *host_name, const char *exp_name);
   INT EXPRT rpc_client_connect(const char *host_name, INT midas_port, const char *client_name, HNDLE * hConnection);
   INT EXPRT rpc_client_disconnect(HNDLE hConn, BOOL bShutdown);

   INT EXPRT rpc_send_event(INT buffer_handle, const EVENT_HEADER *event, int unused, INT async_flag, INT mode);
   INT EXPRT rpc_flush_event(void);

   INT EXPRT rpc_send_event1(INT buffer_handle, const EVENT_HEADER *event);
   INT EXPRT rpc_send_event_sg(INT buffer_handle, int sg_n, const char* const sg_ptr[], const size_t sg_len[]);

   INT EXPRT rpc_test_rpc();

   void EXPRT rpc_get_convert_flags(INT * convert_flags);
   void EXPRT rpc_convert_single(void *data, INT tid, INT flags, INT convert_flags);
   void EXPRT rpc_convert_data(void *data, INT tid, INT flags, INT size, INT convert_flags);

   /** @addtogroup msfunctionc */
   /** @{ */

   /*---- system services ----*/
   DWORD EXPRT ss_millitime(void);
   DWORD EXPRT ss_time(void);
   double EXPRT ss_time_sec(void);
   DWORD EXPRT ss_settime(DWORD seconds);
   void  EXPRT ss_tzset();
   time_t EXPRT ss_mktime(struct tm* tms);
   std::string EXPRT ss_asctime(void);
   INT EXPRT ss_sleep(INT millisec);
   BOOL EXPRT ss_kbhit(void);
   std::string EXPRT ss_getcwd();

   double EXPRT ss_nan(void);
   int EXPRT ss_isnan(double x);
   int EXPRT ss_isfin(double x);

   void EXPRT ss_clear_screen(void);
   void EXPRT ss_printf(INT x, INT y, const char *format, ...);
   void ss_set_screen_size(int x, int y);

   char EXPRT *ss_getpass(const char *prompt);
   INT EXPRT ss_getchar(BOOL reset);
   char EXPRT *ss_crypt(const char *key, const char *salt);
   char EXPRT *ss_gets(char *string, int size);

   void EXPRT *ss_ctrlc_handler(void (*func) (int));

   INT EXPRT ss_write_tcp(int sock, const char *buffer, size_t buffer_size);

   /*---- direct io routines ----*/
   INT EXPRT ss_directio_give_port(INT start, INT end);
   INT EXPRT ss_directio_lock_port(INT start, INT end);

   /*---- tape routines ----*/
   INT EXPRT ss_tape_open(char *path, INT oflag, INT * channel);
   INT EXPRT ss_tape_close(INT channel);
   INT EXPRT ss_tape_status(char *path);
   INT EXPRT ss_tape_read(INT channel, void *pdata, INT * count);
   INT EXPRT ss_tape_write(INT channel, void *pdata, INT count);
   INT EXPRT ss_tape_write_eof(INT channel);
   INT EXPRT ss_tape_fskip(INT channel, INT count);
   INT EXPRT ss_tape_rskip(INT channel, INT count);
   INT EXPRT ss_tape_rewind(INT channel);
   INT EXPRT ss_tape_spool(INT channel);
   INT EXPRT ss_tape_mount(INT channel);
   INT EXPRT ss_tape_unmount(INT channel);
   INT EXPRT ss_tape_get_blockn(INT channel);

   /*---- disk routines ----*/
   double EXPRT ss_disk_free(const char *path);
   double EXPRT ss_file_size(const char *path);
   time_t EXPRT ss_file_time(const char *path);
   INT EXPRT ss_dir_exist(const char *path);
   INT EXPRT ss_file_exist(const char *path);
   INT EXPRT ss_file_link_exist(const char *path);
   INT EXPRT ss_file_remove(const char *path);
   INT EXPRT ss_file_find(const char *path, const char *pattern, char **plist);
   INT EXPRT ss_dir_find(const char *path, const char *pattern, char **plist);
   INT EXPRT ss_dirlink_find(const char *path, const char *pattern, char **plist);
   INT EXPRT ss_file_find(const char *path, const char *pattern, STRING_LIST*);
   INT EXPRT ss_dir_find(const char *path, const char *pattern, STRING_LIST*);
   INT EXPRT ss_dirlink_find(const char *path, const char *pattern, STRING_LIST*);
   double EXPRT ss_disk_size(const char *path);
   int EXPRT ss_file_copy(const char *src, const char *dst, bool append = false);

   /*---- UTF8 unicode ----*/
   bool ss_is_valid_utf8(const char* s);
   bool ss_repair_utf8(char* s);
   bool ss_repair_utf8(std::string& s);

/** @} */

   /*---- ELog functions ----*/
   INT EXPRT el_retrieve(char *tag, char *date, int *run, char *author,
                         char *type, char *system, char *subject,
                         char *text, int *textsize, char *orig_tag,
                         char *reply_tag, char *attachment1,
                         char *attachment2, char *attachment3, char *encoding);
   INT EXPRT el_submit(int run, const char *author, const char *type, const char *system,
                       const char *subject, const char *text, const char *reply_to,
                       const char *encoding,
                       const char *afilename1, const char *buffer1, INT buffer_size1,
                       const char *afilename2, const char *buffer2, INT buffer_size2,
                       const char *afilename3, const char *buffer3, INT buffer_size3,
                       char *tag, INT tag_size);
   INT EXPRT el_search_message(char *tag, int *fh, BOOL walk, char* filename, int filename_size);
   INT EXPRT el_search_run(int run, char *return_tag);
   INT EXPRT el_delete_message(const char *tag);

   /*---- alarm functions ----*/
   INT EXPRT al_check(void);
   INT EXPRT al_trigger_alarm(const char *alarm_name, const char *alarm_message,
                              const char *default_class, const char *cond_str, INT type);
   INT EXPRT al_trigger_class(const char *alarm_class, const char *alarm_message, BOOL first);
   INT EXPRT al_reset_alarm(const char *alarm_name);
   BOOL EXPRT al_evaluate_condition(const char* alarm_name, const char *condition, std::string *pvalue);
   INT al_get_alarms(std::string *presult);
   INT EXPRT al_define_odb_alarm(const char *name, const char *condition, const char *aclass, const char *message);

   /*---- analyzer functions ----*/
   void EXPRT test_register(ANA_TEST * t);
   void EXPRT add_data_dir(char *result, char *file);
   void EXPRT lock_histo(INT id);

   void EXPRT open_subfolder(const char *name);
   void EXPRT close_subfolder(void);

   /*---- image history functions ----*/
   int hs_image_retrieve(std::string image_name, time_t start, time_t stop,
                      std::vector<time_t> &vtime, std::vector<std::string> &vfilename);

/* we need a duplicate of mxml/strlcpy.h or nobody can use strlcpy() from libmidas.a */
#ifndef HAVE_STRLCPY
#ifndef _STRLCPY_H_
#define _STRLCPY_H_
extern "C" {
   size_t EXPRT strlcpy(char *dst, const char *src, size_t size);
   size_t EXPRT strlcat(char *dst, const char *src, size_t size);
}
#endif
#endif

#endif                          /* _MIDAS_H */

/**dox***************************************************************/
/** @} *//* end of msectionh */
/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
