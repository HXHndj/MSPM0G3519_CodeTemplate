# 电赛-软件马拉松项目
---
在这个项目中，我们需要利用单片机资源和板上外设，实现一个建议的信号测量装置
我们能够利用到的资源有：
1. Timer定时器
2. ADC模数转换
3. DMA直接内存访问

整体程序设计的思路是：ADC通过一定条件，连续采集一定的信号值，然后通过算法计算出波形、峰峰值、有效值、信号类型等

为此设计了如下的逻辑结构：
1. Timer每10us发布一个事件，然后ADC订阅这个事件，触发ADC采集，以此连续采集2048个数据
2. ADC每采集一个数据，就触发DMA进行数据转运，直到采集完2048个数据
3. 最后全部数据采集完成，进行算法运算，并输出结果
---
为了更好的理解ADC和DMA的运作原理，下进行介绍：
## ADC转运
> 对于ADC来说，他一共有四种转运模式，分别为：单通道单次转运，单通道连续转运，多通道单次转运，多通道连续转运
> 这四个模式，其实就是SysConfig里面的两个选项的组合，一个是*Sampling Mode Configuration*中的*Conversion Mode*，另一个则是*Enable Repeat Mode*

ADC的转运需要两个信号，一个是START信号，一个是ENC信号。在后续每个ADC通道的*ADC Conversion Memory 0 Configuration*里的*Optional Configuration*中：
+ Trigger will ... conversion register ，这个会将START信号“传递”给下个通道，如果没有下个通道，则指向自己
+ Valid trigger will ... conversion register ， 这个会指向下一个通道，如果没有下个通道，则指向自己，但不会传递START

  这个配置和前面提到过的两个设置组合，可以提供多种的触发模式。在此仅讲解各设置的原理：
1. 对于单通道单次转运模式，在系统初始化后，ADC的ENC自动配置为1，如果此时有一个START信号（调用startConevrsion或者事件触发），就会触发一次转运
   如果此时是*Trigger will ... conversion register*，则会自己给自己一个START信号，如果此时勾选了*Enable Repeat Mode*，ENC不会被置0，则会继续触发ADC转运，达成了一次触发，一直转运的效果。

   如果此时*Valid trigger will ... conversion register*，则会自己指向自己，但不会传递START信号。此时ENC仍为1，等待下一次触发信号。达到了一次触发，一次转运。这样的模式配合Timer可以让数据采集的时序完全听取Timer。

2. 对于多通道来说，他们同样也可以有上述过程，并且每一个通道都可以单独设置START信号的传递模式。如前3位为*Trigger will ... conversion register*，最后一个为*Valid trigger will ... conversion register*，然后开启*Enable Repeat Mode*。
   这样就可以一次触发，四通道依次采集，然后等待下一次触发。START信号由函数或者事件控制，而ENC始终为1.同样也可以一次触发，一直转运，只需要全部设为*Trigger will ... conversion register*即可。

DMA的配置相对简单，可以参考视频**BV1JT421k7Bb**，单通道可以直接利用DMA转运，也可以开启FIFO；多通道必须开启FIFO，具体看视频。

同时，为了测量小信号，我们还可以配置ADC的硬件均值模式，这样可以减轻外界噪声干扰。在这款MSOM0G3519上没有OPA，故只能用这种简陋的方法。
