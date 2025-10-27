#pragma once

struct SystemIOObject
{
    int fd; // File descriptor

    virtual void generate_fd() = 0;
    virtual int handle_io_data() = 0;
};