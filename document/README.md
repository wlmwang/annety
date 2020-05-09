## 性能
	Annety 的跨平台性参考了 chromium；网络模型参考了 muduo；校验和算法参考了 nginx；以及还有他们之
	间的混合体，比如：Thread/LOG/CHECK 参考了 chromium 与 muduo、定时器参考了 nginx 与 muduo、信号
	处理器参考了 leveldb 与 muduo，等等。由于 annety 核心的网络模型参考了 muduo(one loop per thread)，
	所以库的性能是遵循陈硕老师在 blog 上发布的测试结果（我自己在云服务器测试吞吐结果：比 boost1.40 的
	asio-1.4.3 高、比 libevent-2.0.6-rc 高，与纯压测工具 netperf-2.5.0 相当）：
* [muduo 与 boost asio 吞吐量对比](https://blog.csdn.net/Solstice/article/details/5863411)
* [muduo 与 libevent2 吞吐量对比](https://blog.csdn.net/Solstice/article/details/5864889)
	
## 关于
	Annety 是一个基于 C++11 的 Reactor 模式的多线程网络库。它使用了许多从开源项目修改而来的代码，其
	中包括有：chromium, muduo, nginx, leveldb等。
	创建 annety 网络库初衷主要是为了给自己写一个趁手的底层工具。由于是基础库，对该库的要求也较为
	严格，包括：高性能、高可用、易使用、易阅读。是的，在这个项目起初，我就给了一个硬性的要求：引
	用一些优秀的开源代码！说实话，为了这个要求，项目进展非常慢！相比读懂这些源码，意会其设计意图
	更为困难！历时近两年，阅读了大量的源码和相关书籍，终于完成该库的编写。这一切都是值得的！使
	annety 具有优秀的网络模式，恰到好处的线程模型，以及上乘的代码质量！
	感谢开源，感谢他们！
