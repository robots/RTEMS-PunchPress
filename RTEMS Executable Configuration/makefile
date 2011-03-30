################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: PunchPress.exe

# Tool invocations
PunchPress.exe: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: RTEMS Linker'
	/afs/ms.mff.cuni.cz/u/b/bures/BIG/ers-labs/rtems-4.10.0/bin/i386-rtems4.10-gcc -B/afs/ms.mff.cuni.cz/u/b/bures/BIG/ers-labs/rtems-4.10.0/i386-rtems4.10/pc386/lib/ -specs bsp_specs -qrtems -mtune=i386 -Wl,-Ttext,0x00100000 -o"PunchPress.exe" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(ASM_DEPS)$(C_DEPS)$(EXECUTABLES)$(S_UPPER_DEPS) PunchPress.exe
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
