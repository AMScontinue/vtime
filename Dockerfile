# 使用官方的 C++ 镜像作为基础镜像
FROM ubuntu:22.04

# 将当前目录下的所有文件复制到 Docker 容器的 /usr/src/hooker 目录中
COPY . /usr/src/hooker

# 设置工作目录为 /usr/src/hooker
WORKDIR /usr/src/hooker

# 安装gcc
RUN apt-get update && apt-get install -y gcc
# 安装make
RUN apt-get update && apt-get install -y make
# 解决libopcodes-2.38-system.so: No such file or directory，需要找到现有的镜像里面对应/usr/lib/x86_64-linux-gnu/libopcodes-xxx-system.so是什么
# RUN ln -s /usr/lib/x86_64-linux-gnu/libopcodes-2.40-system.so /usr/lib/x86_64-linux-gnu/libopcodes-2.38-system.so 
# 编译你的 C++ 程序
RUN make -C apps/basic
RUN make

# 设置环境变量
ENV LIBZPHOOK=./apps/basic/libzphook_basic.so
ENV LD_PRELOAD=./libzpoline.so

# 当容器启动时运行的命令
CMD ["./main/main"]