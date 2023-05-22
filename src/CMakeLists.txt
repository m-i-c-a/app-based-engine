set( GLSL_DIR ${CMAKE_HOME_DIRECTORY}/data/glsl )
set( SPIRV_OUTPUT_DIR ${CMAKE_HOME_DIRECTORY}/data/spirv )
set( GLSL_SOURCE_FILES ${CMAKE_HOME_DIRECTORY}/data/glsl/array_sum.comp )

foreach( GLSL ${GLSL_SOURCE_FILES} )
    get_filename_component( FILE_NAME ${GLSL} NAME )
    set( SPIRV ${SPIRV_OUTPUT_DIR}/${FILE_NAME}.spv )
    add_custom_command( 
        OUTPUT ${SPIRV}
        COMMAND $ENV{VULKAN_SDK}/bin/glslc ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL} )
    list( APPEND SPIRV_BINARY_FILES ${SPIRV} )
endforeach( GLSL )

message( STATUS ${SPIRV_BINARY_FILES} )

add_custom_target( shaders DEPENDS ${SPIRV_BINARY_FILES} )

add_executable( ${PROJECT_NAME} 
    main.cpp
    BaseApp.cpp BaseApp.hpp
    WindowedApp.cpp WindowedApp.hpp
    HeadlessApp.cpp HeadlessApp.hpp
    App.cpp App.hpp
    Buffer.cpp Buffer.hpp 
    StagingBuffer.cpp StagingBuffer.hpp
    vkn.cpp vkn.hpp
    vulkan_init.cpp vulkan_init.hpp
    ${GLSL_SHADERS}
    ${SPV_SHADERS} )

add_dependencies( ${PROJECT_NAME} shaders )

target_compile_features( ${PROJECT_NAME} PRIVATE cxx_std_20 )
target_include_directories( ${PROJECT_NAME} PRIVATE 
    $ENV{VULKAN_SDK}/include
    ${CMAKE_HOME_DIRECTORY}/external )
target_link_libraries( ${PROJECT_NAME} PRIVATE 
    $ENV{VULKAN_SDK}/lib/libvulkan.so 
    glfw )
