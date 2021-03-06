if (CGAL_target_use_pointmatcher_included)
  return()
endif()
set(CGAL_target_use_pointmatcher_included TRUE)

function(CGAL_target_use_pointmatcher target)
  target_include_directories(${target} PUBLIC ${libpointmatcher_INCLUDE_DIR})
  target_compile_options( ${target} PUBLIC -DCGAL_LINKED_WITH_POINTMATCHER)
  target_link_libraries(${target} PUBLIC ${libpointmatcher_LIBRARIES})
endfunction()
