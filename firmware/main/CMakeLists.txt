idf_component_register(
    SRCS "games_main.c" "tetris_model.c" "word_game.c" "word_bank.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_hw_support esp_timer nvs_flash
)
if("$ENV{PASSPORT_DEVICE_SELF_TEST}" STREQUAL "1")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE PASSPORT_DEVICE_SELF_TEST=1)
endif()
