/*
 * br header file
 */

#ifndef BR_HEADER_H
#define BR_HEADER_H

#define MAX_GROUP_STRINGS 15   /* Maximum number of strings in the array */
#define MAX_STR_LEN 20         /* Maximum length of each string */
char allowed_groups[MAX_GROUP_STRINGS][MAX_STR_LEN] = {
	"wheel",
	""   // Empty string to mark the end of the array
};

#endif /* BR_HEADER_H */
