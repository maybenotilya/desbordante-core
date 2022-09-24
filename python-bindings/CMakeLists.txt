# configure for python bindings
find_package(PythonLibs 3.7 REQUIRED)
find_package(Boost ${BOOST_VERSION} REQUIRED COMPONENTS python3)

# create the library
add_library(desbordante MODULE PythonBindings.cpp DictToParamsMap.cpp)

# Set up the libraries and header search paths for the library
target_link_libraries(desbordante PUBLIC ${Boost_LIBRARIES} ${CMAKE_PROJECT_NAME}_lib easyloggingpp)
target_include_directories(desbordante PRIVATE ${PYTHON_INCLUDE_DIRS})

# Remove "lib" prefix because Python does not allow this prefix
set_target_properties(desbordante PROPERTIES PREFIX "")
