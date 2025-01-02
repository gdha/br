/*
 * Tool become root (br) if you belong to the right group defined in br.h
 * Author: Gratien Dhaese
 * Copyright: GPL v3
 *
 * user = your username on the Linux system (most likely non-root) [char]
 * num_groups = the amount of groups the user belongs to [int]
 * numAllowedGroups = amount of groups that allow access to the bash shell with root priveleges [int]
 * member = user account within a certian group
 * allowed_groups array = list the groups listed in the br.h configuration file
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <grp.h>
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

    // Set the UID to root (0)
    if (setuid(0) == -1) {
       perror("setuid to 0 fails");
       exit(EXIT_FAILURE);
    }

    // Define some variable to contain the group of the RUSER
    gid_t group_list[MAX_GROUP_STRINGS];
    int num_groups;             // Nummer of groups variable
    char user[256];             // Buffer to hold the username
    int result;                 // Contains exit code user lookup
    // gid_t gid = getgid();       // Get the group ID of the current process
    struct group grp;           // Group structure to store group details
    struct group *groupResult;  // Pointer to hold the result of the group lookup
    char buffer[1024];          // Buffer for additional group information

    // Get the number of groups the user is a member of
    num_groups = getgroups(MAX_GROUP_STRINGS, group_list);

    if (num_groups == -1) {
        perror("getgroups");
        return 1;
    }

    // Get the user's login name
    result = getlogin_r(user, sizeof(user));
    if (result == 0) {
        // Success: user contains the logged-in user's name
        // printf("logged in as %s\n", user);
        ;
    } else {
        // Handle errors
        if (result == ERANGE) {
            fprintf(stderr, "Buffer size is too small for username\n");
        } else {
            perror("getlogin_r");
        }
    }

    // Count the amount of listed groups in the allowed_groups array (see br.h header file)
    int numAllowedGroups = countStrings(allowed_groups, MAX_GROUP_STRINGS);

    // Iterate over each group the user is member of to check if the user belongs to it
    for (int i = 0; i < num_groups; i++) {
        // Get group entry by its ID
        int ret = getgrgid_r(group_list[i], &grp, buffer, sizeof(buffer), &groupResult);
        // grp = getgrgid_r(group_list[i]);
        if (ret == 0) {
            if (groupResult == NULL) {
                // No group found for the given GID
                printf("no group found for GID: %d\n", group_list[i]);
            }
        } else {
            // Handle error
            fprintf(stderr, "Error: %s\n", strerror(ret));
        }

        // Check if the user is a member (member is an username) of the group
        char **members = grp.gr_mem;

        while (*members != NULL) {

            if (strcmp(*members, user) == 0) {
		// igrp is an integer counter used for the amount of strings in array allowed_groups
		for (int igrp = 0; igrp < numAllowedGroups; igrp++) {
                    if (strcmp(grp.gr_name, allowed_groups[igrp]) == 0)  {
                        // User is part of the 'wheel' group and is allowed to proceed
                        printf("%s is a member of group %s (gid: %d) and allowed access.\n\n", user, grp.gr_name, grp.gr_gid);

                        // Clear the environment
                        clearenv();
                        // We noticed that TERM=xterm was missing so we define on purpose
                        setenv("TERM", "xterm", 1);

                        // We need to set enviroment HOME to /root
                        // However, we noticed that HOME is not defined under root, but when we do exit HOME=/root instead of the user-home
                        setenv("HOME", "/root", 1);

                        // Specify the path to bash
                        char *path_to_bash = "/bin/bash";
    
                        // Specify the option to load the .bash_profile
                        char *bash_option = "--login";

                        // Execute bash with .bash_profile
                        if (execl(path_to_bash, "bash", bash_option, NULL) == -1) {
                            perror("execl");
                            exit(EXIT_FAILURE);
                        }
                    } // end of if (strcmp(grp->gr_name, allowed_groups[igrp])
                } // end of for (int igrp = 0; igrp < numAllowedGroups; igrp++)
	    } // end of if (strcmp(*members, user) == 0)
            members++;
        }  // of while
    } // of for (i = 0; i < num_groups; i++)

    // When you at this point the user was not part of the 'wheel' group so we say:
    perror("Not authorized!");
    return 0;
}
