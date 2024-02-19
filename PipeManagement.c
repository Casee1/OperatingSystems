#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>


#define RESP_PIPE_10029 "RESP_PIPE_10029"
#define REQ_PIPE_10029 "REQ_PIPE_10029"

unsigned int size = 0;
char* data1;
char* data2;

void create_pipe()
{

    if (mkfifo(RESP_PIPE_10029, 0666) == -1) {
        printf("ERROR\ncannot create the response pipe\n");
        unlink(RESP_PIPE_10029);

    }
}

void message_pipe_created(int req_pipe_10029)
{
    printf("ERROR\ncannot open the request pipe\n");
    close(req_pipe_10029);
    exit(2);
}

void close_pipe_for_writing(int resp_pipe_10029)
{
    printf("ERROR\ncannot open the request pipe\n");
    close(resp_pipe_10029);
    exit(1);
}

void print_success(int resp_pipe_10029)
{
    const char* REQUEST_MESSAGE = "BEGIN";
    int request_message_size = strlen(REQUEST_MESSAGE);
    int check1 = write(resp_pipe_10029,&request_message_size, 1);
    int check2 = write(resp_pipe_10029, REQUEST_MESSAGE, request_message_size); 

    if(check1 == 1 && check2 == request_message_size){
        printf("SUCCESS\n");
    }
}

void create_shm(int memory,int resp_pipe_10029)
{
    const char* SHM_NAME = "/xxdCWN";
    const char* SUCCESS = "SUCCESS";
    const char* ERROR = "ERROR";
    const char* SHM_STRING = "CREATE_SHM";

    int success_size = strlen(SUCCESS);
    int error_size = strlen(ERROR);
    int shm_string_size = strlen(SHM_STRING);
    
    int shm_op = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0664);
    write(resp_pipe_10029,&shm_string_size, 1);
    write(resp_pipe_10029, SHM_STRING, shm_string_size);
    if(shm_op == -1)
    {
        write(resp_pipe_10029, &error_size, 1);
        write(resp_pipe_10029,ERROR, error_size);
        exit(0);
    }

    if(ftruncate(shm_op,memory) == -1)
    {
        write(resp_pipe_10029, &error_size, 1);
        write(resp_pipe_10029,ERROR, error_size);
        shm_unlink(SHM_NAME);
        exit(0);
    }

    data1 = (char*)mmap(NULL, memory, PROT_READ | PROT_WRITE , MAP_SHARED, shm_op, 0);

    if (data1 == MAP_FAILED) {
        write(resp_pipe_10029, &error_size, 1);
        write(resp_pipe_10029, ERROR, error_size);
        shm_unlink(SHM_NAME);
        exit(0);
    }

    write(resp_pipe_10029, &success_size, 1);
    write(resp_pipe_10029, SUCCESS, success_size);
}

void write_to_shm(int resp_pipe_10029, unsigned int offset, unsigned int value)
{
    const char* WRITE_TO_SHM_STRING = "WRITE_TO_SHM";
    const char* SUCCESS = "SUCCESS";
    const char* ERROR = "ERROR";

    int write_to_shm_size = strlen(WRITE_TO_SHM_STRING);
    int success_size = strlen(SUCCESS);
    int error_size = strlen(ERROR);


    write(resp_pipe_10029, &write_to_shm_size, 1);
    write(resp_pipe_10029, WRITE_TO_SHM_STRING, write_to_shm_size);
    if (offset < 0 || offset > 4079868 || offset + sizeof(unsigned int) > 4079868 ) {
        write(resp_pipe_10029, &error_size, 1);
        write(resp_pipe_10029, ERROR, error_size);
        
    } 
    else 
    {
        
        memcpy(data1+offset,&value, sizeof(unsigned int));
        write(resp_pipe_10029, &success_size, 1);
        write(resp_pipe_10029, SUCCESS, success_size);
    }
}

void map_file(int resp_pipe_10029, char* file)
{   
    const char* MAP_FILE_STRING = "MAP_FILE";
    const char* SUCCESS = "SUCCESS";
    const char* ERROR = "ERROR";

    int map_file_size = strlen(MAP_FILE_STRING);
    int success_size = strlen(SUCCESS);
    int error_size = strlen(ERROR);

    write(resp_pipe_10029, &map_file_size, 1);
    write(resp_pipe_10029, MAP_FILE_STRING, map_file_size);
    unsigned int file_open = open(file, O_RDONLY);
    if(file_open == -1)
    {
        write(resp_pipe_10029, &error_size, 1);
        write(resp_pipe_10029, ERROR, error_size);
        exit(0);
    }

    unsigned int size = lseek(file_open,0,SEEK_END);

    data2 = (char*)mmap(NULL,size,O_RDONLY ,MAP_SHARED,file_open,0);
    if(data2 == MAP_FAILED)
    {
        write(resp_pipe_10029, &error_size,1);
        write(resp_pipe_10029,ERROR,error_size);
        //close(file_open);
        return;
    }
    
    write(resp_pipe_10029,&success_size,1);
    write(resp_pipe_10029,SUCCESS, success_size);

}

void read_from_file_offset(int resp_pipe_10029,unsigned int offset, unsigned int no_of_bytes,unsigned int file_size)
{
    const char* READ_FROM_FILE_OFFSET_STRING = "READ_FROM_FILE_OFFSET";
    const char* SUCCESS = "SUCCESS";
    const char* ERROR = "ERROR";

    int read_from_file_offset_size = strlen(READ_FROM_FILE_OFFSET_STRING);
    int success_size = strlen(SUCCESS);
    int error_size = strlen(ERROR);

    write(resp_pipe_10029, &read_from_file_offset_size, 1);
    write(resp_pipe_10029, READ_FROM_FILE_OFFSET_STRING, read_from_file_offset_size);

    char* file = (char*)malloc(no_of_bytes * sizeof(char));


    if (data1 == NULL || data2 == NULL) 
    {
        write(resp_pipe_10029, &error_size, 1);
        write(resp_pipe_10029, ERROR, error_size);
        return;
    }


    if(file_size <= offset + no_of_bytes)
    {
        write(resp_pipe_10029, &error_size, 1);
        write(resp_pipe_10029,ERROR, error_size);
        return;
    }

    int i = 0;
    for(int j = offset; j < offset + no_of_bytes; j ++)
    {
        file[i] = data2[j];
        data1[i] = data2[j];
        i++;
    }

    write(resp_pipe_10029, &success_size, 1);
    write(resp_pipe_10029, SUCCESS, success_size);


}

void exit_f(int req_pipe_10029, int resp_pipe_10029,char* buffer)
{
    close(req_pipe_10029);
    close(resp_pipe_10029);
}

int main() {

    create_pipe();
  
    int req_pipe_10029 = open(REQ_PIPE_10029, O_RDONLY);

    if (req_pipe_10029 == -1) {
        message_pipe_created(req_pipe_10029);
    }

    unsigned int resp_pipe_10029 = open(RESP_PIPE_10029, O_WRONLY);

    if(resp_pipe_10029 == -1){
        close_pipe_for_writing(resp_pipe_10029);
    }

    print_success(resp_pipe_10029);
    
    
    int loop = 1;
    while(loop)
    {
         

        const char* ECHO = "ECHO";
        int echo_size = strlen(ECHO);
        const char* VARIANT = "VARIANT";
        int variant_size = strlen(VARIANT);
        int varianta = 10029;
        read(req_pipe_10029, &size, 1);
        char* buffer = (char*)malloc(size * sizeof(char)); 
        read(req_pipe_10029, buffer, size);

        if(strncmp(buffer, "ECHO", 4) == 0)
        {
            write(resp_pipe_10029,&echo_size,1);
            write(resp_pipe_10029, ECHO, echo_size);
            write(resp_pipe_10029,&variant_size,1);
            write(resp_pipe_10029,VARIANT, variant_size);
            write(resp_pipe_10029,&varianta,4);
        }


        if(strncmp(buffer, "CREATE_SHM", 10) == 0)
        {
            int memory = 0;
            read(req_pipe_10029,&memory, sizeof(memory));
            create_shm(memory,resp_pipe_10029);
        }
        
        

        if(strncmp(buffer, "WRITE_TO_SHM", 12) == 0)
        {
            
            unsigned int value;
            unsigned int offset;
            read(req_pipe_10029, &offset, sizeof(unsigned int));
            read(req_pipe_10029, &value, sizeof(unsigned int));
            
            write_to_shm(resp_pipe_10029,offset,value);
        }

        unsigned int file_size = 0;
        
        
        if(strncmp(buffer,"MAP_FILE", 8) == 0)
        {
            read(req_pipe_10029, &file_size, 1);
            char* file = (char*)malloc(file_size * sizeof(char));
            read(req_pipe_10029,file,file_size);
            file[file_size] = '\0';
            map_file(resp_pipe_10029, file);
        }

        if(strncmp(buffer, "READ_FROM_FILE_OFFSET", 21) == 0)
        {   unsigned int offset = 0;
            read(req_pipe_10029,&offset,sizeof(unsigned int));

            unsigned int no_of_bytes = 0;
            read(req_pipe_10029, &no_of_bytes,sizeof(unsigned int));

            read_from_file_offset(resp_pipe_10029,offset,no_of_bytes,file_size+1);
        }

        

        if(strcmp(buffer, "EXIT") == 0)
        {
            exit_f(resp_pipe_10029,req_pipe_10029,buffer);
            loop = 0;
        }
        
    }

    return 0;
}