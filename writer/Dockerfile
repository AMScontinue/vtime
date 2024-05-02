# 使用官方的 C++ 镜像作为基础镜像
FROM ubuntu:22.04

# 将当前目录下的所有文件复制到 Docker 容器的 /usr/src/writer 目录中
COPY . /usr/src/writer

# 设置工作目录为 /usr/src/writer
WORKDIR /usr/src/writer

# 安装gcc
RUN apt-get update && apt-get install -y gcc
# 编译你的 C++ 程序
RUN gcc -o writer writer.c

# 当容器启动时运行的命令
CMD ["./writer"]