#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/ioctl.h>

int main(int argc, char **argv)
{
    int buff[1024];
    int fd=open("/dev/test_int_43",O_RDWR);
    int i;
    int n,k;
    unsigned int res;
    unsigned int sum;
    pid_t pid;

    if(argc==2)
        n=atoi(argv[1]);
    else
        n=2;
    
    pid=fork();
//    pid=0;

    if(pid==0)
        k=0;
    else
        k=1;
    sum=0;
    for(i=0; i<n;++i)
    {   
        buff[i]=i+k;
        sum+=i+k;
//        printf("%d - %d\n",i,buff[i]);
    }


    if(fd<0)
    {
        printf("Can not open file\n");
        return 0;
    }

//    int n_read=read(fd,buff,sizeof(buff)/4);
    
    printf("count of summing numbers %d\n",n);
    res=write(fd,buff,n*4);
    printf("result %d \n",res);
    if(res==sum)
        printf("calc OK\n");
    else
        printf("calc ERROR\n");
    printf("result: %u timer: %d transaction time: %d total time: %d\n",buff[0],buff[1],buff[2],buff[3]);

    close(fd);
    return 0;
}
