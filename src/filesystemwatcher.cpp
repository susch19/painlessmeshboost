#include "filesystemwatcher.h"
#include <sys/inotify.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>

#include <exception>
#include <stdexcept>
FileSystemWatcher::FileSystemWatcher(const std::string& path)
{
    fd = inotify_init();
    if(fd < 0)
        throw std::runtime_error("fehler :(");

    int watchFileDescriptor = inotify_add_watch(fd, path.c_str(), IN_CREATE | IN_MODIFY);
    if(watchFileDescriptor < 0)
        throw std::runtime_error("fehler 2:(");

    const int BUFFER_SIZE = 1024 * (16 + sizeof(inotify_event));

    char buffer[BUFFER_SIZE];



}

inotify_event_t FileSystemWatcher::ReadEvent()
{
    struct inotify_event event;

    int length = read(fd, &event, sizeof (event));
    if(length < 0)
        throw std::runtime_error("fehler 3:(");

    if (event.len)
    {
        char buffer[event.len];
        length = read(fd, buffer, event.len);
    }

}
