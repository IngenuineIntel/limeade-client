// user_env.c
// manages operating environment for the program
//
// preamble:
// This program is designed to be run as its own user, called `limeade`. If
// such a user exists, the program will attempt to change its UID to that of
// this user, and if it fails, throws a warning. If the user does not exists,
// the environment pertaining to this user must be created, and it is created
// here.
//
// what this program expects:
// - for the executing user to be in the `limeade` user group
// - for the executing user not to be root
// - for there to be a readable `/etc/limeade` directory only for said user group
// - for the permissions of `/etc/limeade` to be 660
// - (optional) for there to be a config file `/etc/limeade/limeade-client.ini`
//
// for example: (this code will be somewhere useful at some point)
//
// ~$ groupadd --system limeade
// ~$ mkdir /etc/limeade
// ~$ chown root:limeade /etc/limeade
// ~$ chmod 660 /etc/limeade
//

#include<dirent.h>
#include<errno.h>
#include<grp.h>
#include<pwd.h>
#include<unistd.h>

#include"logging.h"

static const char TARGET_USER_NAME[] = "limeade";
static const char *TARGET_GROUP_NAME = TARGET_USER_NAME;

#define ERR(...)\
do { log_crit(__VA_ARGS__); return -1; } while(0)

int confirm_system_configuration(void)
{
  if(geteuid() == 0)
  {
    struct passwd *pw = getpwnam(TARGET_USER_NAME);
    if(!pw || setuid(pw->pw_uid) < 0)
      ERR("Failed to descalate priviledges into user 'limeade'");
    log_info("Changed user from root to 'limeade'");
  }

  {
    register struct group *ugrp = getgrnam(TARGET_GROUP_NAME);

    if(!ugrp)
      ERR("No 'limeade' user group to be a member of");

    // 30 is preseumptuous, but also a _lot_
    gid_t groups[30];
    if(getgroups(30, groups) < 0)
      ERR("Failed to acquire user groups");

    register gid_t target_gid = ugrp->gr_gid;
    for(register int i = 0; i < 30; i++)
      if(groups[i] == target_gid)
        goto found_group;

    ERR("Process isn't a member of the 'limeade' user group");
  }

found_group:

  {
    register DIR *dfd = opendir("/etc/limeade");
    if(!dfd && errno == EACCES)
    {
      log_warn("Permissions denied when accessing '/etc/limeade'");
      log_warn("Default configurations will be used.");
    }
  }

  return 0;
}


#undef ERR

