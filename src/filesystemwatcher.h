#ifndef FILESYSTEMWATCHER_H
#define FILESYSTEMWATCHER_H
#include <string>
#include <sys/inotify.h>

typedef struct inotify_event inotify_event_t;

class FileSystemWatcher
{
public:
    FileSystemWatcher(const std::string& path);
private:
    inotify_event_t ReadEvent();
    int fd;
};

#endif // FILESYSTEMWATCHER_H
