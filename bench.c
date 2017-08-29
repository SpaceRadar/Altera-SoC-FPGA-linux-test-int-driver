#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/ioctl.h>

int main(int argc, char **argv)
{
    int buff[1024];
    int fd=open("/dev/test_int_43",O_RDWR);
    int i;
    int n;
    unsigned int res;
    if(fd<0)
    {
        printf("Can not open file\n");
        return 0;
    }

//    int n_read=read(fd,buff,sizeof(buff)/4);
    if(argc==2)
        n=atoi(argv[1]);
    else
        n=2;
    for(i=0; i<n;++i)
    {   
        buff[i]=i;
//        printf("%d - %d\n",i,buff[i]);
    }
    
    printf("count of summing numbers %d\n",n);
    res=write(fd,buff,n*4);
    printf("result %d\n",res);



    close(fd);
    return 0;
}
