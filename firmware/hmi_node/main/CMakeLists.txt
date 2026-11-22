idf_component_register(
    SRCS 
        "waveshare_rgb_lcd_port.c"
        "lvgl_port.c"
        "main.c"
        "espnow_rx.c"
        "ui_dashboard.c"
        "ui_fault.c"
        "ui_config.c"
        "ui_pumpiq.c"
        "ui_cafm.c"
    INCLUDE_DIRS ".")

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
target_compile_options(__idf_main PRIVATE -Wno-unused-variable)