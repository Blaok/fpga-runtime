additional_commands = {
    "add_xocc_compile_target": {
        "pargs": 1,
        "flags": ["SAVE_TEMPS",],
        "kwargs": {
            "TARGET": 1,
            "OUTPUT": 1,
            "REPORT_DIR": 1,
            "LOG_DIR": 1,
            "TEMP_DIR": 1,
            "INPUT": 1,
        }
    },
    "add_xocc_link_target": {
        "pargs": 1,
        "flags": ["SAVE_TEMPS",],
        "kwargs": {
            "TARGET": 1,
            "OUTPUT": 1,
            "OPTIMIZE": 1,
            "REPORT_DIR": 1,
            "LOG_DIR": 1,
            "TEMP_DIR": 1,
            "INPUT": 1,
        }
    },
    "add_xocc_targets": {
        "pargs": 1,
        "flags": [],
        "kwargs": {
            "KERNEL": 1,
            "PLATFORM": 1,
            "INPUT": 1,
            "PREFIX": 1,
            "HLS_SRC": 1,
            "SW_EMU_XO": 1,
            "HW_XO": 1,
            "SW_EMU_XCLBIN": 1,
            "HW_EMU_XCLBIN": 1,
            "HW_XCLBIN": 1,
            "DRAM_MAPPING": "*",
        }
    },
    "add_xocc_targets_with_alias": {
        "pargs": 1,
        "flags": [],
        "kwargs": {
            "KERNEL": 1,
            "PLATFORM": 1,
            "INPUT": 1,
            "PREFIX": 1,
            "HLS_SRC": 1,
            "SW_EMU_XO": 1,
            "HW_XO": 1,
            "SW_EMU_XCLBIN": 1,
            "HW_EMU_XCLBIN": 1,
            "HW_XCLBIN": 1,
            "DRAM_MAPPING": "*",
        }
    },
}
