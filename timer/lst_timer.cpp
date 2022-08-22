#include "lst_timer.h"
#include "../http/http_conn.h"

sort_timer_lst::sort_timer_lst()
{
    head = NULL;
    tail = NULL;
}
sort_timer_lst::~sort_timer_lst()
{
    util_timer *tmp = head;
    while (tmp)
    {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if (!head)
    {
        head = tail = timer;
        return;
    }
    //if (timer->expire > tail->expire)
    //{
      //  timer->next = tail;
       // tail->prev = timer;
       // tail = timer;
       // return;
    //}
    tail->next = timer;
    timer->prev = tail;
    timer->next = NULL;
    tail = timer;
    //add_timer(timer, tail);
}
void sort_timer_lst::adjust_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    util_timer *tmp = timer->next;
    if (!tmp || (timer->expire < tmp->expire))
    {
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        timer->next = NULL;
        add_timer(timer);
	// add_timer(timer, tail);
    }
    else
    {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer);
    }
}
void sort_timer_lst::del_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if ((timer == head) && (timer == tail))
    {
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }
    if (timer == tail)
    {
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}
void sort_timer_lst::tick()
{
    if (!head)
    {
        return;
    }
    
    time_t cur = time(NULL);
    util_timer *tmp = head;
    while (tmp)
    {
        if (cur < tmp->expire)
        {
            break;
        }
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if (head)
        {
            head->prev = NULL;
        }
        delete tmp;
        tmp = head;
		
		
		
		

		 
    }
}

void sort_timer_lst::add_timer(util_timer *timer, util_timer *lst_tail)
{
    //util_timer *next = lst_tail;
    //util_timer *tmp = next->prev;
    //while (tmp)
    //{
      //  if (timer->expire > tmp->expire)
       // {
         //   tmp->next = timer;
           // timer->prev = tmp;
           // next->prev = timer;
           // timer->next = next;
           // break;
        //}
        //next = tmp;
        //tmp = tmp->prev;
    //}
    //if (!tmp)
    //{
      //  next->prev = timer;
       // timer->prev = NULL;
       // timer->next = next;
       // head = timer;
    //}
    if(timer->expire>=lst_tail->expire){
    	lst_tail->next = timer;
	timer->prev = lst_tail;
	timer->next = NULL;
	tail = timer;
	return;
    }
    util_timer* nxt = lst_tail;
    util_timer* tmp = nxt->prev;
    while(tmp){
    	if(timer->expire>=tmp->expire){
		tmp->next = timer;
		timer->prev = tmp;
		nxt->prev = timer;
		timer->next = nxt;
		break;
	}
	nxt = tmp;
	tmp = tmp->prev;
    }

}

void Utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_timer_lst.tick();
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

void Utils::append(util_timer*timer){
	m_timerlock.lock();
	my_del_timer.push_back(timer);
	m_timerlock.unlock();
	
}

void Utils::deal_timer()
{
	while(1){
		m_timerlock.lock();
		if(my_del_timer.empty()){
			m_timerlock.unlock();
			break;
		}
		util_timer* temp = my_del_timer.front();
		my_del_timer.pop_front();
		m_timerlock.unlock();
		if(temp)
		m_timer_lst.del_timer(temp);
	}
    //timer->cb_func(timer->user_data);
    //if(timer)
    //{
    //	m_timer_lst.del_timer(timer);
    //}
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

//class Utils;
void cb_func(client_data *user_data)
{
    //LOG_ERROR("ERROR1");
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    //LOG_ERROR("ERROR2");
    assert(user_data);
    //LOG_ERROR("ERROR3");
    close(user_data->sockfd);
    //LOG_ERROR("ERROR4");
    http_conn::m_user_count--;
    //LOG_ERROR("DEL TIMER,COUNT:%d",http_conn::m_user_count);
}
