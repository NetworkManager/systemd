/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2015 Lubomir Rintel

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mount.h>

#include "systemd/sd-id128.h"

#include "machine-secret-setup.h"
#include "macro.h"
#include "util.h"
#include "mkdir.h"
#include "log.h"
#include "fileio.h"
#include "path-util.h"

int machine_secret_setup(const char *root) {
        const char *etc_machine_secret;
        static sd_id128_t machine_secret;
        char str[SD_ID128_STRING_MAX];
        _cleanup_close_ int fd = -1;
        int r;

        if (isempty(root))  {
                etc_machine_secret = "/etc/machine-secret";
        } else {
                char *x;

                x = strjoina(root, "/etc/machine-secret");
                etc_machine_secret = path_kill_slashes(x);
        }

        mkdir_parents(etc_machine_secret, 0755);
        fd = open(etc_machine_secret, O_RDWR|O_CREAT|O_CLOEXEC|O_NOCTTY, 0400);
        if (fd < 0) {
                if (errno == EEXIST)
                        return 0;
                log_error_errno(errno, "Can't create /etc/machine-secret: %m");
                return -errno;
        }

        r = sd_id128_randomize(&machine_secret);
        if (r < 0)
                return log_error_errno(r, "Failed to generate a randomized IPv6 stable secret: %m");

        sd_id128_to_string(machine_secret, str);
        r = loop_write(fd, str, SD_ID128_STRING_MAX, false);
        if (r < 0)
                return log_error_errno(r, "Could not write the IPv6 stable secret: %m");

        return 0;
}
