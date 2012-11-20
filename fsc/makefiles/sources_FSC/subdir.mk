################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../sources_FSC/manejador_de_socketes.c \
../sources_FSC/manejador_memcached.c \
../sources_FSC/operaciones_fuse.c \
../sources_FSC/serializadores.c 

OBJS += \
./sources_FSC/manejador_de_socketes.o \
./sources_FSC/manejador_memcached.o \
./sources_FSC/operaciones_fuse.o \
./sources_FSC/serializadores.o 

C_DEPS += \
./sources_FSC/manejador_de_socketes.d \
./sources_FSC/manejador_memcached.d \
./sources_FSC/operaciones_fuse.d \
./sources_FSC/serializadores.d 


# Each subdirectory must supply rules for building sources it contributes
sources_FSC/%.o: ../sources_FSC/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


