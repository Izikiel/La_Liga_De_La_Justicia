################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/diccionario.c \
../source/diccionario_buddy.c \
../source/diccionario_dinamicas.c \
../source/justice_engine.c 

OBJS += \
./source/diccionario.o \
./source/diccionario_buddy.o \
./source/diccionario_dinamicas.o \
./source/justice_engine.o 

C_DEPS += \
./source/diccionario.d \
./source/diccionario_buddy.d \
./source/diccionario_dinamicas.d \
./source/justice_engine.d 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"../../memcached-1.6/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


