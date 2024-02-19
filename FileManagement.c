#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <regex.h>

#define MAGIC "j0"
#define HEADER_SIZE 2
#define MIN_VERSION 54
#define MAX_VERSION 91
#define MIN_SECTIONS 4
#define MAX_SECTIONS 20
#define MAX_LINE_LENGTH 1024
#define SECTION_SIZE 14

void list_dir(char *path, int recursive, off_t size_greater, char *permission)
{
    DIR *dir = opendir(path);
    struct dirent *entry;
    char fullpath[512];
    struct stat s;

    if (dir == NULL)
    {
        perror("ERROR\ninvalid directory path\n");
        exit(4);
    }

    while ((entry = readdir(dir)) != NULL)
    {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            stat(fullpath, &s);
            if (s.st_size > size_greater)
            {
                if (permission != NULL)
                {
                    mode_t permission_bits = s.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);

                    char permission_string[10];
                    permission_string[9] = 0;

                    snprintf(permission_string, sizeof(permission_string), "%c%c%c%c%c%c%c%c%c",
                             (permission_bits & S_IRUSR) ? 'r' : '-',
                             (permission_bits & S_IWUSR) ? 'w' : '-',
                             (permission_bits & S_IXUSR) ? 'x' : '-',
                             (permission_bits & S_IRGRP) ? 'r' : '-',
                             (permission_bits & S_IWGRP) ? 'w' : '-',
                             (permission_bits & S_IXGRP) ? 'x' : '-',
                             (permission_bits & S_IROTH) ? 'r' : '-',
                             (permission_bits & S_IWOTH) ? 'w' : '-',
                             (permission_bits & S_IXOTH) ? 'x' : '-');

                    if (strcmp(permission_string, permission) != 0)
                    {
                        continue;
                    }
                }

                printf("%s\n", fullpath);
            }

            if (entry->d_type == DT_DIR && recursive)
            {
                list_dir(fullpath, 1, size_greater, permission);
            }
        }
    }

    closedir(dir);
}

typedef struct
{
    char name[9];
    int type;
    int offset;
    int size;
} SectionHeader;

int parse(int fd)
{
    int version;
    u_int8_t nr_sections;
    char magic[3];
    lseek(fd, -2, SEEK_END);
    if (read(fd, magic, 2) != 2)
    {
        printf("ERROR\nwrong magic\n");
        return 0;
    }
    magic[2] = '\0';

    if (strcmp(magic, MAGIC) != 0)
    {
        printf("ERROR\nwrong magic\n");
        return 0;
    }

    short header;

    lseek(fd, -4, SEEK_END);
    read(fd, &header, 2);

    lseek(fd, -header, SEEK_END);

    if (read(fd, &version, sizeof(version)) != sizeof(version))
    {
        printf("ERROR\nwrong version\n");
        return 0;
    }
    if (version < MIN_VERSION || version > MAX_VERSION)
    {
        printf("ERROR\nwrong version\n");
        return 0;
    }
    if (read(fd, &nr_sections, sizeof(nr_sections)) != sizeof(nr_sections))
    {
        printf("ERROR\nwrong sect_nr\n");
        return 0;
    }
    if (nr_sections < MIN_SECTIONS || nr_sections > MAX_SECTIONS)
    {
        printf("ERROR\nwrong sect_nr\n");
        return 0;
    }
    SectionHeader headers[MAX_SECTIONS];

    for (int i = 0; i < nr_sections; i++)
    {
        read(fd, &headers[i].name, 8);
        headers[i].name[8] = '\0';
        read(fd, &headers[i].type, 4);
        read(fd, &headers[i].offset, 4);
        read(fd, &headers[i].size, 4);
        if (headers[i].type != 11 && headers[i].type != 73 && headers[i].type != 30)
        {
            printf("ERROR\nwrong sect_types\n");
            return 0;
        }
    }

    printf("SUCCESS\nversion=%d\nnr_sections=%d\n", version, nr_sections);
    for (int i = 0; i < nr_sections; i++)
    {
        printf("section%d: %s %d %d\n", i + 1, headers[i].name, headers[i].type, headers[i].size);
    }
    return 1;
}

int extract(char *file_path, int section, int line)
{
    int fd = open(file_path, O_RDONLY);
    if (fd == -1)
    {
        printf("ERROR\ninvalid file\n");
        return 0;
    }
    int version;
    u_int8_t nr_sections;
    char magic[] = "00";
    lseek(fd, -2, SEEK_END);
    read(fd, magic, 2);
    if (strcmp(magic, MAGIC) != 0)
    {
        printf("ERROR\ninvalid file\n");
        close(fd);
        return 0;
    }

    short header;
    lseek(fd, -4, SEEK_END);
    read(fd, &header, 2);
    lseek(fd, -header, SEEK_END);

    if (read(fd, &version, sizeof(version)) != sizeof(version) ||
        version < MIN_VERSION || version > MAX_VERSION)
    {
        printf("ERROR\ninvalid file\n");
        close(fd);
        return 0;
    }
    if (read(fd, &nr_sections, sizeof(nr_sections)) != sizeof(nr_sections) ||
        nr_sections < MIN_SECTIONS || nr_sections > MAX_SECTIONS)
    {
        printf("ERROR\ninvalid file\n");
        close(fd);
        return 0;
    }
    SectionHeader headers[MAX_SECTIONS];
    for (int i = 0; i < nr_sections; i++)
    {
        read(fd, &headers[i].name, 8);
        headers[i].name[8] = '\0';
        read(fd, &headers[i].type, 4);
        read(fd, &headers[i].offset, 4);
        read(fd, &headers[i].size, 4);
        if (headers[i].type != 11 && headers[i].type != 73 && headers[i].type != 30)
        {
            printf("ERROR\ninvalid file\n");
            close(fd);
            return 0;
        }
    }

    if (section < 1 || section > nr_sections)
    {
        printf("ERROR\ninvalid section\n");
        close(fd);
        return 0;
    }

    SectionHeader target_header = headers[section - 1];
    lseek(fd, target_header.offset, SEEK_SET);

    char lineV[MAX_LINE_LENGTH];
    int line_count = 0;
    int bytes_read = 0;
    int total_bytes_read = 0;
    while (line_count < target_header.size && (bytes_read = read(fd, lineV, MAX_LINE_LENGTH)) > 0)
    {
        total_bytes_read += bytes_read;
        for (int i = 0; i < bytes_read; i++)
        {

            if (lineV[i] == '\n')
            {
                line_count++;
            }
        }
    }
    if (line > line_count || line < 1)
    {
        printf("ERROR\ninvalid line\n");
        close(fd);
        return 0;
    }

    lseek(fd, target_header.offset, SEEK_SET);
    line_count = 0;
    printf("SUCCESS\n");

    while (line_count < target_header.size && (bytes_read = read(fd, lineV, MAX_LINE_LENGTH)) > 0)
    {
        total_bytes_read += bytes_read;
        for (int i = 0; i < bytes_read; i++)
        {
            if (lineV[i] == '\0')
            {
                close(fd);
                return 0;
            }
            if (line_count == line - 1)
            {
                // lineV[i] = '\0'; // Replace newline with null terminator
                printf("%c", lineV[i]);
            }
            if (lineV[i] == '\n')
            {
                line_count++;
            }

            if (line_count == line)
            {
                close(fd);
                return 1;
            }
        }
    }
    return 0;
}


int validate_file(const char *file_path)
{
    int fd = open(file_path, O_RDONLY);
    int version;
    u_int8_t nr_sections;
    char magic[3];
    lseek(fd, -2, SEEK_END);
    if (read(fd, magic, 2) != 2)
    {
        return 0;
    }
    magic[2] = '\0';

    if (strcmp(magic, MAGIC) != 0)
    {
        return 0;
    }

    short header;

    lseek(fd, -4, SEEK_END);
    read(fd, &header, 2);

    lseek(fd, -header, SEEK_END);

    if (read(fd, &version, sizeof(version)) != sizeof(version))
    {
        return 0;
    }
    if (version < MIN_VERSION || version > MAX_VERSION)
    {
        return 0;
    }
    if (read(fd, &nr_sections, sizeof(nr_sections)) != sizeof(nr_sections))
    {
        return 0;
    }
    if (nr_sections < MIN_SECTIONS || nr_sections > MAX_SECTIONS)
    {
        return 0;
    }

    // Check for SF files with at least 4 sections, each containing exactly 14 lines
    if (strcmp(magic, MAGIC) == 0 && nr_sections >= 4)
    {
        SectionHeader headers[MAX_SECTIONS];
        for (int i = 0; i < nr_sections; i++)
        {
            read(fd, &headers[i].name, 8);
            headers[i].name[8] = '\0';
            read(fd, &headers[i].type, 4);
            read(fd, &headers[i].offset, 4);
            read(fd, &headers[i].size, 4);
            if (headers[i].type != 11 && headers[i].type != 73 && headers[i].type != 30)
            {
                return 0;
            }
            if (headers[i].size != SECTION_SIZE)
            {
                return 0;
            }
        }
        // Check if the file contains at least 4 sections, each containing exactly 14 lines
        int section_count = 0;
        char section_data[SECTION_SIZE];
        for (int i = 0; i < nr_sections; i++)
        {
            lseek(fd, headers[i].offset, SEEK_SET);
            if (read(fd, section_data, SECTION_SIZE) != SECTION_SIZE)
            {
                return 0;
            }
            int line_count = 1;
            for (int j = 0; j < SECTION_SIZE; j++)
            {
                if (section_data[j] == '\n')
                {
                    line_count++;
                }
            }
            if (line_count == 14)
            {
                section_count++;
            }
        }
        if (section_count < 4)
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }

    return 1;
}


void findall(char *path)
{
    DIR *dir = opendir(path);
    struct dirent *entry;
    char fullpath[512];
    struct stat s;

    if (dir == NULL)
    {
        perror("ERROR\ninvalid directory path\n");
        exit(4);
    }

    while ((entry = readdir(dir)) != NULL)
    {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            stat(fullpath, &s);
            if(S_ISREG(s.st_mode) && validate_file(path))
            {
                printf("%s\n", fullpath);
            }
        }

        if (entry->d_type == DT_DIR && validate_file(path))
        {
            
            findall(fullpath);
        }
    }
    closedir(dir);
}


int main(int argc, char **argv)
{
    if (argc >= 2)
    {
        if (strcmp(argv[1], "variant") == 0)
        {
            printf("10029\n");
        }
        else if (strcmp(argv[1], "parse") == 0)
        {
            if (argc < 3)
            {
                printf("Usage: %s parse path=<file_path>\n", argv[0]);
                return 1;
            }
            char *file_path;
            int fd;
            for (int i = 2; i < argc; i++)
            {
                if (strncmp(argv[i], "path=", 5) == 0)
                {
                    file_path = argv[i] + 5;
                }
            }
            if ((fd = open(file_path, O_RDONLY)) == -1)
            {
                perror("open");
                return 1;
            }
            int success = parse(fd);
            close(fd);
            return !success;
        }
        else if (strcmp(argv[1], "list") == 0)
        {
            int recursive = 0;
            char *path = NULL;
            int size_greater_than = 0;
            char *permissions = NULL;

            for (int i = 2; i < argc; i++)
            {
                if (strncmp(argv[i], "path=", 5) == 0)
                {
                    path = &argv[i][5];
                }
                else if (strncmp(argv[i], "recursive", 8) == 0)
                {
                    recursive = 1;
                }
                else if (strncmp(argv[i], "size_greater=", 13) == 0)
                {
                    size_greater_than = atoi(&argv[i][13]);
                }
                else if (strncmp(argv[i], "permissions=", 12) == 0)
                {
                    permissions = &argv[i][12];
                }
            }
            printf("SUCCESS\n");
            list_dir(path, recursive, size_greater_than, permissions);
        }
        else if (strcmp(argv[1], "extract") == 0)
        {
            if (argc < 5)
            {
                printf("Usage: %s extract path=<file_path> section=<section_num> line=<line_num>\n", argv[0]);
                return 1;
            }
            char *file_path = NULL;
            int section_num = 0;
            int line_num = 0;

            for (int i = 2; i < argc; i++)
            {
                if (strncmp(argv[i], "path=", 5) == 0)
                {
                    file_path = &argv[i][5];
                }
                else if (strncmp(argv[i], "section=", 8) == 0)
                {
                    section_num = atoi(&argv[i][8]);
                }
                else if (strncmp(argv[i], "line=", 5) == 0)
                {
                    line_num = atoi(&argv[i][5]);
                }
            }

            int success = extract(file_path, section_num, line_num);
            if (!success)
            {
                return 1;
            }
        }

        else if (strcmp(argv[1], "findall") == 0)
        {
            if (argc < 3)
            {
                printf("ERROR\ninvalig arguments\n");
                return 1;
            }
            char *dir_path;
            for (int i = 2; i < argc; i++)
            {
                if (strncmp(argv[i], "path=", 5) == 0)
                {
                    dir_path = argv[i] + 5;
                }
            }
            printf("SUCCESS\n");
            findall(dir_path);
        }
        else
        {
            printf("Unknown command: %s\n", argv[1]);
            printf("Usage: %s command [args]\n", argv[0]);
            return 1;
        }
    }
    return 0;
}