idf_component_register(SRCS "passport_proxy.c" "proxy_protocol.c"
    INCLUDE_DIRS "include" PRIV_REQUIRES esp-tls esp_timer nvs_flash lwip)
if(NOT IDF_VERSION_MAJOR EQUAL 6 OR NOT IDF_VERSION_MINOR EQUAL 0 OR NOT IDF_VERSION_PATCH EQUAL 1)
    message(FATAL_ERROR "passport_proxy requires the pinned ESP-IDF 6.0.1 TLS layout")
endif()
# Pinned to ESP-IDF 6.0.1: attach a CONNECT socket before the original TLS
# state machine creates mbedTLS with the original SNI and certificate policy.
target_include_directories(${COMPONENT_LIB} PRIVATE "$ENV{IDF_PATH}/components/esp-tls/private_include")
target_link_options(${COMPONENT_LIB} INTERFACE
    "-Wl,--wrap=esp_tls_conn_new_sync" "-Wl,--wrap=esp_tls_conn_new_async")
