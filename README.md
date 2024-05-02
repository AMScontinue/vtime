# 本项目zpoline部分说明（wyc）

## 文件结构
- apps/basic：    hook函数所在地
- main：          用户程序，主要用来模拟调用time()等syscall
- writer：        一个用来写入共享内存的调试工具，与zpoline等其他文件无关，是个独立的程序
- Dockerfile：    zpoline这部分的Dockerfile
- main.c：        zpoline功能的实现，下下来之后就约等于没动过

## 单机上直接运行的方法
- 先运行writer，writer中开辟共享内存，并且向其中写入数据。注意writer写完数据会休止100秒，100秒后会将共享内存段删除，届时hooker还是会找不到共享内存
```
cd ./writer
gcc -o writer writer.c
./writer
cd ..
```
- 再运行hooker，  
首先
```
sudo sh -c "echo 0 > /proc/sys/vm/mmap_min_addr"
```
然后生成libzphook_basic.so
```
make -C apps/basic
```
然后生成libzpoline.so
```
make
```
然后编译用户程序
```
cd ./main
gcc -o main main.c
cd ..
```
最后设置环境变量并运行
```
LIBZPHOOK=./apps/basic/libzphook_basic.so LD_PRELOAD=./libzpoline.so ./main/main
```

## 打包成镜像运行的方法
首先打包writer，Dockerfile所在路径是```./writer/Dockerfile```。目前想使用的是ubuntu:22.04，但是还没实际跑过，之前用gcc镜像的时候没有出现过问题。命令是  
（下面只是一个示例，harbor仓库中v2应该是现在可用的版本，v1是上一版，数据格式与现在不同） 
```
docker run harbor.sail.se.sjtu.edu.cn/kubevtime/writer:v2
``` 
然后打包hooker，Dockerfile所在路径是```./Dockerfile```。目前想要运行的命令是
``` 
docker run --privileged -it --ipc=host harbor.sail.se.sjtu.edu.cn/kubevtime/hooker:[tag]
``` 

## 现在的进展
0502碰到问题之后，目前准备尝试三点
- ```FROM ubuntu:22.04```
- ```--privileged```
- ```--ipc=host```（这一点之前一直有在用，不用宿主机的共享内存的话writer和hooker是没法连上的。deployment.yaml可以设置```hostIPC: true```也没问题。所以问题应该不出在这里）  
  

但是此时用了```FROM ubuntu:22.04```之后，运行容器的时候有报错
```
error while loading shared libraries: libopcodes-2.38-system.so: cannot open shared object file: No such file or directory
```
这个报错之前也见过，主要是不同版本的linux里该文件位置或者版本名称不同。此前的容器可以在Dockerfile中用如下命令解决
```
RUN ln -s /usr/lib/x86_64-linux-gnu/libopcodes-2.40-system.so /usr/lib/x86_64-linux-gnu/libopcodes-2.38-system.so 
```
但是现在ubuntu:22.04中，我进容器看了下，```/usr/lib/x86_64-linux-gnu/libopcodes-2.38-system.so```这个文件确实在它默认的位置，照理来说应该是没有问题能找得到，但是还是有上面的报错。所以目前使用```FROM ubuntu:22.04```之后容器里zpoline没有正常跑起来过。大概目前是这样

