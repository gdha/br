/*
 * Tool become root (br) if you belong to the right group defined in br.h
 * Author: Gratien Dhaese
 * Copyright: GPL v3
 *
 * user = your username on the Linux system (most likely non-root) [char]
 * num_groups = the amount of groups the user belongs to [int]
 * numAllowedGroups = amount of groups that allow access to the bash shell with root privileges [int]
 * member = user account within a certain group
 * allowed_groups array = list the groups listed in the br.h configuration file
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <errno.h>
#include "br.h"


int countStrings(char allowedGroups[][MAX_STR_LEN], int maxNumStrings) {
    int count = 0;

    // Iterate through the array until you encounter a null terminator
    for (int i = 0; i < maxNumStrings; i++) {
        // If the first character of the string is null, it means we've reached the end of the array
        if (allowedGroups[i][0] == '\0') {
            break;
        }
        count++;
    }

    return count;
}

int main(void) {

    // Define some variable to contain the group of the RUSER
    gid_t group_list[MAX_GROUP_STRINGS];
    int num_groups;             // Number of groups variable
    char buffer[4096];          // Buffer for additional group information
    struct group grp = {0};     // Group structure to store group details and zero initialize all fields
    struct group *groupResult = NULL;

    // Get the real username from the real UID — cannot be spoofed unlike getlogin_r()
    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        fprintf(stderr, "Failed to look up user for UID %d\n", (int)getuid());
        return 1;
    }
    const char *user = pw->pw_name;

    // Get the number of supplementary groups the process belongs to
    num_groups = getgroups(MAX_GROUP_STRINGS, group_list);
    if (num_groups == -1) {
        perror("getgroups");
        return 1;
    }

    // Count the amount of listed groups in the allowed_groups array (see br.h header file)
    int numAllowedGroups = countStrings(allowed_groups, MAX_GROUP_STRINGS);

    // Iterate over each group the process belongs to
    for (int i = 0; i < num_groups; i++) {
        // Get group entry by its GID
        int ret = getgrgid_r(group_list[i], &grp, buffer, sizeof(buffer), &groupResult);
        if (ret != 0) {
            perror("getgrgid_r");
            continue;
        }
        if (groupResult == NULL) {
            continue;
        }

        // Check if this group is in the allowed_groups list
        for (int igrp = 0; igrp < numAllowedGroups; igrp++) {
            if (strcmp(grp.gr_name, allowed_groups[igrp]) == 0) {
                // User's process is a member of an allowed group — proceed to elevate
                printf("%s is a member of group %s (gid: %d) and allowed access.\n\n",
                       user, grp.gr_name, grp.gr_gid);

                // Elevate to root only after authorization is confirmed
                if (setuid(0) == -1) {
                    perror("setuid to 0 fails");
                    exit(EXIT_FAILURE);
                }

                // Clear the environment to prevent environment variable attacks
                // environ = NULL is used for portability (clearenv is GNU-specific)
                extern char **environ;
                environ = NULL;

                // Restore a safe, minimal environment for the root shell
                setenv("TERM",    "xterm", 1);
                setenv("HOME",    "/root", 1);
                setenv("USER",    "root",  1);
                setenv("LOGNAME", "root",  1);
                setenv("PATH",    "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", 1);

                // Specify the path to bash
                const char *path_to_bash = "/bin/bash";

                // Specify the option to load the .bash_profile
                const char *bash_option = "--login";

                // Execute bash with .bash_profile
                if (execl(path_to_bash, "bash", bash_option, NULL) == -1) {
                    perror("execl");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    // User was not part of any allowed group
    fprintf(stderr, "Not authorized!\n");
    return 1;
}
