#ifndef PROJECTSOL_ORDEREDRECORDLIST_H
#define PROJECTSOL_ORDEREDRECORDLIST_H

//Data structure holding a list of records with two fields, a long "value" and a char * "name"
//The records are kept in order of ascending "value".
//All non-void functions return 0 on success and -1 on failure.
typedef struct ordered_recordlist *ordered_recordlist;
//Initializes "list", setting the maximum name size
int ordered_recordlist_init(ordered_recordlist *list, int name_size);
//Frees all resources held by "list"
int ordered_recordlist_destroy(ordered_recordlist list);
//Adds a record to list, setting name as "name" and value as "value"
int ordered_recordlist_add(ordered_recordlist list, char *name, long value);
//Prints the contents of all records to the standard input in this format:
//value1 name1
//value2 name2
//...
void ordered_recordlist_printall(ordered_recordlist list);

#endif //PROJECTSOL_ORDEREDRECORDLIST_H
