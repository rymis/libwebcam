
ADD_EXECUTABLE(example_1 example_1.c)
ADD_EXECUTABLE(example_2 example_2.c)
ADD_EXECUTABLE(example_3 example_3.c)

TARGET_LINK_LIBRARIES(example_1 mihl)
TARGET_LINK_LIBRARIES(example_2 mihl)
TARGET_LINK_LIBRARIES(example_3 mihl)

