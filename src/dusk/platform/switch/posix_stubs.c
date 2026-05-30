#include <errno.h>
#include <signal.h>
#include <spawn.h>
#include <sched.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iconv.h>
#include <stdio.h>

// Switch/libnx compatibility stubs for optional POSIX APIs that some deps reference.
// They fail gracefully when called.

int sigaction(int signum, const struct sigaction* act, struct sigaction* oldact) {
    (void)signum; (void)act; (void)oldact;
    errno = ENOSYS;
    return -1;
}

pid_t waitpid(pid_t pid, int* status, int options) {
    (void)pid; (void)status; (void)options;
    errno = ENOSYS;
    return -1;
}

int execvp(const char* file, char* const argv[]) {
    (void)file; (void)argv;
    errno = ENOSYS;
    return -1;
}

int posix_spawnattr_init(posix_spawnattr_t* attr) { (void)attr; errno = ENOSYS; return ENOSYS; }
int posix_spawnattr_destroy(posix_spawnattr_t* attr) { (void)attr; return 0; }
int posix_spawn_file_actions_init(posix_spawn_file_actions_t* fa) { (void)fa; errno = ENOSYS; return ENOSYS; }
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t* fa) { (void)fa; return 0; }
int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t* fa, int fd) { (void)fa; (void)fd; errno = ENOSYS; return ENOSYS; }
int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t* fa, int fd, int newfd) { (void)fa; (void)fd; (void)newfd; errno = ENOSYS; return ENOSYS; }
int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t* fa, int fd, const char* path, int oflag, mode_t mode) {
    (void)fa; (void)fd; (void)path; (void)oflag; (void)mode; errno = ENOSYS; return ENOSYS;
}
int posix_spawnp(pid_t* pid, const char* file, const posix_spawn_file_actions_t* fa,
                 const posix_spawnattr_t* attrp, char* const argv[], char* const envp[]) {
    (void)pid; (void)file; (void)fa; (void)attrp; (void)argv; (void)envp; errno = ENOSYS; return ENOSYS;
}

int pipe(int fds[2]) { (void)fds; errno = ENOSYS; return -1; }
pid_t vfork(void) { errno = ENOSYS; return -1; }
int setsid(void) { errno = ENOSYS; return -1; }
int fdatasync(int fd) { (void)fd; return 0; }
ssize_t readlink(const char* path, char* buf, size_t bufsiz) { (void)path; (void)buf; (void)bufsiz; errno = ENOSYS; return -1; }

int pthread_setname_np(pthread_t thread, const char* name) { (void)thread; (void)name; return 0; }
int pthread_sigmask(int how, const sigset_t* set, sigset_t* oldset) { (void)how; (void)set; (void)oldset; return 0; }
int pthread_getschedparam(pthread_t thread, int* policy, struct sched_param* param) { (void)thread; if (policy) *policy = 0; if (param) param->sched_priority = 0; return 0; }
int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param* param) { (void)thread; (void)policy; (void)param; return 0; }
int sched_get_priority_min(int policy) { (void)policy; return 0; }
int sched_get_priority_max(int policy) { (void)policy; return 0; }

int fchown(int fd, uid_t owner, gid_t group) { (void)fd; (void)owner; (void)group; errno = ENOSYS; return -1; }
uid_t geteuid(void) { return 0; }
long sysconf(int name) { (void)name; errno = ENOSYS; return -1; }
long pathconf(const char* path, int name) { (void)path; (void)name; errno = ENOSYS; return -1; }

int socket(int domain, int type, int protocol) {
    (void)domain; (void)type; (void)protocol;
    errno = ENOSYS;
    return -1;
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    (void)sockfd; (void)addr; (void)addrlen;
    errno = ENOSYS;
    return -1;
}

ssize_t send(int sockfd, const void* buf, size_t len, int flags) {
    (void)sockfd; (void)buf; (void)len; (void)flags;
    errno = ENOSYS;
    return -1;
}

int inet_pton(int af, const char* src, void* dst) { (void)af; (void)src; (void)dst; errno = ENOSYS; return -1; }

void flockfile(FILE* file) { (void)file; }
void funlockfile(FILE* file) { (void)file; }

int gethostname(char* name, size_t len) {
    if (name && len > 0) {
        name[0] = '\0';
    }
    errno = ENOSYS;
    return -1;
}

int sysctl(int* name, unsigned int namelen, void* oldp, size_t* oldlenp, const void* newp, size_t newlen) {
    (void)name; (void)namelen; (void)oldp; (void)oldlenp; (void)newp; (void)newlen;
    errno = ENOSYS;
    return -1;
}

int sysctlbyname(const char* name, void* oldp, size_t* oldlenp, const void* newp, size_t newlen) {
    (void)name; (void)oldp; (void)oldlenp; (void)newp; (void)newlen;
    errno = ENOSYS;
    return -1;
}

int getpagesize(void) {
    return 4096;
}

iconv_t iconv_open(const char* tocode, const char* fromcode) {
    (void)tocode; (void)fromcode;
    errno = ENOSYS;
    return (iconv_t)-1;
}

int iconv_close(iconv_t cd) {
    (void)cd;
    errno = ENOSYS;
    return -1;
}

size_t iconv(iconv_t cd, char** inbuf, size_t* inbytesleft, char** outbuf, size_t* outbytesleft) {
    (void)cd; (void)inbuf; (void)inbytesleft; (void)outbuf; (void)outbytesleft;
    errno = ENOSYS;
    return (size_t)-1;
}

void* __aarch64_read_tp(void) { return 0; }
