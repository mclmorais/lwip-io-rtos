################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
third_party/lwip-1.4.1/apps/httpserver_raw/httpd.obj: C:/ti/tivaware_c_series_2_1_4_178/third_party/lwip-1.4.1/apps/httpserver_raw/httpd.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/codecomposer/ccsv8/tools/compiler/ti-cgt-arm_18.1.3.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O2 --include_path="C:/codecomposer/ccsv8/tools/compiler/ti-cgt-arm_18.1.3.LTS/include" --include_path="C:/lwipio/enet_io/FreeRTOS/portable/CCS/ARM_CM4F" --include_path="C:/lwipio/enet_io/FreeRTOS/include" --include_path="C:/lwipio/enet_io/FreeRTOS/portable/CCS/ARM_CM4F" --include_path="C:/lwipio/enet_io" --include_path="C:/ti/tivaware_c_series_2_1_4_178/examples/boards/ek-tm4c1294xl" --include_path="C:/ti/tivaware_c_series_2_1_4_178" --include_path="C:/ti/tivaware_c_series_2_1_4_178/third_party/lwip-1.4.1/src/include" --include_path="C:/ti/tivaware_c_series_2_1_4_178/third_party/lwip-1.4.1/src/include/ipv4" --include_path="C:/ti/tivaware_c_series_2_1_4_178/third_party/lwip-1.4.1/apps" --include_path="C:/ti/tivaware_c_series_2_1_4_178/third_party/lwip-1.4.1/ports/tiva-tm4c129/include" --include_path="C:/ti/tivaware_c_series_2_1_4_178/third_party" --advice:power=all -g --gcc --define=ccs="ccs" --define=PART_TM4C1294NCPDT --define=TARGET_IS_TM4C129_RA0 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --ual --preproc_with_compile --preproc_dependency="third_party/lwip-1.4.1/apps/httpserver_raw/$(basename $(<F)).d_raw" --obj_directory="third_party/lwip-1.4.1/apps/httpserver_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


