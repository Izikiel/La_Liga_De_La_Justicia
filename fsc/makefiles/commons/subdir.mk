################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../commons/bitarray.c \
../commons/config.c \
../commons/error.c \
../commons/log.c \
../commons/sockete.c \
../commons/string.c \
../commons/temporal.c 

OBJS += \
./commons/bitarray.o \
./commons/config.o \
./commons/error.o \
./commons/log.o \
./commons/sockete.o \
./commons/string.o \
./commons/temporal.o 

C_DEPS += \
./commons/bitarray.d \
./commons/config.d \
./commons/error.d \
./commons/log.d \
./commons/sockete.d \
./commons/string.d \
./commons/temporal.d 


# Each subdirectory must supply rules for building sources it contributes
commons/%.o: ../commons/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


