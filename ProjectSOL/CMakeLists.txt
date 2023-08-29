cmake_minimum_required(VERSION 3.25)
project(ProjectSOL C)

set(CMAKE_C_STANDARD 11)

add_executable(masterworker utils.h master.c syncqueue.c syncqueue.h utils.c stringlist.c stringlist.h farm.h workerpool.c workerpool.h orderedrecordlist.h)
add_executable(collector collector.c sckset.c sckset.h orderedrecordlist.c orderedrecordlist.h farm.h utils.c utils.h)
