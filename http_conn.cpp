#include"http_conn.h"

//静态变量的初始化
int http_conn::m_epollfd=-1;
int http_conn::m_user_count=0;

 //设置文件描述符非阻塞
void setnonblocking(int fd){
    int old_flag=fcntl(fd,F_GETFL);
    int new_flag=old_flag|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_flag);
}


//向epoll中添加需要监听的文件描述符
void addfd(int epollfd,int fd,bool one_shot){
   epoll_event event;
   event.data.fd=fd;
   //event.events=EPOLLIN|EPOLLRDHUP;
   event.events=EPOLLIN|EPOLLET|EPOLLRDHUP;
   if(one_shot){
     // 防止同一个通信被不同的线程处理
     event.events|=EPOLLONESHOT;
   }
   epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
   //设置文件描述符非阻塞
   setnonblocking(fd);
}

//从epoll中移除需要监听的文件描述符
void removefd(int epollfd,int fd){
   epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
   close(fd);
}

//修改文件描述符,重置socket上EPOLLONESHOT事件，以确保下一次可读时，EPOLLIN事件能被触发
void modfd(int epollfd,int fd,int ev){
    epoll_event event;
    event.data.fd=fd;
    event.events=ev|EPOLLET|EPOLLONESHOT|EPOLLRDHUP;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);
}

//初始化新接收的连接
void http_conn::init(int sockfd,const sockaddr_in &addr){
    m_sockfd=sockfd;
    m_address=addr;

    //端口复用
    int reuse=1;
    setsockopt(m_sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    
    //添加到epoll对象中
    addfd(m_epollfd,sockfd,true);
    m_user_count++;//总用户数+1
}

//关闭连接
void  http_conn::close_conn(){
    if(m_sockfd!=-1){
        removefd(m_epollfd,m_sockfd);
        m_sockfd=-1;
        m_user_count--;//关闭一个连接，总用户数-1
    }   
}

//非阻塞的读
//循环读取客户数据，直到无数据刻度或者对方关闭连接
bool http_conn::read(){
  if(m_read_index>=READ_BUFFER_SIZE){
    return false;
  }
  //读取到的字节
  int bytes_read=0;
  while(true){
    bytes_read=recv(m_sockfd,m_read_buf+m_read_index, READ_BUFFER_SIZE-m_read_index,0);
    if(bytes_read==-1){
      if(errno==EAGAIN||errno==EWOULDBLOCK){
        //没有数据
        break;
      }
      return false;
    }else if(bytes_read==0){
      //对方关闭连接
      return false;
    }
    m_read_index+=bytes_read;
  }
  printf("读取到了数据：%s\n",m_read_buf);
  return true;
}

//主状态机,解析请求
http_conn::HTTP_CODE http_conn::process_read(){
    return NO_REQUEST;
}

//解析请求首行
http_conn::HTTP_CODE http_conn::parse_request_line(char *text){
   return NO_REQUEST;
}


//解析请求头
http_conn::HTTP_CODE http_conn::parse_headers(char *text){
   return NO_REQUEST;
}


//解析请求体
http_conn::HTTP_CODE http_conn::parse_content(char *text){
  return NO_REQUEST;
}

http_conn::LINE_STATUS http_conn::parse_line(){
   return LINE_OK;
}


//非阻塞的写
bool http_conn::write(){
  printf("一次性写完数据\n");
  return true;
}

//由线程池中的工作线程调用，这是处理HTTP请求的入口函数
void http_conn::process(){

  //解析HTTP请求
  HTTP_CODE read_ret=process_read();
  if(read_ret==NO_REQUEST){
    modfd(m_epollfd,m_sockfd,EPOLLIN);
    return;
  }
  //生成响应
}