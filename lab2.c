#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <alloca.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <locale.h>

#define INODES_SIZE 255

char *prog_name = NULL;
int find = 0;

void print_error(const char *s_name, const char *msg, const char *f_name)
{
    fprintf(stderr, "%s: %s %s\n", s_name, msg, (f_name)? f_name : "");
}

int process(char *dir_name, char *file_name, int files) {

    DIR *cd = opendir(dir_name);
    if (!cd) {
        print_error(prog_name, strerror(errno), dir_name);
        return;
    }

    char *curr_name = alloca(strlen(dir_name) + NAME_MAX + 3);
    curr_name[0] = 0;
    strcat(curr_name, dir_name);
    strcat(curr_name, "/");
    size_t curr_name_len = strlen(curr_name);

    struct dirent *entry = alloca( sizeof(struct dirent) );
    struct stat st;

    size_t ilist_len = INODES_SIZE;
    ino_t *ilist = malloc(ilist_len * sizeof(ino_t) );
    int ilist_next = 0;

    errno = 0;

    while ( (entry = readdir(cd) ) ) {

	curr_name[curr_name_len] = 0;
        strcat(curr_name, entry->d_name);

	if (lstat(curr_name, &st) == -1) {
            print_error(prog_name, strerror(errno), curr_name);
        }
        else {
	    ino_t ino = st.st_ino;

	    if (ilist_next == ilist_len) {
                ilist_len *= 2;
                ilist = (ino_t*)realloc(ilist, ilist_len*sizeof(ino_t) );
            }

            int i = 0;
            while ( (i < ilist_next) && (ino != ilist[i]) )
                ++i;

	    if (i == ilist_next)
            {
                ilist[ilist_next] = ino;
                ++ilist_next;

                if ( S_ISDIR(st.st_mode) )
                {
                    if ( (strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) )  {
                        files += process(curr_name, file_name, 0);
			
                    }
                }
                else if ( S_ISREG(st.st_mode) )
                {
		    //проверка, найден ли файл
		    if (strcmp(entry->d_name, file_name) == 0) {
			find = 1;

			char *path;
			path = malloc(strlen(dir_name) + strlen(file_name) + 2);
			if (path) {
			    strcpy(path, dir_name);
			    strcat(path, "/");
			    strcat(path, file_name);
			}
			struct tm * timeinfo;
  			timeinfo = localtime ( &st.st_mtime );
			char buffer [150];
			unsigned int mode = (unsigned int) st.st_mode;
			printf("%s %ld ", path, (long) st.st_size);
			strftime(buffer, 150, "%d %b %G", timeinfo);
			printf("%s %3o %ld\n", buffer, mode - 32768, (long) st.st_ino);
		    }

		    ++files; 
                }
            }	
	}
    }

    if (errno != 0) {
        print_error(prog_name, strerror(errno), curr_name);
    }
	
    //успешно ли закрылась директория или с ошибкой
    if (closedir(cd) == -1) {
        print_error(prog_name, strerror(errno), dir_name);
    }

    return files;
}


int main(int argc, char *argv[]) {

    setlocale(LC_ALL, "");
    prog_name = basename(argv[0]);

    //соответствие количества аргументов к.строки заданному
    if (argc != 3) {
        print_error(prog_name, "Wrong count of arguments.", 0);
        return 1;
    }

    char *dir_name = realpath(argv[1], NULL);
    if (dir_name == NULL) {
        print_error(prog_name, "Error opening directory", argv[1]);
        return 1;
    }

    long files_count = 0;

    files_count = process(dir_name, argv[2], files_count);
    if (find == 0) {
	print_error(prog_name, "File not found", 0);
    }

    printf("The whole count of scanned files: \n");
    printf("%ld\n", files_count);

    return 0;
}
