//
// Created by PC on 2019/9/24.
//

#ifndef INC_3_3_2_PUSHER_SAFE_QUEUE_H
#define INC_3_3_2_PUSHER_SAFE_QUEUE_H

#include <queue>

using namespace std;
template<typename T>
class SafeQueue{
typedef void (*ReleaseCallback)(T *);
typedef void (*SyncHandle)(queue<T> &);

public:
    SafeQueue(){
        pthread_mutex_init(&mutex,0); //动态初始化
        pthread_cond_init(&cond,0);
    }
    ~SafeQueue(){
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }
    /**
     * 入队
     * @param value
     */
    void push(T value){
        pthread_mutex_lock(&mutex);
        if(work){
            //工作状态需要push
            q.push(value);
            pthread_cond_signal(&cond);
        }else{
            //非工作状态
            if(releaseCallback){
                releaseCallback(&value);
            }
        }
        pthread_mutex_unlock(&mutex);
    }
    /**
     * 出队
     * @param value
     * @return
     */
    int pop(T &value){
        int ret = 0;
        pthread_mutex_lock(&mutex);
        while(work && q.empty()){
            //工作状态，说明确实需要pop，但是队列为空，需要等待
            pthread_cond_wait(&cond,&mutex);
        }
        if(!q.empty()){
            value= q.front();
            q.pop();
            ret = 1;
        }
        pthread_mutex_unlock(&mutex);
        return ret;
    }
    /**
     * 设置队列的工作状态
     * @param work
     */
    void setWork(int work){
        pthread_mutex_lock(&mutex);
        this->work = work;
        pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mutex);
    }
    /**
     * 判断队列是否为空
     * @return
     */
    int empty(){
        return q.empty();
    }

    /**
     * 获取队列大小
     * @return
     */
    int size(){
        return q.size();
    }

    /**
     * 清空队列
     */
    void clear(){
        //先锁起来
        pthread_mutex_lock(&mutex);
        uint size = q.size();
        for (int i = 0; i < size; ++i) {
            //取出队首元素
            T value = q.front();
            if(releaseCallback){
                releaseCallback(&value);
            }
            q.pop();
        }
        //解锁
        pthread_mutex_unlock(&mutex);
    }

    void setReleaseCallback(ReleaseCallback releaseCallback){
        this->releaseCallback = releaseCallback;
    }

    void setSyncHandle(SyncHandle syncHandle){
        this->syncHandle = syncHandle;
    }

    void sync(){
        pthread_mutex_lock(&mutex);
        syncHandle(q);
        pthread_mutex_unlock(&mutex);
    }
private:
    queue<T> q;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int work;//标记队列是否工作
    ReleaseCallback releaseCallback;
    SyncHandle syncHandle;
};

#endif //INC_3_3_2_PUSHER_SAFE_QUEUE_H