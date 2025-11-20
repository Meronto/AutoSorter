#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>


typedef struct{
    char extension[10];
    char folder[50];

} Expansion;


void scan_directory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        printf("Warning: Couldn't open folder %s\n", path);
        return;
    }
    
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char *dot = strrchr(entry->d_name, '.');
        if (dot != NULL) {
            printf("File: %s, extension: %s\n", entry->d_name, dot);
        } else {
            printf("File: %s (no extension)\n", entry->d_name);
        }
    }
    
    closedir(dir);
}


void trim(char *str) {
    int start = 0;
    int end = strlen(str) -1;

    while (start <= end && str[start] == ' '){
        start++;
    }
    while (end >= start && str[end] == ' '){
        end--;
    }
    strncpy(str, str + start, end - start + 1);
    str[end - start + 1] = '\0';

}

int config_parsion(const char *config_file, Expansion *expansions, int max_size) {
    FILE *file = fopen(config_file, "r");
    if (!file) {
        printf("Warning: Couldn't open %s", config_file);
        return 0;
    }
    char line[256];
    int count = 0;
       while (fgets(line, sizeof(line), file) && count < max_size) {
        if (line[0] == '\n' || line[0] == '#' || line[0] == '\0')
            continue;

        char *arrow = strstr(line, "->");
        if (!arrow) {
            printf("Warning: invalid line format: %s", line);
            continue;
        }

        strncpy(expansions[count].extension, line, arrow - line);
        expansions[count].extension[arrow - line] = '\0';

        strcpy(expansions[count].folder, arrow + 2);

        trim(expansions[count].extension);
        trim(expansions[count].folder);

        count++;
    }

    fclose(file);
    return count;

}

void log_action(FILE *log_file, const char *action, const char *source, const char *destination) {
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    fprintf(log_file, "[%s] %s: %s -> %s\n", time_str, action, source, destination);
    
    fflush(log_file);
}


void sort_files(const char *source_path, const char *dest_root, Expansion *expansions, int count) {
    FILE *log_file = fopen("log.txt", "a");
    if (!log_file) {
        printf("Warning: Cannot create log file\n");
    }
    
    DIR *dir = opendir(source_path);
    if (!dir) {
        printf("Cannot open folder %s\n", source_path);
        return;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char *dot = strrchr(entry->d_name, '.');
        char *file_ext = dot ? dot + 1 : NULL;

        char destination_folder[256] = {0};
        int matched = 0;
        if (file_ext) {
            for (int i = 0; i < count; i++) {
                if (strcmp(file_ext, expansions[i].extension) == 0) {
                    strcpy(destination_folder, expansions[i].folder);
                    matched = 1;
                    break;
                }
            }
        }

        if (!matched) {
            strcpy(destination_folder, "Unknown");
        }

        char source_file_path[2048];
        snprintf(source_file_path, sizeof(source_file_path), "%s/%s", source_path, entry->d_name);

        char full_dest_folder[2048];
        snprintf(full_dest_folder, sizeof(full_dest_folder), "%s/%s", dest_root, destination_folder);

        if (mkdir(full_dest_folder, 0755) != 0) {
            if (errno != EEXIST) {
                printf("Error creating directory %s\n", full_dest_folder);
                if (log_file) {
                    log_action(log_file, "ERROR", source_file_path, "Failed to create directory");
                }
                continue;
            }
        }

        char dest_file_path[2048];
        snprintf(dest_file_path, sizeof(dest_file_path), "%s/%s", full_dest_folder, entry->d_name);

        if (rename(source_file_path, dest_file_path) == 0) {
            printf("Moved: %s -> %s\n", source_file_path, dest_file_path);
            
            if (log_file) {
                log_action(log_file, "MOVED", source_file_path, dest_file_path);
            }
        } else {
            printf("Failed to move %s\n", source_file_path);
            
            if (log_file) {
                log_action(log_file, "FAILED", source_file_path, dest_file_path);
            }
        }
    }

    closedir(dir);
    
    if (log_file) {
        fclose(log_file);
        printf("\nLog saved to log.txt\n");
    }
}

void decode_url_path(char *path) {
    // Удаляем file:// префикс если есть
    if (strncmp(path, "file://", 7) == 0) {
        // Копируем всё после file://
        char temp[2048];
        strcpy(temp, path + 7);
        strcpy(path, temp);
    }
    
    // Декодируем URL-символы (%20 -> пробел и т.д.)
    int j = 0;
    for (int i = 0; path[i] != '\0'; i++) {
        if (path[i] == '%' && i + 2 < strlen(path)) {
            char hex[3];
            hex[0] = path[i + 1];
            hex[1] = path[i + 2];
            hex[2] = '\0';
            
            int char_code = (int)strtol(hex, NULL, 16);
            path[j++] = (char)char_code;
            i += 2;
        } else if (path[i] != '\n' && path[i] != '\r') {
            path[j++] = path[i];
        }
    }
    path[j] = '\0';
}


void clean_path(char *path) {
    if (!path || path[0] == '\0') {
        return;
    }
    decode_url_path(path);
    
    int start = 0;
    int end = strlen(path) - 1;
    
    while (start <= end && isspace(path[start])) {
        start++;
    }
    while (end >= start && isspace(path[end])) {
        end--;
    }
    
    strncpy(path, path + start, end - start + 1);
    path[end - start + 1] = '\0';
    
    #ifdef _WIN32
        for (int i = 0; path[i] != '\0'; i++) {
            if (path[i] == '/') {
                path[i] = '\\';
            }
        }
    #else
        for (int i = 0; path[i] != '\0'; i++) {
            if (path[i] == '\\') {
                path[i] = '/';
            }
        }
    #endif
}




int main() {
    char source_folder[256];
    char dest_folder[256];
    
    Expansion expansions[100];
    int count = config_parsion("./config/config.conf", expansions, 100);
    
    if (count == 0) {
        printf("Warning: Couldn't read the config\n");
        return 1;
    }
    
    printf("Configs loaded: %d\n", count);
    for (int i = 0; i < count; i++) {
        printf("%s -> %s\n", expansions[i].extension, expansions[i].folder);
    }
    
    printf("\nEnter source folder (where files are): ");
    fgets(source_folder, sizeof(source_folder), stdin);
    source_folder[strcspn(source_folder, "\n")] = 0;
    clean_path(source_folder);

    printf("Enter destination folder (where to sort): ");
    fgets(dest_folder, sizeof(dest_folder), stdin);
    dest_folder[strcspn(dest_folder, "\n")] = 0;
    clean_path(dest_folder); 
    
    printf("\nStarting file sorting from %s to %s...\n\n", source_folder, dest_folder);
    
    sort_files(source_folder, dest_folder, expansions, count);
    
    printf("\nDone!\n");
    
    return 0;
}

