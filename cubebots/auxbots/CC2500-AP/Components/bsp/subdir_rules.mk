################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
Components/bsp/bsp.obj: ../Components/bsp/bsp.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.6.LTS/bin/cl430" --cmd_file="C:\Users\Jacob Miske\Documents\UROPandResearch\UROPcsail\cubebots\auxbots\Applications\configuration\smpl_nwk_config.dat" --cmd_file="C:\Users\Jacob Miske\Documents\UROPandResearch\UROPcsail\cubebots\auxbots\Applications\configuration\Access Point\smpl_config_AP.dat"  -vmsp --use_hw_mpy=none --include_path="C:/ti/ccsv7/ccs_base/msp430/include" --include_path="C:/Users/Jacob Miske/Documents/UROPandResearch/UROPcsail/cubebots/auxbots/Components/simpliciti/nwk" --include_path="C:/Users/Jacob Miske/Documents/UROPandResearch/UROPcsail/cubebots/auxbots/Components/simpliciti/nwk_applications" --include_path="C:/Users/Jacob Miske/Documents/UROPandResearch/UROPcsail/cubebots/auxbots/utils" --include_path="C:/Users/Jacob Miske/Documents/UROPandResearch/UROPcsail/cubebots/auxbots/Components/mrfi" --include_path="C:/Users/Jacob Miske/Documents/UROPandResearch/UROPcsail/cubebots/auxbots/Components/bsp/boards/EZ430RF" --include_path="C:/Users/Jacob Miske/Documents/UROPandResearch/UROPcsail/cubebots/auxbots/Components/bsp/drivers" --include_path="C:/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.6.LTS/include" --include_path="C:/Users/Jacob Miske/Documents/UROPandResearch/UROPcsail/cubebots/auxbots/Components/bsp" --advice:power=all --define=__MSP430F2274__ -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU19 --preproc_with_compile --preproc_dependency="Components/bsp/bsp.d_raw" --obj_directory="Components/bsp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


