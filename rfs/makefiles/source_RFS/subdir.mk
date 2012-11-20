################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source_RFS/ex2_block_types.c \
../source_RFS/file_operations.c \
../source_RFS/manage_fs.c \
../source_RFS/procesador_de_pedidos.c \
../source_RFS/server_threading.c 

OBJS += \
./source_RFS/ex2_block_types.o \
./source_RFS/file_operations.o \
./source_RFS/manage_fs.o \
./source_RFS/procesador_de_pedidos.o \
./source_RFS/server_threading.o 

C_DEPS += \
./source_RFS/ex2_block_types.d \
./source_RFS/file_operations.d \
./source_RFS/manage_fs.d \
./source_RFS/procesador_de_pedidos.d \
./source_RFS/server_threading.d 


# Each subdirectory must supply rules for building sources it contributes
source_RFS/%.o: ../source_RFS/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


