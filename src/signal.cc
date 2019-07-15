#include <unistd.h>
#include <assert.h>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>

#include <map>
#include <functional>

class Signal {
    using SignalCallback = std::function<void()>;
    using SignalMap = std::map<int, SignalCallback>;
  public:
    Signal() {
        sigemptyset(&mask_);
        sfd_ = ::signalfd(-1, &mask_, SFD_NONBLOCK|SFD_CLOEXEC);
    }

    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    void signal(int signo, const SignalCallback& callback)
    { signal(signo, SignalCallback(callback)); }

    void signal(int signo, SignalCallback&& callback);

    int signalfd() const noexcept { return sfd_; }

    void handleRead();
  private:
    sigset_t mask_;
    int sfd_; 
    SignalMap signalMap_;
};

void Signal::signal(int signo, SignalCallback&& callback) {
    sigaddset(&mask_, signo); 
    sigprocmask(SIG_BLOCK, &mask_, NULL); // sigprocmask是必要的
    signalMap_[signo] = std::move(callback);
    ::signalfd(sfd_, &mask_, SFD_NONBLOCK|SFD_CLOEXEC);
}

void Signal::handleRead() {
    struct signalfd_siginfo sigInfo;
    int r = ::read(sfd_, &sigInfo, sizeof(sigInfo));
    assert(r==sizeof(sigInfo));

    auto iter = signalMap_.find(sigInfo.ssi_signo);
    if (iter != signalMap_.end()) {
       (iter->second)(); 
    }
}


class SignalPoller {
    using SignalCallback = std::function<void()>;
  public:
    SignalPoller()
      : signal_() {
        pfd_.fd = signal_.signalfd();
        pfd_.events = POLLIN;
    }
    SignalPoller(const SignalPoller&) = delete;
    SignalPoller& operator=(const SignalPoller&) = delete;

    void poll();

    void signal(int signo, const SignalCallback& callback)
    { signal(signo, SignalCallback(callback)); }

    void signal(int signo, SignalCallback&& callback);

  private:
    Signal signal_;
    struct pollfd pfd_;
};

void SignalPoller::signal(int signo, SignalCallback&& callback) {
   signal_.signal(signo, std::move(callback)); 
}

void SignalPoller::poll() {
    int r = ::poll(&pfd_, 1, -1);
    assert(r==1);
    if (pfd_.revents & POLLIN)
        signal_.handleRead();
}


int main() {
    SignalPoller signalPoller;
    signalPoller.signal(SIGUSR1, 
                        [] {
                            printf("Receive SIGUSR1\n"); 
                        });
    signalPoller.poll();
    signalPoller.signal(SIGUSR2, 
                        [] {
                            printf("Receive SIGUSR2\n");
                        });

    signalPoller.poll();
}
