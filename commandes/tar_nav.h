#ifndef TARNAV_H
#define TARNAV_H

char ** decompose(char * prompt, char * delimiter);
int string_contains_tar(char * string);
int current_dir_is_tar();
int tar_file_exists(char * path, char * tar);
char * get_tar_name();
char * get_tar_name_file(char * path);
char * get_path_without_tar();
char * flatten(char ** tokens, char * delimiter);
char * path_is_valid(char * path);
int file_exists_in_tar(char * path, char *  tar);


#endif