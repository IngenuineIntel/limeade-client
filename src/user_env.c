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

#include<errno.h>
#include<grp.h>
#include<dirent.h>
#include<unistd.h>

#include"logging.h"

#define ERR(...)\
do { log_crit(__VA_ARGS__); return -1; } while(0)

int confirm_system_configuration(void)
{
  /* confirms the following:
   * - the executing user is in the `limeade` user group
   * - the executing user is NOT root
   * - there is a directly `/etc/limeade` which is owned by
   *   `root:limeade` with file perm 660
   *
   * If all these are found to be true, 0 is returned. Otherwise, a
   * large clarifying message is printed describing the necessary
   * system configuration and how to fix it, along with a -1 return.
   */

  // 1. root
  if(geteuid() == 0)
    ERR("This program must not be run as root!");

  // 2. user group
  {
    struct group *u_grp = getgrnam("limeade");
    
    if(!u_grp)
      ERR("No 'limeade' user group to be a member of");

    // 20 is presumptuous, but 20 user groups is a _lot_
    gid_t groups[20];
    if(getgroups(20, groups) < 0)
      ERR("Failed to acquire user groups");

    register gid_t target_gid = u_grp->gr_gid;
    for(register int i = 0; i < 20; i++)
      if(groups[i] == target_gid)
        goto found_group;

    ERR("Process isn't a member of the `limeade` user group");
  }
found_group:

  // 3. access to /etc/limeade
  // not going to check if it's more readable than it should be, not within the
  // scope of this program to find mistakes in sysadmin'ing
  {
    DIR *dfd = opendir("/etc/limeade");
    if(!dfd && errno == EACCES)
      ERR("%s %s",
          "Permission denied when accessing '/etc/limeade'.",
          "Does the directory belong to 'root:limeade' with perm 660?");
  }
  return 0;
}

#undef ERR

