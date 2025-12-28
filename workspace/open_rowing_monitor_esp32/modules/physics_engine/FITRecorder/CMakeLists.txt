zephyr_library_include_directories(.)
zephyr_library_sources(FITRecorder.cpp)

zephyr_library_link_libraries(
  modules/garmin_fit/fit_profile
  modules/garmin_fit/fit_convert
  modules/garmin_fit/fit_crc
)
