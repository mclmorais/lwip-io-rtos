################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := C:/codecomposer/ccsv8/tools/compiler/ti-cgt-arm_18.1.3.LTS

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./cgifuncs.obj" \
"./enet_io.obj" \
"./io.obj" \
"./io_fs.obj" \
"./startup_ccs.obj" \
"./FreeRTOS/croutine.obj" \
"./FreeRTOS/event_groups.obj" \
"./FreeRTOS/list.obj" \
"./FreeRTOS/queue.obj" \
"./FreeRTOS/tasks.obj" \
"./FreeRTOS/timers.obj" \
"./FreeRTOS/portable/CCS/ARM_CM4F/port.obj" \
"./FreeRTOS/portable/CCS/ARM_CM4F/portasm.obj" \
"./FreeRTOS/portable/MemMang/heap_2.obj" \
"./drivers/pinout.obj" \
"./third_party/lwip-1.4.1/apps/httpserver_raw/httpd.obj" \
"./utils/locator.obj" \
"./utils/lwiplib.obj" \
"./utils/uartstdio.obj" \
"./utils/ustdlib.obj" \
"../enet_io_ccs.cmd" \
$(GEN_CMDS__FLAG) \
-llibc.a \
-l"C:/ti/tivaware_c_series_2_1_4_178/driverlib/ccs/Debug/driverlib.lib" \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include FreeRTOS/subdir_vars.mk
-include FreeRTOS/portable/CCS/ARM_CM4F/subdir_vars.mk
-include FreeRTOS/portable/MemMang/subdir_vars.mk
-include drivers/subdir_vars.mk
-include third_party/lwip-1.4.1/apps/httpserver_raw/subdir_vars.mk
-include utils/subdir_vars.mk
-include subdir_rules.mk
-include FreeRTOS/subdir_rules.mk
-include FreeRTOS/portable/CCS/ARM_CM4F/subdir_rules.mk
-include FreeRTOS/portable/MemMang/subdir_rules.mk
-include drivers/subdir_rules.mk
-include third_party/lwip-1.4.1/apps/httpserver_raw/subdir_rules.mk
-include utils/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
enet_io.out \

EXE_OUTPUTS__QUOTED += \
"enet_io.out" \

BIN_OUTPUTS += \
enet_io.bin \

BIN_OUTPUTS__QUOTED += \
"enet_io.bin" \


# All Target
all: enet_io.out

# Tool invocations
enet_io.out: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@echo 'Building target: "$@"'
	@echo 'Invoking: ARM Linker'
	"C:/codecomposer/ccsv8/tools/compiler/ti-cgt-arm_18.1.3.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me -O2 --advice:power=all --define=ccs="ccs" --define=PART_TM4C1294NCPDT --define=TARGET_IS_TM4C129_RA0 -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --ual -z -m"enet_io_ccs.map" --heap_size=0 --stack_size=1024 -i"C:/codecomposer/ccsv8/tools/compiler/ti-cgt-arm_18.1.3.LTS/lib" -i"C:/codecomposer/ccsv8/tools/compiler/ti-cgt-arm_18.1.3.LTS/include" --reread_libs --diag_wrap=off --display_error_number --warn_sections --xml_link_info="enet_io_linkInfo.xml" --rom_model -o "enet_io.out" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

enet_io.bin: $(EXE_OUTPUTS)
	@echo 'Building files: $(strip $(EXE_OUTPUTS__QUOTED))'
	@echo 'Invoking: ARM Hex Utility'
	"C:/codecomposer/ccsv8/tools/compiler/ti-cgt-arm_18.1.3.LTS/bin/armhex"  -o "enet_io.bin" $(EXE_OUTPUTS__QUOTED)
	@echo 'Finished building: $(strip $(EXE_OUTPUTS__QUOTED))'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(BIN_OUTPUTS__QUOTED)$(EXE_OUTPUTS__QUOTED)
	-$(RM) "cgifuncs.obj" "enet_io.obj" "io.obj" "io_fs.obj" "startup_ccs.obj" "FreeRTOS\croutine.obj" "FreeRTOS\event_groups.obj" "FreeRTOS\list.obj" "FreeRTOS\queue.obj" "FreeRTOS\tasks.obj" "FreeRTOS\timers.obj" "FreeRTOS\portable\CCS\ARM_CM4F\port.obj" "FreeRTOS\portable\CCS\ARM_CM4F\portasm.obj" "FreeRTOS\portable\MemMang\heap_2.obj" "drivers\pinout.obj" "third_party\lwip-1.4.1\apps\httpserver_raw\httpd.obj" "utils\locator.obj" "utils\lwiplib.obj" "utils\uartstdio.obj" "utils\ustdlib.obj" 
	-$(RM) "cgifuncs.d" "enet_io.d" "io.d" "io_fs.d" "startup_ccs.d" "FreeRTOS\croutine.d" "FreeRTOS\event_groups.d" "FreeRTOS\list.d" "FreeRTOS\queue.d" "FreeRTOS\tasks.d" "FreeRTOS\timers.d" "FreeRTOS\portable\CCS\ARM_CM4F\port.d" "FreeRTOS\portable\MemMang\heap_2.d" "drivers\pinout.d" "third_party\lwip-1.4.1\apps\httpserver_raw\httpd.d" "utils\locator.d" "utils\lwiplib.d" "utils\uartstdio.d" "utils\ustdlib.d" 
	-$(RM) "FreeRTOS\portable\CCS\ARM_CM4F\portasm.d" 
	-@echo 'Finished clean'
	-@echo ' '

post-build:
	-"C:/codecomposer/ccsv8/utils/tiobj2bin/tiobj2bin" "enet_io.out" "enet_io.bin" "C:/codecomposer/ccsv8/tools/compiler/ti-cgt-arm_18.1.3.LTS/bin/armofd" "C:/codecomposer/ccsv8/tools/compiler/ti-cgt-arm_18.1.3.LTS/bin/armhex" "C:/codecomposer/ccsv8/utils/tiobj2bin/mkhex4bin"
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

