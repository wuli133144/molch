option(GENERATE_LUA_BINDINGS "Enable the Lua binding for testing purposes" ON)

if(GENERATE_LUA_BINDINGS AND (NOT APPLE))
    find_package(SWIG)
    if(SWIG_FOUND)
        include(${SWIG_USE_FILE})
        find_package(Lua)
        if (LUA_FOUND)
            include_directories(${LUA_INCLUDE_DIR})
            include_directories(${CMAKE_SOURCE_DIR})

            find_program(LUA_INTERPRETER NAMES lua lua53 lua5.3 lua52 lua5.2 lua51 lua5.1 luajit)

            swig_add_module(molch-interface lua molch.i)
            swig_link_libraries(molch-interface molch ${LUA_LIBRARIES})

            # copy the lua binding
            execute_process(COMMAND cmake -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/molch.lua" "${CMAKE_CURRENT_BINARY_DIR}/molch.lua")

            subdirs(scenarios)
        endif()
    endif()
endif()