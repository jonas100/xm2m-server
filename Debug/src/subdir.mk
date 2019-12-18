################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/clientsession-cmdline.cpp \
../src/clientsession.cpp \
../src/reportwriter.cpp \
../src/resultsrepo.cpp \
../src/xm2m-server.cpp 

OBJS += \
./src/clientsession-cmdline.o \
./src/clientsession.o \
./src/reportwriter.o \
./src/resultsrepo.o \
./src/xm2m-server.o 

CPP_DEPS += \
./src/clientsession-cmdline.d \
./src/clientsession.d \
./src/reportwriter.d \
./src/resultsrepo.d \
./src/xm2m-server.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


