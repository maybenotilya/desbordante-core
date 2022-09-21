# configure for python bindings
find_package(PythonLibs 3.7 REQUIRED)
find_package(Boost ${BOOST_VERSION} REQUIRED COMPONENTS python3 system)

# include directories
include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

# create the library
add_library(desbordante SHARED PythonBindings.cpp DictToParamsMap.cpp)

# link
target_link_libraries(desbordante LINK_PUBLIC ${Boost_LIBRARIES} Threads::Threads easyloggingpp Desbordante_lib)
# Can't link easyloggingpp; undefined symbol: _ZN2el4base9elStorageE

# Suppress prefix "lib" because Python does not allow this prefix
set_target_properties(desbordante PROPERTIES PREFIX "")
