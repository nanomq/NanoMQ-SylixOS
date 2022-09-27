/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MEMORY OPERATION TRACER
 * this file is used to track memory operations, to moniting memory leakage, memory corruption
 *
 * Author: Zeng.Bo <sylixos@gmail.com>
 */

#define  __SYLIXOS_KERNEL /* need some kernel function */
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>
#include <execinfo.h>
#include <pthread.h>

/*
 * fast malloc config
 */
#define MTRACER_FAST_MALLOC_EN       1

#if MTRACER_FAST_MALLOC_EN > 0
#define MTRACER_NODE_ATTR            __attribute__((aligned(8)))
#else
#define MTRACER_NODE_ATTR
#endif

/*
 * util macros
 */
#define MTRACER_SHOWSTACK_SIZE       100

#define MTRACER_MARK_MAGIC           0xdeadbeef
#define MTRACER_MARK_SIZE            8

#define MTRACER_LOCK()               pthread_mutex_lock(&__mtracer_locker)
#define MTRACER_UNLOCK()             pthread_mutex_unlock(&__mtracer_locker)

#define MTRACER_LOG(fmt, arg...)     fdprintf(__mtracer_log_fd, fmt, ##arg)
#define MTRACER_ERR(fmt, arg...)     fdprintf(STD_ERR, fmt, ##arg)

#if LW_CFG_CPU_WORD_LENGHT == 32
#define MTRACER_ULFMT                "08lx"
#else
#define MTRACER_ULFMT                "016lx"
#endif

#define MTRACER_GET_RAW_FUNC(func)   __mtracer_raw##_##func = dlsym(RTLD_NEXT, #func)

/*
 * trace information manage
 */
typedef struct mtracer_node {
    struct mtracer_node  *next;
    struct mtracer_node  *prev;

    const char           *operation;
    void                 *ret_addr;
    void                 *raw_mem;
    void                 *usr_mem;
    size_t               usr_size;
    int                  malloc_cnt;
} MTRACER_NODE_ATTR mtracer_node_t;

typedef struct mtracer_memhdr {
    mtracer_node_t       *node;
    unsigned long        node_reverse;
    char                 mark[MTRACER_MARK_SIZE];
} mtracer_memhdr_t;

static pthread_mutex_t   __mtracer_locker;
static mtracer_node_t    __mtracer_node_list;
static int               __mtracer_backtrace_current   = 0;
static int               __mtracer_backtrace_threshold = -1;
static unsigned long     __mtracer_backtrace_mem       = 0;
static int               __mtracer_log_fd              = STD_OUT;
static BOOL              __mtracer_log_fd_istty        = LW_TRUE;
static UINT32            __mtracer_mem_mark[2] = {
    MTRACER_MARK_MAGIC,
    ~MTRACER_MARK_MAGIC,
};

/*
 * the memory related raw functions
 */
static void *(*__mtracer_raw_malloc)(size_t  nbytes) = NULL;
static void *(*__mtracer_raw_mallocalign)(size_t  nbytes, size_t align) = NULL;
static void  (*__mtracer_raw_free)(void *mem) = NULL;

/*
 * node list base operations
 */
static inline void __mtracer_list_add (mtracer_node_t *new, mtracer_node_t *prev, mtracer_node_t *next)
{
    next->prev = new;
    new->next  = next;
    new->prev  = prev;
    prev->next = new;
}

static inline void mtracer_list_add (mtracer_node_t *new, mtracer_node_t *head)
{
    __mtracer_list_add(new, head, head->next);
}

static inline void __mtracer_list_del (mtracer_node_t *prev, mtracer_node_t *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void mtracer_list_del (mtracer_node_t *node)
{
    __mtracer_list_del(node->prev, node->next);
    node->next = NULL;
    node->prev = NULL;
}

static inline int mtracer_list_empty (mtracer_node_t *head)
{
    return (head->next == head);
}

#define mtracer_list_for_each(node)  \
    for (node = __mtracer_node_list.prev; node != &__mtracer_node_list; node = node->prev)

/*
 * memory node manager
 */
static inline void *mtracer_raw2usr (void *raw_mem, int offset)
{
    return  ((char *)raw_mem + sizeof(mtracer_memhdr_t) + offset);
}

static inline mtracer_memhdr_t *mtracer_usr2hdr (void *usr_mem)
{
    return  ((mtracer_memhdr_t *)((char *)usr_mem - sizeof(mtracer_memhdr_t)));
}

static inline void *mtracer_hdr2usr (mtracer_memhdr_t *hdr)
{
    return  ((char *)hdr + sizeof(mtracer_memhdr_t));
}

static inline void *mtracer_usr2raw (void *usr_mem)
{
    return  mtracer_usr2hdr(usr_mem)->node->raw_mem;
}

static inline void *mtracer_usr2tail (void *usr_mem, size_t usr_size)
{
    return  ((void *)((char *)usr_mem + usr_size));
}

static inline void mtracer_node_insert (mtracer_node_t *node)
{
    mtracer_list_add(node, &__mtracer_node_list);
}

static inline void mtracer_node_remove (mtracer_node_t *node)
{
    if (mtracer_list_empty(&__mtracer_node_list)) {
        return;
    }
    mtracer_list_del(node);
}

static void mtracer_node_show (mtracer_memhdr_t *hdr, int force, int fd, int fd_isatty)
{
    Dl_info         dlinfo;
    addr_t          mod_base;
    size_t          mod_len;
    void            *tail;
    mtracer_node_t  *node;
    const char      *memerr = "[memory OK]";

    if (hdr->node_reverse != ~((unsigned long)hdr->node)) {
        if (fd_isatty) {
            fdprintf(fd, "module: unknown, memory addr: 0x%"MTRACER_ULFMT" \033[33m[memory may be broken, node invalid]\033[0m\n",
                     mtracer_hdr2usr(hdr));
        } else {
            fdprintf(fd, "module: unknown, memory addr: 0x%"MTRACER_ULFMT" [memory may be broken, node invalid]\n",
                     mtracer_hdr2usr(hdr));
        }
        return;
    }

    node = hdr->node;
    tail = mtracer_usr2tail(node->usr_mem, node->usr_size);

    if (memcmp(hdr->mark, __mtracer_mem_mark, MTRACER_MARK_SIZE) ||
        memcmp(tail, __mtracer_mem_mark, MTRACER_MARK_SIZE)) {
        if (fd_isatty) {
            memerr = "\033[33m[memory may be broken]\033[0m";
        } else {
            memerr = "[memory may be broken]";
        }
        force  = 1;
    }

    if (!force) {
        return;
    }

    dladdr(node->ret_addr, &dlinfo);
    API_ModuleGetBase(getpid(), (PCHAR)dlinfo.dli_fname, &mod_base, &mod_len);

    fdprintf(fd,
             "%s(0x%"MTRACER_ULFMT"), caller: %s(0x%"MTRACER_ULFMT"), callee: %s, "
             "memory addr: 0x%"MTRACER_ULFMT", malloc count: %d %s\n",
             dlinfo.dli_fname, (unsigned long)mod_base, dlinfo.dli_sname,
             (unsigned long)node->ret_addr, node->operation,
             (unsigned long)node->usr_mem, node->malloc_cnt, memerr);
}

static void  mtracer_backtrace_show (int depth)
{
    void  *frame[MTRACER_SHOWSTACK_SIZE];
    int   i, cnt;

    if (depth > MTRACER_SHOWSTACK_SIZE) {
        depth = MTRACER_SHOWSTACK_SIZE;
    } else if (depth < 0) {
        depth = 1;
    }

    cnt = backtrace(frame, depth);
    if (cnt > 0) {
        Dl_info  dlinfo;

        for (i = 0; i < cnt; i++) {
            if (dladdr(frame[i], &dlinfo) && dlinfo.dli_sname) {
                addr_t  mod_base;
                size_t  mod_len;

                API_ModuleGetBase(getpid(), (PCHAR)dlinfo.dli_fname, &mod_base, &mod_len);
                MTRACER_LOG("[%02d] %p (%s@%p+0x%lx %s+%zu)\n",
                           cnt - i,
                           frame[i],
                           dlinfo.dli_fname,
                           (void *)mod_base,
                           (unsigned long)frame[i] - mod_base,
                           dlinfo.dli_sname,
                           ((size_t)frame[i] - (size_t)dlinfo.dli_saddr));

            } else {
                MTRACER_LOG("[%02d] %p (<unknown>)\n", cnt - i, frame[i]);
            }
        }

        MTRACER_LOG("\n");
    }
}

LW_CONSTRUCTOR_BEGIN
int mtracer_init (void)
{
    int  ret;

    MTRACER_GET_RAW_FUNC(malloc);
    MTRACER_GET_RAW_FUNC(mallocalign);
    MTRACER_GET_RAW_FUNC(free);

    __mtracer_node_list.next = &__mtracer_node_list;
    __mtracer_node_list.prev = &__mtracer_node_list;

    ret = pthread_mutex_init(&__mtracer_locker, LW_NULL);
    if (ret) {
        MTRACER_ERR("WARNING: memtracer locker create error!\n");
    }

    return  (0);
}
LW_CONSTRUCTOR_END(mtracer_init)

/*
 * finished memtracer
 */
LW_DESTRUCTOR_BEGIN
int mtracer_exit (void)
{
    mtracer_node_t  *node;

    if (mtracer_list_empty(&__mtracer_node_list)) {
        return  (0);
    }

    if (__mtracer_log_fd == STD_OUT) {
        printf("\nprocess exit, unfreed memory list >>\n");
    } else {
        printf("\nprocess exit, there is unfreed memory currently, more details in your log file\n");
        MTRACER_LOG("\nprocess exit, unfreed memory list >>\n");
    }

    fdprintf(__mtracer_log_fd, "\n");

    MTRACER_LOCK();
    mtracer_list_for_each(node) {
        mtracer_node_show(mtracer_usr2hdr(node->usr_mem), 1, __mtracer_log_fd, __mtracer_log_fd_istty);
    }
    MTRACER_UNLOCK();

    pthread_mutex_destroy(&__mtracer_locker);

    return  (0);
}
LW_DESTRUCTOR_END(mtracer_exit)

/*
 * config memtracer by usr
 */
void mtracer_config (int backtrace_malloc_cnt, unsigned long backtrace_mem_addr, int log_fd)
{
    __mtracer_backtrace_threshold = backtrace_malloc_cnt;
    __mtracer_backtrace_mem       = backtrace_mem_addr;

    if (log_fd > 0) {
        __mtracer_log_fd = log_fd;
        __mtracer_log_fd_istty = isatty(log_fd);
    }
}

/*
 * show the memory trace infomation
 */
int mtracer_show (void)
{
    mtracer_node_t   *node;
    BOOL             stdout_isatty;

    if (mtracer_list_empty(&__mtracer_node_list)) {
        printf("no memory operation exception.\n");
        return  (0);
    }

    printf("\nunfreed memory list >>\n");

    stdout_isatty = isatty(STD_OUT);
    MTRACER_LOCK();
    mtracer_list_for_each(node) {
        mtracer_node_show(mtracer_usr2hdr(node->usr_mem), 1, STD_OUT, stdout_isatty);
    }
    MTRACER_UNLOCK();

    return  (0);
}

/*
 *  lib_malloc
 */
__weak_reference(lib_malloc, malloc);

void *lib_malloc (size_t  nbytes)
{
    char              *raw;
    char              *usr;
    mtracer_node_t    *node;
    mtracer_memhdr_t  *hdr;
    void              *tail;
    int               backtrace = 0;

#if MTRACER_FAST_MALLOC_EN > 0
    raw  = __mtracer_raw_malloc(sizeof(mtracer_node_t)   +
                                sizeof(mtracer_memhdr_t) +
                                nbytes + MTRACER_MARK_SIZE);
    node = (mtracer_node_t *)raw;
    usr  = mtracer_raw2usr(raw, sizeof(mtracer_node_t));

#else
    node = __mtracer_raw_malloc(sizeof(mtracer_node_t));
    if (!node) {
        return  (NULL);
    }

    raw  = __mtracer_raw_malloc(sizeof(mtracer_memhdr_t) + nbytes + MTRACER_MARK_SIZE);
    if (!raw) {
        __mtracer_raw_free(node);
        return  (NULL);
    }
    usr  = mtracer_raw2usr(raw, 0);
#endif /* MTRACER_FAST_MALLOC_EN > 0 */

    hdr  = mtracer_usr2hdr(usr);
    tail = mtracer_usr2tail(usr, nbytes);

    hdr->node         = node;
    hdr->node_reverse = ~((unsigned long)node);

    memcpy(hdr->mark, __mtracer_mem_mark, MTRACER_MARK_SIZE);
    memcpy(tail, __mtracer_mem_mark, MTRACER_MARK_SIZE);

    MTRACER_LOCK();
    node->operation  = "malloc";
    node->usr_size   = nbytes;
    node->raw_mem    = raw;
    node->usr_mem    = usr;
    node->ret_addr   = __builtin_return_address(0);
    node->malloc_cnt = ++__mtracer_backtrace_current;
    mtracer_node_insert(node);
    if ((__mtracer_backtrace_current == __mtracer_backtrace_threshold) ||
        (__mtracer_backtrace_mem == (unsigned long)usr)) {
        backtrace = __mtracer_backtrace_current;
    }
    MTRACER_UNLOCK();

    if (backtrace) {
        MTRACER_LOG("\nmemory malloc reach to %d times, mem = 0x%"MTRACER_ULFMT", backtrace show >>\n\n",
                   backtrace, (unsigned long)usr);
        mtracer_backtrace_show(MTRACER_SHOWSTACK_SIZE);
    }

    return  (usr);
}

/*
 *  lib_free
 */
__weak_reference(lib_free, free);

void lib_free (void *usr)
{
    mtracer_node_t    *node;
    mtracer_memhdr_t  *hdr;

    if (!usr) {
        MTRACER_LOG("ERROR: user free NULL memory\n");
        mtracer_backtrace_show(MTRACER_SHOWSTACK_SIZE);
        return;
    }

    hdr  = mtracer_usr2hdr(usr);
    if (hdr->node_reverse != ~((unsigned long)hdr->node)) {
        if (__mtracer_log_fd_istty) {
            MTRACER_LOG("module: unknown, memory addr: 0x%"MTRACER_ULFMT" "
                       "\033[33m[memory may be broken, node invalid]\033[0m\n", (unsigned long)usr);
        } else {
            MTRACER_LOG("module: unknown, memory addr: 0x%"MTRACER_ULFMT" "
                       "[memory may be broken, node invalid]\n", (unsigned long)usr);
        }
        return;
    }

    node = hdr->node;

    MTRACER_LOCK();
    mtracer_node_show(hdr, 0, __mtracer_log_fd, __mtracer_log_fd_istty);
    mtracer_node_remove(node);
    MTRACER_UNLOCK();

    if ((void *)node != node->raw_mem) {
        __mtracer_raw_free(node->raw_mem);
        __mtracer_raw_free(node);
    } else {
        __mtracer_raw_free(node);
    }
}

/*
 *  lib_mallocalign
 */
__weak_reference(lib_mallocalign, mallocalign);

void *lib_mallocalign (size_t  nbytes, size_t align)
{
    char              *raw;
    char              *usr;
    mtracer_node_t    *node;
    mtracer_memhdr_t  *hdr;
    void              *tail;
    int               extra;
    int               offset;
    int               backtrace = 0;

    if (align & (align - 1)) {
        return  (NULL);
    }

    if (align < 8) {
        return  (lib_malloc(nbytes));
    }

    node = __mtracer_raw_malloc(sizeof(mtracer_node_t));
    if (!node) {
        return  (NULL);
    }

    extra  = ROUND_UP(sizeof(mtracer_memhdr_t), align);
    raw    = __mtracer_raw_mallocalign(extra + nbytes + MTRACER_MARK_SIZE, align);
    if (!raw) {
        __mtracer_raw_free(node);
        return  (NULL);
    }

    offset = extra - sizeof(mtracer_memhdr_t);
    usr    = mtracer_raw2usr(raw, offset);
    hdr    = mtracer_usr2hdr(usr);
    tail   = mtracer_usr2tail(usr, nbytes);

    hdr->node         = node;
    hdr->node_reverse = ~((unsigned long)node);

    memcpy(hdr->mark, __mtracer_mem_mark, MTRACER_MARK_SIZE);
    memcpy(tail, __mtracer_mem_mark, MTRACER_MARK_SIZE);

    MTRACER_LOCK();
    node->operation  = "mallocalign";
    node->usr_size   = nbytes;
    node->raw_mem    = raw;
    node->usr_mem    = usr;
    node->ret_addr   = __builtin_return_address(0);
    node->malloc_cnt = ++__mtracer_backtrace_current;
    mtracer_node_insert(node);
    if ((__mtracer_backtrace_current == __mtracer_backtrace_threshold) ||
        (__mtracer_backtrace_mem == (unsigned long)usr)) {
        backtrace = __mtracer_backtrace_current;
    }
    MTRACER_UNLOCK();

    if (backtrace) {
        MTRACER_LOG("\nmemory malloc reach to %d times, mem = 0x%"MTRACER_ULFMT", backtrace show >>\n\n",
                   backtrace, (unsigned long)usr);
        mtracer_backtrace_show(MTRACER_SHOWSTACK_SIZE);
    }

    return  (usr);
}

/*
 *  lib_realloc
 */
__weak_reference(lib_realloc, realloc);

void *lib_realloc (void *usr, size_t  new_size)
{
    char              *raw;
    char              *new;
    char              *new_usr;
    mtracer_node_t    *node;
    mtracer_node_t    *new_node;
    mtracer_memhdr_t  *hdr;
    void              *tail;
    int               backtrace   = 0;
    int               need_insert = 0;

    if (!usr) {
        return  (lib_malloc(new_size));
    }

    hdr  = mtracer_usr2hdr(usr);
    node = hdr->node;

    mtracer_node_show(hdr, 0, __mtracer_log_fd, __mtracer_log_fd_istty);

    if (node->usr_size == new_size) {
        return  (usr);
    }

    /*
     * just update node information
     */
    if (node->usr_size > new_size) {
        MTRACER_LOCK();
        tail = mtracer_usr2tail(usr, new_size);
        memcpy(tail, __mtracer_mem_mark, MTRACER_MARK_SIZE);

        node->operation  = "realloc";
        node->usr_size   = new_size;
        node->ret_addr   = __builtin_return_address(0);
        node->malloc_cnt = ++__mtracer_backtrace_current;
        if (__mtracer_backtrace_current == __mtracer_backtrace_threshold) {
            backtrace = __mtracer_backtrace_current;
        }
        MTRACER_UNLOCK();

        if (backtrace) {
            MTRACER_LOG("\nmemory malloc reach to %d times, mem = 0x%"MTRACER_ULFMT", backtrace show >>\n\n",
                       backtrace, (unsigned long)usr);
            mtracer_backtrace_show(MTRACER_SHOWSTACK_SIZE);
        }

        return  (usr);
    }

    /*
     * malloc new memory
     * note: if raw == node, means using fast malloc
     */
    raw = node->raw_mem;
    if (raw == (char *)node) {
        new  = __mtracer_raw_malloc(sizeof(mtracer_node_t)   +
                                    sizeof(mtracer_memhdr_t) +
                                    new_size + MTRACER_MARK_SIZE);
        if (!new) {
            return  (NULL);
        }

        new_node = (mtracer_node_t *)new;
        new_usr  = mtracer_raw2usr(new, sizeof(mtracer_node_t));

        MTRACER_LOCK();
        mtracer_node_remove(node); /* should remove old node */
        MTRACER_UNLOCK();

        need_insert = 1; /* need insert new node */

    } else {
        new = __mtracer_raw_malloc(sizeof(mtracer_memhdr_t) + new_size + MTRACER_MARK_SIZE);
        if (!new) {
            return  (NULL);
        }

        new_node = node; /* not fast malloc, using old node */
        new_usr  = mtracer_raw2usr(new, 0);
    }

    memcpy(new_usr, usr, node->usr_size); /* copy usr data */
    __mtracer_raw_free(raw); /* now we can free old memory */

    node = new_node;
    hdr  = mtracer_usr2hdr(new_usr);
    tail = mtracer_usr2tail(new_usr, new_size);

    hdr->node         = node;
    hdr->node_reverse = ~((unsigned long)node);

    memcpy(hdr->mark, __mtracer_mem_mark, MTRACER_MARK_SIZE);
    memcpy(tail, __mtracer_mem_mark, MTRACER_MARK_SIZE);

    MTRACER_LOCK();
    node->operation  = "realloc";
    node->usr_size   = new_size;
    node->raw_mem    = new;
    node->usr_mem    = new_usr;
    node->ret_addr   = __builtin_return_address(0);
    node->malloc_cnt = ++__mtracer_backtrace_current;
    if ((__mtracer_backtrace_current == __mtracer_backtrace_threshold) ||
        (__mtracer_backtrace_mem == (unsigned long)new_usr)) {
        backtrace = __mtracer_backtrace_current;
    }
    if (need_insert) {
        mtracer_node_insert(node); /* if using fast malloc, this is a new node */
    }
    MTRACER_UNLOCK();

    if (backtrace) {
        MTRACER_LOG("\nmemory malloc reach to %d times, mem = 0x%"MTRACER_ULFMT", backtrace show >>\n\n",
                   backtrace, (unsigned long)new_usr);
        mtracer_backtrace_show(MTRACER_SHOWSTACK_SIZE);
    }

    return  (new_usr);
}

/*
 *  lib_xmalloc
 */
__weak_reference(lib_xmalloc, xmalloc);

void *lib_xmalloc (size_t  nbytes)
{
    void  *ptr = lib_malloc(nbytes);

    if (ptr == NULL) {
        MTRACER_ERR("lib_xmalloc() process not enough memory\n");
        exit(0);
    }

    return  (ptr);
}

/*
 * lib_xmallocalign
 */
__weak_reference(lib_xmallocalign, xmallocalign);

void *lib_xmallocalign (size_t  nbytes, size_t align)
{
    void  *ptr = lib_mallocalign(nbytes, align);

    if (ptr == NULL) {
        MTRACER_ERR("lib_xmallocalign() process not enough memory\n");
        exit(0);
    }

    return  (ptr);
}

/*
 *  lib_memalign
 */
__weak_reference(lib_memalign, memalign);

void *lib_memalign (size_t align, size_t  nbytes)
{
    return  (lib_mallocalign(nbytes, align));
}

/*
 * lib_xmemalign
 */
__weak_reference(lib_xmemalign, xmemalign);

void *lib_xmemalign (size_t align, size_t  nbytes)
{
    void  *ptr = lib_memalign(align, nbytes);

    if (ptr == NULL) {
        MTRACER_ERR("lib_xmemalign() process not enough memory\n");
        exit(0);
    }

    return  (ptr);
}

/*
 *  lib_calloc
 */
__weak_reference(lib_calloc, calloc);

void *lib_calloc (size_t  num, size_t  nbytes)
{
    size_t total = num * nbytes;
    void *pmem;

    pmem = lib_malloc(total);
    if (pmem) {
        lib_bzero(pmem, total);
    }

    return  (pmem);
}

/*
 *  lib_xcalloc
 */
__weak_reference(lib_xcalloc, xcalloc);

void *lib_xcalloc (size_t  num, size_t  nbytes)
{
    void  *ptr = lib_calloc(num, nbytes);

    if (ptr == NULL) {
        MTRACER_ERR("lib_xcalloc() process not enough memory\n");
        exit(0);
    }

    return  (ptr);
}

/*
 *  lib_xrealloc
 */
__weak_reference(lib_xrealloc, xrealloc);

void *lib_xrealloc (void *ptr, size_t  new_size)
{
    void  *new_ptr = lib_realloc(ptr, new_size);

    if ((new_ptr == NULL) && new_size) {
        MTRACER_ERR("lib_xrealloc() process not enough memory\n");
        exit(0);
    }

    return  (new_ptr);
}

/*
 *  lib_posix_memalign
 */
__weak_reference(lib_posix_memalign, posix_memalign);

int  lib_posix_memalign (void **memptr, size_t align, size_t size)
{
    if (memptr == NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }

    if (align & (align - 1)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    size = (size) ? size : 1;

    *memptr = lib_mallocalign(size, align);
    if (*memptr) {
        return  (ERROR_NONE);

    } else {
        errno = ENOMEM;
        return  (ENOMEM);
    }
}

/*
 *  lib_malloc_new
 */
__weak_reference(lib_malloc_new, malloc_new);

void *lib_malloc_new (size_t  nbytes)
{
    return  (lib_malloc(nbytes));
}

/*
 *  lib_strdup
 */
__weak_reference(lib_strdup, strdup);

char *lib_strdup (const char *str)
{
    char *mem = NULL;
    size_t size;

    if (str == NULL) {
        return  (NULL);
    }

    size = lib_strlen(str);
    mem = (char *)lib_malloc(size + 1);
    if (mem) {
        lib_strcpy(mem, str);
    }

    return  (mem);
}

/*
 *  lib_xstrdup
 */
__weak_reference(lib_xstrdup, xstrdup);

char *lib_xstrdup (const char *str)
{
    char *str_ret = lib_strdup(str);

    if (str_ret == NULL) {
        MTRACER_ERR("lib_xstrdup() process not enough memory\n");
        exit(0);
    }

    return  (str_ret);
}

/*
 *  lib_strndup
 */
__weak_reference(lib_strndup, strndup);

char *lib_strndup (const char *str, size_t size)
{
    size_t len;
    char *news;

    if (str == NULL) {
        return  (NULL);
    }

    len = lib_strnlen(str, size);
    news = (PCHAR)lib_malloc(len + 1);
    if (news == NULL) {
        return  (NULL);
    }

    news[len] = PX_EOS;

    return  (lib_memcpy(news, str, len));
}

/*
 *  lib_xstrndup
 */
__weak_reference(lib_xstrndup, xstrndup);

char *lib_xstrndup (const char *str, size_t size)
{
    char *str_ret = lib_strndup(str, size);

    if (str_ret == NULL) {
        MTRACER_ERR("lib_xstrndup() process not enough memory\n");
        exit(0);
    }

    return  (str_ret);
}

/*
 * end
 */
