# Limeade Client Engine

The Limeade Client Engine (or LCE (pronouced "lick")) is the kernel module
responsible for acquiring the system call aspect of the data that the Limeade
Client is responsible for sending.

## How It Works

![Flowchart](/static/lce_diag.svg)

The `/proc/lce` file is readable only by processes whose effective or
supplementary groups include the `limeade` group. The module reads that group's
numeric GID from `/etc/group` when the proc file is first opened.
